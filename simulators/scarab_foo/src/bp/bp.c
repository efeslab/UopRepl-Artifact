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
 * File         : bp.c
 * Author       : HPS Research Group
 * Date         : 12/9/1998
 * Description  :
 ***************************************************************************************/

#include "debug/debug_macros.h"
#include "debug/debug_print.h"
#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"
#include "isa/isa_macros.h"

#include "bp//bp_conf.h"
#include "bp/bp.h"
#include "bp/bp_targ_mech.h"
#include "bp/cbp_to_scarab.h"
#include "bp/gshare.h"
#include "bp/hybridgp.h"
#include "bp/tagescl.h"
#include "libs/cache_lib.h"
#include "model.h"
#include "thread.h"
#include "uop_cache.h"
#include "icache_stage.h"

#include "bp/bp.param.h"
#include "core.param.h"
#include "debug/debug.param.h"
#include "frontend/pin_trace_fe.h"
#include "statistics.h"

#include "prefetcher/fdip.h"
#include "prefetcher/pref.param.h"

/******************************************************************************/
/* include the table of possible branch predictors */

#include "bp/bp_table.def"

#include "memory/memory.param.h"
/******************************************************************************/
/* Collect stats for tcache */

extern void tc_do_stat(Op*, Flag);


/******************************************************************************/
/* Macros */

#define DEBUG(proc_id, args...) _DEBUG(proc_id, DEBUG_BP, ##args)
#define DEBUG_BTB(proc_id, args...) _DEBUG(proc_id, DEBUG_BTB, ##args)


/******************************************************************************/
/* Global Variables */

Bp_Recovery_Info* bp_recovery_info = NULL;
Bp_Data*          g_bp_data        = NULL;
Flag              USE_LATE_BP      = FALSE;
extern List       op_buf;

/******************************************************************************/
// Local prototypes

/******************************************************************************/
/* set_bp_data set the global bp_data pointer (so I don't have to pass it around
 * everywhere */
void set_bp_data(Bp_Data* new_bp_data) {
  g_bp_data = new_bp_data;
}

/******************************************************************************/
/* set_bp_recovery_info: set the global bp_data pointer (so I don't have to pass
 * it around everywhere */
void set_bp_recovery_info(Bp_Recovery_Info* new_bp_recovery_info) {
  bp_recovery_info = new_bp_recovery_info;
}

/******************************************************************************/
/*  init_bp_recovery_info */

void init_bp_recovery_info(uns8              proc_id,
                           Bp_Recovery_Info* new_bp_recovery_info) {
  ASSERT(proc_id, new_bp_recovery_info);
  memset(new_bp_recovery_info, 0, sizeof(Bp_Recovery_Info));

  new_bp_recovery_info->proc_id = proc_id;

  new_bp_recovery_info->recovery_cycle = MAX_CTR;
  new_bp_recovery_info->redirect_cycle = MAX_CTR;

  bp_recovery_info = new_bp_recovery_info;

  init_hash_table(&per_branch_stat, "Per Branch Hit/Miss and Recovery/Redirect Stall cycles",
                  15000000, sizeof(Per_Branch_Stat));
}


/******************************************************************************/
/* bp_sched_recover: called on a mispredicted op when it's misprediction is
   first realized */


