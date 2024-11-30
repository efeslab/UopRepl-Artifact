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
 * File         : icache_stage.c
 * Author       : HPS Research Group
 * Date         : 12/7/1998
 * Description  :
 ***************************************************************************************/

#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"

#include "bp/bp.h"
#include "icache_stage.h"
#include "map.h"
#include "op_pool.h"
#include "packet_build.h"
#include "thread.h"

#include "bp/bp.param.h"
#include "cmp_model.h"
#include "core.param.h"
#include "debug/debug.param.h"
#include "frontend/frontend.h"
#include "frontend/pin_trace_fe.h"
#include "memory/memory.h"
#include "memory/memory.param.h"
#include "prefetcher/l2l1pref.h"
#include "prefetcher/stream_pref.h"
#include "statistics.h"
#include "libs/list_lib.h"

#include "prefetcher/fdip.h"
#include "prefetcher/pref.param.h"
/**************************************************************************************/
/* Macros */

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_ICACHE_STAGE, ##args)

#define STAGE_MAX_OP_COUNT  UC_ISSUE_WIDTH
#define DUMMY_ADDR_UC_FETCH 0x1

/**************************************************************************************/
/* Global Variables */

extern Cache uop_cache;
Icache_Stage* ic = NULL;
List op_buf;
Counter                last_issued_op_num = 0;

// For REPL_NEW_OPT
// static FILE*      new_opt_stream;

extern Addr PowerPWStart;
extern Addr PowerPWEnd;
extern Addr PowerPrevSearch;
extern Cmp_Model              cmp_model;
extern Memory*                mem;
extern Rob_Stall_Reason       rob_stall_reason;
extern Rob_Block_Issue_Reason rob_block_issue_reason;
extern Addr                   runahead_pc;
extern Flag                   runahead_disable;
extern uns64                  last_runahead_uid;
extern uns64                  max_runahead_uid;
extern Counter                last_runahead_op;
extern Counter                max_runahead_op;
extern Counter                max_op_num;
extern Flag                   mem_req_failed;
extern Counter                last_recover_cycle;
static Pb_Data* ic_pb_data;
uns64 timestamp_new_opt_istage;

/**************************************************************************************/
/* Local prototypes */

static inline Icache_State icache_issue_ops(Break_Reason*, uns*,
                                            Inst_Info** line,
                                            Flag uop_cache_issue_ops);
static inline Inst_Info**  lookup_cache(Flag* uop_cache_fetch);
static Inst_Info**         ic_pref_cache_access(void);
int32_t                    inst_lost_get_full_window_reason(void);
static inline void         log_stats_ic_miss(void);

/**************************************************************************************/
/* set_icache_stage: */

void set_icache_stage(Icache_Stage* new_ic) {
  ic = new_ic;
}

/**************************************************************************************/
/* set_pb_data: */

void set_pb_data(Pb_Data* new_pb_data) {
  ic_pb_data = new_pb_data;
}

/**************************************************************************************/
/* init_icache_stage: */

void init_icache_stage(uns8 proc_id, const char* name) {
  ASSERT(0, ic);
  DEBUG(proc_id, "Initializing %s stage\n", name);

  memset(ic, 0, sizeof(Icache_Stage));

  ic->proc_id = proc_id;
  ic->sd.name = (char*)strdup(name);

  // For REPL_NEW_OPT
  // new_opt_stream = file_tag_fopen(NULL, "new_opt_stream", "w");

  /* initialize the ops array */
  ic->sd.max_op_count = STAGE_MAX_OP_COUNT;
  ic->sd.ops          = (Op**)malloc(sizeof(Op*) * STAGE_MAX_OP_COUNT);

  /* initialize the cache structure */
  init_cache(&ic->icache, "ICACHE", ICACHE_SIZE, ICACHE_ASSOC, ICACHE_LINE_SIZE,
             0, ICACHE_REPL);

  /* init icache_line_info struct - this struct keeps data about corresponding
   * icache lines */
  if(WP_COLLECT_STATS) {
    // init_cache(&ic->icache_line_info, "IC LI", ICACHE_SIZE, ICACHE_ASSOC,
    // ICACHE_LINE_SIZE,
    //           sizeof(Icache_Data), REPL_TRUE_LRU);
    init_cache(&ic->icache_line_info, "IC LI", ICACHE_SIZE, ICACHE_ASSOC,
               ICACHE_LINE_SIZE, sizeof(Icache_Data), ICACHE_REPL);
  }

  // moved the init code from here to reset
  reset_icache_stage();

  if(model->id != CMP_MODEL)
    ic_pb_data = (Pb_Data*)malloc(sizeof(Pb_Data));
  ic_pb_data->proc_id = proc_id;
  init_packet_build(ic_pb_data, PB_ICACHE);

  if(IC_PREF_CACHE_ENABLE)
    init_cache(&ic->pref_icache, "IC_PREF_CACHE", IC_PREF_CACHE_SIZE,
               IC_PREF_CACHE_ASSOC, ICACHE_LINE_SIZE, 0, REPL_TRUE_LRU);

  memset(ic->rand_wb_state, 0, NUM_ELEMENTS(ic->rand_wb_state));

  //FDIP
  if (FDIP_ENABLE) {
    fdip_init(g_bp_data, ic);
  }
}

/**************************************************************************************/
/* icache_init_trace:  */

void init_icache_trace() {
  if (FDIP_ENABLE)
    ASSERT(0, LOOKAHEAD_BUF_SIZE && PERFECT_NT_BTB);
  if (LOOKAHEAD_BUF_SIZE) {
    ASSERT(0, MEMTRACE); //Lookahead buffer only works in trace mode
    init_list(&op_buf, "op_buf", sizeof(Op*), TRUE);
    while (list_get_count(&op_buf) < LOOKAHEAD_BUF_SIZE) {
      Op* new_op = alloc_op(ic->proc_id);
      frontend_fetch_op(ic->proc_id, new_op);
      if(return_repl_policy() == 1){
        // add_timestamp_addr(new_op->inst_info->addr); 
      }
      Op** ptr = dl_list_add_tail(&op_buf);
      *ptr = new_op;
      if (new_op->table_info->cf_type)
        max_runahead_uid = new_op->inst_uid;
      max_runahead_op = new_op->op_num;
      op_count[ic->proc_id]++;          /* increment instruction counters */
      unique_count_per_core[ic->proc_id]++;
      unique_count++;
    }
    Op **ptr = list_get_head(&op_buf);
    ic->next_fetch_addr = (*ptr)->inst_info->addr;
    runahead_pc = (*ptr)->inst_info->addr;
    runahead_disable = FALSE;
    if (FDIP_ENABLE)
      fdip_update();
  } else {
    ic->next_fetch_addr = frontend_next_fetch_addr(ic->proc_id);
  }
  ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->next_fetch_addr)
}

/**************************************************************************************/
/* reset_icache_stage: */

void reset_icache_stage() {
  uns ii;
  for(ii = 0; ii < STAGE_MAX_OP_COUNT; ii++)
    ic->sd.ops[ii] = NULL;
  ic->sd.op_count = 0;

  /* set up the initial fetch state */
  ic->next_fetch_addr                = td->inst_addr;
  ic->off_path                       = FALSE;
  ic->back_on_path                   = FALSE;
  op_count[ic->proc_id]              = 1;
  unique_count_per_core[ic->proc_id] = 1;
}

