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
 * File         : packet_build.c
 * Author       : HPS Research Group
 * Date         : 2/21/2001
 * Description  :
 ***************************************************************************************/

#include "globals/assert.h"
#include "globals/global_defs.h"
#include "globals/global_types.h"
#include "globals/global_vars.h"
#include "globals/utils.h"

#include "icache_stage.h"
#include "uop_cache.h"
#include "op.h"
#include "packet_build.h"

#include "bp/bp.param.h"
#include "prefetcher/fdip.h"
#include "core.param.h"
#include "debug/debug.param.h"
#include "memory/memory.param.h"
#include "packet_build.param.h"
#include "statistics.h"

uns64 timestamp_new_opt_packet;

/**************************************************************************************/
/* Initialize the packet build structures */

void init_packet_build(Pb_Data* pb_data, Packet_Build_Identifier pb_ident) {
  pb_data->pb_ident = pb_ident;  // set the packet build identifier

  if(pb_data->pb_ident == PB_ICACHE) {
    if(PACKET_BREAK_ON_FUS) {
      char *prev, *curr, buf[50];
      uns32 type = 0;
      int   i;

      memset(pb_data->fu_info, 0, sizeof(Fu) * PB_NUM_FU_TYPES);
      prev = curr = PACKET_BREAK_FU_TYPES;
      while(1) {
        while(*curr && *curr != ',')
          curr++;
        strncpy(buf, prev, curr - prev);
        buf[curr - prev] = '\0';

        if(!strcmp(buf, "B")) {
          pb_data->fu_info[PB_FU_B].type |= (OP_CF_BIT);
          pb_data->fu_info[PB_FU_B].max++;
        } else if(!strcmp(buf, "IS")) {
          pb_data->fu_info[PB_FU_IS].type |= (OP_IADD_BIT | OP_ICMP_BIT |
                                              OP_LOGIC_BIT);
          pb_data->fu_info[PB_FU_IS].max++;
        } else if(!strcmp(buf, "IC")) {
          pb_data->fu_info[PB_FU_IC].type |= (OP_IMUL_BIT | OP_SHIFT_BIT);
          pb_data->fu_info[PB_FU_IC].max++;
        } else if(!strcmp(buf, "I")) {
          pb_data->fu_info[PB_FU_I].type |= (OP_IADD_BIT | OP_ICMP_BIT |
                                             OP_LOGIC_BIT | OP_IMUL_BIT |
                                             OP_SHIFT_BIT);
          pb_data->fu_info[PB_FU_I].max++;
        } else if(!strcmp(buf, "FM")) {
          pb_data->fu_info[PB_FU_FM].type |= (OP_FMUL_BIT | OP_FMA_BIT);
          pb_data->fu_info[PB_FU_FM].max++;
        } else if(!strcmp(buf, "FD")) {
          pb_data->fu_info[PB_FU_FD].type |= (OP_FDIV_BIT);
          pb_data->fu_info[PB_FU_FD].max++;
        } else if(!strcmp(buf, "FX")) {
          pb_data->fu_info[PB_FU_FX].type |= (OP_FCVT_BIT | OP_FADD_BIT |
                                              OP_FCMP_BIT | OP_FCMOV_BIT |
                                              OP_FMA_BIT);
          pb_data->fu_info[PB_FU_FX].max++;
        } else if(!strcmp(buf, "F")) {
          pb_data->fu_info[PB_FU_F].type |= (OP_FMUL_BIT | OP_FDIV_BIT |
                                             OP_FCVT_BIT | OP_FADD_BIT |
                                             OP_FCMP_BIT | OP_FCMOV_BIT |
                                             OP_FMA_BIT);
          pb_data->fu_info[PB_FU_F].max++;
        } else if(!strcmp(buf, "MI")) {
          pb_data->fu_info[PB_FU_MI].type |= (OP_IMEM_BIT);
          pb_data->fu_info[PB_FU_MI].max++;
        } else if(!strcmp(buf, "MF")) {
          pb_data->fu_info[PB_FU_MF].type |= (OP_FMEM_BIT);
          pb_data->fu_info[PB_FU_MF].max++;
        } else if(!strcmp(buf, "M")) {
          pb_data->fu_info[PB_FU_M].type |= (OP_IMEM_BIT | OP_FMEM_BIT |
                                             OP_GATHER_BIT | OP_SCATTER_BIT);
          pb_data->fu_info[PB_FU_M].max++;
        } else if(!strcmp(buf, "RB")) {
          pb_data->fu_info[PB_FU_RB].type |= (OP_IADD_BIT | OP_IMEM_BIT |
                                              OP_FMEM_BIT | OP_GATHER_BIT |
                                              OP_SCATTER_BIT |  // OP_CF_BIT |
                                              OP_CMOV_BIT | OP_ICMP_BIT |
                                              OP_LDA_BIT);
          pb_data->fu_info[PB_FU_RB].max++;
        } else if(!strcmp(buf, "INRB")) {
          pb_data->fu_info[PB_FU_INRB].type |= N_BIT_MASK(NUM_OP_TYPES);
          pb_data->fu_info[PB_FU_INRB].type &= ~(
            OP_IADD_BIT | OP_IMEM_BIT | OP_FMEM_BIT |  // OP_CF_BIT |
            OP_CMOV_BIT | OP_ICMP_BIT | OP_LDA_BIT | OP_FMUL_BIT | OP_FDIV_BIT |
            OP_FCVT_BIT | OP_FADD_BIT | OP_FCMP_BIT | OP_FCMOV_BIT);
          pb_data->fu_info[PB_FU_INRB].max++;
        } else if(!strcmp(buf, "G")) {
          pb_data->fu_info[PB_FU_G].type |= N_BIT_MASK(NUM_OP_TYPES);
          pb_data->fu_info[PB_FU_G].max++;
        } else
          FATAL_ERROR(pb_data->proc_id, "Invalid FU type\n");

        if(!*curr)
          break;

        curr++;
        prev = curr;
      }

      for(i = 0; i < PB_NUM_FU_TYPES; i++) {
        if(pb_data->fu_info[i].type)
          type |= pb_data->fu_info[i].type;
      }
      ASSERTM(pb_data->proc_id, type == N_BIT_MASK(NUM_OP_TYPES),
              "Functional units not complete");
    }
  }
}