void bp_sched_recovery(Bp_Recovery_Info* bp_recovery_info, Op* op,
                       Counter cycle, Flag late_bp_recovery,
                       Flag force_offpath) {
  ASSERT(op->proc_id, bp_recovery_info->proc_id == op->proc_id);

  if(bp_recovery_info->recovery_cycle == MAX_CTR ||
     op->op_num <= bp_recovery_info->recovery_op_num) {
    const Addr next_fetch_addr = op->oracle_info.npc;

    uns fetch_latency;
    if(return_repl_policy() == 1){
      // add_timestamp_addr(next_fetch_addr);//, timestamp_new_opt_istage);  
    }
    /*Yilong Print*/
    // printf("[BP] Recover Lookup address: %lld\n", next_fetch_addr);
    Flag uc_hit = in_uop_cache(next_fetch_addr, NULL, FALSE, FALSE);
    if(UOP_INCLUSION == UOPI_ACTUAL){
      uc_hit &= in_icache(op->oracle_info.npc);
    }
    inc_bstat_miss(op, uc_hit);
    if (uc_hit) {
      fetch_latency = UOP_CACHE_LATENCY;
      INC_STAT_EVENT(bp_recovery_info->proc_id, BP_RECOVERY_FETCH_CYCLES_UC, UOP_CACHE_LATENCY);
    } else {
      fetch_latency = ICACHE_LATENCY;
      INC_STAT_EVENT(bp_recovery_info->proc_id, BP_RECOVERY_FETCH_CYCLES_IC, ICACHE_LATENCY);
    }
    const uns latency = late_bp_recovery ? fetch_latency + 1 + LATE_BP_LATENCY :
                                           fetch_latency + 1;
    DEBUG(
      bp_recovery_info->proc_id,
      "Recovery signaled for op_num:%s @ 0x%s  next_fetch:0x%s offpath:%d\n",
      unsstr64(op->op_num), hexstr64s(op->inst_info->addr),
      hexstr64s(next_fetch_addr), op->off_path);
    ASSERT(op->proc_id, !op->oracle_info.recovery_sch);
    op->oracle_info.recovery_sch          = TRUE;
    bp_recovery_info->recovery_cycle      = cycle + latency;
    bp_recovery_info->recovery_fetch_addr = next_fetch_addr;
    if(op->proc_id)
      ASSERT(op->proc_id, bp_recovery_info->recovery_fetch_addr);

    bp_recovery_info->recovery_op_num        = op->op_num;
    bp_recovery_info->recovery_cf_type       = op->table_info->cf_type;
    bp_recovery_info->recovery_info          = op->recovery_info;
    bp_recovery_info->recovery_info.op_num   = op->op_num;
    if (FDIP_ENABLE)
      bp_recovery_info->recovery_info.npc    = next_fetch_addr;
    bp_recovery_info->recovery_inst_info     = op->inst_info;
    bp_recovery_info->recovery_force_offpath = op->off_path;
    bp_recovery_info->recovery_op            = op;
    bp_recovery_info->oracle_cp_num          = op->oracle_cp_num;
    bp_recovery_info->recovery_unique_num    = op->unique_num;
    bp_recovery_info->recovery_inst_uid      = op->inst_uid;
    bp_recovery_info->wpe_flag               = FALSE;
    bp_recovery_info->late_bp_recovery       = late_bp_recovery;

    if(force_offpath) {
      ASSERT(op->proc_id, late_bp_recovery);
      bp_recovery_info->recovery_fetch_addr    = op->oracle_info.late_pred_npc;
      bp_recovery_info->recovery_info.new_dir  = op->oracle_info.late_pred;
      bp_recovery_info->recovery_force_offpath = TRUE;
      bp_recovery_info->late_bp_recovery_wrong = TRUE;
    } else {
      bp_recovery_info->late_bp_recovery_wrong = FALSE;
    }
  }
}

void inc_bstat_fetched(Op* op) {
  Flag new_entry;
  Per_Branch_Stat* bstat = (Per_Branch_Stat*) hash_table_access_create(&per_branch_stat, op->inst_info->addr, &new_entry);
  if (new_entry) {
    memset(bstat, 0, sizeof(*bstat));
    bstat->addr = op->inst_info->addr;
    bstat->cf_type = op->table_info->cf_type;
  }

  const uns8 mispred =  op->oracle_info.mispred;
  const uns8 misfetch = op->oracle_info.misfetch;
  const uns8 btb_miss = op->oracle_info.btb_miss;

  if (!mispred && !misfetch && !btb_miss) {
    if(return_repl_policy() == 1){
      // add_timestamp_addr(op->oracle_info.npc);//, timestamp_new_opt_istage);  
    }
    /*Yilong Print*/
    // printf("[BP] bstat_fetched Lookup address: %lld\n", op->oracle_info.npc);
    if (in_uop_cache(op->oracle_info.npc, NULL, FALSE, FALSE)) {
      bstat->bpu_hit_uc_hit += 1;
    }else {
      bstat->bpu_hit_uc_miss += 1;
      if (!in_icache(op->inst_info->addr)) bstat->bpu_hit_uc_ic_miss += 1;
    }
  }

  // target if taken
  if (op->oracle_info.pred && !op->oracle_info.mispred)
    bstat->target = op->oracle_info.npc;
}

