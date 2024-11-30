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
 * File         : libs/cache_lib.h
 * Author       : HPS Research Group
 * Date         : 2/6/1998
 * Description  : Header for libs/cache_lib.c
 ***************************************************************************************/

#ifndef __CACHE_LIB_H__
#define __CACHE_LIB_H__

#include "globals/global_defs.h"
#include "libs/list_lib.h"
#include <stdint.h>

/**************************************************************************************/

#define INIT_CACHE_DATA_VALUE \
  ((void*)0x8badbeef) /* set data pointers to this initially */


/**************************************************************************************/

typedef enum Repl_Policy_enum {
  REPL_TRUE_LRU,    /* actual least-recently-used replacement */
  REPL_RANDOM,      /* random replacement */
  REPL_NOT_MRU,     /* not-most-recently-used replacement */
  REPL_ROUND_ROBIN, /* round-robin replacement */
  REPL_IDEAL,       /* ideal replacement */
  REPL_ISO_PREF, /* lru with some entries (isolated misses) higher priority */
  REPL_LOW_PREF, /* prefetched data have lower priority */
  REPL_SHADOW_IDEAL,  /* ideal replacement with shadow cache */
  REPL_IDEAL_STORAGE, /* if the data doesn't have a temporal locality then it
                         isn't stored at the cache */
  REPL_MLP,           /* mlp based replacement  -- uses MLP_REPL_POLICY */
  REPL_PARTITION,     /* Based on the partition*/
  REPL_OPT,     /* Based on the Belady's algorithm and Hawkeye*/
  REPL_SRRIP,     /* Based on RRIP paper*/
  REPL_DRRIP,     /* Based on RRIP paper*/
  REPL_SHIP_PP,     /* Based on SHiP++ paper*/
  REPL_MOCKINGJAY,  /* Based on mockingjay paper */ 
  REPL_EXECUTION_COUNT,
  REPL_NEW_OPT,
  REPL_CACHEACCESS,
  REPL_COMPRATE,
  REPL_PMC,
  REPL_DEBUG,
  REPL_FOO,   /* Based on FOO Paper */
  REPL_HIT_TIMES, /* Assign Weight from Profile*/
  REPL_GHRP, /* Based on GHRP Paper */
  REPL_THERMO,
  REPL_FIX_RANDOM, /* Random at certain rate, given by RANDOM_HOTNESS */
  REPL_IC_PMC,
  NUM_REPL
} Repl_Policy;


typedef enum Drrip_Repl_Policy_enum {
  /* Based on RRIP paper*/
  TYPE_SRRIP,  
  TYPE_BRRIP, 
  TYPE_FOLLOWER,    
  NUM_DRRIP_REPL
} Drrip_Repl_Policy;

typedef enum Load_Type_enum {
  /* Based on RRIP paper*/
  TYPE_LOAD,  
  TYPE_RFO, 
  TYPE_PREF, 
  TYPE_WRITEBACK,    
  NUM_LOAD_TYPE
} Load_Type;

typedef enum Exe_Count_Type_enum {
  TYPE_EXE_COUNT_DEFAULT,
  TYPE_CACHE_ACCESS,   
  TYPE_ISTAGE_FETCH_ADDR, 
  TYPE_OP_FETCH_ADDR,  
  TYPE_CACHE_STORE,  
  TYPE_HIT_TO_ACCESS_RATE,
  NUM_EXECUTION_COUNT_TYPE
} Exe_Count_Type;

typedef enum Usage_Count_Type_enum {
  TYPE_USAGE_COUNT_DEFAULT,
  TYPE_TRANSIENT,
  TYPE_WHOLE,   
  TYPE_BITREDUCTION,
  NUM_USAGE_COUNT_TYPE
} Usage_Count_Type;

