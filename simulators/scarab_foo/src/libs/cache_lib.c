/* Copyright 2020 HPS/SAFARI Research Groups
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/***************************************************************************************
 * File         : libs/cache_lib.c
 * Author       : HPS Research Group
 * Date         : 2/6/1998
 * Description  : This is a library of cache functions.
 ***************************************************************************************/

#include <stdlib.h>
#include "debug/debug_macros.h"
#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"

#include "core.param.h"
#include "debug/debug.param.h"
#include "general.param.h"
#include "libs/cache_lib.h"
#include "memory/memory.param.h"
#include "op.h"
#include "statistics.h" // For local pitfall detection

#include <search.h>
#include <math.h>
#define DUMMY_ADDR_OPT_FETCH 0x1
#define CHOSEN_SET 208
#define PRINT_FUNC 0x0//((strcmp(cache->name, "UOP_CACHE")==0) && (set==CHOSEN_SET)) //(cache->repl_policy == REPL_DEBUG && strcmp(cache->name, "UOP_CACHE")==0)//0x0//(strcmp(cache->name, "UOP_CACHE")==0)
#define DISABLE_OPT 0x0
#define SAT_INC(x,max)  (x<max)?x+1:x
#define SAT_DEC(x)      (x>0)?x-1:x
// For SHiP++ 
// SHCT. Signature History Counter Table
// per-core 16K entry. 14-bit signature = 16k entry. 3-bit per entry
#define maxSHCTR 7
#define SHCT_SIZE (1<<14)
#define NUM_LEADER_SETS   64
#define RRIP_OVERRIDE_PERC   0

// Mockingjay
#define LOG2_BLOCK_SIZE 6 //(log2(56)) // 56bits*8
#define LOG2_SET(num_sets) ((int)(log2(num_sets))) // 8
#define LOG2_SIZE(num_sets,assoc)  ((int)(LOG2_SET(num_sets) + log2(assoc) + LOG2_BLOCK_SIZE)) // 17 // (set, way)
#define LOG2_SAMPLED_SETS(num_sets,assoc) ((int)(LOG2_SIZE(num_sets,assoc) - 16)) // 4 16)) // 1 // 9 //16)

#define HISTORY 8
#define GRANULARITY 8

#define INF_RD(assoc) (assoc * HISTORY - 1) // way // 63
#define INF_ETR(assoc) ((int)((assoc * HISTORY / GRANULARITY) - 1)) // way // 7
#define MAX_RD(assoc) (INF_RD(assoc) - 22) // 41

#define SAMPLED_CACHE_WAYS 5
#define LOG2_SAMPLED_CACHE_SETS 4
#define SAMPLED_CACHE_TAG_BITS(num_sets,assoc) (31 - LOG2_SIZE(num_sets,assoc)) // 14
#define PC_SIGNATURE_BITS(num_sets,assoc) (LOG2_SIZE(num_sets,assoc) - 10) // 7
#define TIMESTAMP_BITS 8

#define NUM_CPUS 1
#define TEMP_DIFFERENCE ((double)1.0/16.0)
#define FLEXMIN_PENALTY (2.0 - (double)(log2(NUM_CPUS))/4.0) // 2
#define MAX(x, y) (((x) > (y)) ? (x) : (y))
#define MIN(x, y) (((x) < (y)) ? (x) : (y))

// DeleteMe
#define ideal_num_entries 256

/**************************************************************************************/
/* Macros */

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_CACHE_LIB, ##args)


/**************************************************************************************/
/* Static Prototypes */

static inline uns  cache_index(Cache* cache, Addr addr, Addr* tag,
                               Addr* line_addr);
static inline void update_repl_policy(Cache*, Cache_Entry*, uns, uns, Flag, Load_Type, Addr);
static inline Cache_Entry* find_repl_entry(Cache*, uns8, uns, uns*, Load_Type, Addr);

/* for ideal replacement */
static inline void*        access_unsure_lines(Cache*, uns, Addr, Flag);
static inline Cache_Entry* insert_sure_line(Cache*, uns, Addr);
static inline void         invalidate_unsure_line(Cache*, uns, Addr);

/* for OPT replacement */
static inline void* add_access_to_liveness_history(Cache* cache, uns set, Addr tag);
static inline Flag should_opt_cache(Cache* cache, uns set, Addr tag, Addr addr, uns lines_found);
static inline int key_compare(const void *l, const void *r);
static inline int key_compare_concat(const void *l, const void *r);

/* for DRRIP replcement */
static inline void after_eviction(Cache* cache, uns set, uns* way, Flag has_invalid);

/* for mockingjay replacement */
Flag is_sampled_set(int32 set, int32 num_sets, int32 assoc);
uns64 CRC_HASH( uns64 _blockAddress );
uns64 get_pc_signature(uns64 pc, Flag hit, Flag prefetch, uns32 core, int32 num_sets, int32 assoc);
uns32 get_sampled_cache_index(uns64 full_addr, uns32 set, uns num_sets);
uns64 get_sampled_cache_tag(uns64 x, uns num_sets, uns assoc);
int32 search_sampled_cache(uns64 blockAddress, uns32 set, Cache *cache);
void detrain(uns32 set, int32 way, Cache *cache);
int32 temporal_difference(int32 init, int32 sample32, int32 assoc);
int32 increment_timestamp(int32 input);
int32 time_elapsed(int32 global, int32 local);
void m_update_replacement_state(uns32 cpu, uns32 set, uns32 way, uns64 full_addr, uns64 pc, Load_Type load_type, Flag hit, Cache *cache, Addr *line_addr);

uns64 find_future_timestamp_file(FILE *demand_record, Cache* cache, Addr addr, uns64 cur_timestamp);

/**************************************************************************************/
/* Global Variables */

char rand_repl_state[31];
// Hash_Table rdp; /* for mockingjay replacement policy */ 
// Hash_Table sampled_cache; /* for mockingjay replacement policy */ 
// static FILE*      new_opt_stream;
uns64 timestamp_new_opt;

/**************************************************************************************/

static inline uns cache_index(Cache* cache, Addr addr, Addr* tag,
                              Addr* line_addr) {
  if(cache->tag_incl_offset){ /* This means Byte addressable Uop cache. */
    ASSERT(0, strcmp(cache->name, "UOP_CACHE") == 0);
    *tag = addr & ~(cache->set_mask << cache->shift_bits);
    *line_addr = addr;
  }else{
    *tag = addr >> cache->shift_bits & cache->tag_mask;
    *line_addr = addr & ~cache->offset_mask;
  }
  return addr >> cache->shift_bits & cache->set_mask;
}

uns ext_cache_index(Cache* cache, Addr addr, Addr* tag, Addr* line_addr) {
  return cache_index(cache, addr, tag, line_addr);
}


/**************************************************************************************/
/* init_cache: */

void init_cache(Cache* cache, const char* name, uns cache_size, uns assoc,
                uns line_size, uns data_size, Repl_Policy repl_policy) {
  uns num_lines = cache_size / line_size;
  uns num_sets  = cache_size / line_size / assoc;
  uns ii, jj;

  DEBUG(0, "Initializing cache called '%s'.\n", name);

  /* set the basic parameters */
  strncpy(cache->name, name, MAX_STR_LENGTH);
  cache->data_size   = data_size;
  cache->num_lines   = num_lines;
  cache->assoc       = assoc;
  cache->num_sets    = num_sets;
  cache->line_size   = line_size;
  cache->repl_policy = repl_policy;

  /* set some fields to make indexing quick */
  cache->set_bits    = LOG2(num_sets);
  cache->shift_bits  = LOG2(line_size);               /* use for shift amt. */
  cache->set_mask    = N_BIT_MASK(LOG2(num_sets));    /* use after shifting */
  cache->tag_mask    = ~cache->set_mask;              /* use after shifting */
  cache->offset_mask = N_BIT_MASK(cache->shift_bits); /* use before shifting */

  cache->tag_incl_offset = FALSE; /* Default not use Byte Addressable */

  /* allocate memory for NMRU replacement counters  */
  cache->repl_ctrs = (uns*)calloc(num_sets, sizeof(uns));

  /* allocate memory for all the sets (pointers to line arrays)  */
  cache->entries = (Cache_Entry**)malloc(sizeof(Cache_Entry*) * num_sets);

  /* Allocate memory for set repl bit*/
  cache->entiresLRU = (Flag*) malloc(sizeof(Flag) * num_sets);
  for(int iii = 0; iii < num_sets; ++iii) cache->entiresLRU[iii] = FALSE;

  /* allocate memory for the unsure lists (if necessary) */
  if(cache->repl_policy == REPL_IDEAL)
    cache->unsure_lists = (List*)malloc(sizeof(List) * num_sets);

  if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT)) {
    cache->OPT = (OPTgen*)malloc(sizeof(OPTgen) * num_sets);
    cache->num_cache_opt = 0;
    cache->num_dont_cache_opt = 0;
    // cache->addr_info_lists = (Addr_Info*)malloc(sizeof(Addr_Info) * num_sets);
    for(jj = 0; jj < num_sets; jj++) {
      cache->OPT[jj].opt_access = 0;
      cache->OPT[jj].liveness_history = (uns*)calloc(OPTGEN_VECTOR_SIZE, sizeof(uns));
      cache->OPT[jj].perset_mytimer = 0;
      cache->OPT[jj].tag_history = (Addr*)calloc(OPTGEN_VECTOR_SIZE, sizeof(Addr));
      // cache->addr_info_lists[jj].tag =  (Addr*)calloc(OPTGEN_VECTOR_SIZE, sizeof(Addr));
    }
    cache->map_root = 0;
  }

  if(cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_OPT
  ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS||cache->repl_policy == REPL_COMPRATE
  ||cache->repl_policy == REPL_PMC) {
    if (cache->repl_policy == REPL_NEW_OPT && UOP_CACHE_ENABLE_RECORD){
      cache->file_pointer = file_tag_fopen(NULL, "new_opt_stream", "w");
      cache->file_pointer_backup = file_tag_fopen(NULL, "access1", "w");
      cache->pre_timestamp_new_opt = 0;
      cache->timestamp_new_opt = 0;//-640; //0;
    } else if (cache->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){
      cache->file_pointer = file_tag_fopen(NULL, "new_opt_stream", "r");
      cache->file_pointer_backup = file_tag_fopen(NULL, "access2", "w");
      cache->file_pointer_switch = file_tag_fopen(NULL, "access3", "w");// make use of the switch file pointer
      cache->pre_timestamp_new_opt = 0;
      cache->timestamp_new_opt = 0;//-640; //0;
      cache->enable_timestamp = FALSE;
      cache->timestamp_new_opt_insert = 0;
    } else if (cache->repl_policy == REPL_OPT){
      cache->file_pointer = file_tag_fopen(NULL, "opt_stream", "w");
    } else if (cache->repl_policy == REPL_EXECUTION_COUNT){
      cache->file_pointer = file_tag_fopen(NULL, "execution_count_stream", "w");
    } else if (cache->repl_policy == REPL_CACHEACCESS && UOP_CACHE_ENABLE_RECORD){
      cache->file_pointer = file_tag_fopen(NULL, "cacheaccess_stream", "w");
    } else if (cache->repl_policy == REPL_CACHEACCESS && (!UOP_CACHE_ENABLE_RECORD) && (UOP_CACHE_EXE_COUNT==1||UOP_CACHE_EXE_COUNT == 4)){
      cache->file_pointer = file_noouttag_fopen(NULL, FOOTPRINT, "r");
    } else if (cache->repl_policy == REPL_CACHEACCESS && (!UOP_CACHE_ENABLE_RECORD) && UOP_CACHE_EXE_COUNT==5){
      cache->file_pointer = file_tag_fopen(NULL, "opt_stream", "r");
    } else if (cache->repl_policy == REPL_COMPRATE && UOP_CACHE_USAGE_COUNT == TYPE_TRANSIENT){
      cache->file_pointer = file_tag_fopen(NULL, "execution_count_stream", "w");
    } else if (cache->repl_policy == REPL_COMPRATE && UOP_CACHE_ENABLE_RECORD && UOP_CACHE_USAGE_COUNT == TYPE_WHOLE){
      cache->file_pointer = file_tag_fopen(NULL, "cacheaccess_stream", "w");
    } else if (cache->repl_policy == REPL_COMPRATE && (!UOP_CACHE_ENABLE_RECORD) && UOP_CACHE_USAGE_COUNT == TYPE_WHOLE){
      cache->file_pointer = file_noouttag_fopen(NULL, FOOTPRINT, "r");
    } else if (cache->repl_policy == REPL_PMC && UOP_CACHE_USAGE_COUNT == TYPE_TRANSIENT){
      cache->file_pointer = file_tag_fopen(NULL, "pmc_count_stream", "w");
      cache->file_pointer_switch = file_tag_fopen(NULL, "pmc_count_stream_switch", "w");
    } else if (cache->repl_policy == REPL_PMC && UOP_CACHE_ENABLE_RECORD && UOP_CACHE_USAGE_COUNT == TYPE_WHOLE){
      cache->file_pointer = file_noouttag_fopen(NULL, FOOTPRINT, "w");
      cache->file_pointer_switch = file_noouttag_fopen(NULL, FOOTPRINT_SWITCH, "w");
    } else if (cache->repl_policy == REPL_PMC && (!UOP_CACHE_ENABLE_RECORD) && UOP_CACHE_USAGE_COUNT == TYPE_WHOLE){
      cache->file_pointer = file_noouttag_fopen(NULL, FOOTPRINT, "r");
      cache->file_pointer_switch = file_noouttag_fopen(NULL, FOOTPRINT_SWITCH, "r");
    } 
    ASSERTM(0, cache->file_pointer, "Could not open OPT log file\n");
  }

  /* allocate memory for all of the lines in each set */
  for(ii = 0; ii < num_sets; ii++) {
    cache->entries[ii] = (Cache_Entry*)malloc(sizeof(Cache_Entry) * assoc);
    /* allocate memory for all of the data elements in each line */
    for(jj = 0; jj < assoc; jj++) {
      cache->entries[ii][jj].valid = FALSE;
      if(data_size) {
        cache->entries[ii][jj].data = (void*)malloc(data_size);
        memset(cache->entries[ii][jj].data, 0, data_size);
      } else
        cache->entries[ii][jj].data = INIT_CACHE_DATA_VALUE;
    }

    /* initialize the unsure lists (if necessary) */
    if(cache->repl_policy == REPL_IDEAL) {
      char list_name[MAX_STR_LENGTH+1];
      snprintf(list_name, MAX_STR_LENGTH, "%.*s unsure [%d]",
               MAX_STR_LENGTH - 20, cache->name, ii);  // 21 guaruntees the
                                                       // string will always be
                                                       // smaller than
                                                       // MAX_STR_LENGTH
      init_list(&cache->unsure_lists[ii], list_name, sizeof(Cache_Entry),
                USE_UNSURE_FREE_LISTS);
    }
  }
  cache->num_demand_access = 0;
  cache->last_update       = 0;

  if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP || 
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
    for(ii = 0; ii < num_sets; ii++) {
      /* allocate rrpv for all of the data elements in each line */
      for(jj = 0; jj < assoc; jj++) {
        cache->entries[ii][jj].rrpv = pow(2, RRIP_BIT_SIZE)-1;
      }
    }
  }
  if(cache->repl_policy == REPL_DRRIP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE == REPL_DRRIP)) {
    cache->brrip_counter = 0;
    cache->psel_counter = 0;
    cache->drrip_repl_policy = (uns*)malloc(sizeof(uns) * num_sets);
    for(ii = 0; ii < num_sets; ii++) {
      if ((ii + 1) % (num_sets / 32 - 1) == 0)
        cache->drrip_repl_policy[ii] = TYPE_BRRIP;
      else if (ii % (num_sets / 32 + 1) == 0)
        cache->drrip_repl_policy[ii] = TYPE_SRRIP;
      else
        cache->drrip_repl_policy[ii] = TYPE_FOLLOWER;
    }
  }
  if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE == REPL_SHIP_PP)) {
    for(ii = 0; ii < num_sets; ii++) {
      for(jj = 0; jj < assoc; jj++) {
            cache->entries[ii][jj].rrpv = pow(2, RRIP_BIT_SIZE)-1;
            cache->entries[ii][jj].line_reuse = FALSE;
            cache->entries[ii][jj].pref = FALSE;
            cache->entries[ii][jj].line_sig = 0;
        }
    }
    cache->SHCT = (uns32*)malloc(sizeof(uns32) * SHCT_SIZE);
    // Assume weakly re-use start
    for(jj = 0; jj < SHCT_SIZE; jj++) {
      cache->SHCT[jj] = 1;
    }

    uns leaders=0;
    cache->ship_sample = (uns32*)malloc(sizeof(uns32) * num_sets);
    for(jj = 0; jj < num_sets; jj++) {
      cache->ship_sample[jj] = 0;
    }
    while(leaders<NUM_LEADER_SETS){
      uns randval = rand()%num_sets;
      //printf("Rand number in init is %u\n", randval);
      if(cache->ship_sample[randval]==0){
        cache->ship_sample[randval]=1;
        leaders++;
      }
    }
  }

  if (PRINT_FUNC && (strcmp(cache->name, "UOP_CACHE")==0)){
    printf("cache->name is %s in init_cache\n", cache->name);
    printf("LOG2_SET(num_sets) = %u %d %u \n", (LOG2_SET(num_sets)),  10000000, (log2(num_sets)));
    printf("LOG2_SET(num_sets) = %u %u \n", (log2(num_sets)), (LOG2_SET(num_sets)));
    printf("num_sets = %u, assoc = %u\n", num_sets, assoc);
    printf("LOG2_SIZE(num_sets,assoc) = %u\n", LOG2_SIZE(num_sets,assoc));
    printf("LOG2_SAMPLED_SETS(num_sets,assoc) = %u\n", LOG2_SAMPLED_SETS(num_sets,assoc));
    printf("INF_RD(assoc) = %u\n", INF_RD(assoc));
    printf("INF_ETR(assoc) = %u\n", INF_ETR(assoc));
    printf("MAX_RD(assoc) = %u\n", MAX_RD(assoc));
    printf("SAMPLED_CACHE_TAG_BITS(num_sets,assoc) = %u\n", SAMPLED_CACHE_TAG_BITS(num_sets,assoc));
    printf("PC_SIGNATURE_BITS(num_sets,assoc) = %u\n", PC_SIGNATURE_BITS(num_sets,assoc));
    // printf(" = %d\n", );
  }

  if(cache->repl_policy == REPL_MOCKINGJAY) {
    cache->etr_clock = (int32*)malloc(sizeof(int32) * (num_sets));
    cache->current_timestamp = (int32*)malloc(sizeof(int32) * (num_sets));
    for(ii = 0; ii < num_sets; ii++) {
      cache->etr_clock[ii] = GRANULARITY;
      cache->current_timestamp[ii] = 0;
    }
    
    // init_hash_table(&rdp, "Reuse Distance Predictor (RDP)", 15000000, 
    //                 sizeof(RDP_Entry));
    // init_hash_table(&sampled_cache, "Sampled Cache", 15000000, 
    //                 sizeof(SampledCacheLine) * SAMPLED_CACHE_WAYS); 
    //sizeof(SampledCacheLine*));
    uns modifier = 1;
    for (size_t i = 0; i < LOG2_SET(num_sets); i++)
    {
      modifier = modifier<<1;
    }
    // uns modifier = 1 << LOG2_SET(num_sets);
    uns limit = 1 << LOG2_SAMPLED_CACHE_SETS;
    cache->sampled_cache = (SampledCacheLine**)malloc(sizeof(SampledCacheLine*) * (num_sets + modifier*limit));
    uns32 true_limit = num_sets<<(LOG2_BLOCK_SIZE)|((1<<LOG2_BLOCK_SIZE)-1);
    if (PRINT_FUNC)  printf("Total sets are 0x%s, true limit is 0x%s, %llu\n", hexstr64s(num_sets + modifier*limit), hexstr64s(true_limit), true_limit);
    for(uns32 ii = 0; ii < /*true_limit*/ num_sets + modifier*limit; ii++) {
        // if (is_sampled_set(set, num_sets, assoc)) { 
            if (PRINT_FUNC) printf("Will sample set 0x%s\n", hexstr64s(ii)); 
              cache->sampled_cache[ii] = (SampledCacheLine*)malloc(sizeof(SampledCacheLine) * SAMPLED_CACHE_WAYS);
              // if (PRINT_FUNC) printf("Correpsonding sampled set is 0x%s\n", hexstr64s(ii)); 
        // }
    }
    cache->map_root = 0;
    // for(uns32 set = 0; set < num_sets; set++) {
    //     if (is_sampled_set(set, num_sets, assoc)) { 
    //         printf("Will sample set 0x%s\n", hexstr64s(set)); 
    //         for (int i = 0; i < limit; i++) {
    //           // Flag new_entry;
    //           // hash_table_access_create(&sampled_cache, set + modifier*i, 
    //           //                   &new_entry);
    //           cache->sampled_cache[set + modifier*i] = (SampledCacheLine*)malloc(sizeof(SampledCacheLine) * SAMPLED_CACHE_WAYS);
    //           printf("Correpsonding sampled set is 0x%s\n", hexstr64s(set + modifier*i)); 
    //         }
    //     }
    // }
    // sampled_cache = (SampledCacheLine**)malloc(sizeof(SampledCacheLine*) * num_sets);
    // for(ii = 0; ii < num_sets; ii++) {
    //   sampled_cache[ii] = (SampledCacheLine*)malloc(sizeof(SampledCacheLine) * SAMPLED_CACHE_WAYS);
    // }
  }

  if(cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS
  ||cache->repl_policy == REPL_COMPRATE||cache->repl_policy == REPL_PMC || cache->repl_policy == REPL_FOO || cache->repl_policy == REPL_HIT_TIMES){
    cache->map_root = 0;
    cache->exe_count_type = UOP_CACHE_EXE_COUNT;
  }

  if(cache->repl_policy == REPL_NEW_OPT && (!UOP_CACHE_ENABLE_RECORD)){
    cache->map_root = 0;
    read_init();
  }

  if(cache->repl_policy == REPL_CACHEACCESS && (!UOP_CACHE_ENABLE_RECORD)){
    if(cache->exe_count_type == TYPE_CACHE_ACCESS||cache->exe_count_type == TYPE_CACHE_STORE){
      // // read_init_cacheaccess();
      // read_init_cacheaccess_granularities();
      read_init_cacheaccess_concat();
    } else if (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE){
      read_init_ratecacheaccess();
    } 
  }

  if(cache->repl_policy == REPL_COMPRATE && UOP_CACHE_USAGE_COUNT == TYPE_WHOLE && (!UOP_CACHE_ENABLE_RECORD)){
    // // read_init_ratecacheaccess_concat();
    // read_init_cacheaccess_concat();
    read_init_ratecacheaccess();
  }

  if(cache->repl_policy == REPL_PMC){//} && UOP_CACHE_USAGE_COUNT == TYPE_WHOLE && (!UOP_CACHE_ENABLE_RECORD)){
    if (UOP_CACHE_USAGE_COUNT == TYPE_WHOLE && (!UOP_CACHE_ENABLE_RECORD))
    read_init_cacheaccess_granularities();
    read_init_cacheaccess_granularities_switch();
    cache->pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->pre_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->pre_pre_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->pre_pre_pre_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->sec_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->sec_pre_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->sec_pre_pre_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    cache->sec_pre_pre_pre_pre_chosen_entry = (int16*)malloc(sizeof(int16) * cache->num_sets);
    for (int16 ii = 0; ii < cache->num_sets; ii++)
    {
      cache->pre_chosen_entry[ii] = -1;
      cache->pre_pre_chosen_entry[ii] = -1;
      cache->pre_pre_pre_chosen_entry[ii] = -1;
      cache->pre_pre_pre_pre_chosen_entry[ii] = -1;
      cache->sec_pre_chosen_entry[ii] = -1;
      cache->sec_pre_pre_chosen_entry[ii] = -1;
      cache->sec_pre_pre_pre_chosen_entry[ii] = -1;
      cache->sec_pre_pre_pre_pre_chosen_entry[ii] = -1;
    }
    
  }

  /* For cache partitioning */
  if(cache->repl_policy == REPL_PARTITION) {
    cache->num_ways_allocted_core = (uns*)malloc(sizeof(uns) * NUM_CORES);
    cache->num_ways_occupied_core = (uns*)malloc(sizeof(uns) * NUM_CORES);
    cache->lru_index_core         = (uns*)malloc(sizeof(uns) * NUM_CORES);
    cache->lru_time_core = (Counter*)malloc(sizeof(Counter) * NUM_CORES);
  }

  /* allocate memory for the back-up lists (if necessary) */
  if(cache->repl_policy == REPL_SHADOW_IDEAL) {
    cache->shadow_entries = (Cache_Entry**)malloc(sizeof(Cache_Entry*) *
                                                  num_sets);
    /* allocate memory for all of the lines in each set */
    for(ii = 0; ii < num_sets; ii++) {
      cache->shadow_entries[ii] = (Cache_Entry*)malloc(sizeof(Cache_Entry) *
                                                       assoc);
      /* allocate memory for all of the data elements in each line */
      for(jj = 0; jj < assoc; jj++) {
        cache->shadow_entries[ii][jj].valid = FALSE;
        if(data_size) {
          cache->shadow_entries[ii][jj].data = (void*)malloc(data_size);
          memset(cache->shadow_entries[ii][jj].data, 0, data_size);
        } else
          cache->shadow_entries[ii][jj].data = INIT_CACHE_DATA_VALUE;
      }
    }
  }

  else if(cache->repl_policy == REPL_IDEAL_STORAGE) {
    cache->shadow_entries = (Cache_Entry**)malloc(sizeof(Cache_Entry*) *
                                                  num_sets);
    cache->queue_end      = (uns*)malloc(sizeof(uns) * num_sets);
    /* allocate memory for all of the lines in each set */
    for(ii = 0; ii < num_sets; ii++) {
      cache->shadow_entries[ii] = (Cache_Entry*)malloc(sizeof(Cache_Entry) *
                                                       ideal_num_entries);
      /* allocate memory for all of the data elements in each line */
      for(jj = 0; jj < ideal_num_entries; jj++) {
        cache->shadow_entries[ii][jj].valid = FALSE;
        if(data_size) {
          cache->shadow_entries[ii][jj].data = (void*)malloc(data_size);
          memset(cache->shadow_entries[ii][jj].data, 0, data_size);
        } else
          cache->shadow_entries[ii][jj].data = INIT_CACHE_DATA_VALUE;
      }
      cache->queue_end[ii] = 0;
    }
  }
}

