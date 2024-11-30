/***************************************************************************************
 * File         : uop_cache.h
 * Author       : Peter Braun
 * Date         : 10.28.2020
 * Description  : Interface for interacting with uop cache object.
 *                  Following Kotra et. al.'s MICRO 2020 description of uop cache baseline
 *                  Instr comes from icache. Theoretically
 *                  we have higher BW fetching direct with uop cache
 ***************************************************************************************/

#ifndef __UOP_CACHE_H__
#define __UOP_CACHE_H__

#include "op.h"
#include "icache_stage.h" //needed for get_pw_lookahead_buffer

/**************************************************************************************/
/* Types */


/**************************************************************************************/
/* Prototypes */

// only one instance of uop cache

void init_uop_cache(void);

/* return whether the instr pc is cached (this does not consider that the whole PW 
    could already have been fetched, potentially introducing 1 incorrect cycle of latency)*/
Flag in_uop_cache(Addr pc, const Counter* op_num, Flag update_repl, Flag record_access); 

void end_accumulate(void);
/* accumulate uop into buffer. If terminating condition reached, call insert_uop_cache */
void accumulate_op(Op* op);

Flag uop_cache_fill_prefetch(Addr pw_start_addr, Flag on_path);
Flag uop_cache_issue_prefetch(Addr pw_start_addr, Flag on_path);

#endif /* #ifndef __UOP_CACHE_H__ */