typedef enum Feature_Type_enum {
  TYPE_FEATURE_DEFAULT,
  TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK,
  TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK,
  TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK,
  TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK,
  TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK,
  TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK,
  TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK,
  TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK,
  TYPE_STANDARDLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_STANDARDLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_STANDARDLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_ALLLOOKUP_ALLSTARTPW_FIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_FIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_ALLLOOKUP_ALLSTARTPW_NOFIXENDLINE_REMOVELOCK_SWITCH,
  TYPE_ALLLOOKUP_LOOKSINGINSERTDECODEALL_NOFIXENDLINE_REMOVELOCK_SWITCH,
  NUM_FEATURE_TYPE
} Feature_Type;


typedef struct OPTgen_struct
{
    int16* liveness_history;
    uns64 opt_access;

    uns64 CACHE_SIZE;

    uns64 perset_mytimer;
    Addr*    tag_history;              /* tag for the line */
    
} OPTgen;

typedef struct SampledCacheLine_struct 
{
    Flag valid;
    uns64 tag;
    uns64 signature;
    int32 timestamp;
    
} SampledCacheLine;

// typedef struct ExecutionCount_struct 
// {
//     Addr addr;
//     uns64 count;
    
// } ExecutionCount;

// typedef struct RDP_Entry_struct {
//   uns32 value;              /* To debug hash error */
// } RDP_Entry;

typedef struct Cache_Entry_struct {
  uns8    proc_id;
  Flag    valid;            /* valid bit for the line */
  Addr    tag;              /* tag for the line */
  Addr    base;             /* address of first element */
  Counter last_access_time; /* for replacement policy */
  Counter insertion_time;   /* for replacement policy */
  void*   data;             /* pointer to arbitrary data */
  Flag    pref;             /* extra replacement info */
  Flag    dirty; /* Dirty bit should have been here, however this is used only in
                 warmup now */
  Addr pw_start_addr; /* for uop cache: start addr of prediction window */
  Addr    addr;             /* for stored pointer of OPT and other replacement policy  */
  Addr    addr_end;             /* for stored pointer of REPL_CACHEACCESS and REPL_EXECUTION_COUNT policy  */
  uns     op_length;            /* for stored pointer of REPL_CACHEACCESS and REPL_EXECUTION_COUNT policy  */
  uns64   count_num;            /* for storing count number used along with LRU */
  uns     rrpv;             /* for RRIP replacement policy */
  // Flag    is_prefetch;      /* for SHiP++ replacement policy */ /* same as pref */
  uns32   fill_core;        /* for SHiP++ replacement policy */
  Flag    line_reuse;       /* for SHiP++ replacement policy */
  uns64   line_sig;         /* for SHiP++ replacement policy */
  int32     etr;              /* for mockingjay replacement policy */
  Addr    actual_addr;        /* for replacement policy, hack prediction window */

  Counter hitTimeRemains; /* For Hit time Repl. */

  Counter pmcWeight; /* For storing the PMC  Weight */
  Counter evictionHint; /* For Dynamic Evict Hint */
  Flag exhaustedInterval; /* For Dynamic Evict Hint */

  /* This is for GHRP implementation */
  uint64_t signature;
  Flag prediction;
  /***********************************/

} Cache_Entry;

typedef struct Key_Map_Struct
{
      Addr addr; // pc for rdp
      Addr addr_end;
      int64 last_quanta; // value for rdp // execution count for new repl
      int8 last_line_num;
      // Cache_Entry   entry; 
} Key_Map;

// DO NOT CHANGE THIS ORDER
typedef enum Cache_Insert_Repl_enum {
  INSERT_REPL_DEFAULT = 0, /* Insert with default replacement information */
  INSERT_REPL_LRU,         /* Insert into LRU position */
  INSERT_REPL_LOWQTR, /* Insert such that it is Quarter(Roughly) of the repl
                         order*/
  INSERT_REPL_MID, /* Insert such that it is Middle(Roughly) of the repl order*/
  INSERT_REPL_MRU, /* Insert into MRU position */
  NUM_INSERT_REPL
} Cache_Insert_Repl;

