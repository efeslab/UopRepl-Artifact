/***************************************************************************************
 * File         : uop_cache.h
 * Author       : Peter Braun
 * Date         : 10.28.2020
 * Description  : Interface for interacting with uop cache object.
 *                  Following Kotra et. al.'s MICRO 2020 description of uop cache baseline
 ***************************************************************************************/

#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"
#include "isa/isa_macros.h"

#include "bp/bp.h"
#include "op_pool.h"

#include "core.param.h"
#include "debug/debug.param.h"
#include "general.param.h"
#include "statistics.h"

#include "libs/cache_lib.h"
#include "memory/memory.h"
#include "memory/memory.param.h"
#include "prefetcher/pref.param.h"
#include "uop_cache.h"

/**************************************************************************************/
/* Macros */

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_UOP_CACHE, ##args)

// Uop cache is byte-addressable, so tag/set index are generated from full address (no offset)
// Uop cache uses icache tag + icache offset as full TAG
#define UOP_CACHE_LINE_SIZE       ICACHE_LINE_SIZE
#define UOP_QUEUE_SIZE            100 // at least UOP_CACHE_ASSOC * UOP_CACHE_MAX_UOPS_LINE
#define UOP_CACHE_LINE_DATA_SIZE  sizeof(Uop_Cache_Data)

/**************************************************************************************/
/* Local Prototypes */

static inline Flag insert_uop_cache(void);
static inline Flag in_uop_cache_search(Addr search_addr, Flag update_repl, Flag record_access);
static Flag pw_insert(Uop_Cache_Data pw);

/**************************************************************************************/
/* Global Variables */

Cache uop_cache;

// uop trace/bbl accumulation
static Uop_Cache_Data accumulating_pw = {0};
static Addr uop_q[UOP_QUEUE_SIZE];
static Counter uop_q_fetch[UOP_QUEUE_SIZE];

// k: instr addr
Hash_Table inf_size_uop_cache;
// indexed by start addr of PW
Hash_Table pc_to_pw;

/**************************************************************************************/
/* init_uop_cache */

void init_uop_cache() {
  if (INF_SIZE_UOP_CACHE || INF_SIZE_UOP_CACHE_PW_SIZE_LIM) {
    init_hash_table(&inf_size_uop_cache, "infinite sized uop cache", 15000000, sizeof(int));
  }
  // if (UOP_CACHE_SIZE == 0) { //todo
  //   return;
  // }
  // used for prefetching
  init_hash_table(&pc_to_pw, "Log of all PWs decoded", 15000000, 
                    sizeof(Uop_Cache_Data));
  // The cache library computes the number of entries from line_size and cache_size,
  // but UOP_CACHE_LINE_SIZE must be 1 to enable indexing with the full byte-granularity address.

  init_cache(&uop_cache, "UOP_CACHE", UOP_CACHE_SIZE * UOP_CACHE_LINE_SIZE, UOP_CACHE_ASSOC,
             UOP_CACHE_LINE_SIZE,UOP_CACHE_LINE_DATA_SIZE, UOP_CACHE_REPL_POLICY);
  uop_cache.tag_incl_offset = TRUE;
  /* Init for FOO's oracle traces.*/
  if((&uop_cache)->repl_policy == REPL_FOO) initFooCore();
  /* Init Bypass Map*/
  /* todo: comment this line to be correct */
  if((&uop_cache)->repl_policy == REPL_PMC){
    initBypassMap();
    if(UOP_CACHE_DYNAMIC_EVICT) initEvictionMap();
  }
  if((&uop_cache)->repl_policy == REPL_HIT_TIMES) initHitTimesMap();
}