void inc_bstat_miss(Op* op, Flag uc_hit) {
  Per_Branch_Stat* bstat = (Per_Branch_Stat*) hash_table_access(&per_branch_stat, op->inst_info->addr);
  ASSERT(bp_recovery_info->proc_id, bstat);

  const uns8 mispred =  op->oracle_info.mispred;
  const uns8 misfetch = op->oracle_info.misfetch;
  const uns8 btb_miss = op->oracle_info.btb_miss;
  ASSERT(bp_recovery_info->proc_id, mispred || misfetch || btb_miss);

  const Flag in_ic = in_icache(op->inst_info->addr);

  if (uc_hit) {
    if (mispred)        bstat->mispred_uc_hit += 1;
    else if (misfetch)  bstat->misfetch_uc_hit += 1;
    else                bstat->btb_miss_uc_hit += 1;
  } else {
    if (mispred) {
      bstat->mispred_uc_miss += 1;
      if (!in_ic) bstat->mispred_uc_ic_miss += 1;
    } else if (misfetch) {
      bstat->misfetch_uc_miss += 1;
      if (!in_ic) bstat->misfetch_uc_ic_miss += 1;
    } else {
      bstat->btb_miss_uc_miss += 1;
      if (!in_ic) bstat->btb_miss_uc_ic_miss += 1;
    }
    bstat->recover_redirect_extra_fetch_latency += ICACHE_LATENCY - UOP_CACHE_LATENCY;
  }
}

/******************************************************************************/
/* bp_sched_redirect: called on an op that caused the fetch stage to suspend
   (eg. a btb miss).  The pred_npc is what is used for the new pc. */

void bp_sched_redirect(Bp_Recovery_Info* bp_recovery_info, Op* op,
                       Counter cycle) {
  if(bp_recovery_info->redirect_cycle == MAX_CTR ||
     op->op_num < bp_recovery_info->redirect_op_num) {
    DEBUG(bp_recovery_info->proc_id, "Redirect signaled for op_num:%s @ 0x%s\n",
          unsstr64(op->op_num), hexstr64s(op->inst_info->addr));

    uns fetch_latency;
    if(return_repl_policy() == 1){
      // add_timestamp_addr(op->oracle_info.npc);//, timestamp_new_opt_istage);  
    }
    /*Yilong Print*/
    // printf("[BP] sched_redirect Lookup address: %lld\n", op->oracle_info.npc);
    Flag uc_hit = in_uop_cache(op->oracle_info.npc, NULL, FALSE, FALSE);  // should this be pred_npc? We may not have predicted correctly.
    if(UOP_INCLUSION == UOPI_ACTUAL){
      uc_hit &= in_icache(op->oracle_info.npc);
    }
    inc_bstat_miss(op, uc_hit);
    if (uc_hit) {
      fetch_latency = UOP_CACHE_LATENCY;
      INC_STAT_EVENT(bp_recovery_info->proc_id, BP_REDIRECT_FETCH_CYCLES_UC, UOP_CACHE_LATENCY);
    } else {
      fetch_latency = ICACHE_LATENCY;
      INC_STAT_EVENT(bp_recovery_info->proc_id, BP_REDIRECT_FETCH_CYCLES_IC, ICACHE_LATENCY);
    }
    bp_recovery_info->redirect_cycle = cycle + 1 + fetch_latency +
                                       (op->table_info->cf_type == CF_SYS ?
                                          EXTRA_CALLSYS_CYCLES :
                                          0);
    bp_recovery_info->redirect_op     = op;
    bp_recovery_info->redirect_op_num = op->op_num;
    if (FDIP_ENABLE && !op->oracle_info.mispred && !op->oracle_info.misfetch) {
      fdip_redirect(op->oracle_info.pred_npc);
    }
  }
}


/******************************************************************************/
/* init_bp:  initializes all branch prediction structures */