/**************************************************************************************/
/* Reset_packet_build:  just resets counter values. */

inline void reset_packet_build(Pb_Data* pb_data) {
  if(pb_data->pb_ident == PB_ICACHE) {
    memset(pb_data->break_conditions, 0, sizeof(Counter) * PB_NUM_CONDITIONS);
    if(PACKET_BREAK_ON_FUS) {
      int ii;
      for(ii = 0; ii < PB_NUM_FU_TYPES; ii++)
        pb_data->fu_info[ii].count = 0;
    }
  }
}


/**************************************************************************************/
/* Packet_build:
   return 0 if packet should break after current op
   return 1 if otherwise
*/

Flag packet_build(Pb_Data* pb_data, Break_Reason* break_fetch, Op* const op,
                  Flag uop_cache_issue_ops) {
  Break_Reason model_break_result;

  ASSERT(pb_data->proc_id, pb_data->proc_id == op->proc_id);

  if(pb_data->pb_ident == PB_ICACHE) {
    // only have constraints on the number of loads & stores per packet
    if(NUM_LOAD_STORE_PER_PACKET) {
      pb_data->break_conditions[NUM_LOAD_STORE] += (op->table_info->mem_type !=
                                                    0);
      if(pb_data->break_conditions[NUM_LOAD_STORE] >
         NUM_LOAD_STORE_PER_PACKET) {
        *break_fetch = 100;
        return PB_BREAK_BEFORE;
      }
    }

    // break when no more functional units are available
    if(PACKET_BREAK_ON_FUS) {
      int ii;
      // search for avail fus starting with the most specific FU type
      for(ii = 0; ii < PB_NUM_FU_TYPES; ii++) {
        if((1 << op->table_info->op_type) & pb_data->fu_info[ii].type) {
          if(pb_data->fu_info[ii].count < pb_data->fu_info[ii].max) {
            pb_data->fu_info[ii].count++;
            break;
          } else {
            *break_fetch = 100;
            /* used for tcache */
            return PB_BREAK_BEFORE;
          }
        }
      }
    }

    // this must be called as the last BREAK_BEFORE condition
    model_break_result = model->break_hook ? model->break_hook(op) : BREAK_DONT;
    if(model_break_result) {
      *break_fetch = BREAK_MODEL_BEFORE;
      return PB_BREAK_BEFORE;
    }

    // log UC access. This statement must be reached for each uop fetched.
    // break if in uc fetch mode and cur instr is NOT from UC
    // ideally we should break and we try same cycle to fetch from IC
    if(return_repl_policy() == 1 || ENABLE_SIDE_CACHE){
      add_timestamp_addr(op->inst_info->addr);
    }

    // printf("%lld,\n", op->inst_info->addr); //todo
    /* Yilong Print*/
    // printf("[Packet_build] Lookup uop address %lld\n", op->inst_info->addr);

    /* Here to adjust FOO Core*/
    if(return_repl_policy() == 3) updateFooPW();

    if (!in_uop_cache(op->inst_info->addr, &op->op_num, TRUE, FALSE, op) && uop_cache_issue_ops) {
      STAT_EVENT(0, UOP_CACHE_MISS);
      *break_fetch = BREAK_UC_MISS;
      op->fetched_from_uop_cache = FALSE;
      // Inaccuracy: should break before op. But how does this op get refetched from frontend?
      // by breaking after I always fetch one extra op per PW from UC
      Flag frontend_resteer = op->oracle_info.mispred || op->oracle_info.btb_miss || op->oracle_info.misfetch;
      // UOC to IC switches can happen at any branch, whether or not the branch causes a frontend resteer.
      // The reason we distinguish between correct and incorrect predictions is that FDIP should be able
      // to prefetch correct predictions. Incorrect predictions require other techniques.
      // FDIP cannot prefetch ahead of time on neither a misprediction nor BTB miss nor misfetch.
      STAT_EVENT(ic->proc_id, UOP_CACHE_ICACHE_SWITCH_BR_NOT_TAKEN_RESTEERED
                 + 2 * op->oracle_info.dir + !frontend_resteer);
      return PB_BREAK_AFTER;
    } else if (uop_cache_issue_ops) {
      if (return_repl_policy() == 2 && return_feature(7) == 7){
        write_countcacheaccess_switch(op->inst_info->addr, 1);
      }
      op->fetched_from_uop_cache = TRUE;
      STAT_EVENT(0, UOP_CACHE_HIT);
    }else{
      STAT_EVENT(0, UOP_CACHE_MISS);
      op->fetched_from_uop_cache = FALSE;
    }

    if (fdip_is_max_op(op)) {
      *break_fetch = BREAK_FDIP_RUNAHEAD;
      return PB_BREAK_AFTER;
    }

    // hit fetch barrier
    if(IS_CALLSYS(op->table_info) || op->table_info->bar_type & BAR_FETCH) {
      *break_fetch = BREAK_BARRIER;
      return PB_BREAK_AFTER;
    }

    if(ENABLE_ICACHE_PACKET_BREAKING) {
      // reached a max number of control flow instructions
      pb_data->break_conditions[NUM_CF] += (op->table_info->cf_type != 0);
      if(pb_data->break_conditions[NUM_CF] == CFS_PER_CYCLE) {
        *break_fetch = BREAK_CF;
        return PB_BREAK_AFTER;
      }

      // fetch across cache lines
      if(pb_data->pb_ident == PB_ICACHE  && !FETCH_ACROSS_CACHE_LINES) {
        if(op->eom == 1){ // Only process the last op in one Macro Instruction
          Addr nextAddr = ADDR_PLUS_OFFSET(op->inst_info->addr, op->inst_info->trace_info.inst_size);
          if(ROUND_DOWN(op->inst_info->addr, ICACHE_LINE_SIZE) !=
             ROUND_DOWN(nextAddr, ICACHE_LINE_SIZE)) {
            *break_fetch = BREAK_LINE_END;
            return PB_BREAK_AFTER;
          }
        }
      }
    }

    // issue width reached
    const int issue_width = uop_cache_issue_ops ? UC_ISSUE_WIDTH : IC_ISSUE_WIDTH;
    if(pb_data->pb_ident == PB_ICACHE) {
      if(ic->sd.op_count + 1 == issue_width) {
        *break_fetch = BREAK_ISSUE_WIDTH;
        return PB_BREAK_AFTER;
      }
    }

    if(ENABLE_ICACHE_PACKET_BREAKING) {
      // control flow instruction
      if(op->table_info->cf_type)
        return PB_BREAK_AFTER;
    }
  }

  return PB_BREAK_DONT;
}