Flag pw_insert(Uop_Cache_Data pw) {
  Uop_Cache_Data* cur_line_data = NULL;
  Addr line_addr;  
  Addr repl_line_addr;
  pw.used = 0;

  if (return_exe_count_type()==4){
      // 1: TYPE_ISTAGE_FETCH_ADDR; 2: TYPE_OP_FETCH_ADDR; 3: TYPE_CACHE_STORE
      // for execution count hash update
      // printf("In pw_insert\n");
      if ((&uop_cache)->repl_policy == REPL_CACHEACCESS && UOP_CACHE_ENABLE_RECORD) {
        uopcache_update_execution_count_concat(pw.first, pw.last);
      } else if ((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT) {
        // uopcache_update_execution_count(pw.first, return_file_pointer());
        // uopcache_update_execution_count_concat_tran(pw.first, pw.last); // This is for detach address solution
        uopcache_update_execution_count_concat(pw.first, pw.last); // This is for counting PW with size solution
      }
    }
  if((&uop_cache)->repl_policy == REPL_COMPRATE) {
    if (UOP_CACHE_USAGE_COUNT == TYPE_TRANSIENT){
      // write_ratecacheaccess_concat(pw.first, pw.last, 0, 1);
      write_countcacheaccess_concat(pw.first, pw.last, 1);
      fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", pw.first, pw.last, find_entry_count_concat(pw.first, pw.last));
      // fprintf((&uop_cache)->file_pointer, "%llu %llu %llu %llu\n", pw.first, pw.last, 0, 1);  
    } else if (UOP_CACHE_USAGE_COUNT == TYPE_WHOLE && UOP_CACHE_ENABLE_RECORD){
      write_countcacheaccess_concat(pw.first, pw.last, 1);
      fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", pw.first, pw.last, find_entry_count_concat(pw.first, pw.last)); 
      // fprintf((&uop_cache)->file_pointer, "%llu %llu %llu %llu\n", pw.first, pw.last, 0, 1); 
    } 
  }

  int lines_needed = pw.n_uops / UOP_CACHE_MAX_UOPS_LINE;
  if (pw.n_uops % UOP_CACHE_MAX_UOPS_LINE) lines_needed++;
  ASSERT(0, lines_needed > 0);

  // printf("In pw_insert, n_uops is %u, first address is %llu\n", pw.n_uops, pw.first);

  // Count if containing lines
  if (lines_needed > 0){
    STAT_EVENT(0, UOP_CACHE_ALL_LINES);
  }

  if (lines_needed == 1){
    STAT_EVENT(0, UOP_CACHE_SINGLE_LINE);
  }

  if(lines_needed > 8)
  {
    STAT_EVENT(0, UOP_PW_LENGTH_MORE);
  }
  else{
    STAT_EVENT(0, UOP_PW_LENGTH_1 - 1 + lines_needed );
  }

  { 
    int i = 0;
    for (i = 0; i < pw.n_uops; i+=UOP_CACHE_MAX_UOPS_LINE)
    {
      STAT_EVENT(0,UOP_BLOCK_OPNUM_1 + UOP_CACHE_MAX_UOPS_LINE - 1);
    }
    if(pw.n_uops%UOP_CACHE_MAX_UOPS_LINE) STAT_EVENT(0,UOP_BLOCK_OPNUM_1 - 1 + pw.n_uops%UOP_CACHE_MAX_UOPS_LINE);
  }

  

  // Addr tag;
  line_addr = pw.first & ~(&uop_cache)->offset_mask;
  // tag       = pw.first >> (&uop_cache)->shift_bits & (&uop_cache)->tag_mask;
  uns  set =  pw.first >> (&uop_cache)->shift_bits & (&uop_cache)->set_mask;
  uns64 cycle_num1 = find_cycle_count(pw.first, TRUE);
  uns64 cycle_num2 = find_cycle_count(pw.last, FALSE);
  write_cycle(pw.first,0,TRUE);
  write_cycle(pw.last,0,TRUE);
          

  // Is the PW too big?
  if (lines_needed > UOP_CACHE_ASSOC) {
    STAT_EVENT(ic->proc_id, UOP_CACHE_PW_INSERT_FAILED_TOO_LONG + pw.prefetch);
    return FALSE;
  } else { 
    if (return_repl_policy() == 2 && return_feature(3) == 3){
      // printf("Addr %llu in 1\n", pw.first);
      write_countcacheaccess(pw.first, pw.n_uops);
      fprintf(return_file_pointer(), "%llu %llu\n", pw.first, find_entry_count(pw.first)); 
    }
    /* Evict all entries at same start address but different end address */
    int return_value = cache_access_size(&uop_cache, pw.first, pw.last, &line_addr, TRUE, 
                          (void**) &cur_line_data, TYPE_LOAD);
    /* Optimize for FOO avoid inserting small PWs */
    if(FOO_FIX_LEVEL == FOO_ALL_FIXED || FOO_FIX_LEVEL == FOO_ALL_FIXED_BYPASS){
      if(return_value == -1){
        ASSERT(0, uop_cache.repl_policy == REPL_FOO);
        // printf("[Ops] Good Luck:%lld, %lld\n", pw.first,pw.last);
        return FALSE;
      }
    }
    if (return_value > 0) {
      STAT_EVENT(ic->proc_id, UOP_CACHE_PW_INSERT_FAILED_CACHE_HIT + pw.prefetch);
      return FALSE;
    } else {
      if (FALSE){//(&uop_cache)->repl_policy==REPL_SRRIP && UOP_RRIP_PW){
        cur_line_data = (Uop_Cache_Data*) cache_insert_all(&uop_cache, lines_needed,
                        pw.first, &line_addr, &repl_line_addr, TYPE_LOAD, pw);
        if (repl_line_addr){//} && (&uop_cache)->repl_policy==REPL_NEW_OPT && UOP_CACHE_ENABLE_RECORD==0) {
          cache_invalidate_all(&uop_cache, repl_line_addr/*pw.first*/, &line_addr);
          // cache_invalidate(&uop_cache, repl_line_addr, &line_addr);
        }
        // memset(cur_line_data, 0, UOP_CACHE_LINE_DATA_SIZE);
        // *cur_line_data = pw;
      } else {
        Flag should_insert = TRUE;
        /* Insert it, taking appropriate number of lines */
        if (((&uop_cache)->repl_policy==REPL_SRRIP && UOP_RRIP_PW)||(&uop_cache)->repl_policy==REPL_EXECUTION_COUNT||
        (&uop_cache)->repl_policy==REPL_CACHEACCESS && (!UOP_CACHE_ENABLE_RECORD)){//||
        // (&uop_cache)->repl_policy==REPL_COMPRATE && ((!UOP_CACHE_ENABLE_RECORD && UOP_CACHE_USAGE_COUNT==TYPE_WHOLE)||UOP_CACHE_USAGE_COUNT==TYPE_TRANSIENT)){//||
        // (&uop_cache)->repl_policy==REPL_PMC && ((!UOP_CACHE_ENABLE_RECORD && UOP_CACHE_USAGE_COUNT==TYPE_WHOLE)||UOP_CACHE_USAGE_COUNT==TYPE_TRANSIENT)){
          // if ((&uop_cache)->repl_policy==REPL_COMPRATE && ((!UOP_CACHE_ENABLE_RECORD && UOP_CACHE_USAGE_COUNT==TYPE_WHOLE))){
          //   float pre_check_rate;
          //   // pre_check_rate = find_entry_rate_holistic_concat(pw.first, pw.last);
          //   pre_check_rate = find_entry_count_concat(pw.first, pw.last);
          //   if (pre_check_rate == 1){//0){
          //     should_insert = FALSE;
          //   }
          //   printf("pre_check_rate %f, should_insert is %u\n", pre_check_rate, should_insert);
          // }
          // printf("Lines needed for 0x%s 0x%s %u is %u\n", hexstr64s(pw.first), hexstr64s(pw.last), pw.n_uops, lines_needed);
          // printf("Lines needed for %llu %llu %u is %u\n", pw.first, pw.last, pw.n_uops, lines_needed);
          // Addr tag;
          // uns  set = 1;//cache_index(uop_cache, pw.first, &tag, &line_addr);
          line_addr = pw.first & ~(&uop_cache)->offset_mask;
          // tag       = pw.first >> (&uop_cache)->shift_bits & (&uop_cache)->tag_mask;
          uns  set =  pw.first >> (&uop_cache)->shift_bits & (&uop_cache)->set_mask;
          int ii, invalid_lines=0;
          for(ii = 0; ii < (&uop_cache)->assoc; ii++) {
                Cache_Entry* entry = &(&uop_cache)->entries[set][ii];
                if(!entry->valid) {
                  invalid_lines++;
                }
          }
          if (invalid_lines<lines_needed){
            if ((&uop_cache)->repl_policy==REPL_SRRIP){
              uns rrip_ind  = (&uop_cache)->assoc;
              while (rrip_ind == (&uop_cache)->assoc){
                  for(ii = 0; ii < (&uop_cache)->assoc; ii++) {
                    Cache_Entry* entry = &(&uop_cache)->entries[set][ii];
                    if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                      entry->valid = FALSE;
                      invalid_lines++;
                      rrip_ind = ii;
                      if(invalid_lines==lines_needed)
                      break;
                    }
                  }
                  if (rrip_ind == (&uop_cache)->assoc){
                    for(ii = 0; ii < (&uop_cache)->assoc; ii++) {
                      Cache_Entry* entry = &(&uop_cache)->entries[set][ii];
                      // ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
                      if (entry->rrpv < pow(2, RRIP_BIT_SIZE)-1)
                      entry->rrpv += 1;
                    }
                  }
              }
            } else {
              while(invalid_lines<lines_needed){
                uns     new_ind  = 0;
                Counter new_count = MAX_CTR;//0;//MAX_CTR;
                // if ((&uop_cache)->repl_policy==REPL_PMC) new_count = 0;
                float   new_rate = 1;
                uns     new_length = 0;
                for(ii = 0; ii < (&uop_cache)->assoc; ii++) {
                  Cache_Entry* entry = &(&uop_cache)->entries[set][ii];
                  if(entry->valid) { 
                    uns64 last_value;
                    float entry_rate;
                    int8 last_line_num;
                    Flag has_history;
                    if ((&uop_cache)->repl_policy==REPL_EXECUTION_COUNT) {
                      has_history = find_last_quanta_concat((&uop_cache), entry->addr, entry->addr_end, &last_value, &last_line_num, set);
                      if (!has_history) last_value = 0;
                    } else if ((&uop_cache)->repl_policy==REPL_CACHEACCESS) {
                      last_value = find_entry_count_concat(entry->addr, entry->addr_end);
                    } 
                    // else if((&uop_cache)->repl_policy==REPL_COMPRATE && UOP_CACHE_USAGE_COUNT==TYPE_TRANSIENT){
                    //   entry_rate = find_entry_rate_transient_concat(entry->addr, entry->addr_end);
                    // }  else if((&uop_cache)->repl_policy==REPL_COMPRATE && UOP_CACHE_USAGE_COUNT==TYPE_WHOLE) {
                    //   entry_rate = find_entry_rate_holistic_concat(entry->addr, entry->addr_end);
                    // }
                    else if ((&uop_cache)->repl_policy==REPL_COMPRATE) {
                      last_value = find_entry_count_concat(entry->addr, entry->addr_end);
                    }
                    else if ((&uop_cache)->repl_policy==REPL_PMC) {
                      last_value = find_entry_count(entry->addr);
                    } 
                    // printf("Entry %u, In find_repl, before find, check begin addr 0x%s, end addr 0x%s, cache root %u\n", ii, hexstr64s(entry->addr), hexstr64s(entry->addr_end), (&uop_cache)->map_root);
                    // printf("Entry %u, In find_repl, before find, check addr %llu %llu, cache root %u\n", ii, entry->addr, entry->addr_end, (&uop_cache)->map_root);
                    // printf("has_history %u, count 0x%s, counter 0x%s, last_value<new_count is %u\n", has_history, hexstr64s(last_value), hexstr64s(new_count), last_value < new_count);
                    // // printf("has_history %u, last_rate %f, rate %f, last_value<new_count is %u\n", has_history, entry_rate, new_rate, entry_rate < new_rate);
                    // if ((&uop_cache)->repl_policy==REPL_COMPRATE){
                    //   if(entry_rate < new_rate){
                    //     new_ind  = ii;
                    //     new_rate = entry_rate;
                    //   }
                    // } else {
                    if ((&uop_cache)->repl_policy==REPL_PMC){
                      if(last_value < /* >= */ new_count){//||(last_value == new_count && entry->op_length >= lines_needed-invalid_lines)) { // > //<
                        // printf("op_length %u, lines_needed %u invalid_lines %u\n", entry->op_length, lines_needed, invalid_lines);
                        new_ind  = ii;
                        new_count = last_value;
                      }
                    } else if ((&uop_cache)->repl_policy==REPL_CACHEACCESS||(&uop_cache)->repl_policy==REPL_COMPRATE){
                      if(last_value < new_count){//||(last_value == new_count && entry->op_length >= lines_needed-invalid_lines)) { // > //<
                        // printf("op_length %u, lines_needed %u invalid_lines %u\n", entry->op_length, lines_needed, invalid_lines);
                        new_ind  = ii;
                        new_count = last_value;
                      }
                    } else printf("ERROR: Unchosen repl policy\n");
                  }
                }
                // printf("Invalidate %u, count %u, rate %f\n", new_ind, new_count, new_rate);
                Addr chosen_addr = (&(&uop_cache)->entries[set][new_ind])->addr;
                for(ii = 0; ii < (&uop_cache)->assoc; ii++) {
                  Cache_Entry* line = &(&uop_cache)->entries[set][ii];
                  if((line->pw_start_addr == chosen_addr||line->addr == chosen_addr) && line->valid) {
                    line->tag   = 0;
                    line->valid = FALSE;
                    line->base  = 0;
                    line->addr  = 0;
                    line->addr_end  = 0;
                    invalid_lines++;
                  }
                }
              }
            }
            
          }
        }
        
        if(FOO_FIX_LEVEL == FOO_ALL_FIXED){
          if((&uop_cache)->repl_policy == REPL_FOO){
            uns set = pw.first >> (&uop_cache)->shift_bits & (&uop_cache)->set_mask;
            int ii, invalid_lines=0;
            for(ii = 0; ii < (&uop_cache)->assoc; ii++) {
              Cache_Entry* entry = &(&uop_cache)->entries[set][ii];
              if(!entry->valid) {
                invalid_lines++;
              }else{
                Flag insertFlag = checkExistCache(entry->pw_start_addr, entry->op_length);
                if(insertFlag == FALSE) invalid_lines++;
              }
            }
            /* Now we evict lines that FOO consider not in.*/
            /* Need to iterate on all entries since uop_cache do not allow partial saving.*/
            if(invalid_lines < lines_needed){
              Flag insertFlag = checkExistCache(pw.first, pw.n_uops);
              if(insertFlag == TRUE){
                ASSERT(0, 0); /* FOO should Guarantee this*/
              }else{
                /* Can't insert in this unseen value. bypass it.*/
                return FALSE;
              }
            }
            // /* This is because theroatically FOO ensures cache limitation.*/
            // ASSERT(0, invalid_lines >= lines_needed);
          }
        }

        if(FOO_FIX_LEVEL == FOO_NO_FIXED){
          if((&uop_cache)->repl_policy == REPL_FOO){
            uns set = pw.first >> (&uop_cache)->shift_bits & (&uop_cache)->set_mask;
            for(int ii = 0; ii < (&uop_cache)->assoc; ii++) {
              Cache_Entry* entry = &(&uop_cache)->entries[set][ii];
              if(entry->valid) {
                Flag insertFlag = checkExistCache(entry->pw_start_addr, entry->op_length);
                if(insertFlag == FALSE){
                  entry->tag   = 0;
                  entry->valid = FALSE;
                  entry->base  = 0;
                  entry->addr  = 0;
                  entry->addr_end  = 0;
                  entry->actual_addr = 0;
                  entry->pw_start_addr = 0;
                }
              }
            }
          }
        }

        if (should_insert == TRUE){
          for (int jj = 0; jj < lines_needed; jj++) {
            // printf("Line start %llu, %llu, %u, %u, %u\n", pw.first, pw.actual_addr_list[jj], pw.n_uops, UOP_CACHE_MAX_UOPS_LINE, pw.n_uops/UOP_CACHE_MAX_UOPS_LINE);
            // printf("a\n"); // Hit print
            cur_line_data = (Uop_Cache_Data*) cache_insert_concat(&uop_cache, 0,
                            pw.first, pw.last, pw.actual_addr_list[jj], pw.n_uops, &line_addr, &repl_line_addr, TYPE_LOAD);
            // printf("b\n");
            /* invalidate all lines with same pw start addr(evicted) */
            // (instead all with that have their PW start in this line)
            if (repl_line_addr){//} && (&uop_cache)->repl_policy==REPL_NEW_OPT && UOP_CACHE_ENABLE_RECORD==0) {
              if(FOO_FIX_LEVEL == FOO_NO_FIXED){
                if((&uop_cache)->repl_policy == REPL_FOO){
                  printf("[FOO pw_insert] There should not be victim ");
                  ASSERT(0,0);
                }
              }
              cache_invalidate_all(&uop_cache, repl_line_addr/*pw.first*/, &line_addr);
              // printf("Invalidate line 0x%s\n", hexstr64s(repl_line_addr));
              // cache_invalidate(&uop_cache, repl_line_addr, &line_addr);
            }
            memset(cur_line_data, 0, UOP_CACHE_LINE_DATA_SIZE);
            *cur_line_data = pw;
          }
        }
      }
      STAT_EVENT(0, UOP_CACHE_PWS_INSERTED);
      INC_STAT_EVENT(0, UOP_CACHE_LINES_INSERTED, lines_needed);
    }
  }
  return TRUE;
}