void init_bp_data(uns8 proc_id, Bp_Data* bp_data) {
  uns ii;
  ASSERT(bp_data->proc_id, bp_data);
  memset(bp_data, 0, sizeof(Bp_Data));

  bp_data->proc_id = proc_id;
  /* initialize branch predictor */
  bp_data->bp = &bp_table[BP_MECH];
  bp_data->bp->init_func();

  USE_LATE_BP = (LATE_BP_MECH != NUM_BP);

  if(USE_LATE_BP) {
    bp_data->late_bp = &bp_table[LATE_BP_MECH];
    bp_data->late_bp->init_func();
  } else {
    bp_data->late_bp = NULL;
  }


  /* init btb structure */
  bp_data->bp_btb = &bp_btb_table[BTB_MECH];
  bp_data->bp_btb->init_func(bp_data);

  /* init call-return stack */
  bp_data->crs.entries  = (Crs_Entry*)malloc(sizeof(Crs_Entry) * CRS_ENTRIES *
                                            2);
  bp_data->crs.off_path = (Flag*)malloc(sizeof(Flag) * CRS_ENTRIES);
  for(ii = 0; ii < CRS_ENTRIES; ii++) {
    bp_data->crs.entries[ii].addr = 0;
    bp_data->crs.off_path[ii]     = FALSE;
  }

  /* initialize the indirect target branch predictor */
  bp_data->bp_ibtb = &bp_ibtb_table[IBTB_MECH];
  bp_data->bp_ibtb->init_func(bp_data);
  bp_data->target_bit_length = IBTB_HIST_LENGTH / TARGETS_IN_HIST;
  if(!USE_PAT_HIST)
    ASSERTM(bp_data->proc_id,
            bp_data->target_bit_length * TARGETS_IN_HIST == IBTB_HIST_LENGTH,
            "IBTB_HIST_LENGTH must be a multiple of TARGETS_IN_HIST\n");

  g_bp_data = bp_data;

  /* confidence */
  if(ENABLE_BP_CONF) {
    bp_data->br_conf = &br_conf_table[CONF_MECH];
    bp_data->br_conf->init_func();
  }
}

Flag bp_is_predictable(Bp_Data* bp_data, Op* op) {
  return !bp_data->bp->full_func(op);
}


/******************************************************************************/
/* bp_predict_op:  predicts the target of a control flow instruction */