/**************************************************************************************/
/* cache_access: Does a cache lookup based on the address.  Returns a pointer
 * to the cache line data if it is found.  */

void* cache_access(Cache* cache, Addr addr, Addr* line_addr, Flag update_repl, Load_Type load_type) {
  Addr tag;
  uns  set = cache_index(cache, addr, &tag, line_addr);
  uns  ii;

  //if (PRINT_FUNC)printf("Enter cache_access\n");
  // printf("load_type in cache_access out is %u\n", load_type);   

  // if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr) {
  //   DEBUG(0, "Checking liveness history '%s' at (set %u)\n", cache->name, set);
  //   add_access_to_liveness_history(cache, set, tag);
  //   // Initialize quanta
  //   // initialize_quanta(cache, addr, cache->OPT[set].perset_mytimer);
  //   if (PRINT_FUNC){
  //       printf("cache_access Check access history '%s' at (set %u), addr 0x%s, tag 0x%s\n", cache->name, set, hexstr64s(addr), hexstr64s(tag));
  //       for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
  //         printf("%d ", cache->OPT[set].liveness_history[ii]);
  //       }
  //       printf("\n");
  //       printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
  //   }
  // }

  if(cache->repl_policy == REPL_IDEAL_STORAGE) {
    return access_ideal_storage(cache, set, tag, addr);
  }

  // // if(update_repl){
  // if((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_CACHE_ACCESS/*TYPE_CACHE_STORE*//*TYPE_CACHE_ACCESS*/)||
  // (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && cache->exe_count_type == TYPE_CACHE_ACCESS/*TYPE_CACHE_STORE*/)) {
  //   uns64 last_value;
  //   int8 last_line_num;
  //   Flag has_history = find_last_quanta(cache, addr, &last_value, &last_line_num, set);
  //   if (has_history){
  //     uns64 updated_value = last_value+1;
  //     void* data;
  //     update_quanta(cache, addr, updated_value, 0, set, data, FALSE);
  //   } else {
  //     initialize_quanta(cache, addr, 1, set);
  //   }
  //   has_history = find_last_quanta(cache, addr, &last_value, &last_line_num, set);
  //   if (PRINT_FUNC) 
  //   printf("Cache access: Addr 0x%s, count %llu\n", hexstr64s(addr), last_value);

  //   if (cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_CACHE_ACCESS/*TYPE_CACHE_STORE*//*TYPE_CACHE_ACCESS*/){
  //     fprintf(cache->file_pointer, "%llu %llu\n", addr, 1);
  //   } else if (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD)){
  //     uns64 updated_value;
  //     int8 last_line_num;
  //     Flag has_history = find_last_quanta(cache, addr, &updated_value, &last_line_num, set);
  //     fprintf(cache->file_pointer, "%llu %llu\n", addr, updated_value);
  //   }
  // }
  // // }

  // if(cache->repl_policy == REPL_NEW_OPT){//} && UOP_CACHE_ENABLE_RECORD){
  //   if (UOP_CACHE_ENABLE_RECORD){
  //     fprintf(cache->file_pointer, "%llu %llu\n", addr, timestamp_new_opt);
  //   }
  //   fprintf(cache->file_pointer_backup, "%llu %llu\n", addr, timestamp_new_opt);
  //   timestamp_new_opt++;
  // }
  // printf("Cache insert line lookup: %llu %llu\n", addr, cache->timestamp_new_opt);

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];

    if(line->valid && line->tag == tag) {
      /* update replacement state if necessary */
      ASSERT(0, line->data);
      DEBUG(0, "Found line in cache '%s' at (set %u, way %u, base 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base));
      if (PRINT_FUNC)
      printf("In cache_access, found line in cache '%s' at (set %u, way %u, base 0x%s, tag 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base), hexstr64s(line->tag));

      if(update_repl) {
        if(line->pref) {
          line->pref = FALSE;
        }
        cache->num_demand_access++;
        update_repl_policy(cache, line, set, ii, FALSE, load_type, addr);
      }

      if(cache->repl_policy == REPL_MOCKINGJAY){
        // TODOSD full_addr == pc ?
        if (PRINT_FUNC) printf("cache->name is %s\n", cache->name);
        m_update_replacement_state(0, set, ii, addr, addr, load_type, TRUE, cache, line_addr); //victim_addr not used
      }

      if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP || 
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
        cache->entries[set][ii].rrpv = 0;

        if (PRINT_FUNC){
              printf("cache_access Checking access1 '%s' at (set %u)\n", cache->name, set);
              for(int ii=0;ii<cache->assoc;ii++){
                printf("%d ", cache->entries[set][ii].rrpv);
              }
              printf("\n");
          }
      }

      if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SHIP_PP)){
        uns32 sig   = cache->entries[set][ii].line_sig;
        if( load_type != TYPE_WRITEBACK ) 
          {

            if( (load_type == TYPE_PREF) && line->pref )
            {
                // printf("Rand number in cache_access out is %u\n", rand()%100);              
                if( (cache->ship_sample[set] == 1) && (rand()%100 <5)) 
                {
                    // uns32 fill_cpu = cache->entries[set][ii].fill_core;
                    // printf("Rand number in cache_access is %u\n", rand()%100);
                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
            else 
            {
                cache->entries[set][ii].rrpv = 0;

                if( cache->entries[set][ii].pref==1 )
                {
                    cache->entries[set][ii].rrpv = pow(2, RRIP_BIT_SIZE)-1;
                    cache->entries[set][ii].pref = FALSE;
                    // total_prefetch_downgrades++;
                }

                if( (cache->ship_sample[set] == 1) && (cache->entries[set][ii].line_reuse==FALSE) ) 
                {
                    // uns fill_cpu = cache->entries[set][ii].fill_core;

                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
          }
      }

      if(cache->repl_policy == REPL_OPT||cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_MOCKINGJAY
      ||cache->repl_policy == REPL_TRUE_LRU||cache->repl_policy == REPL_SRRIP||cache->repl_policy == REPL_DRRIP||cache->repl_policy == REPL_SHIP_PP
      ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS
      ||cache->repl_policy == REPL_COMPRATE||cache->repl_policy == REPL_PMC||cache->repl_policy == REPL_FOO||cache->repl_policy == REPL_HIT_TIMES){
        cache->entries[set][ii].addr = addr;
      }  

      if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC) printf("cacheaccess Cache hits in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      
      // if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr) {
      //   // Check if we should cache or not first
      //   Flag should_cache = should_opt_cache(cache, set, tag, addr);
      //   // Update quanta and timer
      //   update_quanta(cache, addr, cache->OPT[set].perset_mytimer, set);
      //   cache->OPT[set].perset_mytimer = (cache->OPT[set].perset_mytimer+1) % (OPTGEN_VECTOR_SIZE); //TIMER_SIZE;

      //   if(UOP_OPT_CACHE_ACCESS)
      //   fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, should_cache, 1);  

      //   // It's a hit but REPL_OPT judges it to be miss
      //   if (should_cache==FALSE){
      //     DEBUG(0, "Cache hits but REPL_OPT treats miss in set %u in cache '%s' base 0x%s\n", set,
      //         cache->name, hexstr64s(addr));
      //     printf("cacheaccess Cache hits but REPL_OPT treats miss in set %u in cache '%s' base 0x%s\n", set,
      //         cache->name, hexstr64s(addr));
      //     if (PRINT_FUNC){
      //         printf("cache_access Checking access1 '%s' at (set %u)\n", cache->name, set);
      //         for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
      //           printf("%d ", cache->OPT[set].liveness_history[ii]);
      //         }
      //         printf("\n");
      //         printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
      //     }
      //     //(cache, addr, &line);
      //     return NULL;
      //   }

        
      // }

      if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
        fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 1, 1); 
      }

      return line->data;
    }
  }

  if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccess Cache misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
  /*if it's a miss*/
  // if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr) {
  //   // Check if we should cache or not first
  //   Flag should_cache = should_opt_cache(cache, set, tag, addr);
  //   // Initialize quanta
  //   // initialize_quanta(cache, addr, cache->OPT[set].perset_mytimer);
  //   cache->OPT[set].perset_mytimer = (cache->OPT[set].perset_mytimer+1) % (OPTGEN_VECTOR_SIZE); 

  //   if(UOP_OPT_CACHE_ACCESS)
  //   fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, should_cache, 1);  
  //   /*but OPT points to hit*/
  //   if (should_cache==TRUE){
  //     // TODOSD shoud fix
  //     printf("cacheaccess Cache misses but REPL_OPT treats hit base 0x%s\n", hexstr64s(addr));
  //     if (PRINT_FUNC){
  //       printf("cache_access Checking access2 '%s' at (set %u)\n", cache->name, set);
  //       for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
  //         printf("%d ", cache->OPT[set].liveness_history[ii]);
  //       }
  //       printf("\n");
  //       printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
  //     }
  //     // pretend we have data now
  //     return DUMMY_ADDR_OPT_FETCH; //NULL; //DUMMY line value defined in icache_stage
  //   }
  // }
  if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 0, 1); 
  }
  /* if it's a miss and we're doing ideal replacement, look in the unsure list
   */
  if(cache->repl_policy == REPL_IDEAL) {
    DEBUG(0, "Checking unsure list '%s' at (set %u)\n", cache->name, set);
    return access_unsure_lines(cache, set, tag, update_repl);
  }

  if(cache->repl_policy == REPL_SHADOW_IDEAL) {
    DEBUG(0, "Checking shadow cache '%s' at (set %u), base 0x%s\n", cache->name,
          set, hexstr64s(addr));
    return access_shadow_lines(cache, set, tag);
  }

  if (cache->pre_addr==addr) cache->pre_addr_opt = 0;//addr;
  DEBUG(0, "Didn't find line in set %u in cache '%s' base 0x%s\n", set,
        cache->name, hexstr64s(addr));
  //if (PRINT_FUNC)printf("Exit cache_access\n");
  return NULL;
}

/**************************************************************************************/
/* cache_access: Does a cache lookup based on the address in order to update repl
 * Needed for uop cache, where a single PW entry can span multiple lines.
 * Touch/update update_repl for ALL lines containing this PW
 */

int cache_access_all(Cache* cache, Addr addr, Addr* line_addr, Flag update_repl, void** line_data, Load_Type load_type) {
  Addr tag;
  uns  set = cache_index(cache, addr, &tag, line_addr);
  uns  ii;
  int lines_found = 0;
  Flag cache_real_hit = FALSE;

  if (PRINT_FUNC)
  printf("Enter cache_access_all\n");

  if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr) {
    DEBUG(0, "Checking liveness history '%s' at (set %u)\n", cache->name, set);
    add_access_to_liveness_history(cache, set, tag);
    // Initialize quanta
    // initialize_quanta(cache, addr, cache->OPT[set].perset_mytimer);
    if (PRINT_FUNC){
        printf("Check access history '%s' at (set %u), addr 0x%s, tag 0x%s\n", cache->name, set, hexstr64s(addr), hexstr64s(tag));
        for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
          printf("%d ", cache->OPT[set].liveness_history[ii]);
        }
        printf("\n");
        printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
    }
  }

  if(cache->repl_policy == REPL_NEW_OPT){//} && UOP_CACHE_ENABLE_RECORD){
    cache->enable_timestamp = TRUE;
    if ((!UOP_CACHE_ENABLE_RECORD) && cache->pre_addr!=addr){
      // cache->timestamp_new_opt++;
      // cache->timestamp_new_opt = find_timestamp(cache, addr);
    }
    if (!UOP_CACHE_ENABLE_RECORD){
      // cache->pre_addr = addr;
      // cache->pre_timestamp_new_opt = cache->timestamp_new_opt;
    } 
    
  }
  // printf("Cache line lookup: %llu %llu\n", addr, cache->timestamp_new_opt);
  int8 target_assoc;
  Addr addr_end;
  

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];

    if(line->valid && line->tag == tag && line->pw_start_addr == addr) {
      /* update replacement state if necessary */
      ASSERT(0, line->data);
      DEBUG(0, "Found line in cache '%s' at (set %u, way %u, base 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base));
      if (PRINT_FUNC)
      printf("Found line in cache '%s' at (set %u, way %u, base 0x%s, tag 0x%s, pw start addr 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base), hexstr64s(line->tag), hexstr64s(line->pw_start_addr));

      if(update_repl) {
        if(line->pref) {
          line->pref = FALSE;
        }
        cache->num_demand_access++;
        update_repl_policy(cache, line, set, ii, FALSE, load_type, addr);
      }
      /* This is for Dynamic Eviction Policy */
      if(update_repl){
        if(cache->repl_policy == REPL_PMC){
          line->pmcWeight = find_entry_count(line->pw_start_addr);
          if(UOP_CACHE_DYNAMIC_EVICT){
            line->exhaustedInterval = TRUE;
            line->evictionHint = getEvictionHints(line->pw_start_addr);
          }
        }
      }
      /****************************************/
      *line_data = line->data;
      if (cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr){
          target_assoc = line_data;
      }
      lines_found++;
      addr_end = line->addr_end;

      if(cache->repl_policy == REPL_MOCKINGJAY){
        // TODOSD full_addr == pc ?
        if (PRINT_FUNC) printf("cache->name is %s\n", cache->name);
        m_update_replacement_state(0, set, ii, addr, addr, load_type, TRUE, cache, line_addr); //victim_addr not used
      }

      if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP ||
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
        cache->entries[set][ii].rrpv = 0;

          if (PRINT_FUNC){
              printf("cache_access_all Checking access1 '%s' at (set %u)\n", cache->name, set);
              for(int ii=0;ii<cache->assoc;ii++){
                printf("%d ", cache->entries[set][ii].rrpv);
              }
              printf("\n");
          }
      }

      if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SHIP_PP)){
        uns32 sig   = cache->entries[set][ii].line_sig;
        if( load_type != TYPE_WRITEBACK ) 
          {

            if( (load_type == TYPE_PREF) && line->pref )
            {
                // printf("Rand number in cache_access_all out is %u\n", rand()%100);              
                if( (cache->ship_sample[set] == 1) && (rand()%100 <5)) 
                {
                    // uns32 fill_cpu = cache->entries[set][ii].fill_core;
                    // printf("Rand number in cache_access_all is %u\n", rand()%100);
                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
            else 
            {
                cache->entries[set][ii].rrpv = 0;

                if( cache->entries[set][ii].pref==1 )
                {
                    cache->entries[set][ii].rrpv = pow(2, RRIP_BIT_SIZE)-1;
                    cache->entries[set][ii].pref = FALSE;
                    // total_prefetch_downgrades++;
                }

                if( (cache->ship_sample[set] == 1) && (cache->entries[set][ii].line_reuse==FALSE) ) 
                {
                    // uns fill_cpu = cache->entries[set][ii].fill_core;

                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
          }
      }

      if(cache->repl_policy == REPL_OPT||cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_MOCKINGJAY||cache->repl_policy == REPL_TRUE_LRU
      ||cache->repl_policy == REPL_SRRIP||cache->repl_policy == REPL_DRRIP||cache->repl_policy == REPL_SHIP_PP){
      // ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS){
        cache->entries[set][ii].addr = addr;
        // cache->entries[set][ii].addr_end = addr+lines_found-1;
      }

      if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccessall Cache hits in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));

      if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
        fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 1, 1); 
      }

      cache_real_hit = TRUE;

    }
  }
  if (strcmp(cache->name, "UOP_CACHE")==0 && cache_real_hit==FALSE  && PRINT_FUNC)  printf("cacheaccessall Cache misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
  
  if (cache->repl_policy == REPL_OPT && cache->pre_addr_opt!=addr){
    uns ori_lines_found = lines_found;
    Flag should_cache = should_opt_cache(cache, set, tag, addr, 1);//ori_lines_found);
    if (should_cache==TRUE && (!cache_real_hit)) {
      if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)  printf("cacheaccessall Cache misses but REPL_OPT treats hit in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      // Cache_Entry* line = &cache->entries[set][1];
      // *line_data = line->data;
      // Cache_Entry* line;
      // line->valid            = TRUE;
      // line->tag              = tag;
      // line->base             = *line_addr;
      // line->addr             = addr;
      // line->last_access_time = sim_time;  // FIXME: this fixes valgrind warnings
      //                                     // in update_prf_
      // line->pw_start_addr = addr;
      Cache_Entry line = cache->entries[set][1];//return_quanta_data(cache, addr); //cache->entries[set][1];
      *line_data = line.data;// **temp_data; //return_quanta_data(cache, addr);//line->data;//return_quanta_data(cache, addr);// store to hash as line num
      lines_found = return_quanta_line_num(cache, addr); //+=
    } else if (should_cache==FALSE && (cache_real_hit)){
      if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccessall Cache hits but REPL_OPT treats misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
          cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      //TODOSD
      *line_data = NULL;
      lines_found=0;
      //cache_invalidate(cache, addr, &line);
    }
    if (PRINT_FUNC){
        printf("Checking access2 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
          printf("%d ", cache->OPT[set].liveness_history[ii]);
        }
        printf("\n");
        printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
    }

    update_quanta(cache, addr, cache->OPT[set].perset_mytimer, 1, set, target_assoc, cache_real_hit);//ori_lines_found, set);
    cache->OPT[set].perset_mytimer = (cache->OPT[set].perset_mytimer+1) % (OPTGEN_VECTOR_SIZE); //TIMER_SIZE;

    if(UOP_OPT_CACHE_ACCESS_ALL)
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, should_cache, 1);  
  }

  if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 0, 1); 
  }
  
  if (lines_found == 0) {
    DEBUG(0, "Didn't find line in set %u in cache '%s' base 0x%s\n", set,
        cache->name, hexstr64s(addr));
  }

  cache->pre_addr_opt = addr;

  if (PRINT_FUNC)
  printf("Exit cache_access_all\n");

  return lines_found;
}
 

/**************************************************************************************/
/* cache_access: Does a cache lookup based on the address in order to update repl
 * Needed for uop cache, where a single PW entry can span multiple lines.
 * Touch/update update_repl for ALL lines containing this PW
 */