/**************************************************************************************/
/* insert_uop_cache: private method, only called by accumulate_op
 *                   Drain buffer and insert. Return whether inserted.
 */
Flag insert_uop_cache() {
  // PW may span multiple cache entries. 1 entry per line. Additional terminating conditions per line:
  // 1. max uops per line
  // 2. max imm/disp per line
  // 3. max micro-coded instr per line (not simulated)

  // Invalidate after hitting maximum allowed number of lines, as in gem5
  // Each entry/pw indexed by physical addr of 1st instr, so insert each line using same addr
  // Invalidation: Let LRU handle it by placing PW in one line at a time.   
  
  ASSERT(0, accumulating_pw.n_uops);
  Flag success = FALSE;

  Flag new_entry;
  Uop_Cache_Data* saved_pw = (Uop_Cache_Data*) hash_table_access_create(
                              &pc_to_pw, accumulating_pw.first, &new_entry);
  *saved_pw = accumulating_pw;
  int lines_needed = accumulating_pw.n_uops / UOP_CACHE_MAX_UOPS_LINE;
  if (accumulating_pw.n_uops % UOP_CACHE_MAX_UOPS_LINE) lines_needed++;

  if (INF_SIZE_UOP_CACHE || (INF_SIZE_UOP_CACHE_PW_SIZE_LIM 
      && accumulating_pw.n_uops <= INF_SIZE_UOP_CACHE_PW_SIZE_LIM)) {
    for (int ii = 0; ii < accumulating_pw.n_uops; ii++) {
      Addr opAddr = uop_q[ii];
      Flag new_entry;
      hash_table_access_create(&inf_size_uop_cache, opAddr, 
                                &new_entry);
    }
    success = TRUE;
  } else if (INF_SIZE_UOP_CACHE_PW_SIZE_LIM 
            && accumulating_pw.n_uops > INF_SIZE_UOP_CACHE_PW_SIZE_LIM) {
    success = FALSE;
  } else {
    success = pw_insert(accumulating_pw);
  }

  return success;
}