typedef struct Cache_struct {
  char name[MAX_STR_LENGTH+1]; /* name to identify the cache (for debugging) */
  uns data_size; /* how big are the data items in each cache entry? (for malloc)
                  */

  uns         assoc;       /* associativity */
  uns         num_lines;   /* number of lines in the cache */
  uns         num_sets;    /* number of sets in the cache */
  uns         line_size;   /* size in bytes of one line */
  Repl_Policy repl_policy; /* the replacement policy of the cache */

  uns set_bits;     /* number of bits used in the set mask */
  uns shift_bits;   /* number of bits to shift an address before using (assuming
                       it is shifted) */
  Addr set_mask;    /* mask applied after shifting to get the index */
  Addr tag_mask;    /* mask used to get the tag after shifting */
  Addr offset_mask; /* mask used to get the line offset */
  Flag tag_incl_offset; /* flag to use shift bits mapping policy*/

  uns*          repl_ctrs; /* replacement info */
  Cache_Entry** entries;   /* A dynamically allocated array of all
                              of the cache entries. The array is
                              two-dimensional, sets are row major. */
  Flag * entiresLRU;  /*Used to switch from LRU/HITtime
                              False->LRU True->HitTime*/

  List* unsure_lists;      /* A linked list for each set in the cache that
                              is used when simulating ideal replacement policies */
  Flag perfect;            /* is the cache perfect (for henry mem system) */
  uns repl_pref_thresh; /* threshhold for how many entries are high-priority. */
  Cache_Entry** shadow_entries; /* A dynamically allocated array for shadow
                                   cache */
  uns* queue_end;               /* queue pointer for ideal storage */

  Counter num_demand_access;
  Counter last_update; /* last update cycle */

  uns*     num_ways_allocted_core; /* For cache partitioning */
  uns*     num_ways_occupied_core; /* For cache partitioning */
  uns*     lru_index_core;         /* For cache partitioning */
  Counter* lru_time_core;          /* For cache partitioning */

  OPTgen*  OPT;                     /* For ideal replacement*/
  void *map_root;                   /* For ideal replacement*/ /*For addr to quanta map*/ /*should be per cache*/
  uns64 num_cache_opt;              /* For ideal replacement*/
  uns64 num_dont_cache_opt;         /* For ideal replacement*/

  uns   brrip_counter;                  /* For DRRIP replacement */
  uns   psel_counter;                   /* For DRRIP replacement */
  Drrip_Repl_Policy* drrip_repl_policy; /* For DRRIP replacement */
  uns32*  SHCT;                         /* For SHiP++ replacement */
  uns32*  ship_sample;                  /* For SHiP++ replacement */
  int32*     etr_clock;                   /* for mockingjay replacement policy */
  int32*     current_timestamp;           /* for mockingjay replacement policy */
  SampledCacheLine** sampled_cache;     /* for mockingjay replacement policy */

  Exe_Count_Type exe_count_type; /* the execution count type for REPL_EXECUTION_COUNT */
  FILE*   file_pointer;      /* For record */
  FILE*   file_pointer_backup;      /* For record */
  FILE*   file_pointer_switch;      /* For record */
  uns64   pre_timestamp_new_opt; 
  uns64   timestamp_new_opt; 
  uns64   timestamp_new_opt_insert; 
  Flag    enable_timestamp;       
  Addr    pre_addr;
  Addr    pre_addr_read;
  Addr    pre_addr_opt;
  int16*  pre_chosen_entry;
  int16*  pre_pre_chosen_entry;
  int16*  pre_pre_pre_chosen_entry;
  int16*  pre_pre_pre_pre_chosen_entry;
  int16*  sec_pre_chosen_entry;
  int16*  sec_pre_pre_chosen_entry;
  int16*  sec_pre_pre_pre_chosen_entry;
  int16*  sec_pre_pre_pre_pre_chosen_entry;
} Cache;


/**************************************************************************************/
/* prototypes */