int cache_access_size(Cache* cache, Addr addr, Addr addr_end, Addr* line_addr, Flag update_repl, void** line_data, Load_Type load_type) {
  Addr tag;
  uns  set = cache_index(cache, addr, &tag, line_addr);
  uns  ii;
  int lines_found = 0;
  Flag cache_real_hit = FALSE;

  if (PRINT_FUNC)
  printf("Enter cache_access_all\n");

  if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt != addr) {
    DEBUG(0, "Checking liveness history '%s' at (set %u)\n", cache->name, set);
    add_access_to_liveness_history(cache, set, tag);
    // Initialize quanta
    // initialize_quanta(cache, addr, cache->OPT[set].perset_mytimer);
    if (PRINT_FUNC){
        printf("Check access history '%s' at (set %u), addr 0x%s, tag 0x%s\n", cache->name, set, hexstr64s(addr), hexstr64s(tag));
        for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
          printf("%d ", cache->OPT[set].liveness_history[ii]);
        }
        printf("\n");
        printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
    }
  }

  if(cache->repl_policy == REPL_NEW_OPT){
    cache->enable_timestamp = TRUE;
  }
  // printf("Cache line lookup: %llu %llu\n", addr, cache->timestamp_new_opt);
  int8 target_assoc;
  

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];
    if(line->valid && line->tag == tag && line->pw_start_addr == addr && line->addr_end != addr_end) {
      /* This is for FOO's optimization: */
      if(FOO_FIX_LEVEL == FOO_ALL_FIXED || FOO_FIX_LEVEL == FOO_ALL_FIXED_BYPASS){
        if(cache->repl_policy == REPL_FOO){
          ASSERT(0, line->pw_start_addr == addr && line->addr_end != addr_end);
          if(addr_end < line->addr_end){
            /* Stop Insertion */
            // printf("[Opps] Now in cache: %lld, %lld\n", line->pw_start_addr, line->addr_end);
            return -1;
          }
        }
      }
      cache_invalidate_all(cache, addr, &line_addr);
    }else if(line->valid && line->tag == tag && line->pw_start_addr == addr && line->addr_end == addr_end) {
      /* update replacement state if necessary */
      ASSERT(0, line->data);
      DEBUG(0, "Found line in cache '%s' at (set %u, way %u, base 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base));
      if (PRINT_FUNC)
      printf("Found line in cache '%s' at (set %u, way %u, base 0x%s, tag 0x%s, pw start addr 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base), hexstr64s(line->tag), hexstr64s(line->pw_start_addr));

      if(update_repl) {
        if(line->pref) {
          line->pref = FALSE;
        }
        cache->num_demand_access++;
        update_repl_policy(cache, line, set, ii, FALSE, load_type, addr);
      }

      *line_data = line->data;
      if (cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr){
          target_assoc = line_data;
      }
      lines_found++;

      if(cache->repl_policy == REPL_MOCKINGJAY){
        // TODOSD full_addr == pc ?
        if (PRINT_FUNC) printf("cache->name is %s\n", cache->name);
        m_update_replacement_state(0, set, ii, addr, addr, load_type, TRUE, cache, line_addr); //victim_addr not used
      }

      if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP || 
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
        cache->entries[set][ii].rrpv = 0;

          if (PRINT_FUNC){
              printf("cache_access_all Checking access1 '%s' at (set %u)\n", cache->name, set);
              for(int ii=0;ii<cache->assoc;ii++){
                printf("%d ", cache->entries[set][ii].rrpv);
              }
              printf("\n");
          }
      }

      if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SHIP_PP)){
        uns32 sig   = cache->entries[set][ii].line_sig;
        if( load_type != TYPE_WRITEBACK ) 
          {

            if( (load_type == TYPE_PREF) && line->pref )
            {
                // printf("Rand number in cache_access_all out is %u\n", rand()%100);              
                if( (cache->ship_sample[set] == 1) && (rand()%100 <5)) 
                {
                    // uns32 fill_cpu = cache->entries[set][ii].fill_core;
                    // printf("Rand number in cache_access_all is %u\n", rand()%100);
                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
            else 
            {
                cache->entries[set][ii].rrpv = 0;

                if( cache->entries[set][ii].pref==1 )
                {
                    cache->entries[set][ii].rrpv = pow(2, RRIP_BIT_SIZE)-1;
                    cache->entries[set][ii].pref = FALSE;
                    // total_prefetch_downgrades++;
                }

                if( (cache->ship_sample[set] == 1) && (cache->entries[set][ii].line_reuse==FALSE) ) 
                {
                    // uns fill_cpu = cache->entries[set][ii].fill_core;

                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
          }
      }

      if(cache->repl_policy == REPL_OPT||cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_MOCKINGJAY||cache->repl_policy == REPL_TRUE_LRU
      ||cache->repl_policy == REPL_SRRIP||cache->repl_policy == REPL_DRRIP||cache->repl_policy == REPL_SHIP_PP){
      // ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS){
        cache->entries[set][ii].addr = addr;
        // cache->entries[set][ii].addr_end = addr+lines_found-1;
      }

      if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccessall Cache hits in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));

      if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
        fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 1, 1); 
      }

      cache_real_hit = TRUE;

    }
  }

  if (strcmp(cache->name, "UOP_CACHE")==0 && cache_real_hit==FALSE  && PRINT_FUNC)  printf("cacheaccessall Cache misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
  
  if (cache->repl_policy == REPL_OPT && cache->pre_addr_opt!=addr){
    uns ori_lines_found = lines_found;
    Flag should_cache = should_opt_cache(cache, set, tag, addr, 1);//ori_lines_found);
    if (should_cache==TRUE && (!cache_real_hit)) {
      if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)  printf("cacheaccessall Cache misses but REPL_OPT treats hit in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      Cache_Entry line = cache->entries[set][1];//return_quanta_data(cache, addr); //cache->entries[set][1];
      *line_data = line.data;// **temp_data; //return_quanta_data(cache, addr);//line->data;//return_quanta_data(cache, addr);// store to hash as line num
      lines_found = return_quanta_line_num(cache, addr); //+=
    } else if (should_cache==FALSE && (cache_real_hit)){
      if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccessall Cache hits but REPL_OPT treats misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
          cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      //TODOSD
      *line_data = NULL;
      lines_found=0;
      //cache_invalidate(cache, addr, &line);
    }
    if (PRINT_FUNC){
        printf("Checking access2 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
          printf("%d ", cache->OPT[set].liveness_history[ii]);
        }
        printf("\n");
        printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
    }

    update_quanta(cache, addr, cache->OPT[set].perset_mytimer, 1, set, target_assoc, cache_real_hit);//ori_lines_found, set);
    cache->OPT[set].perset_mytimer = (cache->OPT[set].perset_mytimer+1) % (OPTGEN_VECTOR_SIZE); //TIMER_SIZE;

    if(UOP_OPT_CACHE_ACCESS_ALL)
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, should_cache, 1);  
  }

  /*if it's a miss*/
  if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 0, 1); 
  }
  
  if (lines_found == 0) {
    DEBUG(0, "Didn't find line in set %u in cache '%s' base 0x%s\n", set,
        cache->name, hexstr64s(addr));
  }

  cache->pre_addr_opt = addr;

  if (PRINT_FUNC)
  printf("Exit cache_access_all\n");

  return lines_found;
}



int cache_access_lookup_allpw(Cache* cache, Addr addr, Addr* line_addr, Flag update_repl, void** line_data, Load_Type load_type) {
  Addr tag;
  uns  set = cache_index(cache, addr, &tag, line_addr);
  uns  ii;
  int lines_found = 0;
  Flag cache_real_hit = FALSE;

  if (PRINT_FUNC)
  printf("Enter cache_access_all\n");

  if(cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr) {
    DEBUG(0, "Checking liveness history '%s' at (set %u)\n", cache->name, set);
    add_access_to_liveness_history(cache, set, tag);
    // Initialize quanta
    // initialize_quanta(cache, addr, cache->OPT[set].perset_mytimer);
    if (PRINT_FUNC){
        printf("Check access history '%s' at (set %u), addr 0x%s, tag 0x%s\n", cache->name, set, hexstr64s(addr), hexstr64s(tag));
        for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
          printf("%d ", cache->OPT[set].liveness_history[ii]);
        }
        printf("\n");
        printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
    }
  }

  if(cache->repl_policy == REPL_NEW_OPT){
    cache->enable_timestamp = TRUE;
  }
  // printf("Cache line lookup: %llu %llu\n", addr, cache->timestamp_new_opt);
  int8 target_assoc;
  

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];
    if(line->valid && line->tag == tag && line->pw_start_addr <= addr && line->addr_end >= addr) {
      /* update replacement state if necessary */
      ASSERT(0, line->data);
      DEBUG(0, "Found line in cache '%s' at (set %u, way %u, base 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base));
      if (PRINT_FUNC)
      printf("Found line in cache '%s' at (set %u, way %u, base 0x%s, tag 0x%s, pw start addr 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base), hexstr64s(line->tag), hexstr64s(line->pw_start_addr));

      if(update_repl) {
        if(line->pref) {
          line->pref = FALSE;
        }
        cache->num_demand_access++;
        update_repl_policy(cache, line, set, ii, FALSE, load_type, addr);
      }
      *line_data = line->data;
      if (cache->repl_policy == REPL_OPT && (!DISABLE_OPT) && cache->pre_addr_opt!=addr){
          target_assoc = line_data;
      }
      lines_found++;

      if(cache->repl_policy == REPL_MOCKINGJAY){
        // TODOSD full_addr == pc ?
        if (PRINT_FUNC) printf("cache->name is %s\n", cache->name);
        m_update_replacement_state(0, set, ii, addr, addr, load_type, TRUE, cache, line_addr); //victim_addr not used
      }

      if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP || 
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
        cache->entries[set][ii].rrpv = 0;

          if (PRINT_FUNC){
              printf("cache_access_all Checking access1 '%s' at (set %u)\n", cache->name, set);
              for(int ii=0;ii<cache->assoc;ii++){
                printf("%d ", cache->entries[set][ii].rrpv);
              }
              printf("\n");
          }
      }

      if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SHIP_PP)){
        uns32 sig   = cache->entries[set][ii].line_sig;
        if( load_type != TYPE_WRITEBACK ) 
          {

            if( (load_type == TYPE_PREF) && line->pref )
            {
                // printf("Rand number in cache_access_all out is %u\n", rand()%100);              
                if( (cache->ship_sample[set] == 1) && (rand()%100 <5)) 
                {
                    // uns32 fill_cpu = cache->entries[set][ii].fill_core;
                    // printf("Rand number in cache_access_all is %u\n", rand()%100);
                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
            else 
            {
                cache->entries[set][ii].rrpv = 0;

                if( cache->entries[set][ii].pref==1 )
                {
                    cache->entries[set][ii].rrpv = pow(2, RRIP_BIT_SIZE)-1;
                    cache->entries[set][ii].pref = FALSE;
                    // total_prefetch_downgrades++;
                }

                if( (cache->ship_sample[set] == 1) && (cache->entries[set][ii].line_reuse==FALSE) ) 
                {
                    // uns fill_cpu = cache->entries[set][ii].fill_core;

                    cache->SHCT[sig] = SAT_INC(cache->SHCT[sig], maxSHCTR);
                    cache->entries[set][ii].line_reuse = TRUE;
                }
            }
          }
      }

      if(cache->repl_policy == REPL_OPT||cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_MOCKINGJAY||cache->repl_policy == REPL_TRUE_LRU
      ||cache->repl_policy == REPL_SRRIP||cache->repl_policy == REPL_DRRIP||cache->repl_policy == REPL_SHIP_PP){
      // ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS){
        cache->entries[set][ii].addr = addr;
        // cache->entries[set][ii].addr_end = addr+lines_found-1;
      }

      if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccessall Cache hits in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));

      if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
        fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 1, 1); 
      }

      cache_real_hit = TRUE;

    }
  }

  if (strcmp(cache->name, "UOP_CACHE")==0 && cache_real_hit==FALSE  && PRINT_FUNC)  printf("cacheaccessall Cache misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
  
  if (cache->repl_policy == REPL_OPT && cache->pre_addr_opt!=addr){
    uns ori_lines_found = lines_found;
    Flag should_cache = should_opt_cache(cache, set, tag, addr, 1);//ori_lines_found);
    if (should_cache==TRUE && (!cache_real_hit)) {
      if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)  printf("cacheaccessall Cache misses but REPL_OPT treats hit in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
              cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      Cache_Entry line = cache->entries[set][1];//return_quanta_data(cache, addr); //cache->entries[set][1];
      *line_data = line.data;// **temp_data; //return_quanta_data(cache, addr);//line->data;//return_quanta_data(cache, addr);// store to hash as line num
      lines_found = return_quanta_line_num(cache, addr); //+=
    } else if (should_cache==FALSE && (cache_real_hit)){
      if (strcmp(cache->name, "UOP_CACHE")==0  && PRINT_FUNC) printf("cacheaccessall Cache hits but REPL_OPT treats misses in set %u in cache '%s' base 0x%s preaddr 0x%s\n", set,
          cache->name, hexstr64s(addr), hexstr64s(cache->pre_addr_opt));
      //TODOSD
      *line_data = NULL;
      lines_found=0;
      //cache_invalidate(cache, addr, &line);
    }
    if (PRINT_FUNC){
        printf("Checking access2 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<OPTGEN_VECTOR_SIZE;ii++){
          printf("%d ", cache->OPT[set].liveness_history[ii]);
        }
        printf("\n");
        printf("timer: %llu\n", cache->OPT[set].perset_mytimer);
    }

    update_quanta(cache, addr, cache->OPT[set].perset_mytimer, 1, set, target_assoc, cache_real_hit);//ori_lines_found, set);
    cache->OPT[set].perset_mytimer = (cache->OPT[set].perset_mytimer+1) % (OPTGEN_VECTOR_SIZE); //TIMER_SIZE;

    if(UOP_OPT_CACHE_ACCESS_ALL)
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, should_cache, 1);  
  }

  /*if it's a miss*/
  if ((cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE)||
      (cache->repl_policy == REPL_CACHEACCESS && (UOP_CACHE_ENABLE_RECORD) && (cache->exe_count_type == TYPE_HIT_TO_ACCESS_RATE))){
    fprintf(cache->file_pointer, "%llu %llu %llu\n", addr, 0, 1); 
  }
  
  if (lines_found == 0) {
    DEBUG(0, "Didn't find line in set %u in cache '%s' base 0x%s\n", set,
        cache->name, hexstr64s(addr));
  }

  cache->pre_addr_opt = addr;

  if (PRINT_FUNC)
  printf("Exit cache_access_all\n");

  return lines_found;
}

/**************************************************************************************/
/* cache_insert: returns a pointer to the data section of the new cache line.
   Sets line_addr to the address of the first block of the new line.  Sets
   repl_line_addr to the address of the first block that was replaced

   DON'T call this unless you are sure that the line is not in the
   cache (call after cache_access returned NULL)
*/

void* cache_insert(Cache* cache, uns8 proc_id, Addr addr, Addr* line_addr,
                   Addr* repl_line_addr, Load_Type load_type) {
  return cache_insert_replpos(cache, proc_id, addr, line_addr, repl_line_addr,
                              INSERT_REPL_DEFAULT, FALSE, load_type);
}
/**************************************************************************************/
/* cache_insert_replpos: returns a pointer to the data section of the new cache
   line.  Sets line_addr to the address of the first block of the new line.
   Sets repl_line_addr to the address of the first block that was replaced

   DON'T call this unless you are sure that the line is not in the
   cache (call after cache_access returned NULL)
*/

void* cache_insert_replpos(Cache* cache, uns8 proc_id, Addr addr,
                           Addr* line_addr, Addr* repl_line_addr,
                           Cache_Insert_Repl insert_repl_policy,
                           Flag              isPrefetch,
                           Load_Type          load_type) {
  Addr         tag;
  uns          repl_index;
  uns          set = cache_index(cache, addr, &tag, line_addr);
  Cache_Entry* new_line;

  uns8 num_lines;
  if (strcmp(cache->name, "UOP_CACHE")==0){
    num_lines = proc_id; //uop cache did not use proc_id so we borrow this argument here
  }
  
  if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC){
    printf("Enter cache_insert\n");
    printf("Filling line line->tag %llu line->pw_start_addr %llu\n", tag, addr);
  }

  if(cache->repl_policy == REPL_IDEAL) {
    new_line        = insert_sure_line(cache, set, tag);
    *repl_line_addr = 0;
  } else {
    // printf("Cache line insert: %llu %llu\n", addr, cache->timestamp_new_opt);
    // if (cache->repl_policy == REPL_NEW_OPT && !UOP_CACHE_ENABLE_RECORD){
    //   cache->timestamp_new_opt++;
    // }
    // if ((!UOP_CACHE_ENABLE_RECORD) && cache->pre_addr_read!=addr){
    //   // cache->timestamp_new_opt++;
    //   // printf("Cache pri addr %llu addr %llu\n", cache->pre_addr_read, addr);
    //   cache->timestamp_new_opt = find_timestamp(cache, addr);
    //   cache->timestamp_new_opt_insert++;
    // }
    if (!UOP_CACHE_ENABLE_RECORD){
      cache->pre_addr_read = addr;
      cache->pre_timestamp_new_opt = cache->timestamp_new_opt;
    }
    if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)printf("Enter find_repl_entry\n");
    new_line = find_repl_entry(cache, proc_id, set, &repl_index, load_type, addr);
    if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)printf("Exit find_repl_entry\n");
    /* before insert the data into cache, if the cache has shadow entry */
    /* insert that entry to the shadow cache */
    if((cache->repl_policy == REPL_SHADOW_IDEAL) && new_line->valid)
      shadow_cache_insert(cache, set, new_line->tag, new_line->base);
    if(new_line->valid)  // bug fixed. 4/26/04 if the entry is not valid,
                         // repl_line_addr should be set to 0
      *repl_line_addr = new_line->base;
    else
      *repl_line_addr = 0;
    DEBUG(0,
          "Replacing 2.2f(set %u, way %u, tag 0x%s, base 0x%s) in cache '%s' "
          "with base 0x%s\n",
          set, repl_index, hexstr64s(new_line->tag), hexstr64s(new_line->base),
          cache->name, hexstr64s(*line_addr));
  }

  new_line->proc_id          = proc_id;
  new_line->valid            = TRUE;
  new_line->tag              = tag;
  new_line->base             = *line_addr;
  new_line->last_access_time = sim_time;  // FIXME: this fixes valgrind warnings
                                          // in update_prf_
  new_line->pref = isPrefetch;

  new_line->pw_start_addr = addr; // only means anything for uop cache
  // TODOSD
  if(cache->repl_policy == REPL_SRRIP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SRRIP)
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
    new_line->rrpv = pow(2, RRIP_BIT_SIZE)-2; 

          if (PRINT_FUNC){
              printf("cache_insert_replpos Checking access1 '%s' at (set %u)\n", cache->name, set);
              for(int ii=0;ii<cache->assoc;ii++){
                printf("%d ", cache->entries[set][ii].rrpv);
              }
              printf("\n");
          }
  }
  //printf("Cache repl policy is %u, cache name is %s\n", cache->repl_policy, cache->name);
  if(cache->repl_policy == REPL_OPT||cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_MOCKINGJAY
  ||cache->repl_policy == REPL_TRUE_LRU||cache->repl_policy == REPL_SRRIP||cache->repl_policy == REPL_DRRIP||cache->repl_policy == REPL_SHIP_PP
  ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS
  ||cache->repl_policy == REPL_COMPRATE||cache->repl_policy == REPL_PMC||cache->repl_policy == REPL_FOO||cache->repl_policy == REPL_HIT_TIMES){
    new_line->addr = addr;
    // new_line->addr_end = addr;
    if (PRINT_FUNC) 
    printf("Insert addr 0x%s, cache name '%s'\n", hexstr64s(new_line->addr), cache->name);
    //printf("In cache insert\n");
    // if (cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_CACHE_STORE){
    //   uopcache_update_execution_count(new_line->addr, cache->file_pointer);
    // }
  }

  if (cache->repl_policy == REPL_PMC){
    new_line->count_num = find_entry_count(addr);
  }

  if(cache->repl_policy == REPL_MOCKINGJAY){
      // TODOSD full_addr == pc ?
      if (PRINT_FUNC) printf("cache->name is %s\n", cache->name);
      m_update_replacement_state(0, set, repl_index, addr, addr, load_type, FALSE, cache, line_addr); //victim_addr not used
  }

  // if(cache->repl_policy == REPL_NEW_OPT){//} && UOP_CACHE_ENABLE_RECORD){
  //   if (UOP_CACHE_ENABLE_RECORD){
  //     fprintf(cache->file_pointer, "%llu %llu\n", addr, timestamp_new_opt);
  //   }
  //   timestamp_new_opt++;
  // }
  

  switch(insert_repl_policy) {
    case INSERT_REPL_DEFAULT:
      update_repl_policy(cache, new_line, set, repl_index, TRUE, load_type, addr);
      break;
    case INSERT_REPL_LRU:
      new_line->last_access_time = 123;  // Just choose a small number
      break;
    case INSERT_REPL_MRU:
      new_line->last_access_time = sim_time;
      break;
    case INSERT_REPL_MID:  // Insert such that it is Middle(Roughly) of the repl
                           // order
    case INSERT_REPL_LOWQTR:  // Insert such that it is Quarter(Roughly) of the
                              // repl order
    {
      // first form the lru array
      Counter* access = (Counter*)malloc(sizeof(Counter) * cache->assoc);
      int      ii, jj;
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(entry->valid)
          access[ii] = entry->last_access_time;
        else
          access[ii] = 0;
        // Sort
        for(jj = ii - 1; jj >= 0; jj--) {
          if(access[jj + 1] < access[jj]) {  // move data
            Counter temp   = access[jj];
            access[jj]     = access[jj + 1];
            access[jj + 1] = temp;
          } else {
            break;
          }
        }
      }
      if(insert_repl_policy == INSERT_REPL_MID) {
        new_line->last_access_time = access[cache->assoc / 2];
      } else if(insert_repl_policy == INSERT_REPL_LOWQTR) {
        new_line->last_access_time = access[cache->assoc / 4];
      }
      if(new_line->last_access_time == 0)
        new_line->last_access_time = sim_time;
      free(access);
    } break;
    default:
      ASSERT(0, FALSE);  // should never come here
  }

  if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)printf("Exit cache_insert\n");
  if(cache->repl_policy == REPL_IDEAL_STORAGE) {
    new_line->last_access_time = cache->assoc;
    /* debug */
    /* insert into the entry also */
    {
      uns          lru_ind  = 0;
      Counter      lru_time = MAX_CTR;
      Cache_Entry* main_line;
      uns          ii;

      /* first cache access */
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* line = &cache->entries[set][ii];

        if(line->tag == tag && line->valid) {
          /* update replacement state if necessary */
          ASSERT(0, line->data);
          line->last_access_time = sim_time;
          return new_line->data;
        }
      }
      /* looking for lru */
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          lru_ind = ii;
          break;
        }
        if(entry->last_access_time < lru_time) {
          lru_ind  = ii;
          lru_time = cache->entries[set][ii].last_access_time;
        }
      }
      main_line                   = &cache->entries[set][lru_ind];
      main_line->valid            = TRUE;
      main_line->tag              = tag;
      main_line->base             = *line_addr;
      main_line->last_access_time = sim_time;
    }
  }
  return new_line->data;
}


void* cache_insert_concat(Cache* cache, uns8 proc_id, Addr addr, Addr addr_end, Addr actual_addr, uns8 nops, Addr* line_addr,
                   Addr* repl_line_addr, Load_Type load_type) {
  return cache_insert_replpos_concat(cache, proc_id, addr, addr_end, actual_addr, nops, line_addr, repl_line_addr,
                              INSERT_REPL_DEFAULT, FALSE, load_type);
}
/**************************************************************************************/
/* cache_insert_replpos: returns a pointer to the data section of the new cache
   line.  Sets line_addr to the address of the first block of the new line.
   Sets repl_line_addr to the address of the first block that was replaced

   DON'T call this unless you are sure that the line is not in the
   cache (call after cache_access returned NULL)
*/