static inline Flag in_uop_cache_search(Addr search_addr, Flag update_repl, Flag record_access) {
  static Uop_Cache_Data cur_pw = {0};
  Addr line_addr;
  Uop_Cache_Data* uoc_data = NULL;
  Flag found = FALSE;

  /* Comment this For PW-Granularity Belady*/
  // if (//record_access && 
  // (&uop_cache)->repl_policy==REPL_NEW_OPT){
  //     add_timestamp_addr(search_addr);
  //   }
  // print_timestamp_addr(search_addr);

  if (//record_access && 
  (&uop_cache)->repl_policy==REPL_OPT){
      (&uop_cache)->pre_addr_opt = search_addr;
    }

  // Yilong Print
  // printf("[uop_cache_search] cur PW start address:%lld, end at address:%lld, uop counter:%lld\n", cur_pw.first, cur_pw.last, cur_pw.n_uops);

  /* First check if current pw has this search addr */
  static Addr checkPrevAddr = 0;
  if(checkPrevAddr > search_addr){
    memset(&cur_pw, 0, sizeof(Uop_Cache_Data));
  }
  if (cur_pw.first && search_addr >= cur_pw.first
                   && search_addr <= cur_pw.last) {
    found = TRUE;
    if (return_repl_policy() == 2 && return_feature(3) == 3){
      // printf("Addr %llu in 2\n", cur_pw.first);
      write_countcacheaccess(cur_pw.first, 1);//cur_pw.n_uops);
      fprintf(return_file_pointer(), "%llu %llu\n", cur_pw.first, find_entry_count(cur_pw.first)); 
    }
    if ((&uop_cache)->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){// && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK)){
      fprintf((&uop_cache)->file_pointer_backup, "%llu %llu %llu\n", search_addr, 1, 1); 
    }
    if ((&uop_cache)->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){// && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK)){
        fprintf((&uop_cache)->file_pointer_switch, "%llu %llu %llu\n", search_addr, 1, 1); 
      }
  } else {
    /* Next try to access a new PW starting at this addr */

    /* First check whether the original PW should be evicted */
    /* Comment because we evict when insert. */
    if(FOO_FIX_LEVEL == FOO_NO_FIXED){
      if((&uop_cache)->repl_policy == REPL_FOO){
        if(cur_pw.first){
          ASSERT(0, cur_pw.n_uops > 0);
          Flag insertEvict = checkExistCache(cur_pw.first, cur_pw.n_uops);
          Addr occupy;
          if(insertEvict == FALSE){
            cache_invalidate_all(&uop_cache, cur_pw.first, &occupy);
            if(FOO_PRINT) printf("[FOO] Evict after looking up, start: %lld, end: %lld, uop: %lld\n", cur_pw.first, cur_pw.last, cur_pw.n_uops);
          }
        }
      }
    }

    int lines_num = 0;
    // Addr tagnum;
    line_addr = search_addr & ~(&uop_cache)->offset_mask;
    // tagnum    = search_addr >> (&uop_cache)->shift_bits & (&uop_cache)->tag_mask;
    uns  setnum =  search_addr >> (&uop_cache)->shift_bits & (&uop_cache)->set_mask;
    if (//(UOP_OPT_CACHE_ACCESS || (&uop_cache)->repl_policy!=REPL_TRUE_LRU) && 
    return_feature(1)==1){//} && (&uop_cache)->repl_policy==REPL_NEW_OPT){
      /* Hit Prints*/
      lines_num = cache_access_lookup_allpw(&uop_cache, search_addr, &line_addr, update_repl, 
                          (void**) &uoc_data, TYPE_LOAD);
    } else {
      lines_num = cache_access_all(&uop_cache, search_addr, &line_addr, update_repl, 
                          (void**) &uoc_data, TYPE_LOAD);
      // if((&uop_cache)->repl_policy == REPL_HIT_TIMES && update_repl) printf("[Uop_search] Look up on %lld and get lines %d\n", search_addr,lines_num);
    }
    if (lines_num > 0) {
      if (return_exe_count_type() == 4 && update_repl){
        // 1: TYPE_ISTAGE_FETCH_ADDR; 2: TYPE_OP_FETCH_ADDR; 3: TYPE_CACHE_STORE
        // for execution count hash update
        // Uop_Cache_Data tmp_pw = *uoc_data;
        if ((&uop_cache)->repl_policy == REPL_CACHEACCESS && UOP_CACHE_ENABLE_RECORD) {
          uopcache_update_execution_count_concat(uoc_data->first, uoc_data->last);
          // fprintf((&uop_cache)->file_pointer, "get hit: range: %llu %llu %llu\n", uoc_data->first, uoc_data->first, search_addr);
        } else if ((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT) {
          // uopcache_update_execution_count(uoc_data->first, return_file_pointer());
          // uopcache_update_execution_count_concat_tran(uoc_data->first, uoc_data->last); // This is for detach address solution
          uopcache_update_execution_count_concat(uoc_data->first, uoc_data->last); // This is for counting PW with size solution
        }
      }
      if (return_repl_policy() == 2 && return_feature(3) == 3){
        // write_countcacheaccess(search_addr, 1);
        // fprintf(return_file_pointer(), "%llu %llu\n", search_addr, find_entry_count(search_addr)); 
        // printf("Addr %llu in 3\n", uoc_data->first);
        write_countcacheaccess(uoc_data->first, 1);
        fprintf(return_file_pointer(), "%llu %llu\n", uoc_data->first, find_entry_count(uoc_data->first));
      }
      if ((&uop_cache)->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){// && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK)){
        fprintf((&uop_cache)->file_pointer_backup, "%llu %llu %llu\n", search_addr, 1, 1); 
      }
      if ((&uop_cache)->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){// && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK)){
        fprintf((&uop_cache)->file_pointer_switch, "%llu %llu %llu\n", search_addr, 1, 1); 
      }
      if (return_repl_policy() == 2 && return_feature(4) == 4){
        write_countcacheaccess(search_addr, 1);
        fprintf(return_file_pointer(), "%llu %llu\n", search_addr, find_entry_count(search_addr)); 
      }
      if(TRUE){//update_repl){
        if((&uop_cache)->repl_policy == REPL_COMPRATE) {
          if (UOP_CACHE_USAGE_COUNT == TYPE_TRANSIENT){
            // write_ratecacheaccess_concat(uoc_data->first, uoc_data->last, 1, 1);
            write_countcacheaccess_concat(uoc_data->first, uoc_data->last, 2);
            fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", uoc_data->first, uoc_data->last, find_entry_count_concat(uoc_data->first, uoc_data->last));
            // fprintf((&uop_cache)->file_pointer, "%llu %llu %llu %llu\n", uoc_data->first, uoc_data->last, 1, 1); 
          } else if (UOP_CACHE_USAGE_COUNT == TYPE_WHOLE && UOP_CACHE_ENABLE_RECORD){
            write_countcacheaccess_concat(uoc_data->first, uoc_data->last, 1);
            fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", uoc_data->first, uoc_data->last, find_entry_count_concat(uoc_data->first, uoc_data->last)); 
            // fprintf((&uop_cache)->file_pointer, "%llu %llu %llu %llu\n", uoc_data->first, uoc_data->last, 1, 1);
          }  
        }
      }
      if (TRUE){
        if (uoc_data->prefetch && !uoc_data->used) {
          STAT_EVENT(0, UOP_CACHE_PREFETCH_USED);
        }
        if (!uoc_data->used) {
          STAT_EVENT(0, UOP_CACHE_LINES_USED);
        }
        uoc_data->used += 1;
        cur_pw = *uoc_data;
        // fprintf((&uop_cache)->file_pointer, "After hit: range: %llu %llu %llu\n", cur_pw.first, cur_pw.last, search_addr);
      }
      /* Dynamic Eviction Policy*/
      if(update_repl){ // Ensure this is in packet_build
        if(uop_cache.repl_policy == REPL_PMC && UOP_CACHE_DYNAMIC_EVICT){
          // low all lines eviction hints
          uns set = (search_addr >> uop_cache.shift_bits) & uop_cache.set_mask;
          for(int ii = 0;ii < uop_cache.assoc;++ii){
            Cache_Entry* entry = &(uop_cache.entries[set][ii]);
            if(entry->valid && entry->pw_start_addr != search_addr){
              if(entry->evictionHint > 0) entry->evictionHint -= 1;
              if(entry->evictionHint == 0 && entry->exhaustedInterval == TRUE){
                printf("[Dynamic Evict] WORKS!! %lld %d %d\n", entry->pw_start_addr, find_entry_count(entry->pw_start_addr), getEvictionHints(entry->pw_start_addr));
                entry->pmcWeight >>= 2; // Magic number, not so cold
                entry->exhaustedInterval = FALSE;
              }
            }
          }
        }
      }
      found = TRUE;
    } else {
      write_miss(search_addr,1);
      if ((&uop_cache)->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){// && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK)){
        fprintf((&uop_cache)->file_pointer_backup, "%llu %llu %llu\n", search_addr, 0, 1); 
      }
      if ((&uop_cache)->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){// && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK)){
        fprintf((&uop_cache)->file_pointer_switch, "%llu %llu %llu\n", search_addr, 0, 1); 
      }
      // if (TRUE){//find_miss_count(search_addr)>3 && (float)find_miss_count(search_addr)/find_entry_count(search_addr)>0.5){
      //   // printf("%llu misses,\tset %u,\tmiss count %llu,\ttotal count %llu\n", search_addr, setnum, find_miss_count(search_addr), find_entry_count(search_addr));
      //   printf("%llu misses set %u miss count %llu total count %llu\n", search_addr, setnum, find_miss_count(search_addr), find_entry_count(search_addr));
      //   if (setnum==128){
      //     Cache_Entry* line1 = &(&uop_cache)->entries[setnum][0];
      //     Cache_Entry* line2 = &(&uop_cache)->entries[setnum][1];
      //     Cache_Entry* line3 = &(&uop_cache)->entries[setnum][2];
      //     Cache_Entry* line4 = &(&uop_cache)->entries[setnum][3];
      //     Cache_Entry* line5 = &(&uop_cache)->entries[setnum][4];
      //     Cache_Entry* line6 = &(&uop_cache)->entries[setnum][5];
      //     Cache_Entry* line7 = &(&uop_cache)->entries[setnum][6];
      //     Cache_Entry* line8 = &(&uop_cache)->entries[setnum][7];
      //     printf("%llu %llu %llu %llu %llu %llu %llu %llu\n", find_entry_count(line1->addr), find_entry_count(line2->addr)
      //     , find_entry_count(line3->addr), find_entry_count(line4->addr), find_entry_count(line5->addr)
      //     , find_entry_count(line6->addr), find_entry_count(line7->addr), find_entry_count(line8->addr));
      //     printf("%llu %llu %llu %llu %llu %llu %llu %llu %llu\n", (line1->addr), (line2->addr)
      //     , (line3->addr), (line4->addr), (line5->addr)
      //     , (line6->addr), (line7->addr), (line8->addr), search_addr);
      //     // printf("%llu %llu %llu %llu %llu %llu %llu %llu\n", find_entry_count((&(&uop_cache)->entries[setnum][0])->addr), find_entry_count(&(&uop_cache)->entries[setnum][1].addr)
      //     // , find_entry_count(&(&uop_cache)->entries[setnum][2].addr), find_entry_count(&(&uop_cache)->entries[setnum][3].addr), find_entry_count(&(&uop_cache)->entries[setnum][4].addr)
      //     // , find_entry_count(&(&uop_cache)->entries[setnum][5].addr), find_entry_count(&(&uop_cache)->entries[setnum][6].addr), find_entry_count(&(&uop_cache)->entries[setnum][7].addr));
      //     // printf("%llu %llu %llu %llu %llu %llu %llu %llu %llu\n", (&(&uop_cache)->entries[setnum][0].addr), (&(&uop_cache)->entries[setnum][1].addr)
      //     // , (&(&uop_cache)->entries[setnum][2].addr), (&(&uop_cache)->entries[setnum][3].addr), (&(&uop_cache)->entries[setnum][4].addr)
      //     // , (&(&uop_cache)->entries[setnum][5].addr), (&(&uop_cache)->entries[setnum][6].addr), (&(&uop_cache)->entries[setnum][7].addr), search_addr);
      //   }
      // }
      memset(&cur_pw, 0, sizeof(cur_pw));
      found = FALSE;
    }
  }
  checkPrevAddr = search_addr;
  return found;
}

/**************************************************************************************/
/* in_uop_cache: Iterate over all possible ops and check if contained.
 *                    Other option: use cache to simulate capacity and maintain a map 
 *                      data structure for pcs
 */
Flag in_uop_cache(Addr pc, const Counter* op_num, Flag update_repl, Flag record_access) {
  // A PW can span multiple cache entries. The next line used is either physically 
  // next in the set (use flag) or anywhere in the set (use pointer), depending on impl.
  // Here, don't care about order, just search all lines for pc in question
  STAT_EVENT(0, IN_UOP_CACHE_CALLED);

  if (ORACLE_PERFECT_UOP_CACHE) {
    // if (update_repl) {
    //   STAT_EVENT(0, UOP_CACHE_HIT);
    // }
    return TRUE;
  } else if (INF_SIZE_UOP_CACHE || INF_SIZE_UOP_CACHE_PW_SIZE_LIM) {
    return hash_table_access(&inf_size_uop_cache, pc) != NULL;
  }
  if (UOP_CACHE_SIZE == 0) {
    return FALSE;
  }

  static Counter next_op_num = 1;
  static Addr pre_pc;
  // printf("Op count num: %llu, next_op_num %llu, udpate_repl %llu\n", op_num, next_op_num, update_repl);

  Flag found = in_uop_cache_search(pc, update_repl, record_access);
  // printf("addr 0x%s lookup, cycle %llu, op_num %llu, found is %u\n", hexstr64s(pc), cycle_count, op_num, found);
  write_cycle(pc,cycle_count,FALSE);

  if (update_repl) {
    // STAT_EVENT(0, UOP_CACHE_MISS + found);
    if (op_num) {
      ASSERT(0, *op_num == next_op_num);
      next_op_num++;
    }
  }

  pre_pc = pc;
  
  return found;
}

void end_accumulate(void) {
  int lines_needed = accumulating_pw.n_uops / UOP_CACHE_MAX_UOPS_LINE;
  if (accumulating_pw.n_uops % UOP_CACHE_MAX_UOPS_LINE) lines_needed++;
  if (accumulating_pw.n_uops > 0) {
    // printf("**********************\n"); // todo Yilong Print HIT print
    // printf("%lld, %d ,%d\n", accumulating_pw.first, accumulating_pw.n_uops, lines_needed);
    // printf("[Uop Cache Accumulate]%lld, %lld ,%d\n", accumulating_pw.first, accumulating_pw.last, lines_needed);
    for(int ii = 0; ii < accumulating_pw.n_uops;++ii){
      // printf("%lld,\n", uop_q[ii]); // Run this when getting accumalte traces
    // printf("%lld, %d\n", uop_q[ii], ii);
    // printf("**********************\n");
    }
    // Yilong Print
    // printf("PW insert Latency: %lld\n", cycle_count - uop_q_fetch[accumulating_pw.n_uops-1]); // For cycles insert latency
    // printf("%lld, %d, %d, %lld, %lld\n", accumulating_pw.first, accumulating_pw.n_uops, lines_needed, uop_q_fetch[0], cycle_count);
  }

  if (UOP_CACHE_SIZE == 0 && !INF_SIZE_UOP_CACHE && !INF_SIZE_UOP_CACHE_PW_SIZE_LIM) {
    memset(&accumulating_pw, 0, sizeof(accumulating_pw));
    return;
  }

  if (accumulating_pw.n_uops > 0) {
    // printf("Before insert_uop, accumulate_pw start 0x%s, end 0x%s, n_ops %u\n", hexstr64s(accumulating_pw.first), hexstr64s(accumulating_pw.last), accumulating_pw.n_uops);
    if((&uop_cache)->repl_policy == REPL_FOO){
      /* Comment this for stop bypassing */

      if(FOO_FIX_LEVEL == FOO_NO_FIXED || FOO_FIX_LEVEL == FOO_ASYNC_FIXED || FOO_FIX_LEVEL == FOO_ALL_FIXED_BYPASS){
        ASSERT(0, accumulating_pw.first > 0);
        Flag insertFlag = checkExistCache(accumulating_pw.first, accumulating_pw.n_uops);
        if(insertFlag == TRUE){
          if(FOO_PRINT) 
            printf("[Uop Cache Accumulate]%lld, %lld ,%d\n", accumulating_pw.first, accumulating_pw.last, lines_needed);
          insert_uop_cache();
        }else{
          if(FOO_PRINT)
            printf("[FOO STOP Accumulate]%lld, %lld ,%d\n", accumulating_pw.first, accumulating_pw.last, lines_needed);
          memset(&accumulating_pw, 0, sizeof(accumulating_pw));
          return;
        }
      }else insert_uop_cache();
    }else{
      /* Check Bypass When repl == PMC*/
      if((&uop_cache)->repl_policy == REPL_PMC){
        /* Static Bypass -> Filtered by FOO traces*/
        if(checkByPass(accumulating_pw.first, accumulating_pw.n_uops)){
          memset(&accumulating_pw, 0, sizeof(accumulating_pw));
          return;
        }

        /* Dynamic Bypass -> Check whether current Weight is smallest */
        if(UOP_CACHE_DYNAMIC_BYPASS){
          uns set = (accumulating_pw.first >> uop_cache.shift_bits) & uop_cache.set_mask;
          Flag byPass = FALSE;
          Counter minWeight = MAX_CTR;
          for(int ii = 0; ii < uop_cache.assoc; ii++){
            Cache_Entry* entry = &(uop_cache.entries[set][ii]);
            if(!entry->valid){
              minWeight = 0;
              break;
            }else{
              Counter currentWeight = entry->pmcWeight;
              if(currentWeight < minWeight) minWeight = currentWeight;
            }
          }
          Counter thisWeight = find_entry_count(accumulating_pw.first);
          if(minWeight <= 8) minWeight = 0;
          else minWeight = ((minWeight >> 1) > (minWeight - 8)) ? (minWeight - 8) : (minWeight >> 1);
          if(thisWeight < minWeight) byPass = TRUE;
          if(byPass == TRUE){
            printf("[Bypass] Dynamic Bypass %lld, weight: %d\n", accumulating_pw.first, thisWeight);
            memset(&accumulating_pw, 0, sizeof(accumulating_pw));
            return;
          }
        }
      }
      insert_uop_cache();
    }
    memset(&accumulating_pw, 0, sizeof(accumulating_pw));
    // printf("After insert_uop, accumulate_pw start 0x%s, end 0x%s, n_ops %u\n", hexstr64s(accumulating_pw.first), hexstr64s(accumulating_pw.last), accumulating_pw.n_uops);
  }
}

/**************************************************************************************/
/* accumulate_op: accumulate into buffer. Insert into cache at end of PW. */
void accumulate_op(Op* op) {
  // Prediction Window termination conditions:
  // 1. end of icache line
  // 2. branch predicted taken
  // 3. predetermined number of branch NT (no limit in implementation)
  // 4. uop queue full
  // 5. too many uops to fit in entire set, even after evicting all entries

  // it is possible for an instr to be partially in 2 lines. 
  // For pw termination purposes, assume it is in first line.
  static Counter cons_op_num = 0;
  static Flag branch_taken = 0;
  // printf("%lld, %d ,%d\n", accumulating_pw.first, accumulating_pw.n_uops, -1);
    // printf("%d, %d ,%d\n", INF_SIZE_UOP_CACHE,INF_SIZE_UOP_CACHE_PW_SIZE_LIM, ORACLE_PERFECT_UOP_CACHE);

// TODO: uncomment this line to be correct (for uop_cache_size = 0)
  if (( UOP_CACHE_SIZE == 0 &&!INF_SIZE_UOP_CACHE && !INF_SIZE_UOP_CACHE_PW_SIZE_LIM) 
      || ORACLE_PERFECT_UOP_CACHE) {
    return;
  }

  //todo : modify here to test icache line size ( Also todo in packet_build.c)
  Addr cur_icache_line_addr = get_cache_line_addr(&ic->icache,
                                                  accumulating_pw.first);
  Addr icache_line_addr = get_cache_line_addr(&ic->icache, op->inst_info->addr);

  /******************/
  // Addr uop_cache_line_offset_mask = N_BIT_MASK(LOG2(UOP_ICACHE_LINE_SIZE));
  // Addr cur_icache_line_addr = (accumulating_pw.first) & (~uop_cache_line_offset_mask);
  // Addr icache_line_addr = (op->inst_info->addr) & (~uop_cache_line_offset_mask);
  /******************/

  if (!cur_icache_line_addr) {
    accumulating_pw.first = op->inst_info->addr;
    cur_icache_line_addr = icache_line_addr;
    cons_op_num = op->op_num + 1;
  } else {
    ASSERT(0, op->op_num == cons_op_num);
    cons_op_num++;
  }

  // Flag end_of_icache_line = icache_line_addr != cur_icache_line_addr;
  Flag end_of_icache_line = (icache_line_addr & ~(&uop_cache)->offset_mask) != (cur_icache_line_addr & ~(&uop_cache)->offset_mask);
  // printf("addr: 0x%s 0x%s 0x%s 0x%s\n", hexstr64s(icache_line_addr), hexstr64s(icache_line_addr & ~(&uop_cache)->offset_mask),
  // hexstr64s(cur_icache_line_addr), hexstr64s(cur_icache_line_addr & ~(&uop_cache)->offset_mask));
  // Flag branch_pt = op->table_info->cf_type && op->oracle_info.pred == TAKEN;
  
  /* For not stop accmulating issue when mispred untaken*/
  Flag branch_pt = op->table_info->cf_type && (op->oracle_info.pred == TAKEN || op->oracle_info.dir == TAKEN);

  Flag uop_q_full = (accumulating_pw.n_uops + 1 > UOP_QUEUE_SIZE);

  

  // printf("Accumuate op op->eom == TRUE %u, opnum %llu, address 0x%s\n", op->eom == TRUE, op->op_num, hexstr64s(op->inst_info->addr));
  // if (end_of_icache_line || branch_pt || uop_q_full) {
  if (return_feature(5)==5 && end_of_icache_line) {//end_of_icache_line) {//FALSE){//return_feature(5)==5 && end_of_icache_line) {
    // printf("Reason of ending accumulation, end_of_icache_line %u, branch_pt %u, (branch_pt && op->eom == TRUE) %u, op->eom == TRUE %u, opnum %llu, address 0x%s, uop_q_full %u\n", 
    // end_of_icache_line, branch_pt, (branch_pt && op->eom == TRUE), op->eom == TRUE, op->op_num, hexstr64s(op->inst_info->addr), uop_q_full);
    /* Yilong Print*/
    // printf("[Uop Cache Accumulate] END_OF_LINE\n");   
    end_accumulate();
  }

  // printf("End of Line: %d; pw start: %lld; now addr: %lld\n",end_of_icache_line ? 1 : 0, accumulating_pw.first, op->inst_info->addr);

  if (accumulating_pw.n_uops == 0) {
    // occurs when THIS fxn call drains the uop queue (insertion into uop cache)
    accumulating_pw.first = op->inst_info->addr;
  }
  uop_q[accumulating_pw.n_uops] = op->inst_info->addr;
  uop_q_fetch[accumulating_pw.n_uops] = op->fetch_cycle;
  accumulating_pw.last = op->inst_info->addr;
  // if (return_repl_policy() == 2){
  //   write_countcacheaccess(op->inst_info->addr, 1);
  //   fprintf(return_file_pointer(), "%llu %llu\n", op->inst_info->addr, find_entry_count(op->inst_info->addr)); 
  // }
  if (accumulating_pw.n_uops % UOP_CACHE_MAX_UOPS_LINE==0)
    accumulating_pw.actual_addr_list[accumulating_pw.n_uops / UOP_CACHE_MAX_UOPS_LINE] = op->inst_info->addr;
  
  accumulating_pw.n_uops++;

  // printf("accumulate_pw start 0x%s, end 0x%s, n_ops %u\n", hexstr64s(accumulating_pw.first), hexstr64s(accumulating_pw.last), accumulating_pw.n_uops);
  if ((end_of_icache_line  && return_feature(6)==6 ) || branch_pt || uop_q_full) {
    // printf("Reason of ending accumulation, end_of_icache_line %u, branch_pt %u, (branch_pt && op->eom == TRUE) %u, op->eom == TRUE %u, opnum %llu, address 0x%s, uop_q_full %u\n", 
    // end_of_icache_line, branch_pt, (branch_pt && op->eom == TRUE), op->eom == TRUE, op->op_num, hexstr64s(op->inst_info->addr), uop_q_full);
    /* Yilong Print*/
    // printf("[Uop Cache Accumulate] For END_OF_LINE %u, BRANCH %u \n", end_of_icache_line, branch_pt);
    end_accumulate();
  }
}

Flag uop_cache_fill_prefetch(Addr pw_start_addr, Flag fdip_on_path) {
  Uop_Cache_Data pw;
  if (UOP_CACHE_SIZE == 0) {
    return FALSE;
  }

  // on-path / off-path is not working, even for correct-path prefetching.
  fdip_on_path = FALSE; // just use legacy method.

  if (fdip_on_path) {
    pw = get_pw_lookahead_buffer(pw_start_addr);
  } else {
    Uop_Cache_Data* pw_p = (Uop_Cache_Data*) hash_table_access(&pc_to_pw, pw_start_addr);
    // if PW has not been decoded before, do nothing. Hopefully this is uncommon.
    if (pw_p == NULL) {
      STAT_EVENT(0, UOP_CACHE_PREFETCH_FAILED_PW_NEVER_SEEN);
      return FALSE;
    }
    pw = *pw_p;
  }
  ASSERT(0, pw.first == pw_start_addr);
  pw.prefetch = TRUE;
  Flag prefetched = pw_insert(pw);
  INC_STAT_EVENT(0, UOP_CACHE_PREFETCH, prefetched);
  return prefetched;
}

Flag uop_cache_issue_prefetch(Addr pw_start_addr, Flag on_path) {
  int prefetch_success = FALSE;

  if (UOC_ZERO_LATENCY_PREF) {
    prefetch_success = uop_cache_fill_prefetch(pw_start_addr, on_path);
  } else {
    // If no op is provided, on_path is assumed.
    prefetch_success = new_mem_req(MRT_UOCPRF, 0, pw_start_addr,
              ICACHE_LINE_SIZE, DECODE_CYCLES, NULL, instr_fill_line,
              unique_count,
              0);
    if(!prefetch_success) {
      STAT_EVENT(0, UOP_CACHE_PREFETCH_FAILED_MEMREQ_FAILED);
    }
  }

  return prefetch_success;
}

uns return_exe_count_type(){
  // 1: TYPE_ISTAGE_FETCH_ADDR; 2: TYPE_OP_FETCH_ADDR; 3: TYPE_CACHE_STORE
  if ((&uop_cache)->exe_count_type == TYPE_ISTAGE_FETCH_ADDR) return 1;
  else if ((&uop_cache)->exe_count_type == TYPE_OP_FETCH_ADDR) return 2;
  else if ((&uop_cache)->exe_count_type == TYPE_CACHE_STORE) return 3;
  else if ((&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS) return 4;
  else return 0;
}

uns return_repl_policy(){
  if ((&uop_cache)->repl_policy == REPL_NEW_OPT) {
    // printf("repl->policy is %u\n", (&uop_cache)->repl_policy);
    return 1;
  } else if ((&uop_cache)->repl_policy==REPL_PMC && ((UOP_CACHE_ENABLE_RECORD && UOP_CACHE_USAGE_COUNT==TYPE_WHOLE)||UOP_CACHE_USAGE_COUNT==TYPE_TRANSIENT)){
    return 2;
  }else if((&uop_cache)->repl_policy == REPL_FOO){
    return 3;
  }else return 0;
}

uns return_feature(int check_num){
  if (check_num==1 && (UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH||
     UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH)){
      return 1; // all_look_up: Do not use
  } 
  if (check_num==3 && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH)) {
      return 3; // all_start_pw
  } 
  if (check_num==4 && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH)) {
      return 4; // look single and insert decode all lines
  } 
  if (check_num==5 && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH)) {
      return 5; // fix end of line
  }
  if (check_num==6 && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH)) {
      return 6; // no fix end of line
  } 
  if (check_num==7 && (UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH||
             UOP_CACHE_FEATURE == TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH)) {
      return 7; // do switch
  } else return 0;
}