Addr bp_predict_op(Bp_Data* bp_data, Op* op, uns br_num, Addr fetch_addr) {
  Addr addr = fetch_addr;
  /*Addr line_addr;*/
  Addr* btb_target;
  Addr  pred_target;

  ASSERT(bp_data->proc_id, bp_data->proc_id == op->proc_id);
  ASSERT(bp_data->proc_id, op->table_info->cf_type);

  /* set address used to predict branch */
  op->oracle_info.pred_addr         = addr;
  op->oracle_info.btb_miss_resolved = FALSE;
  op->cf_within_fetch               = br_num;

  /* initialize recovery information---this stuff might be
     overwritten by a prediction function that uses and
     speculatively updates global history */
  op->recovery_info.proc_id          = op->proc_id;
  op->recovery_info.pred_global_hist = bp_data->global_hist;
  op->recovery_info.targ_hist        = bp_data->targ_hist;
  op->recovery_info.new_dir          = op->oracle_info.dir;
  op->recovery_info.crs_next         = bp_data->crs.next;
  op->recovery_info.crs_tos          = bp_data->crs.tos;
  op->recovery_info.crs_depth        = bp_data->crs.depth;
  op->recovery_info.op_num           = op->op_num;
  op->recovery_info.PC               = op->inst_info->addr;
  op->recovery_info.cf_type          = op->table_info->cf_type;
  op->recovery_info.oracle_dir       = op->oracle_info.dir;
  op->recovery_info.branchTarget     = op->oracle_info.target;


  bp_data->bp->timestamp_func(op);
  if(USE_LATE_BP) {
    bp_data->late_bp->timestamp_func(op);
  }

  if(BP_HASH_TOS || IBTB_HASH_TOS) {
    Addr tos_addr;
    uns  new_next = CIRC_DEC2(bp_data->crs.next, CRS_ENTRIES);
    uns  new_tail = CIRC_DEC2(bp_data->crs.tail, CRS_ENTRIES);
    Flag flag     = bp_data->crs.off_path[new_tail];
    switch(CRS_REALISTIC) {
      case 0:
        tos_addr = bp_data->crs.entries[new_tail << 1 | flag].addr;
        break;
      case 1:
        tos_addr = bp_data->crs.entries[bp_data->crs.tos].addr;
        break;
      case 2:
        tos_addr = bp_data->crs.entries[new_next].addr;
        break;
      default:
        tos_addr = 0;
        break;
    }
    op->recovery_info.tos_addr = tos_addr;
  }

  // {{{ special case--system calls
  if(op->table_info->cf_type == CF_SYS) {
    op->oracle_info.pred          = TAKEN;
    op->oracle_info.misfetch      = FALSE;
    op->oracle_info.mispred       = FALSE;
    op->oracle_info.late_misfetch = FALSE;
    op->oracle_info.late_mispred  = FALSE;
    op->oracle_info.btb_miss      = FALSE;
    op->oracle_info.no_target     = FALSE;
    op->oracle_info.pred_npc      = op->oracle_info.npc;
    op->oracle_info.late_pred_npc = op->oracle_info.npc;
    bp_data->bp->spec_update_func(op);
    if(USE_LATE_BP) {
      bp_data->late_bp->spec_update_func(op);
    }
    return op->oracle_info.npc;
  }
  // }}}

  // {{{ access btb for branch information and target

  // we assume that some branch information is stored in the BTB.
  // In the event of a btb miss, the branch will predicted as
  // normal, but will incur the redirect penalty for missing in the
  // btb.  btb_miss and pred_target are set appropriately.

  btb_target = bp_data->bp_btb->pred_func(bp_data, op);
  if(btb_target) {
    // btb hit
    op->oracle_info.btb_miss  = FALSE;
    op->oracle_info.no_target = FALSE;
    pred_target               = *btb_target;
  } else {
    // btb miss
    op->oracle_info.btb_miss  = TRUE;
    op->oracle_info.no_target = TRUE;
    pred_target               = op->oracle_info.target;
  }
  // }}}
  // {{{ handle predictions for individual cf types
  switch(op->table_info->cf_type) {
    case CF_BR:
      if(PERFECT_BP) {
        op->oracle_info.pred      = op->oracle_info.dir;
        op->oracle_info.late_pred = op->oracle_info.dir;
      } else {
        op->oracle_info.pred      = TAKEN;
        op->oracle_info.late_pred = TAKEN;
      }
      if(!op->off_path)
        STAT_EVENT(op->proc_id, CF_BR_USED_TARGET_CORRECT +
                                  (pred_target != op->oracle_info.npc));
      break;

    case CF_CBR:
      // Branch predictors may use pred_global_hist as input.
      op->oracle_info.pred_global_hist = bp_data->global_hist;

      if(PERFECT_BP) {
        op->oracle_info.pred      = op->oracle_info.dir;
        op->oracle_info.no_target = FALSE;
      } else {
        op->oracle_info.pred = bp_data->bp->pred_func(op);
        if(USE_LATE_BP) {
          op->oracle_info.late_pred = bp_data->late_bp->pred_func(op);
        }
      }

      // Update history used by the rest of Scarab.
      bp_data->global_hist = (bp_data->global_hist >> 1) |
                             (op->oracle_info.pred << 31);

      if(PERFECT_CBR_BTB ||
         (PERFECT_NT_BTB && op->oracle_info.pred == NOT_TAKEN)) {
        pred_target               = op->oracle_info.target;
        op->oracle_info.btb_miss  = FALSE;
        op->oracle_info.no_target = FALSE;
      }
      if(!op->off_path && op->oracle_info.pred)
        STAT_EVENT(op->proc_id, CF_CBR_USED_TARGET_CORRECT +
                                  (pred_target != op->oracle_info.npc));
      break;

    case CF_CALL:
      op->oracle_info.pred      = TAKEN;
      op->oracle_info.late_pred = TAKEN;
      if(ENABLE_CRS)
        CRS_REALISTIC ? bp_crs_realistic_push(bp_data, op) :
                        bp_crs_push(bp_data, op);
      if(!op->off_path)
        STAT_EVENT(op->proc_id, CF_CALL_USED_TARGET_CORRECT +
                                  (pred_target != op->oracle_info.npc));
      break;

    case CF_IBR:
      if(PERFECT_BP) {
        op->oracle_info.pred      = op->oracle_info.dir;
        op->oracle_info.late_pred = op->oracle_info.dir;
      } else {
        op->oracle_info.pred      = TAKEN;
        op->oracle_info.late_pred = TAKEN;
      }
      if(ENABLE_IBP) {
        Addr ibp_target = bp_data->bp_ibtb->pred_func(bp_data, op);
        if(ibp_target) {
          pred_target               = ibp_target;
          op->oracle_info.no_target = FALSE;
          op->oracle_info.ibp_miss  = FALSE;
        } else
          op->oracle_info.ibp_miss = TRUE;
      if(!op->off_path)
        STAT_EVENT(op->proc_id, CF_IBR_USED_TARGET_CORRECT +
            (pred_target != op->oracle_info.npc));
      }
      break;

    case CF_ICALL:
      if(PERFECT_BP) {
        op->oracle_info.pred      = op->oracle_info.dir;
        op->oracle_info.late_pred = op->oracle_info.dir;
      } else {
        op->oracle_info.pred      = TAKEN;
        op->oracle_info.late_pred = TAKEN;
      }
      if(ENABLE_IBP) {
        Addr ibp_target = bp_data->bp_ibtb->pred_func(bp_data, op);
        if(ibp_target) {
          pred_target               = ibp_target;
          op->oracle_info.no_target = FALSE;
          op->oracle_info.ibp_miss  = FALSE;
        } else
          op->oracle_info.ibp_miss = TRUE;
      }
      if(ENABLE_CRS)
        CRS_REALISTIC ? bp_crs_realistic_push(bp_data, op) :
                        bp_crs_push(bp_data, op);
      if(!op->off_path)
        STAT_EVENT(op->proc_id, CF_ICALL_USED_TARGET_CORRECT +
                                  (pred_target != op->oracle_info.npc));
      break;

    case CF_ICO:
      op->oracle_info.pred      = TAKEN;
      op->oracle_info.late_pred = TAKEN;
      if(ENABLE_CRS) {
        pred_target = CRS_REALISTIC ? bp_crs_realistic_pop(bp_data, op) :
                                      bp_crs_pop(bp_data, op);
        CRS_REALISTIC ? bp_crs_realistic_push(bp_data, op) :
                        bp_crs_push(bp_data, op);
        if(!op->off_path)
          STAT_EVENT(op->proc_id, CF_ICO_USED_TARGET_CORRECT +
                                    (pred_target != op->oracle_info.npc));
      }
      break;

    case CF_RET:
      if(PERFECT_BP) {
        op->oracle_info.pred      = op->oracle_info.dir;
        op->oracle_info.late_pred = op->oracle_info.dir;
      } else {
        op->oracle_info.pred      = TAKEN;
        op->oracle_info.late_pred = TAKEN;
      }
      if(ENABLE_CRS)
        pred_target = CRS_REALISTIC ? bp_crs_realistic_pop(bp_data, op) :
                                      bp_crs_pop(bp_data, op);
      if(!op->off_path)
        STAT_EVENT(op->proc_id, CF_RET_USED_TARGET_CORRECT +
                                  (pred_target != op->oracle_info.npc));
      break;

    default:
      op->oracle_info.pred      = TAKEN;
      op->oracle_info.late_pred = TAKEN;
      if(!op->off_path)
        STAT_EVENT(op->proc_id, CF_DEFAULT_USED_TARGET_CORRECT +
                                  (pred_target != op->oracle_info.npc));
      break;
  }
  // }}}

  bp_data->bp->spec_update_func(op);
  if(USE_LATE_BP) {
    bp_data->late_bp->spec_update_func(op);
  }

  const Addr pc_plus_offset = ADDR_PLUS_OFFSET(
    op->inst_info->addr, op->inst_info->trace_info.inst_size);

  op->pred_target = pred_target;
  op->pc_plus_offset = pc_plus_offset;

  const Addr prediction = op->oracle_info.pred ? pred_target : pc_plus_offset;
  op->oracle_info.pred_npc = prediction;
  if(USE_LATE_BP) {
    const Addr late_prediction = op->oracle_info.late_pred ? pred_target :
      pc_plus_offset;
    op->oracle_info.late_pred_npc = late_prediction;
  }

  bp_predict_op_evaluate(bp_data, op, prediction);
  return prediction;
}