/**************************************************************************************/
/* reset_all_ops_icache_stage: */
// CMP used for bogus run: may be combined with reset_icache_stage
void reset_all_ops_icache_stage() {
  uns ii;
  for(ii = 0; ii < STAGE_MAX_OP_COUNT; ii++)
    ic->sd.ops[ii] = NULL;
  ic->sd.op_count = 0;

  /* set up the initial fetch state */
  ic->next_fetch_addr = td->inst_addr;
  ic->off_path        = FALSE;
  ic->back_on_path    = FALSE;
}

/**************************************************************************************/
/* recover_icache_stage: */

void recover_icache_stage() {
  Stage_Data* cur = &ic->sd;
  uns         ii;

  ASSERT(ic->proc_id, ic->proc_id == bp_recovery_info->proc_id);
  DEBUG(ic->proc_id,
        "Icache stage recovery signaled.  recovery_fetch_addr: 0x%s\n",
        hexstr64s(bp_recovery_info->recovery_fetch_addr));

  cur->op_count = 0;
  for(ii = 0; ii < ic->sd.max_op_count; ii++) {
    if(cur->ops[ii]) {
      if(FLUSH_OP(cur->ops[ii])) {
        free_op(cur->ops[ii]);
        cur->ops[ii] = NULL;
      } else {
        cur->op_count++;
      }
    }
  }

  ic->back_on_path = !bp_recovery_info->recovery_force_offpath;

  Op* op = bp_recovery_info->recovery_op;
  if(bp_recovery_info->late_bp_recovery && op->oracle_info.btb_miss &&
     !op->oracle_info.btb_miss_resolved) {
    // Late branch predictor recovered before btb miss is resolved (i.e., icache
    // stage should still wait for redirect)
  } else {
    if(ic->next_state != IC_FILL && ic->next_state != IC_WAIT_FOR_MISS) {
      ic->next_state = IC_FETCH;
    }
    if(SWITCH_IC_FETCH_ON_RECOVERY && model->id == CMP_MODEL) {
      ic->next_state = IC_FETCH;
    }
  }
  op_count[ic->proc_id] = bp_recovery_info->recovery_op_num + 1 + LOOKAHEAD_BUF_SIZE;
  ic->next_fetch_addr   = bp_recovery_info->recovery_fetch_addr;
  if(ic->proc_id)
    ASSERT(ic->proc_id, ic->next_fetch_addr);
}


/**************************************************************************************/
/* redirect_icache_stage: */

void redirect_icache_stage() {
  ASSERT(bp_recovery_info->proc_id, bp_recovery_info->proc_id == ic->proc_id);
  Op*  op              = bp_recovery_info->redirect_op;
  Addr next_fetch_addr = op->oracle_info.pred_npc;

  DEBUG(ic->proc_id, "Icache stage redirect signaled. next_fetch_addr: 0x%s\n",
        hexstr64s(next_fetch_addr));
  ASSERT(ic->proc_id, ic->state == IC_WAIT_FOR_REDIRECT);

  Flag main_predictor_wrong = op->oracle_info.mispred ||
                              op->oracle_info.misfetch;
  Flag late_predictor_wrong = (USE_LATE_BP && (op->oracle_info.late_mispred ||
                                               op->oracle_info.late_misfetch));
  ic->back_on_path          = !(op->off_path || main_predictor_wrong ||
                       late_predictor_wrong);
  ic->next_fetch_addr       = next_fetch_addr;
  ic->next_state            = IC_FETCH;
}


/**************************************************************************************/
/* debug_icache_stage: */

void debug_icache_stage() {
  DPRINTF("# %-10s  op_count:%d ", ic->sd.name, ic->sd.op_count);
  DPRINTF(
    "fetch_addr:0x%s  next_fetch_addr:0x%s  path:%s  state:%s  next_state:%s\n",
    hexstr64s(ic->fetch_addr), hexstr64s(ic->next_fetch_addr),
    ic->off_path ? "OFF_PATH" : "ON_PATH ", icache_state_names[ic->state],
    icache_state_names[ic->next_state]);

  // print icache stage
  DPRINTF("# %-10s  op_count:%d\n", "ICache", ic->sd.op_count);
  print_op_array(GLOBAL_DEBUG_STREAM, ic->sd.ops, STAGE_MAX_OP_COUNT,
                 ic->sd.op_count);
}

/**************************************************************************************/
/* in_icache: returns whether instr in icache 
 *            Used for branch stat collection
 */

Flag in_icache(Addr addr) {
  Addr line_addr;
  return cache_access(&ic->icache, addr, &line_addr, FALSE, TYPE_LOAD) != NULL;
}

/**************************************************************************************/
/* lookup_cache: returns instr if found in either uop cache or icache 
 *                If icache miss but UC hit, set ic->line to non-null  
 */
Inst_Info** lookup_cache(Flag* uop_cache_fetch) {
  static Addr icache_line_reg = 0; // Temporary buffer for icache line
  Inst_Info** line = NULL;
  if(icache_line_reg == (ic->fetch_addr & (~(ICACHE_LINE_SIZE - 1)))){ // Hit the register
    line = (Inst_Info**)INIT_CACHE_DATA_VALUE;
  }else{ // Access the icache
    line = (Inst_Info**)cache_access(&ic->icache, ic->fetch_addr, &ic->line_addr, TRUE, TYPE_LOAD);
    icache_line_reg = 0; // Clear the temoprary buffer

    if(PERFECT_ICACHE && !line)
      line = (Inst_Info**)INIT_CACHE_DATA_VALUE;
  }

  if(return_repl_policy() == 1){
    timestamp_new_opt_istage++;
  }
  if (in_uop_cache(ic->fetch_addr, NULL, FALSE, TRUE, NULL)) {
    *uop_cache_fetch = TRUE;
    if (!line){
      line = (Inst_Info**)DUMMY_ADDR_UC_FETCH;
      if(UOP_INCLUSION == UOPI_FORCE)
      {
        line = NULL;
        *uop_cache_fetch = FALSE;
        Addr occupy;
        cache_invalidate_all(&uop_cache, ic->fetch_addr, &occupy);
      }
      if(UOP_INCLUSION == UOPI_ACTUAL){
        line = NULL;
        *uop_cache_fetch = FALSE;
      }
    }
  }
  if(*uop_cache_fetch == FALSE && line != NULL){
    if(icache_line_reg == 0){
      STAT_EVENT(ic->proc_id, POWER_ICACHE_ACCESS);
    }
    icache_line_reg = ic->fetch_addr & (~(ICACHE_LINE_SIZE - 1));
  }
  return line;
}

/**************************************************************************************/
/* icache_cycle: */