FILE* return_file_pointer(){
  return((&uop_cache)->file_pointer);
}

FILE* return_file_pointer_switch(){
  return((&uop_cache)->file_pointer_switch);
}

Flag uopcache_update_execution_count(Addr update_addr, FILE* new_opt_stream){
  if (((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT && 
  ((&uop_cache)->exe_count_type == TYPE_OP_FETCH_ADDR || (&uop_cache)->exe_count_type == TYPE_ISTAGE_FETCH_ADDR
  || (&uop_cache)->exe_count_type == TYPE_CACHE_STORE || (&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS)) || 
  ((&uop_cache)->repl_policy == REPL_CACHEACCESS && 
  ((&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS || (&uop_cache)->exe_count_type == TYPE_CACHE_STORE))){
    uns64 last_value;
    int8 last_line_num;
    Flag has_history = find_last_quanta(&uop_cache, update_addr, &last_value, &last_line_num, 0);
    if (has_history){
      uns64 updated_value = last_value+1;
      void* data;
      update_quanta(&uop_cache, update_addr, updated_value, last_line_num, 0, data, FALSE);
    } else {
      initialize_quanta(&uop_cache, update_addr, 1, 0);
    }
    has_history = find_last_quanta(&uop_cache, update_addr, &last_value, last_line_num, 0);
    // printf("Istage: Addr 0x%s, count %llu\n", hexstr64s(update_addr), last_value);
  }

  
  fprintf(new_opt_stream, "%llu %llu\n", update_addr, 1);
  
}


Flag uopcache_update_execution_count_concat(Addr update_addr, Addr update_addr_end){
  if (((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT && 
  ((&uop_cache)->exe_count_type == TYPE_OP_FETCH_ADDR || (&uop_cache)->exe_count_type == TYPE_ISTAGE_FETCH_ADDR
  || (&uop_cache)->exe_count_type == TYPE_CACHE_STORE || (&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS)) || 
  ((&uop_cache)->repl_policy == REPL_CACHEACCESS && 
  ((&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS || (&uop_cache)->exe_count_type == TYPE_CACHE_STORE))){
    int64 last_value;
    int8 last_line_num;
    // printf("In PW access, before find, check begin addr 0x%s, end addr 0x%s, cache root %u\n", hexstr64s(update_addr), hexstr64s(update_addr_end), (&uop_cache)->map_root);
    Flag has_history = find_last_quanta_concat(&uop_cache, update_addr, update_addr_end, &last_value, &last_line_num, 0);
    // printf("In PW access, has_history %u\n", has_history);
    // printf("In uopcache_update_execution_count_concat, last find value is %u\n", last_value);
    if (has_history){
      uns64 updated_value = last_value+1;
      // printf("Updated value 0x%s\n", hexstr64s(updated_value));
      void* data;
      update_quanta_concat(&uop_cache, update_addr, update_addr_end, updated_value, last_line_num, 0, data, FALSE);
    } else {
      initialize_quanta_concat(&uop_cache, update_addr, update_addr_end, 1, 0);
    }
    has_history = find_last_quanta_concat(&uop_cache, update_addr, update_addr_end, &last_value, last_line_num, 0);
    // printf("uopcache_update_execution_count_concat: Addr 0x%s, Addr_end 0x%s, count %u\n", hexstr64s(update_addr), hexstr64s(update_addr_end), last_value);
  }

  if ((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT && (&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS/*TYPE_CACHE_STORE*//*TYPE_CACHE_ACCESS*/){
      // int64 updated_value;
      // int8 last_line_num;
      // Flag has_history = find_last_quanta_concat((&uop_cache), update_addr, update_addr_end, &updated_value, &last_line_num, 0);
      // fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", update_addr, update_addr_end, updated_value);
      fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", update_addr, update_addr_end, 1);
    } else if ((&uop_cache)->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD)){
      int64 updated_value;
      int8 last_line_num;
      Flag has_history = find_last_quanta_concat((&uop_cache), update_addr, update_addr_end, &updated_value, &last_line_num, 0);
      fprintf((&uop_cache)->file_pointer, "%llu %llu %llu\n", update_addr, update_addr_end, updated_value);
    }
  
}

Flag uopcache_update_execution_count_concat_tran(Addr update_addr, Addr update_addr_end){
  if (((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT && 
  ((&uop_cache)->exe_count_type == TYPE_OP_FETCH_ADDR || (&uop_cache)->exe_count_type == TYPE_ISTAGE_FETCH_ADDR
  || (&uop_cache)->exe_count_type == TYPE_CACHE_STORE || (&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS))){
    for (size_t addr = update_addr; addr < update_addr_end+1; addr++)
    {
      uns64 last_value;
      int8 last_line_num;
      Flag has_history = find_last_quanta(&uop_cache, addr, &last_value, &last_line_num, 0);
      if (has_history){
        uns64 updated_value = last_value+1;
        void* data;
        update_quanta(&uop_cache, addr, updated_value, last_line_num, 0, data, FALSE);
      } else {
        initialize_quanta(&uop_cache, addr, 1, 0);
      }
      has_history = find_last_quanta(&uop_cache, addr, &last_value, last_line_num, 0);
      // printf("Istage: Addr 0x%s, count %llu\n", hexstr64s(addr), last_value);

      if ((&uop_cache)->repl_policy == REPL_EXECUTION_COUNT && (&uop_cache)->exe_count_type == TYPE_CACHE_ACCESS/*TYPE_CACHE_STORE*//*TYPE_CACHE_ACCESS*/){
        fprintf((&uop_cache)->file_pointer, "%llu %llu\n", addr, 1);
      } else if ((&uop_cache)->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD)){
        uns64 updated_value;
        int8 last_line_num;
        Flag has_history = find_last_quanta((&uop_cache), addr, &updated_value, &last_line_num, 0);
        fprintf((&uop_cache)->file_pointer, "%llu %llu\n", addr, updated_value);
      }
    }
  }

  
  
}

void* add_timestamp_addr(Addr addr){
  (&uop_cache)->timestamp_new_opt++;
  if (UOP_CACHE_ENABLE_RECORD){
    fprintf((&uop_cache)->file_pointer, "%llu %llu\n", addr, (&uop_cache)->timestamp_new_opt);//timestamp);
  }
  (&uop_cache)->pre_addr = addr;
}

void* print_timestamp_addr(Addr addr){
  if ((&uop_cache)->pre_addr!=addr){
      (&uop_cache)->timestamp_new_opt++;
    // // printf("0x%s %llu %llu\n", hexstr64s(addr), (&uop_cache)->timestamp_new_opt, cycle_count);
    // printf("0x%s %llu\n", hexstr64s(addr), (&uop_cache)->timestamp_new_opt);
  } 
  (&uop_cache)->pre_addr = addr;
}