/* Separate performing branch prediction from evaluating the prediction into
 * two functions, enabling FDIP.
 */
Addr bp_predict_op_evaluate(Bp_Data* bp_data, Op *op, Addr prediction) {
// If the direction prediction is wrong, but next address happens to be right
  // anyway, do not treat this as a misprediction.
  op->oracle_info.mispred = (op->oracle_info.pred != op->oracle_info.dir) &&
                            (prediction != op->oracle_info.npc);
  op->oracle_info.misfetch = !op->oracle_info.mispred &&
                             prediction != op->oracle_info.npc;

  if(USE_LATE_BP) {
    const Addr late_prediction = op->oracle_info.late_pred_npc;
    op->oracle_info.late_mispred  = (op->oracle_info.late_pred !=
                                    op->oracle_info.dir) &&
                                   (late_prediction != op->oracle_info.npc);
    op->oracle_info.late_misfetch = !op->oracle_info.late_mispred &&
                                    late_prediction != op->oracle_info.npc;
  }

  op->bp_cycle = cycle_count;

  // {{{ stats and debugging
  if(!op->oracle_info.btb_miss) {
    if(!op->off_path)
      STAT_EVENT(op->proc_id, BTB_ON_PATH_HIT);
    else
      STAT_EVENT(op->proc_id, BTB_OFF_PATH_HIT);
  } else {
    if(!op->off_path)
      STAT_EVENT(op->proc_id, BTB_ON_PATH_MISS);
    else
      STAT_EVENT(op->proc_id, BTB_OFF_PATH_MISS);
  }

  STAT_EVENT(op->proc_id, BP_ON_PATH_CORRECT + op->oracle_info.mispred +
                            2 * op->oracle_info.misfetch + 3 * op->off_path);
  STAT_EVENT(op->proc_id,
             LATE_BP_ON_PATH_CORRECT + op->oracle_info.late_mispred +
               2 * op->oracle_info.late_misfetch + 3 * op->off_path);

  if(!op->off_path) {
    if(op->oracle_info.mispred)
      td->td_info.mispred_counter++;
    else
      td->td_info.corrpred_counter++;
  }

  if(op->table_info->cf_type == CF_CBR) {
    STAT_EVENT(op->proc_id, CBR_ON_PATH_CORRECT + op->oracle_info.mispred +
                              2 * op->off_path);
    if(!op->off_path) {
      STAT_EVENT(op->proc_id,
                 CBR_ON_PATH_CORRECT_PER1000INST + op->oracle_info.mispred);
      if(op->oracle_info.mispred)
        _DEBUGA(op->proc_id, 0, "ON PATH HW MISPRED  addr:0x%s  pghist:0x%s\n",
                hexstr64s(op->inst_info->addr),
                hexstr64s(op->oracle_info.pred_global_hist));
      else
        _DEBUGA(op->proc_id, 0, "ON PATH HW CORRECT  addr:0x%s  pghist:0x%s\n",
                hexstr64s(op->inst_info->addr),
                hexstr64s(op->oracle_info.pred_global_hist));
    }
  }
  // }}}

  DEBUG_BTB(
    bp_data->proc_id,
    "BTB:  op_num:%s  off_path:%d  cf_type:%s  addr:0x%s  btb_miss:%d\n",
    unsstr64(op->op_num), op->off_path, cf_type_names[op->table_info->cf_type],
    hexstr64s(op->inst_info->addr), op->oracle_info.btb_miss);

  DEBUG(bp_data->proc_id,
        "BP:  op_num:%s  off_path:%d  cf_type:%s  addr:%s  p_npc:%s  "
        "t_npc:0x%s  btb_miss:%d  mispred:%d  misfetch:%d  no_tar:%d\n",
        unsstr64(op->op_num), op->off_path,
        cf_type_names[op->table_info->cf_type], hexstr64s(op->inst_info->addr),
        hexstr64s(prediction), hexstr64s(op->oracle_info.npc),
        op->oracle_info.btb_miss, op->oracle_info.mispred,
        op->oracle_info.misfetch, op->oracle_info.no_target);

  if(ENABLE_BP_CONF && IS_CONF_CF(op)) {
    bp_data->br_conf->pred_func(op);

    if(!op->off_path) {
      if(op->oracle_info.pred_conf) {
        if(!op->oracle_info.mispred)
          STAT_EVENT(op->proc_id, BP_ON_PATH_CONF_PVP);
        else
          STAT_EVENT(op->proc_id, BP_ON_PATH_CONF_PVP_BOT);
      } else {
        if(op->oracle_info.mispred)
          STAT_EVENT(op->proc_id, BP_ON_PATH_CONF_PVN);
        else
          STAT_EVENT(op->proc_id, BP_ON_PATH_CONF_PVN_BOT);
      }
      if(op->oracle_info.mispred) {
        if(!op->oracle_info.pred_conf)
          STAT_EVENT(op->proc_id, BP_ON_PATH_CONF_SPEC);
        else
          STAT_EVENT(op->proc_id, BP_ON_PATH_CONF_SPEC_BOT);
      }
    }
    if(!(op->oracle_info.pred_conf))
      td->td_info.low_conf_count++;
    DEBUG(bp_data->proc_id, "low_conf_count:%d \n", td->td_info.low_conf_count);
  }

  return prediction;
}