void update_icache_stage() {
  uns          cf_num    = 0;
  Icache_Data* line_info = NULL;
  Addr         dummy_addr;

  STAT_EVENT(ic->proc_id, ICACHE_CYCLE);
  STAT_EVENT(ic->proc_id, ICACHE_CYCLE_ONPATH + ic->off_path);
  INC_STAT_EVENT(ic->proc_id, INST_LOST_TOTAL, IC_ISSUE_WIDTH);

  ic->state = ic->next_state;

  /* Guess: ensure ops all be fetched*/
  if(ic->sd.op_count) { 
    STAT_EVENT(ic->proc_id, FETCH_0_OPS);
    INC_STAT_EVENT(ic->proc_id,
                   INST_LOST_FULL_WINDOW + inst_lost_get_full_window_reason(),
                   IC_ISSUE_WIDTH);
    // printf("opcount %d", 0);
    return;
  }
  // printf("state: %d \n", ic->state);
  switch(ic->state) {
    case IC_FETCH: {
      Break_Reason break_fetch = BREAK_DONT;
      Flag uop_cache_fetch = FALSE;

      ic->off_path &= !ic->back_on_path;
      ic->back_on_path = FALSE;

      if(!FETCH_OFF_PATH_OPS && ic->off_path){
        // Recored the number of off-path events
        INC_STAT_EVENT(ic->proc_id, INST_LOST_WAIT_FOR_REDIRECT, IC_ISSUE_WIDTH);
        STAT_EVENT(ic->proc_id, FETCH_0_OPS);
        STAT_EVENT(ic->proc_id, CYCLE_USED_MISPRED);
        return;
      }

      STAT_EVENT(ic->proc_id, FETCH_ON_PATH + ic->off_path);

      reset_packet_build(ic_pb_data);  // reset packet build counters
      // Used for calculating the useful fetching cycles
      static Counter fetch_count = 0;
      while(!break_fetch) {
        ic->fetch_addr = ic->next_fetch_addr;
        ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->fetch_addr)
        if(ic->proc_id)
          ASSERTM(ic->proc_id, ic->fetch_addr, "ic fetch addr: %llu\n",
                  ic->fetch_addr);
        
        ic->line = lookup_cache(&uop_cache_fetch);
        static Addr last_fetch_addr = 0;
        if (last_fetch_addr != ic->fetch_addr && (!((ic->fetch_addr >= PowerPWStart) && (ic->fetch_addr <= PowerPWEnd) && (ic->fetch_addr > PowerPrevSearch)))) {
          if(ORACLE_PERFECT_UOP_CACHE == FALSE){
            STAT_EVENT(ic->proc_id, POWER_ITLB_ACCESS);
            STAT_EVENT(ic->proc_id, POWER_BTB_READ);
            if(ic->line == NULL || ic->line == (Inst_Info**) DUMMY_ADDR_UC_FETCH){
              log_stats_ic_miss();
            }else{
              STAT_EVENT(0, ICACHE_HIT);
              STAT_EVENT(ic->proc_id, ICACHE_HIT_ONPATH + ic->off_path);
            }
          }
        }
        if(ORACLE_PERFECT_UOP_CACHE){
          if(ic->line == NULL || ic->line == (Inst_Info**) DUMMY_ADDR_UC_FETCH){
            log_stats_ic_miss();
          }else{
            STAT_EVENT(0, ICACHE_HIT);
            STAT_EVENT(ic->proc_id, ICACHE_HIT_ONPATH + ic->off_path);            
          }
          if(last_fetch_addr != ic->fetch_addr){
            if(uop_cache_fetch == FALSE){
              STAT_EVENT(ic->proc_id, POWER_ITLB_ACCESS);
              STAT_EVENT(ic->proc_id, POWER_BTB_READ);
            }
          }
        
        }
        last_fetch_addr = ic->fetch_addr;

        // ideal L2 Icache prefetcher
        if(IDEAL_L2_ICACHE_PREFETCHER) {
          Addr     line_addr;
          L1_Data* data;
          Cache*   l1_cache = model->mem == MODEL_MEM ?
                              &mem->uncores[ic->proc_id].l1->cache :
                              NULL;
          data = l1_cache ? (L1_Data*)cache_access(l1_cache, ic->fetch_addr,
                                                   &line_addr, TRUE, TYPE_PREF) :
                            NULL;
          if(data) {  // second level cache hit
            STAT_EVENT(ic->proc_id, L2_IDEAL_FILL_ICACHE);
          } else
            STAT_EVENT(ic->proc_id, L2_IDEAL_MISS_ICACHE);
        }

        if(FDIP_ENABLE && last_issued_op_num == max_op_num && last_runahead_op != max_runahead_op) {
          ic->next_state = IC_WAIT_FOR_FDIP;
          break_fetch = BREAK_FDIP_RUNAHEAD;
        } else if(ic->line == NULL) { /* cache miss */
          DEBUG(ic->proc_id, "Cache miss on op_num:%s @ 0x%s\n",
                unsstr64(op_count[ic->proc_id]), hexstr64s(ic->fetch_addr));
          /* if the icache is available, wait for a miss */
          /* otherwise, refetch next cycle */
          if(model->mem == MODEL_MEM) {
            if(ic->proc_id)
              ASSERTM(ic->proc_id, ic->line_addr, "ic fetch addr: %llu\n",
                      ic->fetch_addr);
            ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->line_addr)
            if(new_mem_req(MRT_IFETCH, ic->proc_id, ic->line_addr,
                           ICACHE_LINE_SIZE, 0, NULL, instr_fill_line,
                           unique_count,
                           0)) {  // CMP maybe unique_count_per_core[proc_id]?
              ic->next_state = IC_WAIT_FOR_MISS;

              if(ONE_MORE_CACHE_LINE_ENABLE) {
                Addr         one_more_addr;
                Addr         extra_line_addr;
                Icache_Data* extra_line;

                one_more_addr =
                  ((ic->line_addr >> LOG2(ICACHE_LINE_SIZE)) & 1) ?
                    ((ic->line_addr >> LOG2(ICACHE_LINE_SIZE)) - 1)
                      << LOG2(ICACHE_LINE_SIZE) :
                    ((ic->line_addr >> LOG2(ICACHE_LINE_SIZE)) + 1)
                      << LOG2(ICACHE_LINE_SIZE);

                extra_line = (Icache_Data*)cache_access(
                  &ic->icache, one_more_addr, &extra_line_addr, FALSE, TYPE_LOAD);
                ASSERT(ic->proc_id, one_more_addr == extra_line_addr);
                if(!extra_line) {
                  if(new_mem_req(MRT_IFETCH, ic->proc_id, extra_line_addr,
                                 ICACHE_LINE_SIZE, 0, NULL, NULL, unique_count,
                                 0))
                    STAT_EVENT_ALL(ONE_MORE_SUCESS);
                  else
                    STAT_EVENT_ALL(ONE_MORE_DISCARDED_MEM_REQ_FULL);
                } else
                  STAT_EVENT_ALL(ONE_MORE_DISCARDED_L0CACHE);
              }
            }
          }
          break_fetch = BREAK_ICACHE_MISS;
        } else if (ic->line == (Inst_Info**) DUMMY_ADDR_UC_FETCH) { // icache miss, uc hit
          ASSERT(0, UOP_INCLUSION != UOPI_ACTUAL);
          if(UOP_INCLUSION == UOPI_LATE)
          {
            new_mem_req(MRT_IFETCH, ic->proc_id, ic->line_addr,
            ICACHE_LINE_SIZE, 0, NULL, instr_fill_line,
                           unique_count,
                           0);
          }
          if(fetch_count == 0){
            STAT_EVENT(ic->proc_id, CYCLE_USED_FETCH);
            // STATS Prob
            if(uop_cache_fetch) STAT_EVENT(0, CYCLES_FROM_UC);
            else STAT_EVENT(0, CYCLES_FROM_LEGACY);
          }
          ic->next_state = icache_issue_ops(&break_fetch, &cf_num, ic->line, uop_cache_fetch);
        } else { /* icache hit. Can be either UC hit or miss */
          DEBUG(ic->proc_id, "Cache hit on op_num:%s @ 0x%s \n",
                unsstr64(op_count[ic->proc_id]), hexstr64s(ic->fetch_addr));
          // Record statistic above: for keeping align with the PW model.
          // STAT_EVENT(ic->proc_id, ICACHE_HIT);
          // STAT_EVENT(ic->proc_id, ICACHE_HIT_ONPATH + ic->off_path);
          if(WP_COLLECT_STATS) {
            ASSERT(ic->proc_id, line_info);
            wp_process_icache_hit(line_info, ic->fetch_addr);
          }
          if(fetch_count == 0){
            STAT_EVENT(ic->proc_id, CYCLE_USED_FETCH);
            // STATS Prob
            if(uop_cache_fetch) STAT_EVENT(0, CYCLES_FROM_UC);
            else STAT_EVENT(0, CYCLES_FROM_LEGACY);
          }
          ic->next_state = icache_issue_ops(&break_fetch, &cf_num, ic->line, uop_cache_fetch);
        }
        fetch_count++;
      }
      fetch_count = 0;
      INC_STAT_EVENT(ic->proc_id, INST_LOST_BREAK_DONT + break_fetch,
                     IC_ISSUE_WIDTH > ic->sd.op_count ? IC_ISSUE_WIDTH - ic->sd.op_count
                                                           : 0);
      STAT_EVENT(ic->proc_id, FETCH_0_OPS + ic->sd.op_count);
      STAT_EVENT(ic->proc_id, ST_BREAK_DONT + break_fetch);
      // printf("break = %d\n", break_fetch); 
    } break;

    case IC_WAIT_FOR_MISS: {
      INC_STAT_EVENT(ic->proc_id, INST_LOST_BREAK_ICACHE_MISS, IC_ISSUE_WIDTH - 1);
      STAT_EVENT(ic->proc_id, FETCH_0_OPS);
      STAT_EVENT(0, CYCLE_USED_ICACHE_MISS);
    } break;

    case IC_WAIT_FOR_REDIRECT: {
      INC_STAT_EVENT(ic->proc_id, INST_LOST_WAIT_FOR_REDIRECT, IC_ISSUE_WIDTH);
      STAT_EVENT(ic->proc_id, FETCH_0_OPS);
      STAT_EVENT(ic->proc_id, CYCLE_USED_REDIRECT);
    } break;

    case IC_WAIT_FOR_EMPTY_ROB: {
      DEBUG(ic->proc_id, "Ifetch barrier: Waiting for ROB to become empty \n");
      INC_STAT_EVENT(ic->proc_id, INST_LOST_WAIT_FOR_EMPTY_ROB, IC_ISSUE_WIDTH);
      STAT_EVENT(ic->proc_id, FETCH_0_OPS);
      if(td->seq_op_list.count == 0)
        ic->next_state = IC_FETCH;
    } break;

    case IC_WAIT_FOR_TIMER: {
      INC_STAT_EVENT(ic->proc_id, INST_LOST_WAIT_FOR_TIMER, IC_ISSUE_WIDTH);
      STAT_EVENT(ic->proc_id, FETCH_0_OPS);
      if(cycle_count >= ic->timer_cycle)
        ic->next_state = IC_FETCH;
    } break;

    case IC_WAIT_FOR_FDIP: {
      INC_STAT_EVENT(ic->proc_id, INST_LOST_WAIT_FOR_FDIP, IC_ISSUE_WIDTH);
      STAT_EVENT(ic->proc_id, FETCH_0_OPS);
      if(last_issued_op_num < max_op_num)
        ic->next_state = IC_FETCH;
      if(last_runahead_op == max_runahead_op)
        ic->next_state = IC_FETCH;
    } break;

    case IC_WAIT_UC_IC_SWITCH: {
      ic->next_state = IC_FETCH;
      STAT_EVENT(ic->proc_id, CYCLE_USED_SWITCH);
    } break;

    default:
      FATAL_ERROR(ic->proc_id, "Invalid icache state.\n");
  }
  if (FDIP_ENABLE) {
    fdip_update();
  }
}