void* cache_insert_replpos_concat(Cache* cache, uns8 proc_id, Addr addr, Addr addr_end, Addr actual_addr, uns8 nops, 
                           Addr* line_addr, Addr* repl_line_addr,
                           Cache_Insert_Repl insert_repl_policy,
                           Flag              isPrefetch,
                           Load_Type          load_type) {
  Addr         tag;
  uns          repl_index;
  uns          set = cache_index(cache, addr, &tag, line_addr);
  Cache_Entry* new_line;

  uns8 num_lines;
  if (strcmp(cache->name, "UOP_CACHE")==0){
    num_lines = proc_id; //uop cache did not use proc_id so we borrow this argument here
  }
  
  if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC){
    printf("Enter cache_insert\n");
    printf("Filling line line->tag %llu line->pw_start_addr %llu\n", tag, addr);
  }

  if(cache->repl_policy == REPL_IDEAL) {
    new_line        = insert_sure_line(cache, set, tag);
    *repl_line_addr = 0;
  } else {
    // printf("Cache line insert: %llu %llu\n", addr, cache->timestamp_new_opt);
    // if (cache->repl_policy == REPL_NEW_OPT && !UOP_CACHE_ENABLE_RECORD){
    //   cache->timestamp_new_opt++;
    // }
    // if ((!UOP_CACHE_ENABLE_RECORD) && cache->pre_addr_read!=addr){
    //   // cache->timestamp_new_opt++;
    //   // printf("Cache pri addr %llu addr %llu\n", cache->pre_addr_read, addr);
    //   cache->timestamp_new_opt = find_timestamp(cache, addr);
    //   cache->timestamp_new_opt_insert++;
    // }
    if (!UOP_CACHE_ENABLE_RECORD){
      cache->pre_addr_read = addr;
      cache->pre_timestamp_new_opt = cache->timestamp_new_opt;
    }
    if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)printf("Enter find_repl_entry\n");
    new_line = find_repl_entry(cache, proc_id, set, &repl_index, load_type, addr);
    if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)printf("Exit find_repl_entry\n");
    /* before insert the data into cache, if the cache has shadow entry */
    /* insert that entry to the shadow cache */
    if((cache->repl_policy == REPL_SHADOW_IDEAL) && new_line->valid)
      shadow_cache_insert(cache, set, new_line->tag, new_line->base);
    if(new_line->valid)  // bug fixed. 4/26/04 if the entry is not valid,
                         // repl_line_addr should be set to 0
      *repl_line_addr = new_line->base;
    else
      *repl_line_addr = 0;
    DEBUG(0,
          "Replacing 2.2f(set %u, way %u, tag 0x%s, base 0x%s) in cache '%s' "
          "with base 0x%s\n",
          set, repl_index, hexstr64s(new_line->tag), hexstr64s(new_line->base),
          cache->name, hexstr64s(*line_addr));
  }

  new_line->proc_id          = proc_id;
  new_line->valid            = TRUE;
  new_line->tag              = tag;
  new_line->base             = *line_addr;
  new_line->last_access_time = sim_time;  // FIXME: this fixes valgrind warnings
                                          // in update_prf_
  new_line->pref = isPrefetch;

  new_line->pw_start_addr = addr; // only means anything for uop cache
  // TODO SD
  if(cache->repl_policy == REPL_SRRIP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE == REPL_SRRIP)
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
    new_line->rrpv = pow(2, RRIP_BIT_SIZE)-2; 

          if (PRINT_FUNC){
              printf("cache_insert_replpos Checking access1 '%s' at (set %u)\n", cache->name, set);
              for(int ii=0;ii<cache->assoc;ii++){
                printf("%d ", cache->entries[set][ii].rrpv);
              }
              printf("\n");
          }
  }
  //printf("Cache repl policy is %u, cache name is %s\n", cache->repl_policy, cache->name);
  if(cache->repl_policy == REPL_OPT||cache->repl_policy == REPL_NEW_OPT||cache->repl_policy == REPL_MOCKINGJAY
  ||cache->repl_policy == REPL_TRUE_LRU||cache->repl_policy == REPL_SRRIP||cache->repl_policy == REPL_DRRIP||cache->repl_policy == REPL_SHIP_PP
  ||cache->repl_policy == REPL_EXECUTION_COUNT||cache->repl_policy == REPL_CACHEACCESS 
  ||cache->repl_policy == REPL_COMPRATE||cache->repl_policy == REPL_PMC || cache->repl_policy == REPL_FOO || cache->repl_policy == REPL_HIT_TIMES){
    new_line->addr = addr;
    new_line->addr_end = addr_end;
    new_line->op_length = nops;
    new_line->actual_addr = actual_addr;
    /* For dynamic Eviction */
    new_line->pmcWeight = find_entry_count(addr);
    if(UOP_CACHE_DYNAMIC_EVICT){
      new_line->evictionHint = getEvictionHints(addr);
      new_line->exhaustedInterval = TRUE;
    }
    if (PRINT_FUNC) 
    printf("Insert addr 0x%s 0x%s, concat cache name '%s'\n", hexstr64s(new_line->addr), hexstr64s(new_line->addr_end), cache->name);
    //printf("In cache insert\n");
    // if (cache->repl_policy == REPL_EXECUTION_COUNT && cache->exe_count_type == TYPE_CACHE_STORE){
    //   uopcache_update_execution_count(new_line->addr, cache->file_pointer);
    // }
  }

  if(cache->repl_policy == REPL_HIT_TIMES){
    new_line->hitTimeRemains = getHitTimesValue(addr, nops);
    // HIT PRINT
    // printf("[HITTIMES] Assign a value to new inserted PW(%lld): %lld\n", addr, new_line->hitTimeRemains);
  }

  if (cache->repl_policy == REPL_PMC){
    new_line->count_num = find_entry_count(addr);
    new_line->pmcWeight = find_entry_count(addr);
    if(UOP_CACHE_DYNAMIC_EVICT){
      new_line->evictionHint = getEvictionHints(addr);
      new_line->exhaustedInterval = TRUE;
    }
  }

  if(cache->repl_policy == REPL_MOCKINGJAY){
      // TODOSD full_addr == pc ?
      if (PRINT_FUNC) printf("cache->name is %s\n", cache->name);
      m_update_replacement_state(0, set, repl_index, addr, addr, load_type, FALSE, cache, line_addr); //victim_addr not used
  }

  // if(cache->repl_policy == REPL_NEW_OPT){//} && UOP_CACHE_ENABLE_RECORD){
  //   if (UOP_CACHE_ENABLE_RECORD){
  //     fprintf(cache->file_pointer, "%llu %llu\n", addr, timestamp_new_opt);
  //   }
  //   timestamp_new_opt++;
  // }
  
  /* Here choose the LRU access Queue Position*/
  switch(insert_repl_policy) {
    case INSERT_REPL_DEFAULT:
      /* We can't decrease the counter immediately after insertion*/
      if(cache->repl_policy != REPL_HIT_TIMES){
        update_repl_policy(cache, new_line, set, repl_index, TRUE, load_type, addr);
      }else{
        new_line->last_access_time = sim_time;
      }
      break;
    case INSERT_REPL_LRU:
      new_line->last_access_time = 123;  // Just choose a small number
      break;
    case INSERT_REPL_MRU:
      new_line->last_access_time = sim_time;
      break;
    case INSERT_REPL_MID:  // Insert such that it is Middle(Roughly) of the repl
                           // order
    case INSERT_REPL_LOWQTR:  // Insert such that it is Quarter(Roughly) of the
                              // repl order
    {
      // first form the lru array
      Counter* access = (Counter*)malloc(sizeof(Counter) * cache->assoc);
      int      ii, jj;
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(entry->valid)
          access[ii] = entry->last_access_time;
        else
          access[ii] = 0;
        // Sort
        for(jj = ii - 1; jj >= 0; jj--) {
          if(access[jj + 1] < access[jj]) {  // move data
            Counter temp   = access[jj];
            access[jj]     = access[jj + 1];
            access[jj + 1] = temp;
          } else {
            break;
          }
        }
      }
      if(insert_repl_policy == INSERT_REPL_MID) {
        new_line->last_access_time = access[cache->assoc / 2];
      } else if(insert_repl_policy == INSERT_REPL_LOWQTR) {
        new_line->last_access_time = access[cache->assoc / 4];
      }
      if(new_line->last_access_time == 0)
        new_line->last_access_time = sim_time;
      free(access);
    } break;
    default:
      ASSERT(0, FALSE);  // should never come here
  }

  if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC) printf("Exit cache_insert\n");
  if(cache->repl_policy == REPL_IDEAL_STORAGE) {
    new_line->last_access_time = cache->assoc;
    /* debug */
    /* insert into the entry also */
    {
      uns          lru_ind  = 0;
      Counter      lru_time = MAX_CTR;
      Cache_Entry* main_line;
      uns          ii;

      /* first cache access */
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* line = &cache->entries[set][ii];

        if(line->tag == tag && line->valid) {
          /* update replacement state if necessary */
          ASSERT(0, line->data);
          line->last_access_time = sim_time;
          return new_line->data;
        }
      }
      /* looking for lru */
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          lru_ind = ii;
          break;
        }
        if(entry->last_access_time < lru_time) {
          lru_ind  = ii;
          lru_time = cache->entries[set][ii].last_access_time;
        }
      }
      main_line                   = &cache->entries[set][lru_ind];
      main_line->valid            = TRUE;
      main_line->tag              = tag;
      main_line->base             = *line_addr;
      main_line->last_access_time = sim_time;
    }
  }
  return new_line->data;
}



// void* cache_insert_all(Cache* cache, uns8 proc_id, Addr addr, Addr* line_addr,
//                    Addr* repl_line_addr, Load_Type load_type, Uop_Cache_Data pw) {
//   return cache_insert_all_replpos(cache, proc_id, addr, line_addr, repl_line_addr,
//                               INSERT_REPL_DEFAULT, FALSE, load_type, pw);
// }

// void* cache_insert_all_replpos(Cache* cache, uns8 proc_id, Addr addr,
//                            Addr* line_addr, Addr* repl_line_addr,
//                            Cache_Insert_Repl insert_repl_policy,
//                            Flag              isPrefetch,
//                            Load_Type          load_type, 
//                            Uop_Cache_Data pw) {
//   Addr         tag;
//   uns          repl_index;
//   uns          set = cache_index(cache, addr, &tag, line_addr);
//   Cache_Entry* new_line;

//   uns8 num_lines;
//   if (strcmp(cache->name, "UOP_CACHE")==0){
//     num_lines = proc_id; //uop cache did not use proc_id so we borrow this argument here
//   }
  
//   if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC){
//     printf("Enter cache_insert\n");
//     printf("Filling line line->tag %llu line->pw_start_addr %llu\n", tag, addr);
//   }

//   int ii, invalid_lines=0;
//   for(ii = 0; ii < cache->assoc; ii++) {
//         Cache_Entry* entry = &cache->entries[set][ii];
//         if(!entry->valid) {
//           invalid_lines++;
//         }
//   }
//   if (invalid_lines<num_lines){
//     uns     rrip_ind  = cache->assoc;
//     while (rrip_ind == cache->assoc){
//         for(ii = 0; ii < cache->assoc; ii++) {
//           Cache_Entry* entry = &cache->entries[set][ii];
//           if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
//             entry->valid = FALSE;
//             invalid_lines++;
//             if(invalid_lines==num_lines)
//             break;
//           }
//         }
//         if (rrip_ind == cache->assoc){
//           for(ii = 0; ii < cache->assoc; ii++) {
//             Cache_Entry* entry = &cache->entries[set][ii];
//             ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
//             entry->rrpv += 1;
//           }
//         }
//     }
//   }
//   for (int jj = 0; jj < num_lines; jj++) {

//     if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC) printf("Enter find_repl_entry\n");
//     // new_line = find_repl_entry(cache, proc_id, set, &repl_index, load_type, addr);
//       uns     rrip_ind  = cache->assoc;

//       for(ii = 0; ii < cache->assoc; ii++) {
//         Cache_Entry* entry = &cache->entries[set][ii];
//         if(!entry->valid) {
//           rrip_ind = ii;
//           entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
//           break;
//         }
//       }

//       while (rrip_ind == cache->assoc){
//         for(ii = 0; ii < cache->assoc; ii++) {
//           Cache_Entry* entry = &cache->entries[set][ii];
//           if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
//             rrip_ind  = ii;
//             entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
//             break;
//           }
//         }
//         if (rrip_ind == cache->assoc){
//           for(ii = 0; ii < cache->assoc; ii++) {
//             Cache_Entry* entry = &cache->entries[set][ii];
//             ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
//             entry->rrpv += 1;
//           }
//         }
//       }

//       *way = rrip_ind;
//       return &cache->entries[set][rrip_ind];
//     if (strcmp(cache->name, "UOP_CACHE")==0 && PRINT_FUNC)printf("Exit find_repl_entry\n");
//     if(new_line->valid)  // bug fixed. 4/26/04 if the entry is not valid,
//                          // repl_line_addr should be set to 0
//       *repl_line_addr = new_line->base;
//     else
//       *repl_line_addr = 0;
//     DEBUG(0,
//           "Replacing 2.2f(set %u, way %u, tag 0x%s, base 0x%s) in cache '%s' "
//           "with base 0x%s\n",
//           set, repl_index, hexstr64s(new_line->tag), hexstr64s(new_line->base),
//           cache->name, hexstr64s(*line_addr));


//     new_line->proc_id          = proc_id;
//     new_line->valid            = TRUE;
//     new_line->tag              = tag;
//     new_line->base             = *line_addr;
//     new_line->last_access_time = sim_time;  // FIXME: this fixes valgrind warnings
//                                             // in update_prf_
//     new_line->pref = isPrefetch;

//     new_line->pw_start_addr = addr; // only means anything for uop cache
//     // TODOSD
//     if(cache->repl_policy == REPL_SRRIP) {
//       new_line->rrpv = pow(2, RRIP_BIT_SIZE)-2; 

//             if (PRINT_FUNC){
//                 printf("cache_insert_replpos Checking access1 '%s' at (set %u)\n", cache->name, set);
//                 for(int ii=0;ii<cache->assoc;ii++){
//                   printf("%d ", cache->entries[set][ii].rrpv);
//                 }
//                 printf("\n");
//             }
//     }
    

//     switch(insert_repl_policy) {
//       case INSERT_REPL_DEFAULT:
//         update_repl_policy(cache, new_line, set, repl_index, TRUE, load_type, addr);
//         break;
//       default:
//         ASSERT(0, FALSE);  // should never come here
//     }

//     Uop_Cache_Data* cur_line_data = NULL;
//     if (strcmp(cache->name, "UOP_CACHE")==0)printf("Exit cache_insert\n");
//     memset(new_line->data, 0, sizeof(Uop_Cache_Data));
//     cur_line_data = (Uop_Cache_Data*)(new_line->data);
//     *(cur_line_data) = pw;
//   }
//   return new_line->data;
// }

/**************************************************************************************/
/* invalidate_line_all: Invalidates based on the pw address for uop cache.  */

void cache_invalidate_all(Cache* cache, Addr addr, Addr* line_addr) {
  Addr tag;
  uns  set = cache_index(cache, addr, &tag, line_addr);
  uns  ii;

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];
    if(line->pw_start_addr == addr){
      // if(cache->repl_policy == REPL_OPT){
      //   invalidate_quanta(cache, line->addr, set);
      //   line->addr = 0;
      // }
    }
    if((line->pw_start_addr == addr||line->addr == addr) && line->valid) {
      line->tag   = 0;
      line->valid = FALSE;
      line->base  = 0;
      line->addr  = 0;
      line->addr_end  = 0;
      line->actual_addr = 0;
      if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP || 
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
        line->rrpv = pow(2, RRIP_BIT_SIZE)-1;
      }
      if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SHIP_PP)){
        line->rrpv = pow(2, RRIP_BIT_SIZE)-1;
        line->line_reuse = FALSE;
        line->pref = FALSE;
        line->line_sig = 0;
      }
    }
  }

  if(cache->repl_policy == REPL_IDEAL)
    invalidate_unsure_line(cache, set, tag);
}


/**************************************************************************************/
/* invalidate_line: Invalidates based on the address.  */

void cache_invalidate(Cache* cache, Addr addr, Addr* line_addr) {
  Addr tag;
  uns  set = cache_index(cache, addr, &tag, line_addr);
  uns  ii;

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];
    if(line->tag == tag){
      // if(cache->repl_policy == REPL_OPT){
      //   invalidate_quanta(cache, line->addr, set);
      //   line->addr = 0;
      // }
    }
    if(line->tag == tag && line->valid) {
      line->tag   = 0;
      line->valid = FALSE;
      line->base  = 0;
      if(cache->repl_policy == REPL_SRRIP || cache->repl_policy == REPL_DRRIP || 
  (cache->repl_policy == REPL_PMC && (UOP_CACHE_DEGRADE==REPL_SRRIP||UOP_CACHE_DEGRADE==REPL_DRRIP))
  || (cache->repl_policy == REPL_HIT_TIMES && (UOP_CACHE_DEGRADE == REPL_SRRIP))) {
        line->rrpv = pow(2, RRIP_BIT_SIZE)-1;
      }
      if(cache->repl_policy == REPL_SHIP_PP || (cache->repl_policy == REPL_PMC && UOP_CACHE_DEGRADE==REPL_SHIP_PP)){
        line->rrpv = pow(2, RRIP_BIT_SIZE)-1;
        line->line_reuse = FALSE;
        line->pref = FALSE;
        line->line_sig = 0;
      }
    }
  }

  if(cache->repl_policy == REPL_IDEAL)
    invalidate_unsure_line(cache, set, tag);
}


/**************************************************************************************/
/* get_next_repl_line:  Return a pointer to the lru item in the cache set */

void* get_next_repl_line(Cache* cache, uns8 proc_id, Addr addr,
                         Addr* repl_line_addr, Flag* valid, Load_Type load_type) {
  Addr         line_tag, line_addr;
  uns          repl_index;
  uns          set_index = cache_index(cache, addr, &line_tag, &line_addr);
  Cache_Entry* new_line  = find_repl_entry(cache, proc_id, set_index,
                                          &repl_index, load_type, addr);

  *repl_line_addr = new_line->base;
  *valid          = new_line->valid;
  return new_line->data;
}


/**************************************************************************************/
/* find_repl_entry: Returns the cache lib entry that will be the next to be
   replaced. This call should not change any of the state information. */