/******************************************************************************/
/* bp_target_known_op: called on cf ops when the real target is known
   (either decode time or execute time) */

void bp_target_known_op(Bp_Data* bp_data, Op* op) {
  ASSERT(bp_data->proc_id, bp_data->proc_id == op->proc_id);
  ASSERT(bp_data->proc_id, op->table_info->cf_type);

  // if it was a btb miss, it is time to write it into the btb
  if(op->oracle_info.btb_miss)
    bp_data->bp_btb->update_func(bp_data, op);

  // special case updates
  switch(op->table_info->cf_type) {
    case CF_ICALL:  // fall through
    case CF_IBR:
      if(ENABLE_IBP) {
        if(IBTB_OFF_PATH_WRITES || !op->off_path) {
          bp_data->bp_ibtb->update_func(bp_data, op);
        }
      }
      break;
    default:
      break;  // do nothing
  }
}


/******************************************************************************/
/* bp_resolve_op: called on cf ops when they complete in the functional unit */

void bp_resolve_op(Bp_Data* bp_data, Op* op) {
  if(!UPDATE_BP_OFF_PATH && op->off_path) {
    return;
  }
  if (FDIP_ENABLE) {
    fdip_resolve(op);
  }
  bp_data->bp->update_func(op);
  if(USE_LATE_BP) {
    bp_data->late_bp->update_func(op);
  }

  if(ENABLE_BP_CONF && IS_CONF_CF(op)) {
    bp_data->br_conf->update_func(op);
  }
  if(op->oracle_info.misfetch || op->oracle_info.mispred) {
    INC_STAT_EVENT(op->proc_id, BP_MISP_PENALTY,
                   op->exec_cycle - op->issue_cycle);
  }
}