/**************************************************************************************/
/* icache_issue_ops: On a cache hit, select ops to pass down to the decode
   stage.  Each op that gets issued is executed by the oracle. It will only
   process up to the first control flow operation and then return.  If
   FETCH_ACROSS_CACHE_LINES is set, it will be called again until break_fetch
   becomes true. */

static inline Icache_State icache_issue_ops(Break_Reason* break_fetch,
                                            uns* cf_num, Inst_Info** line, 
                                            Flag uop_cache_issue_ops) {
  Packet_Build_Condition packet_break = PB_BREAK_DONT;
  static uns last_icache_issue_time = 0; /* for computing fetch break latency */
  static Counter issued_real_inst   = 0;
  static Counter issued_uop         = 0;
  uns            fetch_lag;

  ASSERT(ic->proc_id, ic->proc_id == td->proc_id);

  fetch_lag              = cycle_count - last_icache_issue_time;
  last_icache_issue_time = cycle_count;

  while(1) {
    Op*        op = NULL;
    Inst_Info* inst = 0;
    UNUSED(inst);

    if(frontend_can_fetch_op(ic->proc_id) ||
        (!frontend_can_fetch_op(ic->proc_id) && LOOKAHEAD_BUF_SIZE && list_get_count(&op_buf))) {
      if (LOOKAHEAD_BUF_SIZE) {
        Op** ptr = NULL;
        if (frontend_can_fetch_op(ic->proc_id)) {
          Op* new_op = alloc_op(ic->proc_id);
          frontend_fetch_op(ic->proc_id, new_op);
          if(return_repl_policy() == 1){
            // add_timestamp_addr(new_op->inst_info->addr); 
          }
          ptr = dl_list_add_tail(&op_buf);
          *ptr = new_op;
          if (new_op->table_info->cf_type)
            max_runahead_uid = new_op->inst_uid;
          max_runahead_op = new_op->op_num;
        }
        ptr = dl_list_remove_head(&op_buf);
        op = *ptr;
        last_issued_op_num = op->op_num;
        // printf("fetch op? %d\n", op->op_num);
      }
      else {
        op   = alloc_op(ic->proc_id);
        frontend_fetch_op(ic->proc_id, op);
        // printf("fetch op! %d\n", op->op_num);
        if(return_repl_policy() == 1){
          // add_timestamp_addr(op->inst_info->addr); 
        }
      }
      // Disable this to allow system interrupts.
      // ASSERTM(ic->proc_id, ic->next_fetch_addr == op->inst_info->addr,
      //         "Fetch address 0x%llx does not match op address 0x%llx\n",
      //         ic->next_fetch_addr, op->inst_info->addr);
      op->fetch_addr = ic->next_fetch_addr;
      // if(return_repl_policy() == 1){ // REPL_NEW_OPT){
      //   if (UOP_CACHE_ENABLE_RECORD){
      //     // printf("Now print to file\n");
      //     // printf("%llu %llu\n", op->fetch_addr, timestamp_new_opt_istage);
      //     add_timestamp_addr(op->fetch_addr, timestamp_new_opt_istage);
      //     //fprintf(new_opt_stream, "%llu %llu\n", op->fetch_addr, timestamp_new_opt_istage);
      //   }
      //   timestamp_new_opt_istage++;
      // }
      if (return_exe_count_type()==2){
        // 1: TYPE_ISTAGE_FETCH_ADDR; 2: TYPE_OP_FETCH_ADDR; 3: TYPE_CACHE_STORE
        // for execution count hash update
        uopcache_update_execution_count(op->fetch_addr, return_file_pointer());
      }
      ASSERT_PROC_ID_IN_ADDR(ic->proc_id, op->fetch_addr)
      op->off_path  = ic->off_path;
      td->inst_addr = op->inst_info->addr;  // FIXME: BUG 54
      if(!op->off_path) {
        if(op->eom)
          issued_real_inst++;
        issued_uop++;
      }
      inst = op->inst_info;
    } else {
      *break_fetch = BREAK_BARRIER;
      return IC_FETCH;
    }

    if(!op->off_path && op->table_info->mem_type != NOT_MEM &&
       op->oracle_info.va == 0) {
      FATAL_ERROR(ic->proc_id, "Access to 0x0\n");
    }

    if(DUMP_TRACE && DEBUG_RANGE_COND(ic->proc_id))
      print_func_op(op);

    if(DIE_ON_CALLSYS && !op->off_path) {
      ASSERT(ic->proc_id, op->table_info->cf_type != CF_SYS);
    }

    
    packet_break = packet_build(ic_pb_data, break_fetch, op, uop_cache_issue_ops); // to check
    if(packet_break == PB_BREAK_BEFORE) {
      free_op(op);
      break;
    }

    /* add to sequential op list */
    add_to_seq_op_list(td, op);

    ASSERT(ic->proc_id, td->seq_op_list.count <= op_pool_active_ops);

    /* map the op based on true dependencies & set information in
     * op->oracle_info */
    /* num cycles since last group issued */
    op->fetch_lag = fetch_lag;

    thread_map_op(op);

    STAT_EVENT(op->proc_id, FETCH_ALL_INST);
    STAT_EVENT(op->proc_id, ORACLE_ON_PATH_INST + op->off_path);
    STAT_EVENT(op->proc_id, ORACLE_ON_PATH_INST_MEM +
                              (op->table_info->mem_type == NOT_MEM) +
                              2 * op->off_path);

    thread_map_mem_dep(op);
    op->fetch_cycle = cycle_count;

    ic->sd.ops[ic->sd.op_count] = op; /* put op in the exit list */
    op_count[ic->proc_id]++;          /* increment instruction counters */
    unique_count_per_core[ic->proc_id]++;
    unique_count++;

    if(op->fetched_from_uop_cache){
      STAT_EVENT(0, UOPS_FROM_UC);
    }else STAT_EVENT(0, UOPS_FROM_LEGACY);

    /* check trigger */
    if(op->inst_info->trigger_op_fetched_hook)
      model->op_fetched_hook(op);

    /* move on to next instruction in the cache line */
    ic->sd.op_count++;
    INC_STAT_EVENT(ic->proc_id, INST_LOST_FETCH + ic->off_path, 1);

    DEBUG(ic->proc_id,
          "Fetching op from Icache addr: %s off: %d inst_info: %p ii_addr: %s "
          "dis: %s opnum: (%s:%s)\n",
          hexstr64s(op->inst_info->addr), op->off_path, op->inst_info,
          hexstr64s(op->inst_info->addr), disasm_op(op, TRUE),
          unsstr64(op->op_num), unsstr64(op->unique_num));

    /* figure out next address after current instruction */
    if(op->table_info->cf_type) {
      // For pipeline gating
      if(op->table_info->cf_type == CF_CBR)
        td->td_info.fetch_br_count++;

      if(*break_fetch == BREAK_BARRIER) {
        // for fetch barriers (including syscalls), we do not want to do
        // redirect/recovery, BUT we still want to update the branch predictor.
        if (FDIP_ENABLE) {
          fdip_pred(ic->fetch_addr, op);
        } else {
          bp_predict_op(g_bp_data, op, (*cf_num)++, ic->next_fetch_addr);

          //bp_predict_op(g_bp_data, op, (*cf_num)++, ic->fetch_addr);
          op->oracle_info.mispred   = 0;
          op->oracle_info.misfetch  = 0;
          op->oracle_info.btb_miss  = 0;
          op->oracle_info.no_target = 0;
        }
        ic->next_fetch_addr       = ADDR_PLUS_OFFSET(
          ic->next_fetch_addr, op->inst_info->trace_info.inst_size);
        ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->next_fetch_addr)
      } else {
        if (FDIP_ENABLE) {
          ic->next_fetch_addr = fdip_pred(ic->fetch_addr, op);
        }
        else {
          ic->next_fetch_addr = bp_predict_op(g_bp_data, op, (*cf_num)++, ic->next_fetch_addr);
        }

        // initially bp_predict_op can return a garbage, for multi core run,
        // addr must follow cmp addr convention
        ic->next_fetch_addr = convert_to_cmp_addr(ic->proc_id,
                                                  ic->next_fetch_addr);
        ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->next_fetch_addr)
      }
      ASSERT(ic->proc_id,
             (op->oracle_info.mispred << 2 | op->oracle_info.misfetch << 1 |
              op->oracle_info.btb_miss) <= 0x7);

      const uns8 mispred       = op->oracle_info.mispred;
      const uns8 late_mispred  = op->oracle_info.late_mispred;
      const uns8 misfetch      = op->oracle_info.misfetch;
      const uns8 late_misfetch = op->oracle_info.late_misfetch;

      inc_bstat_fetched(op);

      /* if it's a mispredict, kick the oracle off path */
      if(mispred || misfetch ||
         (USE_LATE_BP && (late_mispred || late_misfetch))) {
        ic->off_path = TRUE;

        if(FETCH_OFF_PATH_OPS) {
          if(mispred || misfetch) {
            DEBUG(ic->proc_id,
                  "Cycle %llu: redirected frontend because of the "
                  "early branch predictor to 0x%s\n",
                  cycle_count, hexstr64s(ic->next_fetch_addr));
            frontend_redirect(td->proc_id, op->inst_uid, ic->next_fetch_addr);
          }

          if(USE_LATE_BP) {
            if((mispred || misfetch) && !late_mispred && !late_misfetch) {
              bp_sched_recovery(bp_recovery_info, op, cycle_count,
                                /*late_bp_recovery=*/TRUE,
                                /*force_offpath=*/FALSE);
              DEBUG(ic->proc_id,
                    "Scheduled a recovery to correct addr for cycle %llu\n",
                    cycle_count + LATE_BP_LATENCY);
            } else if((late_mispred || late_misfetch) &&
                      op->oracle_info.pred_npc !=
                        op->oracle_info.late_pred_npc) {
              bp_sched_recovery(bp_recovery_info, op, cycle_count,
                                /*late_bp_recovery=*/TRUE,
                                /*force_offpath=*/TRUE);
              DEBUG(ic->proc_id,
                    "Scheduled a recovery to wrong addr for cycle %llu\n",
                    cycle_count + LATE_BP_LATENCY);
            }
          }
        } else {
          *break_fetch = BREAK_OFFPATH;
        }

        // pipeline gating
        if(!op->off_path)
          td->td_info.last_bp_miss_op = op;
        ///////////////////////////////////////
      }

      
      /* if it's a btb miss, quit fetching and wait for redirect */
      if(op->oracle_info.btb_miss) {
        *break_fetch = BREAK_BTB_MISS;
        DEBUG(ic->proc_id, "Changed icache to wait for redirect %llu\n",
              cycle_count);
        // During a resteer, do not perform off-path prefetching
        if (FDIP_ENABLE)
          runahead_disable = TRUE;
        return IC_WAIT_FOR_REDIRECT;
      }
      
      /* if it's a taken branch, wait for timer */
      if(FETCH_BREAK_ON_TAKEN && op->oracle_info.pred &&
         *break_fetch != BREAK_BARRIER) {
        *break_fetch = BREAK_TAKEN;
        if(FETCH_TAKEN_BUBBLE_CYCLES >= 1) {
          ic->timer_cycle = cycle_count + FETCH_TAKEN_BUBBLE_CYCLES;
          return IC_WAIT_FOR_TIMER;
        } else
          return IC_FETCH;
      }
    } else {
      if(op->eom) {
        ic->next_fetch_addr = ADDR_PLUS_OFFSET(
          ic->next_fetch_addr, op->inst_info->trace_info.inst_size);
        ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->next_fetch_addr)
      }
      // pass the global branch history to all the instructions
      op->oracle_info.pred_global_hist = g_bp_data->global_hist;
      ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->next_fetch_addr)
    }

    if(packet_break == PB_BREAK_AFTER)
      break;
  }

  if(*break_fetch == BREAK_BARRIER) {
    return IC_WAIT_FOR_EMPTY_ROB;
  }
  
  if(*break_fetch == BREAK_UC_MISS){
    /*
      Add one cycle penalty for switch from UC to IC
    */
    STAT_EVENT(ic->proc_id, UOP_CACHE_UC_TO_IC_SWITCH);
    return IC_WAIT_UC_IC_SWITCH;
  }

  return IC_FETCH;
}