extern Cache uop_cache;
Cache_Entry* find_repl_entry(Cache* cache, uns8 proc_id, uns set, uns* way, Load_Type load_type, Addr addr) {
  int ii;
  //printf("In find_repl0\n");
  switch(cache->repl_policy) {
    //TODOSD: add largest vector num's entry from the left to be the repl_entry?
    case REPL_OPT: {
      if (PRINT_FUNC) printf("In find_repl1\n");
      uns     lru_ind  = 0;
      Counter lru_time = MAX_CTR;
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          lru_ind = ii;
          break;
        }
        if(entry->last_access_time < lru_time) {
          lru_ind  = ii;
          lru_time = cache->entries[set][ii].last_access_time;
        }
      }
      *way = lru_ind;
      if(PRINT_FUNC) printf("In find_repl2\n");
      // invalidate_quanta(cache, cache->entries[set][lru_ind].addr, set);
      return &cache->entries[set][lru_ind];
    } break;  
    case REPL_MOCKINGJAY: {
      uns     mockingjay_ind  = 0;
      /* don't modify this code or put anything above it;
      * if there's an invalid block, we don't need to evict any valid ones */
      for (int32 ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if (!entry->valid) {
          mockingjay_ind = ii;
          *way = mockingjay_ind;
          return &cache->entries[set][mockingjay_ind];
        }
      }
      // your eviction policy goes here
      int32 max_etr = 0;
      if (PRINT_FUNC) printf("ETR of set %u: \n", set);
      for (int32 ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if (PRINT_FUNC) printf("%d\t", entry->etr);
        if (abs(entry->etr) > max_etr ||
                (abs(entry->etr) == max_etr &&
                        entry->etr < 0)) {
            max_etr = abs(entry->etr);
            mockingjay_ind = ii;
        }
      }
      if (PRINT_FUNC) printf("\n");
      uns64 pc_signature = get_pc_signature(addr, FALSE, load_type == TYPE_PREF, 0, cache->num_sets, cache->assoc); // default 1 cpu
      uns64 last_value;
      int8 last_line_num;
      Flag has_history = find_last_quanta(cache, pc_signature, &last_value, &last_line_num, set);
      if (load_type != TYPE_WRITEBACK && has_history){
        // RDP_Entry *saved = (RDP_Entry*)hash_table_access(&rdp, pc_signature);
        if ((last_value > MAX_RD(cache->assoc) || (last_value / GRANULARITY > max_etr))){
          // return cache->assoc;
          mockingjay_ind = cache->assoc-1 ; // previously was cache->assoc, how can an assoc be chosen?
        }
      } 
      *way = mockingjay_ind;
      return &cache->entries[set][mockingjay_ind];
    } break;
    case REPL_NEW_OPT: {
      if (UOP_CACHE_ENABLE_RECORD)
      {
          int16     lru_ind  = -1;
          Counter lru_time = MAX_CTR;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              if (PRINT_FUNC) 
              printf("%d %u set %u entry is %u valid\n", cache->repl_policy, set, ii, entry->valid);
              lru_ind = ii;
              break;
            }
          }
          if (lru_ind!=-1){
            *way = lru_ind;
            return &cache->entries[set][lru_ind];
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if (PRINT_FUNC) 
              printf("%d In %u set %u entry %llu address\n", cache->repl_policy, set, ii, entry->addr);
              if(entry->last_access_time < lru_time) {
                lru_ind  = ii;
                lru_time = cache->entries[set][ii].last_access_time;
              }
            }
            if (PRINT_FUNC) 
              printf("%d Finally choose %u way entry\n", cache->repl_policy, lru_ind);
            *way = lru_ind;
            return &cache->entries[set][lru_ind];
            break;
          }
      } else {
        {
          // printf("Cache line insert lookup: %llu %llu\n", addr, cache->timestamp_new_opt);
          // timestamp_new_opt++;
          int16   new_opt_ind  = -1;
          Counter new_opt_time = 0;
          
          /* Used to fix repeated evict same lines from same PW. */
          static Addr prevAddr = 0;
          /********/

          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              if (PRINT_FUNC) 
              printf("%d %u set %u entry is %u valid\n", cache->repl_policy, set, ii, entry->valid);
              new_opt_ind = ii;
              break;
            }
          }
          if (new_opt_ind!=-1){
            prevAddr = addr;
            *way = new_opt_ind;
            return &cache->entries[set][new_opt_ind];
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if (PRINT_FUNC) {
                printf("%d In %u set %u entry %llu address\n", cache->repl_policy, set, ii, entry->addr);
                printf("line->valid %u line->tag %llu line->pw_start_addr %llu\n", entry->valid, entry->tag, entry->pw_start_addr);
              }
              if(entry->pw_start_addr == addr){
                ASSERT(0, prevAddr == addr);
                continue;
              }
              uns64 entry_future_timestamp = find_future_timestamp(cache, entry->addr, cache->timestamp_new_opt);//cache->timestamp_new_opt_insert);//cache->timestamp_new_opt);//timestamp_new_opt);
              // uns64 entry_future_timestamp = find_future_timestamp_file(cache->file_pointer, cache, entry->addr, timestamp_new_opt);
              if(entry_future_timestamp > new_opt_time){
                new_opt_ind  = ii;
                new_opt_time = entry_future_timestamp;
              }
            }
            if (PRINT_FUNC) 
            printf("%d Finally choose %u way entry\n", cache->repl_policy, new_opt_ind);

            prevAddr = addr;
            *way = new_opt_ind;
            return &cache->entries[set][new_opt_ind];
          }
        } break;
      } break;
    }
    case REPL_FOO: {
      Counter victimIndex = 65536; // Magic Number for identifying not found repl
      static Addr prevAddr = 0; /* Avoid repeatedly evicting entries from same PW. */
      for(int ii = 0; ii < cache->assoc; ++ii){
        Cache_Entry* entry = &(cache->entries[set][ii]);
        if(!entry->valid){
          victimIndex = ii;
          break;
        }
      }

      if(FOO_FIX_LEVEL == FOO_ALL_FIXED || FOO_FIX_LEVEL == FOO_ASYNC_FIXED || FOO_FIX_LEVEL == FOO_ALL_FIXED_BYPASS){
        if(victimIndex == 65536){
          for(int ii = 0; ii < cache->assoc; ++ii){
            Cache_Entry* entry = &(cache->entries[set][ii]);
            ASSERT(0, entry->valid == TRUE);
            if(entry->pw_start_addr == addr){
              ASSERT(0, prevAddr == addr);
              continue;
            }
            /*All invalid enties are evicted tenderly*/
            Flag insertFlag = checkExistCache(entry->pw_start_addr, entry->op_length);
            if(insertFlag == FALSE){
              victimIndex = ii;
              break;
            }
          }
        }

        if(victimIndex == 65536){
          printf("[FOO] Do not find victim entry when inserting\n");
          ASSERT(0,0);
        }
      }

      if(FOO_FIX_LEVEL == FOO_NO_FIXED){
        ASSERT(0, victimIndex != 65536); 
      }

      prevAddr = addr;
      *way = victimIndex;
      if(FOO_PRINT) printf("[FOO Insert] Successfully find victim entry to insert addr:%lld\n", addr);
      return &(cache->entries[set][victimIndex]);
    }break;
    case REPL_CACHEACCESS: {
      if (UOP_CACHE_ENABLE_RECORD){
        {
          uns     lru_ind  = 0;
          Counter lru_time = MAX_CTR;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              lru_ind = ii;
              break;
            }
            if(entry->last_access_time < lru_time) {
              lru_ind  = ii;
              lru_time = cache->entries[set][ii].last_access_time;
            }
          }
          *way = lru_ind;
          return &cache->entries[set][lru_ind];
        } break;
      } else {
        {
          int16   new_count_ind  = -1;
          Counter new_count_time = MAX_CTR;
          float new_count_rate = 1;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              // if (PRINT_FUNC) 
              printf("%u set %u entry is %u valid\n", set, ii, entry->valid);
              new_count_ind = ii;
              break;
            }
          }
          if (new_count_ind!=-1){
            *way = new_count_ind;
            return &cache->entries[set][new_count_ind];
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              // if (PRINT_FUNC) 
              printf("In %u set %u entry\n", set, ii);
              if (cache->exe_count_type==TYPE_CACHE_ACCESS||cache->exe_count_type==TYPE_CACHE_STORE){
                uns64 entry_count;
                // entry_count = find_entry_count(entry->addr);
                entry_count = find_entry_count_concat(entry->addr, entry->addr_end);
                // if (PRINT_FUNC)  
                printf("addr %llu addr_end %llu count %llu\n", entry->addr, entry->addr_end, entry_count);
                if(entry_count<new_count_time){
                  new_count_ind  = ii;
                  new_count_time = entry_count;
                }
              } else if (cache->exe_count_type==TYPE_HIT_TO_ACCESS_RATE){
                float entry_rate;
                entry_rate = find_entry_rate(entry->addr);
                if (PRINT_FUNC)  
                printf("addr %llu count %f\n", entry->addr, entry_rate);
                if(entry_rate<new_count_rate){
                  new_count_ind  = ii;
                  new_count_rate = entry_rate;
                }
              }
              
            }
            // if (PRINT_FUNC) 
            printf("Finally choose %u way entry\n", new_count_ind);
            *way = new_count_ind;
            return &cache->entries[set][new_count_ind];
          }
        } break;
      }
    }
    case REPL_COMPRATE: {
      if ((UOP_CACHE_USAGE_COUNT==TYPE_WHOLE||UOP_CACHE_USAGE_COUNT==TYPE_BITREDUCTION) && UOP_CACHE_ENABLE_RECORD){
        {
          uns     lru_ind  = 0;
          Counter lru_time = MAX_CTR;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              lru_ind = ii;
              break;
            }
            if(entry->last_access_time < lru_time) {
              lru_ind  = ii;
              lru_time = cache->entries[set][ii].last_access_time;
            }
          }
          *way = lru_ind;
          return &cache->entries[set][lru_ind];
        } break;
      } else {
        {
          int16   new_count_ind  = -1;
          Counter new_count_time = MAX_CTR;
          float new_count_rate = 1;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              if (PRINT_FUNC) 
              printf("%u set %u entry is %u valid\n", set, ii, entry->valid);
              new_count_ind = ii;
              break;
            }
          }
          if (new_count_ind!=-1){
            *way = new_count_ind;
            return &cache->entries[set][new_count_ind];
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if (PRINT_FUNC) 
              printf("In %u set %u entry\n", set, ii);
              float entry_rate;
              Counter entry_count;
              // // if(UOP_CACHE_USAGE_COUNT==TYPE_TRANSIENT)
              // // entry_rate = find_entry_rate_transient_concat(entry->addr, entry->addr_end);
              // // else if(UOP_CACHE_USAGE_COUNT==TYPE_WHOLE)
              // // entry_rate = find_entry_rate_holistic_concat(entry->addr, entry->addr_end);
              // entry_count = find_entry_count_concat(entry->addr, entry->addr_end);
              entry_rate = find_entry_rate(entry->addr);
              // if (PRINT_FUNC)  
              printf("addr %llu rate %f count %llu new_count_rate %f new_count_ind %u\n", entry->addr, entry_rate, entry_count, new_count_rate, new_count_ind);
              if(entry_rate<=new_count_rate){
                new_count_ind  = ii;
                new_count_rate = entry_rate;
              }
              // if(entry_count<new_count_time){
              //   new_count_ind  = ii;
              //   new_count_time = entry_count;
              // } 
            }
            if (PRINT_FUNC) 
            printf("Finally choose %u way entry\n", new_count_ind);
            *way = new_count_ind;
            return &cache->entries[set][new_count_ind];
          }
        } break;
      }
    }
    case REPL_PMC: {
      /* First-pass run like LRU*/
      if ((UOP_CACHE_USAGE_COUNT==TYPE_WHOLE||UOP_CACHE_USAGE_COUNT==TYPE_BITREDUCTION) && UOP_CACHE_ENABLE_RECORD){
        {
          uns     lru_ind  = 0;
          Counter lru_time = MAX_CTR;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              lru_ind = ii;
              break;
            }
            if(entry->last_access_time < lru_time) {
              lru_ind  = ii;
              lru_time = cache->entries[set][ii].last_access_time;
            }
          }
          *way = lru_ind;
          return &cache->entries[set][lru_ind];
        } break;
      } else {
        {
          STAT_EVENT(0, UOP_CACHE_REPL_ALL);
          // static int16 pre_chosen_entry = -1;
          // static int16 pre_pre_chosen_entry = -1;
          int16   new_count_ind  = -1;
          int16   mid_count_ind  = -1;
          Counter new_count_time = MAX_CTR;//0;//MAX_CTR;
          float new_count_rate = 1;
          Counter lru_time = MAX_CTR;

          /* Fix repeat evict lines from same PW*/
          static Addr prevAddr = 0;
          /**********/

          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              if (PRINT_FUNC) 
              printf("%u set %u entry is %u valid\n", set, ii, entry->valid);
              new_count_ind = ii;
              break;
            }
          }
          if (new_count_ind!=-1){
            prevAddr = addr;
            *way = new_count_ind;
            return &cache->entries[set][new_count_ind];
          } else {
            if (return_feature(7)==7){
              for(ii = 0; ii < cache->assoc; ii++) {
                Cache_Entry* entry = &cache->entries[set][ii];
                if (PRINT_FUNC) 
                printf("In %u set %u entry switch\n", set, ii);
                float entry_rate;
                Counter entry_count;
                entry_count = find_entry_count_switch(entry->addr); //(entry->actual_addr); //find_entry_count(entry->addr);
                if (PRINT_FUNC)  
                printf("addr %llu count %llu switch\n", entry->addr, entry_count);
                if(entry_count  < /*>=*/ new_count_time){// < /*>=*/ new_count_time){// && entry_count!=0){
                  new_count_ind  = ii;
                  new_count_time = entry_count;
                } 
              }
              if (new_count_time == 0){
                new_count_time = MAX_CTR;
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if (PRINT_FUNC) 
                  printf("In %u set %u entry\n", set, ii);
                  float entry_rate;
                  Counter entry_count;
                  entry_count = find_entry_count(entry->addr);
                  if (PRINT_FUNC)  
                  printf("addr %llu rate %f count %llu, last value %llu\n", entry->addr, entry_rate, entry_count, entry->last_access_time); 
                  if(entry_count < /*>*/ new_count_time){
                    new_count_ind  = ii;
                    new_count_time = entry_count;
                  }
                  // if(entry->last_access_time < lru_time && entry_count==0) {
                  //   new_count_ind  = ii;
                  //   lru_time = cache->entries[set][ii].last_access_time;
                  // }
                }
              }
            } else {
              for(ii = 0; ii < cache->assoc; ii++) {
                Cache_Entry* entry = &cache->entries[set][ii];
                /* Fix repeated evict lines from same PW */
                if(entry->pw_start_addr == addr){
                  ASSERT(0, prevAddr == addr);
                  continue;
                }
                if (PRINT_FUNC) 
                printf("In %u set %u entry\n", set, ii);
                float entry_rate;
                Counter entry_count;
                // entry_count = find_entry_count(entry->addr);//find_entry_count(entry->actual_addr); //find_entry_count(entry->addr);
                entry_count = entry->pmcWeight;
                if (PRINT_FUNC)  
                printf("addr %llu count %llu\n", entry->addr, entry_count);
                if(entry_count < /*>*/ new_count_time){
                  new_count_ind  = ii;
                  new_count_time = entry_count;
                } 
              }
            }   

            mid_count_ind = new_count_ind;
            uns rrip_ind  = cache->assoc;
            if (PRINT_FUNC) 
            printf("In the middle choose %u way entry for set %u, pre %u, pre_pre %u\n", mid_count_ind, set, cache->pre_chosen_entry[set], cache->pre_pre_chosen_entry[set]);
            if (cache->pre_chosen_entry[set] == new_count_ind || cache->pre_pre_chosen_entry[set] == new_count_ind 
            || cache->pre_pre_pre_chosen_entry[set] == new_count_ind || cache->pre_pre_pre_pre_chosen_entry[set] == new_count_ind
            || cache->sec_pre_chosen_entry[set] == new_count_ind || cache->sec_pre_pre_chosen_entry[set] == new_count_ind 
            || cache->sec_pre_pre_pre_chosen_entry[set] == new_count_ind || cache->sec_pre_pre_pre_pre_chosen_entry[set] == new_count_ind){
              STAT_EVENT(0, UOP_CACHE_REPL_DEGRADE);
              if (UOP_CACHE_DEGRADE==REPL_TRUE_LRU){
                // degrade to LRU
                uns     lru_ind  = 0;
                Counter lru_time = MAX_CTR;
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if(!entry->valid) {
                    lru_ind = ii;
                    break;
                  }
                  if(entry->last_access_time < lru_time) {
                    lru_ind  = ii;
                    lru_time = cache->entries[set][ii].last_access_time;
                  }
                }
                new_count_ind = lru_ind;
              }
              
              else if (UOP_CACHE_DEGRADE==REPL_SRRIP){
                // degrade to SRRIP
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access1 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if(!entry->valid) {
                    rrip_ind = ii;
                    entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                    break;
                  }
                }
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access2 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                while (rrip_ind == cache->assoc){
                  for(ii = 0; ii < cache->assoc; ii++) {
                    Cache_Entry* entry = &cache->entries[set][ii];
                    if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                      rrip_ind  = ii;
                      entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                      break;
                    }
                  }
                  if (rrip_ind == cache->assoc){
                    for(ii = 0; ii < cache->assoc; ii++) {
                      Cache_Entry* entry = &cache->entries[set][ii];
                      if(entry->pw_start_addr == addr){
                        ASSERT(0, addr == prevAddr);
                        ASSERT(0, entry->rrpv == pow(2, RRIP_BIT_SIZE)-2);
                        continue;
                      }
                      ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
                      entry->rrpv += 1;
                    }
                  }
                }
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access3 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                new_count_ind = rrip_ind;
              }

              else if (UOP_CACHE_DEGRADE==REPL_DRRIP){
                //degrade to DRRIP
                uns     rrip_ind  = cache->assoc;
                Flag    has_invalid = FALSE;
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access1 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if(!entry->valid) {
                    has_invalid = TRUE;
                    break;
                  }
                }
                after_eviction(cache, set, &rrip_ind, has_invalid);
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access2 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                while (rrip_ind == cache->assoc){
                  after_eviction(cache, set, &rrip_ind, FALSE);
                  if (rrip_ind == cache->assoc){
                    for(ii = 0; ii < cache->assoc; ii++) {
                      Cache_Entry* entry = &cache->entries[set][ii];
                      //ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
                      if (entry->rrpv < pow(2, RRIP_BIT_SIZE)-1){
                        entry->rrpv += 1;
                      }
                    }
                  }
                }
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access3 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                new_count_ind = rrip_ind;
              }

              else if (UOP_CACHE_DEGRADE==REPL_SHIP_PP){
                //degrade to SHiP++
                uns     rrip_ind  = cache->assoc;
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access1 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if(!entry->valid) {
                    rrip_ind = ii;
                    entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                    break;
                  }
                }
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access2 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                while (rrip_ind == cache->assoc){
                  for(ii = 0; ii < cache->assoc; ii++) {
                    Cache_Entry* entry = &cache->entries[set][ii];
                    if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                      rrip_ind  = ii;
                      entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                      break;
                    }
                  }
                  if (rrip_ind == cache->assoc){
                    for(ii = 0; ii < cache->assoc; ii++) {
                      Cache_Entry* entry = &cache->entries[set][ii];
                      ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
                      entry->rrpv += 1;
                    }
                  }
                }
                if (PRINT_FUNC){
                  printf("find_repl_entry Checking access3 '%s' at (set %u)\n", cache->name, set);
                  for(int ii=0;ii<cache->assoc;ii++){
                    printf("%d ", cache->entries[set][ii].rrpv);
                  }
                  printf("\n");
                }
                new_count_ind = rrip_ind;

              }
              
              
              // original pick from second/third.. smallest count;
              else if (UOP_CACHE_DEGRADE==4){
                new_count_ind = cache->pre_pre_pre_pre_chosen_entry[set];
                if (cache->assoc<4) new_count_ind = cache->pre_chosen_entry[set];
                new_count_time = MAX_CTR;
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if (PRINT_FUNC) 
                  printf("In %u set %u entry\t", set, ii);
                  Counter entry_count;
                  entry_count = find_entry_count(entry->addr);
                  if (PRINT_FUNC)  
                  printf("addr %llu count %llu, chosen_entry %u, ii %u, mid %u\n", entry->addr, entry_count, new_count_ind, ii, mid_count_ind);
                  if(entry_count < /*>*/ new_count_time && ii!=cache->pre_chosen_entry[set] && ii!=cache->pre_pre_chosen_entry[set] && ii!=cache->pre_pre_pre_chosen_entry[set] && ii!=cache->pre_pre_pre_pre_chosen_entry[set]
                  && ii!=cache->sec_pre_chosen_entry[set] && ii!=cache->sec_pre_pre_chosen_entry[set] && ii!=cache->sec_pre_pre_pre_chosen_entry[set] && ii!=cache->sec_pre_pre_pre_pre_chosen_entry[set]){
                    new_count_ind  = ii;
                    new_count_time = entry_count;
                  } 
                }
              }
            }
            // if (UOP_CACHE_REMOVE_LOCK==4) cache->pre_pre_pre_pre_chosen_entry[set] = cache->pre_pre_pre_chosen_entry[set];
            // if (UOP_CACHE_REMOVE_LOCK==3||UOP_CACHE_REMOVE_LOCK==4) cache->pre_pre_pre_chosen_entry[set] = cache->pre_pre_chosen_entry[set];
            // if (UOP_CACHE_REMOVE_LOCK==2||UOP_CACHE_REMOVE_LOCK==3||UOP_CACHE_REMOVE_LOCK==4) cache->pre_pre_chosen_entry[set] = cache->pre_chosen_entry[set];
            // if (UOP_CACHE_REMOVE_LOCK==1||UOP_CACHE_REMOVE_LOCK==2||UOP_CACHE_REMOVE_LOCK==3||UOP_CACHE_REMOVE_LOCK==4) cache->pre_chosen_entry[set] = new_count_ind;
            if (UOP_CACHE_REMOVE_LOCK==8) cache->sec_pre_pre_pre_pre_chosen_entry[set] = cache->sec_pre_pre_pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->sec_pre_pre_pre_chosen_entry[set] = cache->sec_pre_pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==6||UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->sec_pre_pre_chosen_entry[set] = cache->sec_pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==5||UOP_CACHE_REMOVE_LOCK==6||UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->sec_pre_chosen_entry[set] = cache->pre_pre_pre_pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==4||UOP_CACHE_REMOVE_LOCK==5||UOP_CACHE_REMOVE_LOCK==6||UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->pre_pre_pre_pre_chosen_entry[set] = cache->pre_pre_pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==3||UOP_CACHE_REMOVE_LOCK==4
            ||UOP_CACHE_REMOVE_LOCK==5||UOP_CACHE_REMOVE_LOCK==6||UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->pre_pre_pre_chosen_entry[set] = cache->pre_pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==2||UOP_CACHE_REMOVE_LOCK==3||UOP_CACHE_REMOVE_LOCK==4
            ||UOP_CACHE_REMOVE_LOCK==5||UOP_CACHE_REMOVE_LOCK==6||UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->pre_pre_chosen_entry[set] = cache->pre_chosen_entry[set];
            if (UOP_CACHE_REMOVE_LOCK==1||UOP_CACHE_REMOVE_LOCK==2||UOP_CACHE_REMOVE_LOCK==3||UOP_CACHE_REMOVE_LOCK==4
            ||UOP_CACHE_REMOVE_LOCK==5||UOP_CACHE_REMOVE_LOCK==6||UOP_CACHE_REMOVE_LOCK==7||UOP_CACHE_REMOVE_LOCK==8) cache->pre_chosen_entry[set] = new_count_ind;

            // if (cache->repl_policy == REPL_PMC && (!UOP_CACHE_ENABLE_RECORD) && UOP_CACHE_USAGE_COUNT==TYPE_WHOLE){
            //   for(ii = 0; ii < cache->assoc; ii++) {
            //     Cache_Entry* entry = &cache->entries[set][ii];
            //     if (ii!=new_count_ind)
            //     minus_countcacheaccess(entry->addr, 1);
            //   }
            // }
            if (PRINT_FUNC) 
            printf("Finally choose %u way entry\n", new_count_ind);

            prevAddr = addr;
            *way = new_count_ind;
            return &cache->entries[set][new_count_ind];
          }
        } break;
      }
    }
    case REPL_SHADOW_IDEAL:
    case REPL_TRUE_LRU: {
      uns     lru_ind  = 0;
      Counter lru_time = MAX_CTR;
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          if (PRINT_FUNC && strcmp(cache->name, "UOP_CACHE")==0)
          printf("%d In true_lru %u set %u entry is %u valid\n", cache->repl_policy, set, ii, entry->valid);
          lru_ind = ii;
          break;
        }
        if (PRINT_FUNC && strcmp(cache->name, "UOP_CACHE")==0)
        printf("%d In true_lru In %u set %u entry, time %llu\n", cache->repl_policy, set, ii, entry->last_access_time);
        if(entry->last_access_time < lru_time) {
          lru_ind  = ii;
          lru_time = cache->entries[set][ii].last_access_time;
        }
      }
      if (PRINT_FUNC && strcmp(cache->name, "UOP_CACHE")==0)
        printf("%d In true_lru Finally choose %u way entry\n", cache->repl_policy, lru_ind);
      if(strcmp(cache->name, "ICACHE") == 0 && (cache->entries[set][lru_ind].valid == TRUE) && UOP_INCLUSION == 3){ // UOPI_ACTUAL
        uns ucSet = (cache->entries[set][lru_ind].addr >> uop_cache.shift_bits) & uop_cache.set_mask;
        for(int jj = 0; jj < uop_cache.assoc; jj++){
          if(uop_cache.entries[ucSet][jj].valid == TRUE && 
            ((uop_cache.entries[ucSet][jj].addr >> uop_cache.shift_bits) == (cache->entries[set][lru_ind].addr >> cache->shift_bits))){
            // printf("UC Evict %lld in set %d; IC Evict %lld in set %d\n", uop_cache.entries[ucSet][jj].addr, ucSet, cache->entries[set][lru_ind].addr, set);
            uop_cache.entries[ucSet][jj].valid = FALSE;
            uop_cache.entries[ucSet][jj].tag   = 0;
            uop_cache.entries[ucSet][jj].base  = 0;
            uop_cache.entries[ucSet][jj].addr  = 0;
            uop_cache.entries[ucSet][jj].addr_end  = 0;
            uop_cache.entries[ucSet][jj].actual_addr = 0;
            uop_cache.entries[ucSet][jj].pw_start_addr = 0;
          }
        }
      }
      *way = lru_ind;
      return &cache->entries[set][lru_ind];
    } break;
    case REPL_HIT_TIMES:{
      Flag isFound = FALSE;
      uns newEntryInd = 0;
      Flag isLRU = cache->entiresLRU[set];

      static Addr prevAddr = 0;
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid){
          newEntryInd = ii;
          isFound = TRUE;
          break;
        }
      }
      
      /* Try to evict */
      if(!isFound){
        if(isLRU){ /* Use weight to detect*/
          /* Attention: Can't repeatedly choose the one just inserted. */
          Counter tmpMax = MAX_CTR;
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            ASSERT(0, entry->valid);
            if(entry->pw_start_addr == addr){
              /* Since we first evict all pw with same start address, so existing 
              lines with same addr from same PW just inserted*/
              ASSERT(0, addr == prevAddr);
              continue;
            }
            if(entry->hitTimeRemains < 10000){ /* 10000 just a magic number for selected PWs*/
              isFound = TRUE;
              if(tmpMax > entry->hitTimeRemains){
                tmpMax = entry->hitTimeRemains;
                newEntryInd = ii;
              }
            }
          }
          // if(isFound) printf("[HitTimes] entry: %d, find %lld, remaintimes: %lld to evict\n", newEntryInd ,cache->entries[set][newEntryInd].pw_start_addr, cache->entries[set][newEntryInd].hitTimeRemains);
        }
        /* Degrade to LRU or just can't choose entry by weight */
        if(!isFound){
          if(UOP_CACHE_DEGRADE == REPL_TRUE_LRU){
            Counter lru_time = MAX_CTR;
            for(ii = 0; ii < cache->assoc; ii++){
              Cache_Entry* entry = &cache->entries[set][ii];
              if(entry->last_access_time < lru_time) {
                newEntryInd  = ii;
                lru_time = entry->last_access_time;
              }
            }
          }else if(UOP_CACHE_DEGRADE == REPL_SRRIP){
            // degrade to SRRIP
            uns rrip_ind  = cache->assoc;
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(!entry->valid) {
                rrip_ind = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                break;
              }
            }
            while (rrip_ind == cache->assoc){
              for(ii = 0; ii < cache->assoc; ii++) {
                Cache_Entry* entry = &cache->entries[set][ii];
                if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                  rrip_ind  = ii;
                  entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                  break;
                }
              }
              if (rrip_ind == cache->assoc){
                for(ii = 0; ii < cache->assoc; ii++) {
                  Cache_Entry* entry = &cache->entries[set][ii];
                  if(entry->pw_start_addr == addr){
                    ASSERT(0, addr == prevAddr);
                    ASSERT(0, entry->rrpv == pow(2, RRIP_BIT_SIZE) - 2);
                    continue;
                  }
                  ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
                  entry->rrpv += 1;
                }
              }
            }
            newEntryInd = rrip_ind;
          }else ASSERT(0,FALSE);
        }
        cache->entiresLRU[set] = (isLRU == TRUE ? FALSE : TRUE);
      }

      prevAddr = addr;
      *way = newEntryInd;
      return &cache->entries[set][newEntryInd];
    } break;
    case REPL_EXECUTION_COUNT: {
      uns     new_ind  = 0;
      Counter new_count = MAX_CTR;//0;//MAX_CTR;
      // if (PRINT_FUNC) 
      printf("Find repl entry: Set %u\t", set);
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          new_ind = ii;
          break;
        }
        uns64 last_value;
        int8 last_line_num;
        // Flag has_history = find_last_quanta(cache, entry->addr, &last_value, &last_line_num, set);
        if (PRINT_FUNC) 
        printf("Entry %u, In find_repl, before find, check begin addr 0x%s, end addr 0x%s, cache root %u\n", ii, hexstr64s(entry->addr), hexstr64s(entry->addr_end), cache->map_root);
        Flag has_history = find_last_quanta_concat(cache, entry->addr, entry->addr_end, &last_value, &last_line_num, set);
        if (PRINT_FUNC) 
        printf("has_history %u, count 0x%s, counter 0x%s, last_value<new_count is %u\n", has_history, hexstr64s(last_value), hexstr64s(new_count), last_value < new_count);
        if (!has_history) last_value = 0;
        if(last_value < new_count) { // > //<
          new_ind  = ii;
          new_count = last_value;
        }
        // printf("0Addr 0x%s, 0Addr_end 0x%s, count %u\t", hexstr64s(entry->addr), hexstr64s(entry->addr_end), last_value);
        // // has_history = find_last_quanta(cache, entry->addr, &last_value, &last_line_num, set);
        // has_history = find_last_quanta_concat(cache, entry->addr, entry->addr_end, &last_value, &last_line_num, set);
        // if (PRINT_FUNC) 
        printf("Addr 0x%s, Addr_end 0x%s, count %llu, new_ind %u\n", hexstr64s(entry->addr), 
        hexstr64s(entry->addr_end), last_value, new_ind);
      }
      if (PRINT_FUNC) 
      printf("Chosen entry is %u, addr 0x%s, addr_end 0x%s\n\n", new_ind, hexstr64s(cache->entries[set][new_ind].addr), hexstr64s(cache->entries[set][new_ind].addr_end));
      *way = new_ind;
      return &cache->entries[set][new_ind];
    } break;
    case REPL_SHIP_PP:
    case REPL_SRRIP: {
      static Addr prevAddr = 0;
      uns  rrip_ind  = cache->assoc;
      if (PRINT_FUNC){
        printf("find_repl_entry Checking access1 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          rrip_ind = ii;
          entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
          break;
        }
      }
      if (PRINT_FUNC){
        printf("find_repl_entry Checking access2 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      while (rrip_ind == cache->assoc){
        for(ii = 0; ii < cache->assoc; ii++) {
          Cache_Entry* entry = &cache->entries[set][ii];
          if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
            rrip_ind  = ii;
            entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
            break;
          }
        }
        if (rrip_ind == cache->assoc){
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(entry->pw_start_addr == addr){
              ASSERT(0, prevAddr == addr);
              // ASSERT(0, entry->rrpv == pow(2, RRIP_BIT_SIZE) - 2); // Comment for SHIP not initializing
              continue;
            }
            ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
            entry->rrpv += 1;
          }
        }
      }
      if (PRINT_FUNC){
        printf("find_repl_entry Checking access3 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      prevAddr = addr;
      *way = rrip_ind;
      if (PRINT_FUNC) 
      printf("Finally choose %u way entry\n", rrip_ind);
      return &cache->entries[set][rrip_ind];
    } break;
    case REPL_DRRIP: {
      static Addr prevAddr = 0;
      uns     rrip_ind  = cache->assoc;
      Flag    has_invalid = FALSE;
      if (PRINT_FUNC){
        printf("find_repl_entry Checking access1 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          has_invalid = TRUE;
          break;
        }
      }
      after_eviction(cache, set, &rrip_ind, has_invalid);
      if (PRINT_FUNC){
        printf("find_repl_entry Checking access2 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      while (rrip_ind == cache->assoc){
        after_eviction(cache, set, &rrip_ind, FALSE);
        if (rrip_ind == cache->assoc){
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(entry->pw_start_addr == addr){
              ASSERT(0, prevAddr == addr);
              // ASSERT(0, entry->rrpv == pow(2, RRIP_BIT_SIZE) - 2); // Comment for possible not initialize
              continue;
            }
            //ASSERT(0, entry->rrpv < pow(2, RRIP_BIT_SIZE)-1);
            if (entry->rrpv < pow(2, RRIP_BIT_SIZE)-1){
              entry->rrpv += 1;
            }
          }
        }
      }
      if (PRINT_FUNC){
        printf("find_repl_entry Checking access3 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      *way = rrip_ind;
      prevAddr = addr;
      if (PRINT_FUNC) 
      printf("Finally choose %u way entry\n", rrip_ind);
      return &cache->entries[set][rrip_ind];
    } break;
    case REPL_RANDOM:
    case REPL_NOT_MRU:
    case REPL_ROUND_ROBIN:
    case REPL_LOW_PREF: {
      uns repl_index = cache->repl_ctrs[set];
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid)
          repl_index = ii;
      }
      *way = repl_index;
      return &cache->entries[set][repl_index];
    } break;
    case REPL_IDEAL:
      printf("ERROR: Can't determine entry to be replaced when using ideal "
             "replacement\n");
      ASSERT(0, FALSE);
      break;
    case REPL_IDEAL_STORAGE: {
      printf("ERROR: Check if ideal storage replacement works\n");
      ASSERT(0, FALSE);
      Cache_Entry* new_line = &(
        cache->shadow_entries[set][cache->queue_end[set]]);
      cache->queue_end[set] = (cache->queue_end[set] + 1) % ideal_num_entries;
      return new_line;
    } break;
    case REPL_PARTITION: {
      uns8 way_proc_id;
      uns  lru_ind             = 0;
      uns  total_assigned_ways = 0;

      for(way_proc_id = 0; way_proc_id < NUM_CORES; way_proc_id++) {
        cache->num_ways_occupied_core[way_proc_id] = 0;
        cache->lru_time_core[way_proc_id]          = MAX_CTR;
        ASSERT(way_proc_id, cache->num_ways_allocted_core[way_proc_id]);
        total_assigned_ways += cache->num_ways_allocted_core[way_proc_id];
      }

      ASSERT(proc_id, total_assigned_ways == cache->assoc);

      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          lru_ind = ii;
          *way    = lru_ind;
          return &cache->entries[set][lru_ind];
        }
        cache->num_ways_occupied_core[entry->proc_id]++;
        if(entry->last_access_time < cache->lru_time_core[entry->proc_id]) {
          cache->lru_index_core[entry->proc_id] = ii;
          cache->lru_time_core[entry->proc_id]  = entry->last_access_time;
        }
      }

      // find the core that overoccupies its partition the most
      int max_extra_occ = 0;
      int repl_proc_id  = -1;
      for(way_proc_id = 0; way_proc_id < NUM_CORES; way_proc_id++) {
        if(cache->num_ways_allocted_core[way_proc_id] <
           cache->num_ways_occupied_core[way_proc_id]) {
          int extra_occ = cache->num_ways_occupied_core[way_proc_id] -
                          cache->num_ways_allocted_core[way_proc_id];
          if(extra_occ > max_extra_occ) {
            max_extra_occ = extra_occ;
            repl_proc_id  = way_proc_id;
          }
        }
      }
      int proc_id_extra_occ = cache->num_ways_occupied_core[proc_id] -
                              cache->num_ways_allocted_core[proc_id];
      if(cache->num_ways_allocted_core[proc_id] >
           cache->num_ways_occupied_core[proc_id] ||
         max_extra_occ > proc_id_extra_occ + 1 ||
         ((max_extra_occ > proc_id_extra_occ) &&
          ((proc_id + set) % NUM_CORES > (repl_proc_id + set) % NUM_CORES))) {
        /* the complicated condition above ensures unbiased
           distribution of over-occupancy in case a workload does
           not occupy its allocated way partition */
        ASSERT(0, repl_proc_id >= 0);
        lru_ind = cache->lru_index_core[repl_proc_id];
      } else {
        lru_ind = cache->lru_index_core[proc_id];
      }
      *way = lru_ind;
      return &cache->entries[set][lru_ind];
    }
    default:
      ASSERT(0, FALSE);
  }
}