/******************************************************************************/
/* bp_retire_op: called to update critical branch predictor state that should
 * only be updated on the right path and retire the timestamp of the branch.
 */

void bp_retire_op(Bp_Data* bp_data, Op* op) {
  if (FDIP_ENABLE) {
    fdip_retire(op);
  }

  bp_data->bp->retire_func(op);
  if(USE_LATE_BP) {
    bp_data->late_bp->retire_func(op);
  }
}


/******************************************************************************/
/* bp_recover_op: called on the last mispredicted op when the recovery happens
 */

void bp_recover_op(Bp_Data* bp_data, Cf_Type cf_type, Recovery_Info* info) {
  /* always recover the global history */
  if(cf_type == CF_CBR) {
    bp_data->global_hist = (info->pred_global_hist >> 1) |
                           (info->new_dir << 31);
  } else {
    bp_data->global_hist = info->pred_global_hist;
  }
  bp_data->targ_hist = info->targ_hist;

  /* this event counts updates to BP, so it's really branch resolutions */
  STAT_EVENT(bp_data->proc_id, POWER_BRANCH_MISPREDICT);
  STAT_EVENT(bp_data->proc_id, POWER_BTB_WRITE);

  /* type-specific recovery */
  if(cf_type == CF_ICALL || cf_type == CF_IBR) {
    bp_data->bp_ibtb->recover_func(bp_data, info);
  }
  bp_data->bp->recover_func(info);
  if(USE_LATE_BP) {
    bp_data->late_bp->recover_func(info);
  }

  /* always recover the call return stack */
  CRS_REALISTIC ? bp_crs_realistic_recover(bp_data, info) :
                  bp_crs_recover(bp_data);

  if(ENABLE_BP_CONF && bp_data->br_conf->recover_func)
    bp_data->br_conf->recover_func();

  if (FDIP_ENABLE) {
    fdip_recover(info);
  }
}