/**************************************************************************************/
/* icache_fill_line: */

Flag icache_fill_line(Mem_Req* req)  // cmp FIXME maybe needed to be optimized
{
  Addr         repl_line_addr;
  Addr         repl_line_addr2;
  Inst_Info**  line;
  Addr         dummy_addr;
  Addr         dummy_addr2;
  Icache_Data* line_info = NULL;
  UNUSED(line);
  Load_Type load_type = TYPE_LOAD;
  if (req->type == MRT_IPRF || req->type == MRT_UOCPRF || req->type == MRT_FDIPPRF || req->type == MRT_DPRF) load_type = TYPE_PREF;
  else if (req->type == MRT_WB || req->type == MRT_WB_NODIRTY) load_type = TYPE_WRITEBACK;

  // cmp
  if(model->id == CMP_MODEL) {
    set_icache_stage(&cmp_model.icache_stage[req->proc_id]);
  }

  ASSERT(ic->proc_id, ic->proc_id == req->proc_id);

  if(req->dirty_l0) {
    STAT_EVENT(ic->proc_id, DIRTY_WRITE_TO_ICACHE);
    printf("fetch_addr:%s line_addr:%s req_addr:%s off:%d\n",
           hexstr64s(ic->fetch_addr), hexstr64s(ic->line_addr),
           hexstr64s(req->addr), ic->off_path);
  }

  /* get new line in the cache */
  if((ic->line_addr == req->addr) && ((ic->state == IC_WAIT_FOR_MISS) ||
                                      (ic->next_state == IC_WAIT_FOR_MISS))) {
    if(IC_PREF_CACHE_ENABLE &&  // cmp FIXME prefetchers
       (USE_CONFIRMED_OFF ? req->off_path_confirmed : req->off_path)) {
      Addr pref_line_addr;

      line = (Inst_Info**)cache_insert(&ic->pref_icache, ic->proc_id,
                                       ic->fetch_addr, &pref_line_addr,
                                       &repl_line_addr, load_type);
      DEBUG(
        ic->proc_id,
        "Insert PREF_ICACHE fetch_addr0x:%s line_addr:%s index:%ld addr:0x%s\n",
        hexstr64(ic->fetch_addr), hexstr64(pref_line_addr),
        (long int)(req - mem->req_buffer), hexstr64s(req->addr));
      STAT_EVENT(ic->proc_id, IC_PREF_CACHE_FILL);

      ic->next_state = IC_FETCH;
      return TRUE;
    }

    ic->line = (Inst_Info**)cache_insert(&ic->icache, ic->proc_id,
                                         ic->fetch_addr, &ic->line_addr,
                                         &repl_line_addr, load_type);

    STAT_EVENT(ic->proc_id, ICACHE_FILL);

    if(WP_COLLECT_STATS) {  // cmp IGNORE
      line_info = (Icache_Data*)cache_insert(&ic->icache_line_info, ic->proc_id,
                                             ic->fetch_addr, &dummy_addr2,
                                             &repl_line_addr2, load_type);
      line_info->fetched_by_offpath = USE_CONFIRMED_OFF ?
                                        req->off_path_confirmed :
                                        req->off_path;
      line_info->offpath_op_addr   = req->oldest_op_addr;
      line_info->offpath_op_unique = req->oldest_op_unique_num;
      line_info->fetch_cycle       = cycle_count;
      line_info->onpath_use_cycle  = req->off_path ? 0 : cycle_count;
      line_info->read_count[0]     = 0;
      line_info->read_count[1]     = 0;
      if (FDIP_ENABLE)
        line_info->HW_prefetch     = (req->type == MRT_IFETCH);
      else
        line_info->HW_prefetch       = (req->type == MRT_IPRF);
      wp_process_icache_fill(line_info, req);
    }

    ic->next_state = IC_FETCH;
    STAT_EVENT(ic->proc_id, ICACHE_FILL_CORRECT_REQ);
  } else {
    if(IC_PREF_CACHE_ENABLE &&  // cmp FIXME prefetchers
       (USE_CONFIRMED_OFF ? req->off_path_confirmed : req->off_path)) {
      Addr pref_line_addr;

      line = (Inst_Info**)cache_insert(&ic->pref_icache, ic->proc_id, req->addr,
                                       &pref_line_addr, &repl_line_addr, load_type);
      DEBUG(
        ic->proc_id,
        "Insert PREF_ICACHE fetch_addr0x:%s line_addr:%s index:%ld addr:0x%s\n",
        hexstr64(req->addr), hexstr64(pref_line_addr),
        (long int)(req - mem->req_buffer), hexstr64s(req->addr));
      STAT_EVENT(ic->proc_id, IC_PREF_CACHE_FILL);

      return TRUE;
    }

    line = (Inst_Info**)cache_insert(&ic->icache, ic->proc_id, req->addr,
                                     &dummy_addr, &repl_line_addr, load_type);

    if(WP_COLLECT_STATS) {  // cmp IGNORE
      line_info = (Icache_Data*)cache_insert(&ic->icache_line_info, ic->proc_id,
                                             req->addr, &dummy_addr2,
                                             &repl_line_addr2, load_type);
      STAT_EVENT(ic->proc_id, ICACHE_FILL);

      line_info->fetched_by_offpath = USE_CONFIRMED_OFF ?
                                        req->off_path_confirmed :
                                        req->off_path;
      line_info->offpath_op_addr   = req->oldest_op_addr;
      line_info->offpath_op_unique = req->oldest_op_unique_num;
      line_info->fetch_cycle       = cycle_count;
      line_info->onpath_use_cycle  = req->off_path ? 0 : cycle_count;
      line_info->read_count[0]     = 0;
      line_info->read_count[1]     = 0;
      if (FDIP_ENABLE)
        line_info->HW_prefetch     = (req->type == MRT_IFETCH);
      else
        line_info->HW_prefetch       = (req->type == MRT_IPRF);
      wp_process_icache_fill(line_info, req);
    }

    STAT_EVENT(ic->proc_id, ICACHE_FILL_INCORRECT_REQ);
  }

  return TRUE;
}