/**************************************************************************************/
/* update_repl_policy: */

static inline void update_repl_policy(Cache* cache, Cache_Entry* cur_entry,
                                      uns set, uns way, Flag repl, Load_Type load_type, Addr PC) {
  switch(cache->repl_policy) {
    case REPL_OPT:
      break;
    case REPL_IDEAL_STORAGE:
    case REPL_SHADOW_IDEAL:
    case REPL_TRUE_LRU:
    //TODOSD: add largest vector num's entry from the left to be the repl_entry?
    case REPL_NEW_OPT:
    case REPL_FOO: /* Seems no update*/
    case REPL_CACHEACCESS:
    case REPL_COMPRATE:
    // case REPL_PMC:
    case REPL_SRRIP:
    case REPL_DRRIP:
    case REPL_MOCKINGJAY:
    case REPL_EXECUTION_COUNT:
    case REPL_PARTITION:
      cur_entry->last_access_time = sim_time;
      break;
    case REPL_HIT_TIMES:
      /*HIT print*/
      // printf("[HitTimes] called update_repl_policy %lld, current remain times: %d\n", cur_entry->pw_start_addr, cur_entry->hitTimeRemains);
      cur_entry->last_access_time = sim_time;
      if(cur_entry->hitTimeRemains > 0) --(cur_entry->hitTimeRemains);
      break;
    // case REPL_SRRIP:
    //   cur_entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
    case REPL_PMC:
      if (UOP_CACHE_DEGRADE!=REPL_SHIP_PP){break;}
    case REPL_SHIP_PP: {
      // remember signature of what is being inserted
      uns64 use_PC = (load_type == TYPE_PREF) ? ((PC << 1) + 1) : (PC<<1);
      uns32 new_sig = use_PC%SHCT_SIZE;
      
      if( cache->ship_sample[set] == 1 ) 
      {
          // uns32 fill_cpu = cache->entries[set][way].fill_core;
          
          // update signature based on what is getting evicted
          if (cache->entries[set][way].line_reuse == FALSE) { 
              cache->SHCT[cur_entry->line_sig] = SAT_DEC(cache->SHCT[cur_entry->line_sig]);
          }
          else 
          {
              cache->SHCT[cur_entry->line_sig] = SAT_INC(cache->SHCT[cur_entry->line_sig], maxSHCTR);
          }

          cache->entries[set][way].line_reuse = FALSE;
          cache->entries[set][way].line_sig   = new_sig;  
          // cache->entries[set][way].fill_core  = cpu;
      }

      cache->entries[set][way].pref = (load_type == TYPE_PREF);

      // Now determine the insertion prediciton

      uns32 priority_RRPV = pow(2, RRIP_BIT_SIZE)-2; // default SHIP

      if( load_type == TYPE_WRITEBACK )
      {
        cache->entries[set][way].rrpv = pow(2, RRIP_BIT_SIZE)-1;
      }
      else if (cache->SHCT[new_sig] == 0) {
        cache->entries[set][way].rrpv = (rand()%100>=RRIP_OVERRIDE_PERC)?  pow(2, RRIP_BIT_SIZE)-1: priority_RRPV; //LowPriorityInstallMostly
        //printf("Rand number in update_repl_policy is %u\n", rand()%100);
      }
      else if (cache->SHCT[new_sig] == 7) {
        cache->entries[set][way].rrpv = (load_type == TYPE_PREF) ? 1 : 0; // HighPriority Install
      }
      else {
        cache->entries[set][way].rrpv = priority_RRPV; // HighPriority Install 
      }
    }
    case REPL_RANDOM: {
      char* old_rand_state  = (char*)setstate(rand_repl_state);
      cache->repl_ctrs[set] = rand() % cache->assoc;
      setstate(old_rand_state);
    } break;
    case REPL_NOT_MRU:
      if(way == cache->repl_ctrs[set])
        cache->repl_ctrs[set] = CIRC_INC2(cache->repl_ctrs[set], cache->assoc);
      break;
    case REPL_ROUND_ROBIN:
      cache->repl_ctrs[set] = CIRC_INC2(cache->repl_ctrs[set], cache->assoc);
      break;
    case REPL_IDEAL:
      /* no need to do anything here (nothing changes when we hit on a sure
       * line) */
      /* all unsure hits are handled elsewhere */
      break;
    case REPL_LOW_PREF:
      /* low priority to prefetcher data */
      {
        int     lru_ind  = -1;
        Counter lru_time = MAX_CTR;
        int     ii;
        // cache->repl_ctrs[set] = ....
        for(ii = 0; ii < cache->assoc; ii++) {
          Cache_Entry* entry = &cache->entries[set][ii];
          if(!entry->valid) {
            lru_ind = ii;
            break;
          }
          // compare between prefetcher
          if((entry->last_access_time < lru_time) &&
             cache->entries[set][ii].pref) {
            lru_ind  = ii;
            lru_time = cache->entries[set][ii].last_access_time;
          }
        }
        if(lru_ind == -1) {
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(entry->last_access_time < lru_time) {
              lru_ind  = ii;
              lru_time = cache->entries[set][ii].last_access_time;
            }
          }
        }
        cache->repl_ctrs[set] = lru_ind;
      }
      break;
    default:
      ASSERT(0, FALSE);
  }
}


/**************************************************************************************/
/* access_unsure_lines: */

static inline void* access_unsure_lines(Cache* cache, uns set, Addr tag,
                                        Flag update_repl) {
  List*        list = &cache->unsure_lists[set];
  Cache_Entry* temp;
  int          ii;

  for(temp = (Cache_Entry*)list_start_head_traversal(list); temp;
      temp = (Cache_Entry*)list_next_element(list)) {
    ASSERT(0, temp->valid);
    if(temp->tag == tag) {
      for(ii = 0; ii < cache->assoc; ii++) {
        if(!cache->entries[set][ii].valid) {
          void* data = cache->entries[set][ii].data;
          memcpy(&cache->entries[set][ii], temp, sizeof(Cache_Entry));
          temp->data = data;
          ASSERT(0, dl_list_remove_current(list) == temp);
          ASSERT(0, ++cache->repl_ctrs[set] <=
                      cache->assoc); /* repl ctr holds the sure count */
          if(cache->repl_ctrs[set] == cache->assoc) {
            for(temp = (Cache_Entry*)list_start_head_traversal(list); temp;
                temp = (Cache_Entry*)list_next_element(list)){
                  // ASSERT(0, temp->valid);
                  // if(temp->tag == tag) {
                    free(temp->data);
                    // // dl_list_remove_current(list);
                    // // return;
                  // }
                }
            clear_list(&cache->unsure_lists[set]);
            //   free(temp->data);
            // clear_list(&cache->unsure_lists[set]);
          }
          return cache->entries[set][ii].data;
        }
      }
      ASSERT(0, FALSE);
    }
  }
  return NULL;
}


/**************************************************************************************/
/* insert_sure_line: */

static inline Cache_Entry* insert_sure_line(Cache* cache, uns set, Addr tag) {
  List* list = &cache->unsure_lists[set];
  int   ii;
  if(list_get_head(list) || cache->repl_ctrs[set] == cache->assoc) {
    /* if there is an unsure list already, or if we have all sure entries... */
    int count = 0;
    for(ii = 0; ii < cache->assoc; ii++) {
      Cache_Entry* entry = &cache->entries[set][ii];
      if(entry->valid) {
        Cache_Entry* temp = (Cache_Entry*)dl_list_add_tail(list);
        memcpy(temp, entry, sizeof(Cache_Entry));
        temp->data = malloc(sizeof(cache->data_size));
        memcpy(entry->data, temp->data, sizeof(cache->data_size));
        entry->valid = FALSE;
        count++;
      }
    }
    ASSERT(0, count == cache->repl_ctrs[set]);
    cache->repl_ctrs[set] = 1;
    return &cache->entries[set][0];
  } else {
    /* there should be an invalid entry to use */
    for(ii = 0; ii < cache->assoc; ii++) {
      Cache_Entry* entry = &cache->entries[set][ii];
      if(!entry->valid) {
        ASSERT(0, ++cache->repl_ctrs[set] <= cache->assoc);
        return entry;
      }
    }
    ASSERT(0, FALSE);
  }
}


/**************************************************************************************/
/* invalidate_unsure_line: */

static inline void invalidate_unsure_line(Cache* cache, uns set, Addr tag) {
  List*        list = &cache->unsure_lists[set];
  Cache_Entry* temp;
  for(temp = (Cache_Entry*)list_start_head_traversal(list); temp;
      temp = (Cache_Entry*)list_next_element(list)) {
    ASSERT(0, temp->valid);
    if(temp->tag == tag) {
      free(temp->data);
      dl_list_remove_current(list);
      return;
    }
  }
}

void* access_shadow_lines(Cache* cache, uns set, Addr tag) {
  uns     ii;
  uns     lru_ind  = 0;
  Counter lru_time = MAX_CTR;

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->shadow_entries[set][ii];

    if(line->tag == tag && line->valid) {
      int jj;
      /* found the entry in the shadow cache */
      ASSERT(0, line->data);
      DEBUG(0,
            "Found line in shadow cache '%s' at (set %u, way %u, base 0x%s)\n",
            cache->name, set, ii, hexstr64s(line->base));

      /* search for the oldest entry */

      for(jj = 0; jj < cache->assoc; jj++) {
        Cache_Entry* entry = &cache->entries[set][jj];
        if(!entry->valid) {
          lru_ind = jj;
          break;
        }
        if(entry->last_access_time < lru_time) {
          lru_ind  = jj;
          lru_time = cache->entries[set][jj].last_access_time;
        }
      }


      if(lru_time < line->last_access_time) {
        Cache_Entry tmp_line;
        DEBUG(0,
              "shadow cache line will be swaped:\ncache->addr:0x%s "
              "cache->lru_time:%lld  shadow_tag:0x%s shadow_insert:%lld \n",
              hexstr64s((cache->entries[set][lru_ind]).tag),
              (uns64)(cache->entries[set][lru_ind]).last_access_time,
              hexstr64s(line->tag), (uns64)line->last_access_time);

        /* shadow entry can be inserted to main cache  */

        tmp_line                       = (cache->entries[set][lru_ind]);
        (cache->entries[set][lru_ind]) = *line;
        *line                          = tmp_line;
        line->last_access_time =
          (cache->entries[set][lru_ind]).last_access_time;
        (cache->entries[set][lru_ind]).last_access_time = sim_time;
        DEBUG(0,
              "shadow cache line is swaped\n cache->addr:0x%s "
              "cache->lru_time:%lld  shadow_tag:0x%s shadow_insert:%lld \n",
              hexstr64s((cache->entries[set][lru_ind]).tag),
              (uns64)(cache->entries[set][lru_ind]).last_access_time,
              hexstr64s(line->tag), (uns64)line->last_access_time);
        /* insert counter later */
        return (line->data);
      } else {
        /* make invalidate for that cache entry */
        DEBUG(0,
              "shadow cache can't find the replacment target: cache_tag:0x%s "
              "lru_time:%lld, insert_time:%lld\n",
              hexstr64s(cache->entries[set][lru_ind].tag), (uns64)lru_time,
              (uns64)line->last_access_time);
        line->valid = FALSE; /* so we don't have 2 copies for the same data */
      }
    }
  }

  DEBUG(0, "Didn't find line in set %u in shadow cache '%s' \n", set,
        cache->name);
  return NULL;
}

void* shadow_cache_insert(Cache* cache, uns set, Addr tag, Addr base) {
  int          ii;
  Cache_Entry* new_line;

  /* within the shadow cache entry, we need to use the true replacement policy
   */

  uns     lru_ind  = 0;
  Counter lru_time = MAX_CTR;
  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* entry = &cache->shadow_entries[set][ii];
    if(!entry->valid) {
      lru_ind = ii;
      break;
    }
    if(entry->last_access_time < lru_time) {
      lru_ind  = ii;
      lru_time = cache->shadow_entries[set][ii].last_access_time;
    }
  }

  new_line                   = &(cache->shadow_entries[set][lru_ind]);
  new_line->valid            = TRUE;
  new_line->tag              = tag;
  new_line->base             = base;
  new_line->last_access_time = sim_time;
  DEBUG(0,
        "Insert Shadow cache (set %u, way %u, tag 0x%s, base 0x%s) "
        "last_access_time:%lld : sim_time:%lld\n",
        set, lru_ind, hexstr64s(tag), hexstr64s(base),
        (uns64)new_line->last_access_time, (uns64)sim_time);
  return new_line;
}