void  init_cache(Cache*, const char*, uns, uns, uns, uns, Repl_Policy);
void* cache_access(Cache*, Addr, Addr*, Flag, Load_Type);
int   cache_access_all(Cache*, Addr, Addr*, Flag, void**, Load_Type);
int   cache_access_size(Cache*, Addr, Addr, Addr*, Flag, void**, Load_Type);
int   cache_access_lookup_allpw(Cache*, Addr, Addr*, Flag, void**, Load_Type);
void* cache_insert(Cache*, uns8, Addr, Addr*, Addr*, Load_Type);
void* cache_insert_replpos(Cache* cache, uns8 proc_id, Addr addr,
                           Addr* line_addr, Addr* repl_line_addr,
                           Cache_Insert_Repl insert_repl_policy,
                           Flag              isPrefetch,
                           Load_Type         load_type);
void* cache_insert_concat(Cache* cache, uns8 proc_id, Addr addr, Addr addr_end, Addr actual_addr, uns8 nops, Addr* line_addr, 
                          Addr* repl_line_addr, Load_Type Load_Type, uint64_t signature, Flag prediction, Counter pmcWeight);
void* cache_insert_replpos_concat(Cache* cache, uns8 proc_id, Addr addr, Addr addr_end, Addr actual_addr, uns8 nops, 
                           Addr* line_addr, Addr* repl_line_addr,
                           Cache_Insert_Repl insert_repl_policy,
                           Flag              isPrefetch,
                           Load_Type         load_type,
                           uint64_t          signature,
                           Flag             prediction,
                           Counter          _pmcWeight);
void* cache_insert_lru(Cache*, uns8, Addr, Addr*, Addr*, Load_Type);
void  cache_invalidate(Cache*, Addr, Addr*);
void  cache_invalidate_all(Cache*, Addr, Addr*);
void  cache_flush(Cache*);
void* get_next_repl_line(Cache*, uns8, Addr, Addr*, Flag*, Load_Type);
uns   ext_cache_index(Cache*, Addr, Addr*, Addr*);
Addr  get_cache_line_addr(Cache*, Addr);
uns   cache_index_set(Cache * _cache, Addr _addr);
void* shadow_cache_insert(Cache* cache, uns set, Addr tag, Addr base);
void* access_shadow_lines(Cache* cache, uns set, Addr tag);
void* access_ideal_storage(Cache* cache, uns set, Addr tag, Addr addr);
void  reset_cache(Cache*);
int   cache_find_pos_in_lru_stack(Cache* cache, uns8 proc_id, Addr addr,
                                  Addr* line_addr);
void  set_partition_allocate(Cache* cache, uns8 proc_id, uns num_ways);
uns   get_partition_allocated(Cache* cache, uns8 proc_id);

// add set parameter for debug printing
uns64* initialize_quanta(Cache* cache, Addr addr, int64 quanta, uns set);
Flag find_last_quanta(Cache* cache, Addr addr, int64* last_quanta, int8* last_line_num, uns set);
void* update_quanta(Cache* cache, Addr addr, int64 quanta, int8 line_num, uns set, int8 target_assoc, Flag cache_real_hit);
uns64* initialize_quanta_concat(Cache* cache, Addr addr, Addr addr_end, int64 quanta, uns set);
Flag find_last_quanta_concat(Cache* cache, Addr addr, Addr addr_end, int64* last_quanta, int8* last_line_num, uns set);
void* update_quanta_concat(Cache* cache, Addr addr, Addr addr_end, int64 quanta, int8 line_num, uns set, int8 target_assoc, Flag cache_real_hit);
void* invalidate_quanta(Cache* cache, Addr addr, uns set) ;
Cache_Entry return_quanta_data(Cache* cache, Addr addr);
int8 return_quanta_line_num(Cache* cache, Addr addr);
void add_quanta_data(Cache* cache, Addr addr, int64 quanta, int8 line_num, void** data);
/* for new replacement policy */
uns return_exe_count_type();
uns return_repl_policy();
uns return_feature(int check_num);
FILE* return_file_pointer();
FILE* return_file_pointer_switch();
Flag uopcache_update_execution_count(Addr update_addr, FILE* file_pointer);
Flag uopcache_update_execution_count_concat(Addr update_addr, Addr update_addr_end);
Flag uopcache_update_execution_count_concat_tran(Addr update_addr, Addr update_addr_end);
void* add_timestamp_addr(Addr addr);
void* print_timestamp_addr(Addr addr);
/**************************************************************************************/