/**************************************************************************************/
/* icache_off_path: */

inline Flag icache_off_path(void) {
  return ic->off_path;
}

/*************************************************************************************/
/* ic_pref_cace_access() */

Inst_Info** ic_pref_cache_access(void) {
  Addr        repl_line_addr, inval_line_addr;
  Inst_Info** inserted_line = NULL;

  ASSERT_PROC_ID_IN_ADDR(ic->proc_id, ic->fetch_addr)
  Inst_Info** line = (Inst_Info**)cache_access(&ic->pref_icache, ic->fetch_addr,
                                               &ic->line_addr, FALSE, TYPE_PREF);

  if(ic->off_path && !PREFCACHE_MOVE_OFFPATH) {
    if(line) {
      DEBUG(ic->proc_id, "off_path ic_pref cache hit:fetch_addr:0x%s \n",
            hexstr64(ic->fetch_addr));
      STAT_EVENT(ic->proc_id, IC_PREF_CACHE_HIT_PER_OFFPATH);
      STAT_EVENT(ic->proc_id, IC_PREF_CACHE_HIT_OFFPATH);
    }
    return line;
  }

  if(line) {
    inserted_line = (Inst_Info**)cache_insert(&ic->icache, ic->proc_id,
                                              ic->fetch_addr, &ic->line_addr,
                                              &repl_line_addr, TYPE_PREF);
    DEBUG(ic->proc_id, "ic_pref cache hit:fetch_addr:0x%s \n",
          hexstr64(ic->fetch_addr));
    STAT_EVENT(ic->proc_id, IC_PREF_MOVE_IC);

    STAT_EVENT(ic->proc_id, ICACHE_FILL_CORRECT_REQ);

    STAT_EVENT(ic->proc_id, IC_PREF_CACHE_HIT_PER + MIN2(ic->off_path, 1));
    STAT_EVENT(ic->proc_id, IC_PREF_CACHE_HIT + MIN2(ic->off_path, 1));
    cache_invalidate(&ic->pref_icache, ic->fetch_addr, &inval_line_addr);

    if(PREF_ICACHE_HIT_FILL_L1) {
      if(model->mem == MODEL_MEM) {
        Addr     line_addr;
        Cache*   l1_cache = &mem->uncores[ic->proc_id].l1->cache;
        L1_Data* l1_data  = (L1_Data*)cache_access(l1_cache, ic->fetch_addr,
                                                  &line_addr, TRUE, TYPE_PREF);
        if(!l1_data) {
          Mem_Req tmp_req;
          tmp_req.addr     = ic->fetch_addr;
          tmp_req.off_path = FALSE;
          tmp_req.op_count = 0;
          FATAL_ERROR(0, "This fill code is wrong. Writebacks may be lost.");
          l1_fill_line(&tmp_req);
          STAT_EVENT(ic->proc_id, IC_PREF_MOVE_L1);
        }
      }
    }
  }

  return inserted_line;
}