#define QUEUE_IND(num) (((num) + cache->queue_end[set]) % ideal_num_entries)

void* access_ideal_storage(Cache* cache, uns set, Addr tag, Addr addr) {
  uns          ii;
  uns          valid_start = 0;
  Cache_Entry* new_line;
  int          main_entry_found = FALSE;

  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];
    if(line->tag == tag && line->valid) {
      line->last_access_time = sim_time;
      main_entry_found       = TRUE;
    }
  }

  for(ii = 0; ii < ideal_num_entries; ii++) {
    Cache_Entry* line = &cache->shadow_entries[set][ii];
    if(line->tag == tag && line->valid) {
      uns jj;
      ASSERT(0, line->data);
      if(ii ==
         (cache->queue_end[set] + ideal_num_entries - 1) % ideal_num_entries)
        return line->data;
      for(jj = ((ii - cache->queue_end[set]) % ideal_num_entries);
          jj < ideal_num_entries; jj++) {
        Cache_Entry* cal_line = &cache->shadow_entries[set][QUEUE_IND(jj)];
        if(cal_line->valid) {
          cal_line->last_access_time--;
          DEBUG(0,
                "counter is decreasing. set:%d, queue_end:%u jj:%u ind:%u  "
                "counter:%lld, addr:0x%s\n",
                set, cache->queue_end[set], jj, QUEUE_IND(jj),
                cal_line->last_access_time, hexstr64s(cal_line->base));
          if((cal_line->valid) && (cal_line->last_access_time == 0))
            valid_start = jj;
        }
      }
      for(jj = 0; jj <= valid_start; jj++) {
        Cache_Entry* cal_line = &cache->shadow_entries[set][QUEUE_IND(jj)];
        cal_line->valid       = 0;
        DEBUG(0,
              "Last counter:%u is 0. invalidated ideal storage set:%d, jj:%u "
              "ind:%u counter:%lld, addr:0x%s\n",
              valid_start, set, jj, QUEUE_IND(jj), cal_line->last_access_time,
              hexstr64s(cal_line->base));
      }

      DEBUG(0, "data is found in ideal storage set%u \n", set);

      new_line        = &(cache->shadow_entries[set][cache->queue_end[set]]);
      new_line->valid = TRUE;
      new_line->tag   = tag;
      new_line->base  = addr;
      new_line->last_access_time = cache->assoc;
      cache->queue_end[set] = (cache->queue_end[set] + 1) % ideal_num_entries;
      return (line->data);
    }
  }


  DEBUG(0, "Didn't find line in set %u in ideal_storage cache '%s' \n", set,
        cache->name);
  if(main_entry_found) {
    DEBUG(0, "Only_main set:%u addr:0x%s cycle_time:%lld\n", set,
          hexstr64s(addr), sim_time);
  }
  return NULL;
}


/**************************************************************************************/
/* get_cache_line_addr: */

Addr get_cache_line_addr(Cache* cache, Addr addr) {
  Addr tag;
  Addr line_addr;
  cache_index(cache, addr, &tag, &line_addr);

  return line_addr;
}


/**************************************************************************************/
/* cache_insert_lru: returns a pointer to the data section of the new cache
   line.  Sets line_addr to the address of the first block of the new line.
   Sets repl_line_addr to the address of the first block that was replaced

   DON'T call this unless you are sure that the line is not in the
   cache (call after cache_access returned NULL)

   This function inserts the entry as LRU instead of MRU
*/

void* cache_insert_lru(Cache* cache, uns8 proc_id, Addr addr, Addr* line_addr,
                       Addr* repl_line_addr, Load_Type load_type) {
  Addr         tag;
  uns          repl_index;
  uns          set = cache_index(cache, addr, &tag, line_addr);
  Cache_Entry* new_line;

  if(cache->repl_policy == REPL_IDEAL) {
    new_line        = insert_sure_line(cache, set, tag);
    *repl_line_addr = 0;
  } else {
    new_line = find_repl_entry(cache, proc_id, set, &repl_index, load_type, addr);
    /* before insert the data into cache, if the cache has shadow entry */
    /* insert that entry to the shadow cache */
    if((cache->repl_policy == REPL_SHADOW_IDEAL) && new_line->valid)
      shadow_cache_insert(cache, set, new_line->tag, new_line->base);
    if(new_line->valid)  // bug fixed. 4/26/04 if the entry is not valid,
                         // repl_line_addr should be set to 0
      *repl_line_addr = new_line->base;
    else
      *repl_line_addr = 0;
    DEBUG(0,
          "Replacing (set %u, way %u, tag 0x%s, base 0x%s) in cache '%s' with "
          "base 0x%s\n",
          set, repl_index, hexstr64s(new_line->tag), hexstr64s(new_line->base),
          cache->name, hexstr64s(*line_addr));
  }

  new_line->proc_id = proc_id;
  new_line->valid   = TRUE;
  new_line->tag     = tag;
  // Not used function
  if(cache->repl_policy == REPL_DEBUG||cache->repl_policy == REPL_MOCKINGJAY||cache->repl_policy == REPL_EXECUTION_COUNT){
    new_line->addr = addr;
  }
  new_line->base    = *line_addr;
  // Not used function
  update_repl_policy(cache, new_line, set, repl_index, TRUE, TYPE_LOAD, addr);
  if(cache->repl_policy == REPL_TRUE_LRU)
    new_line->last_access_time = 137;

  if(cache->repl_policy == REPL_IDEAL_STORAGE) {
    new_line->last_access_time = cache->assoc;
    /* debug */
    /* insert into the entry also */
    {
      uns          lru_ind  = 0;
      Counter      lru_time = MAX_CTR;
      Cache_Entry* main_line;
      uns          ii;

      /* first cache access */
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* line = &cache->entries[set][ii];

        if(line->tag == tag && line->valid) {
          /* update replacement state if necessary */
          ASSERT(0, line->data);
          line->last_access_time = sim_time;
          return new_line->data;
        }
      }
      /* looking for lru */
      for(ii = 0; ii < cache->assoc; ii++) {
        Cache_Entry* entry = &cache->entries[set][ii];
        if(!entry->valid) {
          lru_ind = ii;
          break;
        }
        if(entry->last_access_time < lru_time) {
          lru_ind  = ii;
          lru_time = cache->entries[set][ii].last_access_time;
        }
      }
      main_line                   = &cache->entries[set][lru_ind];
      main_line->valid            = TRUE;
      main_line->tag              = tag;
      main_line->base             = *line_addr;
      main_line->last_access_time = sim_time;
    }
  }
  return new_line->data;
}

/**************************************************************************************/
/* reset cache: A function that initializes all lines to invalid state*/

void reset_cache(Cache* cache) {
  uns ii, jj;

  for(ii = 0; ii < cache->num_sets; ii++) {
    for(jj = 0; jj < cache->assoc; jj++) {
      cache->entries[ii][jj].valid = FALSE;
    }
  }
}

/**************************************************************************************/
/* cache_find_pos_in_lru_stack: returns the position of a cache line */
/* return -1 : cache miss  */
/*         0 : MRU         */
/*        ...........     */
/*   assoc-1 : LRU         */

int cache_find_pos_in_lru_stack(Cache* cache, uns8 proc_id, Addr addr,
                                Addr* line_addr) {
  Addr         tag;
  uns          set = cache_index(cache, addr, &tag, line_addr);
  uns          ii;
  int          position;
  Cache_Entry* hit_line = NULL;
  Flag         hit      = FALSE;

  for(ii = 0; ii < cache->assoc; ii++) {
    hit_line = &cache->entries[set][ii];

    if(hit_line->valid && hit_line->tag == tag) {
      hit = TRUE;
      break;
    }
  }

  if(!hit)
    return -1;

  ASSERT(0, hit_line);
  ASSERT(0, hit_line->proc_id == proc_id);
  position = 0;
  for(ii = 0; ii < cache->assoc; ii++) {
    Cache_Entry* line = &cache->entries[set][ii];
    if(hit_line->proc_id == line->proc_id &&
       line->last_access_time > hit_line->last_access_time) {
      position++;
    }
  }
  return position;
}


void set_partition_allocate(Cache* cache, uns8 proc_id, uns num_ways) {
  ASSERT(proc_id, cache->repl_policy == REPL_PARTITION);
  ASSERT(proc_id, cache->num_ways_allocted_core);
  cache->num_ways_allocted_core[proc_id] = num_ways;
}


uns get_partition_allocated(Cache* cache, uns8 proc_id) {
  ASSERT(proc_id, cache->repl_policy == REPL_PARTITION);
  ASSERT(proc_id, cache->num_ways_allocted_core);
  return cache->num_ways_allocted_core[proc_id];
}



/**************************************************************************************/
/* add_access_to_liveness_history: */

static inline void* add_access_to_liveness_history(Cache* cache, uns set, Addr tag) {
  //The current timestep 
  uns64 curr_quanta = cache->OPT[set].perset_mytimer % OPTGEN_VECTOR_SIZE;
  cache->OPT[set].opt_access = cache->OPT[set].opt_access + 1;
  (cache->OPT[set].liveness_history[curr_quanta]) =0; //=0; // +=1;
  cache->OPT[set].tag_history[curr_quanta] = tag;
}


/**************************************************************************************/
/* should_opt_cache: */
static inline Flag should_opt_cache(Cache* cache, uns set, Addr tag, Addr addr, uns lines_found) {
  Flag is_cache = TRUE;

  //The current timestep 
  uns64 curr_quanta = cache->OPT[set].perset_mytimer % OPTGEN_VECTOR_SIZE;
  uns64 last_quanta;
  int8 last_line_num;
  Flag has_history = find_last_quanta(cache, addr, &last_quanta, &last_line_num, set);
  if (!has_history) {
    if (PRINT_FUNC) 
    printf("Don't have addr history\n");
    return FALSE;
  }
  //addr_history[set][tag].last_quanta % OPTGEN_VECTOR_SIZE;
  if (PRINT_FUNC) 
  printf("cur quanta %llu, last quanta %llu\n", curr_quanta, last_quanta);

  uns add_count=1; // otherwise we guess a 1
  if (lines_found>0){ // hit
    add_count = last_line_num;
  } else if (last_quanta>0){ // miss
    add_count = last_quanta;
  }

  int64 i = last_quanta;
  while (i != curr_quanta)
  {
      if(cache->OPT[set].liveness_history[i] >= cache->assoc)//UOP_CACHE_SIZE) /*Currently exclusive for uop cache*/
      {
          if (PRINT_FUNC) 
          printf("It should not be stored because of %u\n", cache->OPT[set].liveness_history[i]);
          is_cache = FALSE;
          break;
      }

      i = (i+1) % OPTGEN_VECTOR_SIZE;
  }

  //if ((is_cache) && (last_quanta != curr_quanta))
  if ((is_cache))
  {

    
      
      if (last_quanta>curr_quanta){
        for (i = 0; i < curr_quanta; i++)
        {
          cache->OPT[set].liveness_history[i]+=add_count;
        }
        for (i = last_quanta; i < OPTGEN_VECTOR_SIZE; i++)
        {
          cache->OPT[set].liveness_history[i]+=add_count;
        }
      } else {
        i = last_quanta;
        while (i != curr_quanta)
        {
            cache->OPT[set].liveness_history[i]+=add_count;
            i = (i+1) % OPTGEN_VECTOR_SIZE;
        }
        ASSERT(0, i == curr_quanta);
      }
      
  }

  // if (!is_cache){
  //       if (last_quanta>curr_quanta){
  //       for (i = 0; i < curr_quanta; i++)
  //       {
  //         cache->OPT[set].liveness_history[i]--;
  //       }
  //       for (i = last_quanta; i < OPTGEN_VECTOR_SIZE; i++)
  //       {
  //         cache->OPT[set].liveness_history[i]--;
  //       }
  //     } else {
  //       i = last_quanta;
  //       while (i != curr_quanta)
  //       {
  //           cache->OPT[set].liveness_history[i]--;
  //           i = (i+1) % OPTGEN_VECTOR_SIZE;
  //       }
  //       ASSERT(0, i == curr_quanta);
  //     }
  // }

  // //if ((is_cache) && (last_quanta != curr_quanta))
  // if ((is_cache))
  // {
      
  //     if (last_quanta>curr_quanta){
  //       for (i = 0; i < curr_quanta; i++)
  //       {
  //         cache->OPT[set].liveness_history[i]++;
  //       }
  //       for (i = last_quanta+1; i < OPTGEN_VECTOR_SIZE; i++)
  //       {
  //         cache->OPT[set].liveness_history[i]++;
  //       }
  //     } else {
  //       i = last_quanta+1;
  //       while (i != curr_quanta)
  //       {
  //           cache->OPT[set].liveness_history[i]++;
  //           i = (i+1) % OPTGEN_VECTOR_SIZE;
  //       }
  //       ASSERT(0, i == curr_quanta);
  //     }
      
  // }

  if (is_cache) cache->num_cache_opt++;
  else cache->num_dont_cache_opt++;

  // if (curr_quanta==last_quanta) is_cache = FALSE;

  return is_cache; 
}

/**************************************************************************************/
/* key_compare: */
static inline int key_compare(const void *l, const void *r) {
  const Key_Map *lm = l;
  const Key_Map *lr = r;
  return lm->addr - lr->addr;
}

static inline int key_compare_concat(const void *l, const void *r) {
  const Key_Map *lm = l;
  const Key_Map *lr = r;
  // if (strcmp(hexstr64s(lm->addr), "7f9a1480cf40")==0) printf("Detected!\n");
  if (lm->addr - lr->addr + lm->addr_end - lr->addr_end==0){
    // printf("In key_compare_concat, ori addr 0x%s, compared addr 0x%s, ori addr_end 0x%s, compared addr_end 0x%s\n",
    // hexstr64s(lm->addr), hexstr64s(lr->addr), hexstr64s(lm->addr_end), hexstr64s(lr->addr_end));
  }
  
  return lm->addr - lr->addr + lm->addr_end - lr->addr_end;
}

// static inline int key_compare(Addr tag1, Addr tag2) {
//   return tag1-tag2;
// }

/**************************************************************************************/
/* initialize_quanta: */
uns64* initialize_quanta(Cache* cache, Addr addr, int64 quanta, uns set) {
    
  Key_Map *a_find = malloc(sizeof(Key_Map));
  a_find->addr = addr;
  void *val = tfind(a_find, &cache->map_root, key_compare); /* find */
  // printf("Init finds %s\n", (*(Key_Map**)val)->quanta);
  // ASSERT(0, val == NULL);

  if (val!=NULL){
    val = tdelete(a_find, &cache->map_root, key_compare); /* insert */
    // free(a_find->addr);
    free(a_find);
  }

  // // Add dump one to avoid tree is empty case.
  // Key_Map *dump = malloc(sizeof(Key_Map));
  // dump->addr = 0x1;
  // dump->quanta = 1;
  // val = tsearch(dump, &cache->map_root, key_compare); /* insert */

  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  a->last_quanta = quanta;
  a->last_line_num = 0; // by default set it to be 0
  val = tsearch(a, &cache->map_root, key_compare); /* insert */
  if (PRINT_FUNC) printf("Init adds %u at addr:0x%s\n", (*(Key_Map**)val)->last_quanta, hexstr64s((*(Key_Map**)val)->addr));
  ASSERT(0, val != NULL);


}


/**************************************************************************************/
/* find_last_quanta: */
Flag find_last_quanta(Cache* cache, Addr addr, int64* last_quanta, int8 *last_line_num, uns set) {
  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  if(PRINT_FUNC) printf("a->addr is %llu, root is %u\n", a->addr, cache->map_root);
  void *val = tfind(a, &cache->map_root, key_compare); /* insert */
  // ASSERT(0, val != NULL);
  // free(a->addr);
  free(a);
  if (val!=NULL) {
    if (PRINT_FUNC) 
    printf("Find finds %u %u at addr:0x%s\n", (*(Key_Map**)val)->last_quanta, (*(Key_Map**)val)->last_line_num, hexstr64s((*(Key_Map**)val)->addr));
    *last_quanta = (*(Key_Map**)val)->last_quanta;
    if ((*(Key_Map**)val)->last_line_num!=NULL && cache->repl_policy!=REPL_EXECUTION_COUNT){
      *last_line_num = (*(Key_Map**)val)->last_line_num;
    }
    return TRUE;
  } else return FALSE;
  // return (*(Key_Map**)val)->last_quanta;

}

// Cache_Entry return_quanta_data(Cache* cache, Addr addr){

//   Key_Map *a = malloc(sizeof(Key_Map));
//   a->addr = addr;
//   if(PRINT_FUNC) printf("a->addr is %llu, root is %u\n", a->addr, cache->map_root);
//   void *val = tfind(a, &cache->map_root, key_compare); /* insert */
//   ASSERT(0, val != NULL);
//   if (val!=NULL) {
//     if (PRINT_FUNC) 
//     printf("Find finds %u %u at addr:0x%s\n", (*(Key_Map**)val)->last_quanta, (*(Key_Map**)val)->last_line_num, hexstr64s((*(Key_Map**)val)->addr));
//     return ((*(Key_Map**)val)->entry);
//   } //else return FALSE;
  
// }

int8 return_quanta_line_num(Cache* cache, Addr addr){

  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  if(PRINT_FUNC) printf("a->addr is %llu, root is %u\n", a->addr, cache->map_root);
  void *val = tfind(a, &cache->map_root, key_compare); /* insert */
  ASSERT(0, val != NULL);
  if (val!=NULL) {
    if (PRINT_FUNC) 
    printf("Find finds %u %u at addr:0x%s\n", (*(Key_Map**)val)->last_quanta, (*(Key_Map**)val)->last_line_num, hexstr64s((*(Key_Map**)val)->addr));
    return (*(Key_Map**)val)->last_line_num;
  } //else return FALSE;

}
      
void add_quanta_data(Cache* cache, Addr addr, int64 quanta, int8 line_num, void** data){

  Key_Map *a_find = malloc(sizeof(Key_Map));
  a_find->addr = addr;
  void *val = tfind(a_find, &cache->map_root, key_compare); /* find */
  // printf("Update finds %u\n", (*(Key_Map**)val)->last_quanta);
  // ASSERT(0, val != NULL);

  val = tdelete(a_find, &cache->map_root, key_compare); /* insert */

  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  a->last_quanta = quanta;
  a->last_line_num = line_num;
  // a->data = *data;
  val = tsearch(a, &cache->map_root, key_compare); /* insert */
  if (PRINT_FUNC) 
  printf("Update data adds %u %u, at addr:0x%s\n", (*(Key_Map**)val)->last_quanta,  (*(Key_Map**)val)->last_line_num, hexstr64s((*(Key_Map**)val)->addr));
  ASSERT(0, val != NULL);

}

/**************************************************************************************/
/* update_quanta: */
void* update_quanta(Cache* cache, Addr addr, int64 quanta, int8 line_num, uns set, int8 target_assoc, Flag cache_real_hit) {

  Key_Map *a_find = malloc(sizeof(Key_Map));
  a_find->addr = addr;
  void *val = tfind(a_find, &cache->map_root, key_compare); /* find */
  // printf("Update finds %u\n", (*(Key_Map**)val)->last_quanta);
  // ASSERT(0, val != NULL);

  val = tdelete(a_find, &cache->map_root, key_compare); /* insert */

  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  a->last_quanta = quanta;
  a->last_line_num = line_num;
  int8 chosen_assoc = target_assoc;
  if (!cache_real_hit) chosen_assoc=1;
  // if (FALSE){//cache_real_hit){
  //   // a->data = *data;
  //   Cache_Entry temp_entry;
  //   // temp_entry = malloc(sizeof(Cache_Entry));
  //   temp_entry.valid = TRUE;
  //   temp_entry.data = (void*)malloc(sizeof(Uop_Cache_Data));
  //   memset(temp_entry.data, cache->entries[set][chosen_assoc].data, sizeof(Uop_Cache_Data));
  //   if (PRINT_FUNC) printf("temp_entry2 data is at addr:0x%s",hexstr64s(&(cache->entries[set][chosen_assoc].data)));
  //   // temp_entry->data = cache->entries[set][chosen_assoc].data;
  //   temp_entry.proc_id = cache->entries[set][chosen_assoc].proc_id;
  //   temp_entry.valid = cache->entries[set][chosen_assoc].valid;
  //   temp_entry.tag = cache->entries[set][chosen_assoc].tag;
  //   temp_entry.base = cache->entries[set][chosen_assoc].base;
  //   temp_entry.last_access_time = cache->entries[set][chosen_assoc].last_access_time;
  //   temp_entry.pref = cache->entries[set][chosen_assoc].pref;
  //   temp_entry.pw_start_addr = cache->entries[set][chosen_assoc].pw_start_addr;
  //   a->entry = temp_entry;
  // }
  val = tsearch(a, &cache->map_root, key_compare); /* insert */
  if (PRINT_FUNC) 
  printf("Update adds %u %u, at addr:0x%s, hit is %d\n", (*(Key_Map**)val)->last_quanta,  (*(Key_Map**)val)->last_line_num, hexstr64s((*(Key_Map**)val)->addr), cache_real_hit);
  ASSERT(0, val != NULL);

  // free(a_find->addr);
  free(a_find);

}


/**************************************************************************************/
/* invalidate_quanta: */
void* invalidate_quanta(Cache* cache, Addr addr, uns set) {

  Key_Map *a_find = malloc(sizeof(Key_Map));
  a_find->addr = addr;
  void *val = tfind(a_find, &cache->map_root, key_compare); /* find */
  //printf("Inv finds %u\n", (*(Key_Map**)val)->last_quanta);
  // ASSERT(0, val != NULL);

  val = tdelete(a_find, &cache->map_root, key_compare); /* insert */
  // free(a_find->addr);
  free(a_find);
  //printf("Inv deleted\n");

}