uns64 find_timestamp(Cache* cache, Addr addr);
uns64 find_future_timestamp(Cache* cache, Addr addr, uns64 cur_timestamp);
uns64 find_entry_count(Addr addr);
uns64 find_entry_count_switch(Addr addr);
uns64 find_entry_count_concat(Addr addr, Addr addr_end);
float find_entry_rate(Addr addr);
float find_entry_rate_transient_concat(Addr addr, Addr addr_end);
float find_entry_rate_holistic_concat(Addr addr, Addr addr_end);
void read_init();
void read_init_cacheaccess();
void read_init_cacheaccess_concat();
void read_init_ratecacheaccess();
void read_init_cacheaccess_granularities();
void read_init_cacheaccess_granularities_switch();
void read_init_ratecacheaccess_concat();
void write_ratecacheaccess_concat(Addr addr, Addr addr_end, uns hit, uns count);
void write_countcacheaccess_concat(Addr addr, Addr addr_end, uns64 count);
void write_countcacheaccess(Addr addr, uns64 count);
void write_countcacheaccess_switch(Addr addr, uns64 count);
void write_miss(Addr addr, uns64 count);
void write_cycle(Addr addr, uns64 cycle, Flag reset_cycle);
uns64 find_miss_count(Addr addr);
uns64 find_cycle_count(Addr addr, Flag first);
void minus_countcacheaccess(Addr addr, uns64 count);

/**************************************************************************************/
  
/**Bypass Function*/
void initBypassMap();
Flag checkByPass(Addr startAddr, Counter _uops);

/* Tool Function for reading next oracle line*/
void readNextTraceLine();

void initFooCore();
/* Called on when each op is packeted by packet_build.c */
void updateFooPW();

Flag checkExistCache(Addr _pwStartAddr, Counter _pwUopSize);

/******Hit Times*******/
void initHitTimesMap();

Counter getHitTimesValue(Addr _pwStartAddr, Counter _pwUopSize);
/***********************/

/******Eviction Hints*****/
void initEvictionMap();
Counter getEvictionHints(Addr _pwStartAddr);

/*****Cold Miss*****/
Flag checkSeen(Addr _pwStartAddr);

/* This is for implementation of GHRP */
#define GHRP_PREDICT_TABLE_INDEX_BITS 16 /* Define the predict table's size */
#define GHRP_PREDICT_TABLE_NUM 3 /* Define the number of predTable for majority vote */
#define GHRP_COUNTER_WIDTH 2
#define GHRP_THRESHOLD 2
#define GHRP_SIGNATURE_BITS 16
#define GHRP_GLOBAL_HISTORY_BITS 16

uint64_t GHRP_mix(uint64_t a, uint64_t b, uint64_t c);

uint64_t GHRP_sign_hash(uint64_t sign, int index);

uint64_t GHRP_get_signature(uint64_t _globalHistory, Addr pc);

void GHRP_update_global_history(Addr pc);

void initGHRP();

Flag GHRP_get_prediction(uint64_t* indexes);

void GHRP_update_predict_table(uint64_t* indexes, Flag dead);

uint64_t* GHRP_get_pred_indexes(uint64_t signatrue);

void GHRP_free_pred_indexes(uint64_t* indexes);

void init_thermo_core();

Counter get_thermo_weight(Addr _addr);

void m_update_replacement_state(uns32 cpu, uns32 set, uns32 way, uns64 full_addr, uns64 pc, Load_Type load_type, Flag hit, Cache *cache, Addr *line_addr);

void init_ic_pmc_core();

Counter get_ic_pmc_weight(Addr _addr);

#endif /* #ifndef __CACHE_LIB_H__ */