/**************************************************************************************/
/* wp_process_icache_hit: */

void wp_process_icache_hit(Icache_Data* line, Addr fetch_addr) {
  L1_Data* l1_line;

  if(!WP_COLLECT_STATS)
    return;

  if(icache_off_path() == FALSE) {
    if(line->fetched_by_offpath) {
      STAT_EVENT(ic->proc_id, ICACHE_HIT_ONPATH_SAT_BY_OFFPATH);
      STAT_EVENT(ic->proc_id, ICACHE_USE_OFFPATH);
      STAT_EVENT(ic->proc_id, DIST_ICACHE_FILL_OFFPATH_USED);
      STAT_EVENT(ic->proc_id, DIST_REQBUF_OFFPATH_USED);
      STAT_EVENT(ic->proc_id, DIST2_REQBUF_OFFPATH_USED_FULL);

      l1_line = do_l1_access_addr(fetch_addr);
      if(l1_line) {
        if(l1_line->fetched_by_offpath) {
          STAT_EVENT(ic->proc_id, L1_USE_OFFPATH);
          STAT_EVENT(ic->proc_id, DIST_L1_FILL_OFFPATH_USED);
          STAT_EVENT(ic->proc_id, L1_USE_OFFPATH_IFETCH);
          l1_line->fetched_by_offpath             = FALSE;
          l1_line->l0_modified_fetched_by_offpath = TRUE;
        }
      }
    } else {
      STAT_EVENT(ic->proc_id, ICACHE_HIT_ONPATH_SAT_BY_ONPATH);
      STAT_EVENT(ic->proc_id, ICACHE_USE_ONPATH);
    }
  } else {
    if(line->fetched_by_offpath) {
      STAT_EVENT(ic->proc_id, ICACHE_HIT_OFFPATH_SAT_BY_OFFPATH);
    } else {
      STAT_EVENT(ic->proc_id, ICACHE_HIT_OFFPATH_SAT_BY_ONPATH);
    }
  }

  if(icache_off_path() == FALSE)
    line->fetched_by_offpath = FALSE;
}

/**************************************************************************************/
/* wp_process_icache_fill: */

void wp_process_icache_fill(Icache_Data* line, Mem_Req* req) {
  if(!WP_COLLECT_STATS)
    return;

  if((req->type == MRT_WB) || (req->type == MRT_WB_NODIRTY) ||
     (req->type == MRT_IPRF)) /* for now we don't consider prefetches */
    return;

  if(req->off_path) {
    STAT_EVENT(ic->proc_id, ICACHE_FILL_OFFPATH);
  } else {
    STAT_EVENT(ic->proc_id, ICACHE_FILL_ONPATH);
    if(req->onpath_match_offpath)
      STAT_EVENT(ic->proc_id, DIST_ICACHE_FILL_ONPATH_PARTIAL);
    else
      STAT_EVENT(ic->proc_id, DIST_ICACHE_FILL_ONPATH);
  }
  STAT_EVENT(ic->proc_id, DIST_ICACHE_FILL);
}

/**************************************************************************************/
/* inst_lost_get_full_window_reason(): */

int32_t inst_lost_get_full_window_reason() {
  if(rob_stall_reason != ROB_STALL_NONE) {
    return rob_stall_reason;
  }

  if(rob_block_issue_reason != ROB_BLOCK_ISSUE_NONE) {
    return rob_block_issue_reason;
  }

  return 0;
}

Op* find_op(Addr pc) {
  Op* op;
  Op** op_p = (Op**)list_get_current(&op_buf);

  if (op_p)
    op = *op_p;

  if (!op_p) {
    op_p = (Op**)list_start_head_traversal(&op_buf);
    op = *op_p;
    if (last_runahead_op && op->op_num <= last_runahead_op) {
      for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
        op = *op_p;
        if (op->op_num == last_runahead_op) {
          op_p = (Op**)list_next_element(&op_buf);
          op = *op_p;
          break;
        }
      }
    }
  }

  for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
    op = *op_p;
    if (op->table_info->cf_type) { // first branch after the last predicted branch
      if (op->fetch_addr == pc) {
        last_runahead_uid = op->inst_uid;
        op_p = (Op**)list_next_element(&op_buf);
        last_runahead_op = op->op_num;
        return op;
      } else {
        Op** op_p_head = (Op**)list_get_head(&op_buf);
        if (op_p == op_p_head)
          break;
        op_p = (Op**)list_prev_element(&op_buf);
        op = *op_p;
        last_runahead_op = op->op_num;
        op_p = (Op**)list_next_element(&op_buf);
        op = *op_p;
        break;
      }
    }
  }
  if (!op_p) {
    op_p = (Op**)list_get_tail(&op_buf);
    op = *op_p;
    last_runahead_op = op->op_num;
  }
  return NULL;
}