/**************************************************************************************/
/* after_eviction: */
static inline void after_eviction(Cache* cache, uns set, uns* way, Flag has_invalid) {
    uns ii;
    uns rrip_ind  = cache->assoc;
    uns psel_msb;
    switch (cache->drrip_repl_policy[set]) {
    case TYPE_SRRIP:
      if (PRINT_FUNC){
        printf("after_eviction Checking access1 '%s' at (set %u)\n", cache->name, set);
        for(int ii=0;ii<cache->assoc;ii++){
          printf("%d ", cache->entries[set][ii].rrpv);
        }
        printf("\n");
      }
      if (has_invalid){
        for(ii = 0; ii < cache->assoc; ii++) {
          Cache_Entry* entry = &cache->entries[set][ii];
          if(!entry->valid) {
            rrip_ind = ii;
            entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
            break;
          }
        }
      } else {
        for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
              rrip_ind  = ii;
              entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
              break;
            }
          }
      }
      if (cache->psel_counter < 1023) // 1024 in Kalla's code
        cache->psel_counter++;
      break;

    case TYPE_BRRIP:
      if (cache->brrip_counter == 1) {
        if (has_invalid){
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              rrip_ind = ii;
              entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
              break;
            }
          }
        } else {
          for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                rrip_ind  = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                break;
              }
            }
        }
      } else {
        if (has_invalid){
          for(ii = 0; ii < cache->assoc; ii++) {
            Cache_Entry* entry = &cache->entries[set][ii];
            if(!entry->valid) {
              rrip_ind = ii;
              entry->rrpv = pow(2, RRIP_BIT_SIZE)-1;
              break;
            }
          }
        } else {
          for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                rrip_ind  = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-1;
                break;
              }
            }
        }
      }
      if (cache->psel_counter > 0)
        cache->psel_counter--;
      if (cache->brrip_counter < 32)
        cache->brrip_counter++;
      else
        cache->brrip_counter = 0;
      break;

    case TYPE_FOLLOWER:
      psel_msb = (cache->psel_counter >> 9);
      if (psel_msb == 0) // following SRRIP
        {
          if (has_invalid){
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(!entry->valid) {
                rrip_ind = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                break;
              }
            }
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                rrip_ind  = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                break;
              }
            }
          }
        }
      else // following BRRIP
          if (cache->brrip_counter == 1)
        {
          if (has_invalid){
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(!entry->valid) {
                rrip_ind = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                break;
              }
            }
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                rrip_ind  = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-2;
                break;
              }
            }
          }
        }
      else
        {
          if (has_invalid){
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(!entry->valid) {
                rrip_ind = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-1;
                break;
              }
            }
          } else {
            for(ii = 0; ii < cache->assoc; ii++) {
              Cache_Entry* entry = &cache->entries[set][ii];
              if(entry->rrpv == pow(2, RRIP_BIT_SIZE)-1) {
                rrip_ind  = ii;
                entry->rrpv = pow(2, RRIP_BIT_SIZE)-1;
                break;
              }
            }
          }
        }
    }
  *way = rrip_ind;
}


Flag is_sampled_set(int32 set, int32 num_sets, int32 assoc) {
    int32 mask_length = LOG2_SET(num_sets)-LOG2_SAMPLED_SETS(num_sets, assoc); //7
    int32 mask = 1;
    for (size_t i = 0; i < mask_length; i++)
    {
      mask = mask<<1;
    }
    mask = mask - 1;
    uns tmp = set;
    for (size_t i = 0; i < (LOG2_SET(num_sets) - mask_length); i++)
    {
      tmp = tmp>>1;
    }
    return (set & mask) == (tmp & mask);
}

uns64 CRC_HASH( uns64 _blockAddress )
{
    static const uns64 crcPolynomial = 3988292384ULL;
    uns64 _returnVal = _blockAddress;
    for( uns i = 0; i < 3; i++)
        _returnVal = ( ( _returnVal & 1 ) == 1 ) ? ( ( _returnVal >> 1 ) ^ crcPolynomial ) : ( _returnVal >> 1 );
    return _returnVal;
}

uns64 get_pc_signature(uns64 pc, Flag hit, Flag prefetch, uns32 core, int32 num_sets, int32 assoc) {
        pc = pc << 1;
        if(hit) {
            pc = pc | 1;
        }
        pc = pc << 1;
        if (prefetch) {
            pc = pc | 1;                            
        }
        pc = CRC_HASH(pc);
        for (size_t i = 0; i < (64 - PC_SIGNATURE_BITS(num_sets, assoc)); i++)
        {
          pc = pc<<1;
        }
        for (size_t i = 0; i < (64 - PC_SIGNATURE_BITS(num_sets, assoc)); i++)
        {
          pc = pc>>1;
        }
        // pc = (pc << X) >> Y;
    return pc;
}

// Sampled Cache is indexed using a concatenation of the 5 set id bits 
// that identify the 32 sampled sets and the bits [3:0] of the block address tag.  
// SampledCache lines are tagged with bits [13:4] of the block addresstag

uns32 get_sampled_cache_index(uns64 full_addr, uns32 set, uns num_sets) {

    // uns32 mask = 1<<LOG2_BLOCK_SIZE-1;
    // uns32 tmpright =  (full_addr>>LOG2_BLOCK_SIZE) & mask;
    // uns32 index = (set<<LOG2_BLOCK_SIZE) | tmpright;
    // return index;

    full_addr = full_addr >> LOG2_BLOCK_SIZE;
    for (size_t i = 0; i < (64 - (LOG2_SAMPLED_CACHE_SETS + LOG2_SET(num_sets))); i++)
    {
      full_addr = full_addr<<1;
    }
    for (size_t i = 0; i < (64 - (LOG2_SAMPLED_CACHE_SETS + LOG2_SET(num_sets))); i++)
    {
      full_addr = full_addr>>1;
    }
    // full_addr = (full_addr << X) >> Y;
    return full_addr;
}

uns64 get_sampled_cache_tag(uns64 x, uns num_sets, uns assoc) {
    // uns32 mask = 1<<SAMPLED_CACHE_TAG_BITS(num_sets,assoc)-1;
    // uns32 tmpright =  (x>>LOG2_BLOCK_SIZE) >> LOG2_BLOCK_SIZE;
    // uns64 tag = tmpright & mask;
    // return tag;

    for (size_t i = 0; i < LOG2_SET(num_sets) + LOG2_BLOCK_SIZE + LOG2_SAMPLED_CACHE_SETS; i++)
    {
      x = x>>1;
    }
    // x >>= X; //x=x>>
    for (size_t i = 0; i < (64 - SAMPLED_CACHE_TAG_BITS(num_sets,assoc) ); i++)
    {
      x = x<<1;
    }
    for (size_t i = 0; i < (64 - SAMPLED_CACHE_TAG_BITS(num_sets,assoc) ); i++)
    {
      x = x>>1;
    }
    // x = (x << X) >> Y;
    return x;
}

int32 search_sampled_cache(uns64 blockAddress, uns32 set, Cache* cache) {
  
    SampledCacheLine* sampled_set = cache->sampled_cache[set];
    for (int way = 0; way < SAMPLED_CACHE_WAYS; way++) {
        if (sampled_set[way].valid && (sampled_set[way].tag == blockAddress)) {
            return way;
        }
    }
    return -1;
}

void detrain(uns32 set, int32 way, Cache *cache) {
    SampledCacheLine temp = cache->sampled_cache[set][way];
    if (!temp.valid) {
        return;
    }
    uns64 last_value;
    int8 last_line_num;
    Flag has_history = find_last_quanta(cache, temp.signature, &last_value, &last_line_num, set);
    if (has_history){//(rdp_tmp.count(temp.signature)) {
      // rdp[temp.signature] = MIN(rdp[temp.signature] + 1, INF_RD(way));
      int64 updated_value = (MIN(last_value + 1, INF_RD(cache->assoc)));
      void* data;
      update_quanta(cache, temp.signature, updated_value, 0, set, data, FALSE);
      // Flag new_entry;
      // RDP_Entry* saved = (RDP_Entry*)hash_table_access(&rdp, temp.signature);
      // hash_table_access_replace(&rdp, temp.signature, 
      //                           (MIN(saved->value + 1, INF_RD(cache->assoc))));
    } else {
      initialize_quanta(cache, temp.signature, INF_RD(cache->assoc), set);
      // Flag new_entry;
      // RDP_Entry* saved = (RDP_Entry*)hash_table_access_create(&rdp, temp.signature, 
      //                           &new_entry);
      // saved->value = INF_RD(cache->assoc);
      // rdp_tmp[temp.signature] = INF_RD(way);
    }
    cache->sampled_cache[set][way].valid = FALSE;
}

int32 temporal_difference(int32 init, int32 sample, int32 assoc) {
    if (sample > init) {
        int32 diff = sample - init;
        diff = diff * TEMP_DIFFERENCE;
        diff = MIN(1, diff);
        return MIN(init + diff, INF_RD(assoc));
    } else if (sample < init) {
        int32 diff = init - sample;
        diff = diff * TEMP_DIFFERENCE;
        diff = MIN(1, diff);
        return MAX(init - diff, 0);
    } else {
        return init;
    }
}

int32 increment_timestamp(int32 input) {
    input++;
    input = input % (1 << TIMESTAMP_BITS);
    return input;
}

int32 time_elapsed(int32 global, int32 local) {
    if (global >= local) {
        return global - local;
    }
    global = global + (1 << TIMESTAMP_BITS);
    return global - local;
}


/* called on every cache hit and cache fill */
void m_update_replacement_state(uns32 cpu, uns32 set, uns32 way, uns64 full_addr, uns64 pc, Load_Type load_type, Flag hit, Cache *cache, Addr *line_addr)
{
    Cache_Entry* entry = &cache->entries[set][way];
    if (load_type == TYPE_WRITEBACK) {
        if(!hit) {
            entry->etr = -INF_ETR(cache->assoc);
        }
        return;
    }
        

    pc = get_pc_signature(pc, hit, load_type == TYPE_PREF, 0, cache->num_sets, cache->assoc); //cpu=0 for single core


    if (is_sampled_set(set, cache->num_sets, cache->assoc)) { // TRUE
        uns32 sampled_cache_index = get_sampled_cache_index(full_addr, set, cache->num_sets);
        uns64 sampled_cache_tag = get_sampled_cache_tag(full_addr, cache->num_sets, cache->assoc);
        if (PRINT_FUNC) printf("tested sampled set is 0x%s, sample index is 0x%s, sample tag is 0x%s\n", hexstr64s(set), hexstr64s(sampled_cache_index), hexstr64s(sampled_cache_tag)); 
        // Addr tag;
        // uns32  set = cache_index(cache, full_addr, &tag, line_addr);
        int32 sampled_cache_way = search_sampled_cache(sampled_cache_tag, sampled_cache_index, cache);
        if (PRINT_FUNC) printf("sampled_cache_way is %d\n", sampled_cache_way); 
        if (sampled_cache_way > -1) {
            uns64 last_signature = cache->sampled_cache[sampled_cache_index][sampled_cache_way].signature;
            uns64 last_timestamp = cache->sampled_cache[sampled_cache_index][sampled_cache_way].timestamp;
            int64 sample = time_elapsed(cache->current_timestamp[set], last_timestamp);
            if (PRINT_FUNC) printf("sample is %d, cur timestamp is %d, last timestamp is %d\n", sampled_cache_way, cache->current_timestamp[set], last_timestamp);

            if (sample <= INF_RD(cache->assoc)) {
                if (load_type == TYPE_PREF) {
                    sample = sample * FLEXMIN_PENALTY;
                }
                int64 last_value;
                int8 last_line_num;
                Flag has_history = find_last_quanta(cache, last_signature, &last_value, &last_line_num, set);
                if (has_history){
                  int64 updated_value = temporal_difference(last_value, sample, cache->assoc);
                  void* data;
                  update_quanta(cache, last_signature, updated_value, 0, set, data, FALSE);
                } else {
                  initialize_quanta(cache, last_signature, sample, set);
                }
                cache->sampled_cache[sampled_cache_index][sampled_cache_way].valid = FALSE;
            }
        }


        int32 lru_way = -1;
        int32 lru_rd = -1;
        for (int32 w = 0; w < SAMPLED_CACHE_WAYS; w++) {
            if (cache->sampled_cache[sampled_cache_index][w].valid == FALSE) {
                lru_way = w;
                lru_rd = INF_RD(cache->assoc) + 1;
                continue;
            }

            uns64 last_timestamp = cache->sampled_cache[sampled_cache_index][w].timestamp;
            int32 sample = time_elapsed(cache->current_timestamp[set], last_timestamp);
            if (sample > INF_RD(cache->assoc)) {
                lru_way = w;
                lru_rd = INF_RD(cache->assoc) + 1;
                detrain(sampled_cache_index, w, cache);
            } else if (sample > lru_rd) {
                lru_way = w;
                lru_rd = sample;
            }
        }
        detrain(sampled_cache_index, lru_way, cache);

        for (int32 w = 0; w < SAMPLED_CACHE_WAYS; w++) {
            if (cache->sampled_cache[sampled_cache_index][w].valid == FALSE) {
                cache->sampled_cache[sampled_cache_index][w].valid = TRUE;
                cache->sampled_cache[sampled_cache_index][w].signature = pc;
                cache->sampled_cache[sampled_cache_index][w].tag = sampled_cache_tag;
                cache->sampled_cache[sampled_cache_index][w].timestamp = cache->current_timestamp[set];
                break;
            }
        }
        
        cache->current_timestamp[set] = increment_timestamp(cache->current_timestamp[set]);
    }

    if(cache->etr_clock[set] == GRANULARITY) {
        for (uns w = 0; w < cache->assoc; w++) {
            Cache_Entry* tmp_entry = &cache->entries[set][w];
            if (((uns32)w != way) && (abs(tmp_entry->etr) < INF_ETR(cache->assoc))) {
                (tmp_entry->etr)--;
            }
        }
        cache->etr_clock[set] = 0;
    }
    cache->etr_clock[set]++;
    
    
    if (way < cache->assoc) {
      uns64 last_value;
      int8 last_line_num;
      Flag has_history = find_last_quanta(cache, pc, &last_value, &last_line_num, set);
      if (!has_history){
        entry->etr = 0;
        // update_quanta(cache, pc, temporal_difference(last_value, sample, cache->assoc), set);
      } else {
        uns64 tmp = last_value;
        if(tmp > MAX_RD(cache->assoc)) {
            entry->etr = INF_ETR(cache->assoc);
        } else {
            entry->etr = tmp / GRANULARITY;
        }
        // initialize_quanta(cache, last_signature, sample, set);
      }
    }
}

// void add_to_future(Cache* cache, Addr addr, uns64 counter, Addr* line_addr, vector<vector<unordered_map<uint64_t, set<uint64_t>>>> *future) {
//         Addr         tag;
//         uns          repl_index;
//         uns          set = cache_index(cache, addr, &tag, line_addr);
//         uns64 last_value;
//     Flag has_history = find_last_quanta(&cache, update_addr, &last_value, 0);
//     if (has_history){
//       uns64 updated_value = last_value+1;
//       update_quanta(&cache, update_addr, updated_value, 0);
//     } else {
//       initialize_quanta(&cache, update_addr, 1, 0);
//     }
//     has_history = find_last_quanta(&uop_cache, update_addr, &last_value, 0);
//         auto it = (*future)[cpu][set].find(ip);
//         if (it == (*future)[cpu][set].end()) {
//             (*future)[cpu][set].emplace_hint(it, ip, set<uint64_t>());
//         }
//         (*future)[cpu][set][ip].insert(counter);
// }


// void read_record(FILE *demand_record, unordered_map<uint64_t, vector<pair<uint64_t, uint64_t>>> *twig_prefetch_match) {
//         uns64 addr, counter;
//         int type = 0;
//         while (true) {
//             if (fscanf(demand_record, "%llu %llu", &ip, &counter) == EOF) break;
//             type = 0;
            
//             vector<vector<unordered_map<uint64_t, set<uint64_t>>>> *future;
//             if (type == 0) {
//                 future = &future_accesses;
//             } else {
//                 future = &future_prefetches;
//             }
//             // NOTE: We only consider prefetch record for Twig!
//             add_to_future(cpu, ip, counter,&future_accesses);
//             // Add twig prefetch record if needed
//             if (twig_prefetch_match != nullptr) {
//                 auto it = twig_prefetch_match->find(ip);
//                 if (it != twig_prefetch_match->end()) {
//                     for (auto &a : it->second) {
//                         add_to_future(cpu, a.first, counter, &future_prefetches);
//                     }
//                 }
//             }
//         }
//         last_timestamp = counter;
//         cout << "The last timestamp: " << last_timestamp << endl;
//         rewind(demand_record);
//     }



uns64 find_future_timestamp_file(FILE *demand_record, Cache* cache, Addr addr, uns64 cur_timestamp){
  uns64 read_addr, read_counter;
  uns64 nearest_timestamp=MAX_CTR;
  // read_init();
  while(TRUE){
    if (fscanf(demand_record, "%llu %llu", &read_addr, &read_counter) == EOF) break;
    if (read_addr == addr && (cur_timestamp<read_counter||nearest_timestamp<read_counter)) {
      if (PRINT_FUNC) printf("read_addr %llu, cur_timestamp %llu, read_counter %llu\n", read_addr, cur_timestamp, read_counter);
      nearest_timestamp = read_counter;
      break;
    }
  }
  if (PRINT_FUNC && nearest_timestamp==MAX_CTR){
    printf("%llu %llu, Read to end but not found\n", read_addr, read_counter);
  } 
  // else if (nearest_timestamp!=MAX_CTR){
  // }
    uns64 last_value;
    int8 last_line_num;
    if (PRINT_FUNC) printf("cache root is %u\n", cache->map_root);
    Flag has_history = find_last_quanta(cache, addr, &last_value, &last_line_num, 0);
    if (has_history){
      uns64 updated_value = nearest_timestamp;
      void* data;
      update_quanta(cache, addr, updated_value, 0, 0, data, FALSE);
    } else {
      initialize_quanta(cache, addr, nearest_timestamp, 0);
    }
  rewind(demand_record);
  return nearest_timestamp;
}



// // Function to find the frame that will not be used
// // recently in future after given index in pg[0..pn-1]
// int new_opt_predict(Cache* cache, uns set, uns* way, Addr addr, uns64 MAX_reference, uns64 index){
// //(int pg[], vector<int>& fr, int pn, int index)


  
//   int ii;
//   uns     opt_ind  = 0;
//   Counter opt_time = MAX_CTR;
//   int res = -1, farthest = index;
//   for(ii = 0; ii < cache->assoc; ii++) {
//     Cache_Entry* entry = &cache->entries[set][ii];
//     if(!entry->valid) {
//       opt_ind = ii;
//       break;
//     }
//     int j;
//     for (j = index; j < MAX_reference; j++) {
//         if (fr[i] == pg[j]) {
//             if (j > farthest) {
//                 farthest = j;
//                 res = i;
//             }
//             break;
//         }
//     }
 
//     // If a page is never referenced in future,
//     // return it.
//     if (j == MAX_reference)
//         return i;

//   }
//   *way = opt_ind;
//   return &cache->entries[set][opt_ind];

//   // // If all of the frames were not in future,
//   // // return any of them, we return 0. Otherwise
//   // // we return res.
//   // return (res == -1) ? 0 : res;
 
    
// }


/** For concatenation of prediction window **/
/**************************************************************************************/
/* initialize_quanta: */
uns64* initialize_quanta_concat(Cache* cache, Addr addr, Addr addr_end, int64 quanta, uns set) {
    
  Key_Map *a_find = malloc(sizeof(Key_Map));
  a_find->addr = addr;
  a_find->addr_end = addr_end;
  void *val = tfind(a_find, &cache->map_root, key_compare_concat); /* find */
  // printf("Init finds %s\n", (*(Key_Map**)val)->quanta);
  // ASSERT(0, val == NULL);

  if (val!=NULL){
    if (PRINT_FUNC) 
      printf("Delete addr 0x%s addr_end 0x%s in init\n", hexstr64s(addr), hexstr64s(addr_end));
    val = tdelete(a_find, &cache->map_root, key_compare_concat); /* insert */
    // free(a_find->addr);
    free(a_find);
  }

  // // Add dump one to avoid tree is empty case.
  // Key_Map *dump = malloc(sizeof(Key_Map));
  // dump->addr = 0x1;
  // dump->quanta = 1;
  // val = tsearch(dump, &cache->map_root, key_compare_concat); /* insert */

  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  a->addr_end = addr_end;
  a->last_quanta = quanta;
  val = tsearch(a, &cache->map_root, key_compare_concat); /* insert */
  if (PRINT_FUNC) 
  printf("concat Init adds %lld at addr:0x%s, addr_end:0x%s\n", (*(Key_Map**)val)->last_quanta, hexstr64s((*(Key_Map**)val)->addr), hexstr64s((*(Key_Map**)val)->addr_end));
  ASSERT(0, val != NULL);


}


/**************************************************************************************/
/* find_last_quanta: */
Flag find_last_quanta_concat(Cache* cache, Addr addr, Addr addr_end, int64* last_quanta, int8 *last_line_num, uns set) {
  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  a->addr_end = addr_end;
  if(PRINT_FUNC) printf("a->addr is %llu, root is %u\n", a->addr, cache->map_root);
  void *val = tfind(a, &cache->map_root, key_compare_concat); /* insert */
  if(PRINT_FUNC) 
  printf("cache->map_root is %u\n", &cache->map_root);
  // ASSERT(0, val != NULL);
  // free(a->addr);
  free(a);
  if (val!=NULL) {
    if (PRINT_FUNC) 
    printf("concat Find finds %lld %u at addr:0x%s addr_end:0x%s\n", (*(Key_Map**)val)->last_quanta, (*(Key_Map**)val)->last_line_num, hexstr64s((*(Key_Map**)val)->addr), hexstr64s((*(Key_Map**)val)->addr_end));
    *last_quanta = (*(Key_Map**)val)->last_quanta;
    // *last_line_num = (*(Key_Map**)val)->last_line_num;
    return TRUE;
  } else return FALSE;
  // return (*(Key_Map**)val)->last_quanta;

}

/**************************************************************************************/
/* update_quanta: */
void* update_quanta_concat(Cache* cache, Addr addr, Addr addr_end, int64 quanta, int8 line_num, uns set, int8 target_assoc, Flag cache_real_hit) {

  Key_Map *a_find = malloc(sizeof(Key_Map));
  a_find->addr = addr;
  a_find->addr_end = addr_end;
  void *val = tfind(a_find, &cache->map_root, key_compare_concat); /* find */
  // printf("Update finds %u\n", (*(Key_Map**)val)->last_quanta);
  // ASSERT(0, val != NULL);

  val = tdelete(a_find, &cache->map_root, key_compare_concat); /* insert */
  if (PRINT_FUNC) 
  printf("Delete addr 0x%s addr_end 0x%s in update\n", hexstr64s(addr), hexstr64s(addr_end));

  Key_Map *a = malloc(sizeof(Key_Map));
  a->addr = addr;
  a->addr_end = addr_end;
  a->last_quanta = quanta;
  // a->last_line_num = line_num;
  val = tsearch(a, &cache->map_root, key_compare_concat); /* insert */
  if (PRINT_FUNC) 
  printf("concat Update adds %lld %u, at addr:0x%s, addr_end:0x%s, hit is %d\n", (*(Key_Map**)val)->last_quanta,  (*(Key_Map**)val)->last_line_num, 
  hexstr64s((*(Key_Map**)val)->addr), hexstr64s((*(Key_Map**)val)->addr_end), cache_real_hit);
  ASSERT(0, val != NULL);

  // free(a_find->addr);
  free(a_find);

}



  