void set_max_op_num(Flag is_branch, Addr last_cl_prefetched) {
  Counter move_cnt = 0;
  Op* op;
  Op** op_p;
  if (max_runahead_op == last_runahead_op) {
    op_p = (Op**)list_get_tail(&op_buf);
    ASSERT(ic->proc_id, op_p);
    op = *op_p;
    max_op_num = op->op_num;
  } else if (is_branch) {
    op_p = (Op**)list_prev_element(&op_buf);
    move_cnt++;
    ASSERT(ic->proc_id, op_p);
    op = *op_p;
    ASSERT(ic->proc_id, op->table_info->cf_type);
    max_op_num = op->op_num;
  } else if (!is_branch) {
    op_p = (Op**)list_get_current(&op_buf);
    ASSERT(ic->proc_id, op_p);
    op = *op_p;
    ASSERT(ic->proc_id, op->table_info->cf_type);
    Op** op_head = (Op**)list_get_head(&op_buf);
    if (op_head == op_p)
      return;
    op_p = (Op**)list_prev_element(&op_buf);
    move_cnt++;
    op = *op_p;
    while (op) {
      Addr cl_addr = get_cache_line_addr(&ic->icache, op->fetch_addr);
      if (op->table_info->cf_type)
        break;
      if (cl_addr == last_cl_prefetched) {
        max_op_num = op->op_num;
        break;
      }
      if (op_head != op_p) {
        op_p = (Op**)list_prev_element(&op_buf);
        move_cnt++;
        op = *op_p;
      } else {
        max_op_num = op->op_num;
        break;
      }
    }
  }

  // restore the current pointer of the buffer
  while (move_cnt) {
    list_next_element(&op_buf);
    move_cnt--;
  }
}

void move_to_prev_op(void) {
  list_prev_element(&op_buf);
}

Flag will_be_accessed(Addr pc) {
  Op* op;
  Op** op_p = (Op**)list_get_current(&op_buf);
  uns64 save_inst_uid = 0;
  Addr save_pc = 0;
  Flag found = FALSE;

  if (op_p) {
    op = *op_p;
    save_inst_uid = op->inst_uid;
    save_pc = op->fetch_addr;
    op_p = (Op**)list_next_element(&op_buf);
    op = *op_p;
  }

  if (!op_p) {
    op_p = (Op**)list_start_head_traversal(&op_buf);
    op = *op_p;
    if (last_runahead_uid && op->inst_uid <= last_runahead_uid) {
      for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
        op = *op_p;
        if (op->table_info->cf_type && op->inst_uid == last_runahead_uid) {
          op_p = (Op**)list_next_element(&op_buf);
          op = *op_p;
          break;
        }
      }
    }
  }

  // find the instruction among all the later ops in the lookahead buffer
  for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
    op = *op_p;
    if (op->fetch_addr == pc) {
      found = TRUE;
      break;
    }
  }

  // recover the current pointer for the future find_op
  if (save_inst_uid) {
    op_p = (Op**)list_start_head_traversal(&op_buf);
    for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
      op = *op_p;
      if (op->inst_uid == save_inst_uid && op->fetch_addr == save_pc)
        break;
    }
  } else {
    (&op_buf)->current = NULL;
  }

  return found;
}

// Retrieve the prediction window by looking into the lookahead buffer.
// This should only be called when on-path because if FDIP is off-path, nothing will be found in a trace. 
Uop_Cache_Data get_pw_lookahead_buffer(Addr start_addr) {
  Uop_Cache_Data pw;
  memset(&pw, 0, sizeof(pw));


  // It is not always true that the branch for which I am prefetching target
  // is still in lookahead buffer - see FDIP_PRED_FTQ_EMPTY

  List_Entry* cur = op_buf.current;
  Op** op_p = (Op**)list_get_current(&op_buf);

  // Move the op pointer to the op just after the last runahead branch.
  // If the branch was just-consumed this is the first element in the buffer.
  // This is the branch's correct target.
  if (op_p) {
    ASSERT(ic->proc_id, (*op_p)->table_info->cf_type);
    op_p = (Op**)list_next_element(&op_buf);
  } else {
    op_p = (Op**)list_start_head_traversal(&op_buf);
    if (last_runahead_uid && (*op_p)->inst_uid <= last_runahead_uid) {
      for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
        if ((*op_p)->table_info->cf_type && (*op_p)->inst_uid == last_runahead_uid) {
          op_p = (Op**)list_next_element(&op_buf);
          break;
        }
      }
    }
  }

  // ASSERT that the op identified matches the target/runahead_pc
  ASSERT(ic->proc_id, (*op_p)->inst_info->addr == start_addr);
  pw.first = start_addr;

  // Next move down runahead buffer, incrementing n_uops until a branch is found.
  for(; op_p; op_p = (Op**)list_next_element(&op_buf)) {
    pw.n_uops++;
    pw.last = (*op_p)->inst_info->addr;
    if ((*op_p)->table_info->cf_type) { // first branch after the last predicted branch, end of pw
      break;
    }
  }

  printf("In istage, n_uops is %u, first address is %llu\n", pw.n_uops, pw.first);

  // Reset current to prevent side effects
  op_buf.current = cur;
  ASSERT(ic->proc_id, (*op_p)->table_info->cf_type);
  ASSERT(ic->proc_id, pw.n_uops);

  return pw;
}

void log_stats_ic_miss() {
  STAT_EVENT(ic->proc_id, ICACHE_MISS);
  STAT_EVENT(ic->proc_id, POWER_ICACHE_MISS);
  STAT_EVENT(ic->proc_id, ICACHE_MISS_ONPATH + ic->off_path);
}

// Wrapper callback for any instruction memreq.
// This must always return TRUE so that memreq is satisfied that
// done_func is finished and does not need to be retried.
// (uop_cache_fill_prefetch will fail for never-seen PWs on the off-path).
Flag instr_fill_line(Mem_Req* req) {
  ASSERT(ic->proc_id, req->type == MRT_IPRF || req->type == MRT_FDIPPRF || req->type == MRT_UOCPRF || req->type == MRT_IFETCH);

  if (mem_req_is_type(req, MRT_IPRF) || mem_req_is_type(req, MRT_IFETCH) || mem_req_is_type(req, MRT_FDIPPRF)) {
    icache_fill_line(req);
  }
  if (mem_req_is_type(req, MRT_FDIPPRF)) {
    fdip_dec_outstanding_prefs(req->addr, req->off_path, req->emitted_cycle);
    // TODO: comment out the line above and uncomment the line below to swith 1-outstanding prefetch counter.
    /*fdip_dec_outstanding_prefs(req->addr);*/
  }
  if (mem_req_is_type(req, MRT_UOCPRF))
    uop_cache_fill_prefetch(req->addr, !req->off_path);
  return TRUE;
}
