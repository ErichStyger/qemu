/*
 * QEMU EZH CPU
 *
 * Copyright (c) 2019-2020 Michael Rolnik
 * Copyright (c) 2026 Stefano Nicora
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see
 * <http://www.gnu.org/licenses/lgpl-2.1.html>
 */

#include "qemu/osdep.h"
#include "qemu/qemu-print.h"
#include "tcg/tcg.h"
#include "cpu.h"
#include "exec/translation-block.h"
#include "tcg/tcg-op.h"
#include "exec/helper-proto.h"
#include "exec/helper-gen.h"
#include "exec/log.h"
#include "exec/translator.h"
#include "exec/target_page.h"

#define HELPER_H "helper.h"
#include "exec/helper-info.c.inc"
#undef  HELPER_H

#define DEBUG (1)

#define REG(x) (cpu_r[x])

static TCGv_i32 cpu_r[NUMBER_OF_CPU_REGISTERS];
static TCGv_i32 cpu_skip;

#define EZH_R0              0x00
#define EZH_R1              0x01
#define EZH_R2              0x02
#define EZH_R3              0x03
#define EZH_R4              0x04
#define EZH_R5              0x05
#define EZH_R6              0x06
#define EZH_R7              0x07
#define EZH_GPO				0x08
#define EZH_GPD      		0x09
#define EZH_CFS			    0x0a
#define EZH_CFM	            0x0b
#define EZH_SP              0x0c
#define EZH_PC              0x0d
#define EZH_GPI             0x0e
#define EZH_RA              0x0f
#define EZH_EU				0x0
#define EZH_ZE				0x1
#define EZH_NZ				0x2
#define EZH_PO				0x3
#define EZH_NE				0x4
#define EZH_AZ				0x5
#define EZH_ZB				0x6
#define EZH_CA				0x7
#define EZH_NC				0x8
#define EZH_CZ				0x9
#define EZH_SPO			    0xa
#define EZH_SNE			    0xb
#define EZH_NBS			    0xc
#define EZH_NEX			    0xd
#define EZH_BS				0xe
#define EZH_EX				0xf
#define EZH_UNS			    0xa
#define EZH_NZS			    0xb

/* flags representing CPU and ALU states */
static TCGv_i32 alu_EU; /* execute unconditionally | <NONE> in ARM equivalent */
static TCGv_i32 alu_ZE; /* zero                    | EQ in ARM equivalent */
static TCGv_i32 alu_PO; /* positive                | PL/GE in ARM equivalent */
static TCGv_i32 alu_NE; /* negative                | MI in ARM equivalent */
static TCGv_i32 alu_AZ; /* above zero              | GT in ARM equivalent */
static TCGv_i32 alu_ZB; /* zero or below           | LE in ARM equivalent */
static TCGv_i32 alu_CA; /* carry set               | CS in ARM equivalent */
static TCGv_i32 alu_NC; /* carry not set           | CC in ARM equivalent */
static TCGv_i32 alu_CZ; /* carry set and zero      | <NONE> in ARM equivalent */

/* representing cpu registers; additionally to r0..r7 */
static TCGv_i32 cpu_GPO;    /* GPIO output */
static TCGv_i32 cpu_GPD;    /* GPIO direction */
static TCGv_i32 cpu_GPI;    /* GPIO input */
static TCGv_i32 cpu_CFS;    /* Bit-slice source configuration */
static TCGv_i32 cpu_CFM;    /* Bit slice event configuration */
static TCGv_i32 cpu_SP;     /* Stack pointer */
static TCGv_i32 cpu_PC;     /* Program counter */
static TCGv_i32 cpu_RA;     /* Return address (== LR in ARM cores) */
static TCGv_i32 cpu_IC;     /* Instruction counter (virtual register) */

static const char reg_names[NUMBER_OF_CPU_REGISTERS][32] = {
    "r0",  "r1",  "r2",  "r3",  "r4",  "r5",  "r6",  "r7",
};

#define DISAS_EXIT   DISAS_TARGET_0  /* We want return to the cpu main loop.  */
#define DISAS_LOOKUP DISAS_TARGET_1  /* We have a variable condition exit.  */
#define DISAS_CHAIN  DISAS_TARGET_2  /* We have a single condition exit.  */

/* This is the state at translation time. */
typedef struct DisasContext {
    DisasContextBase base;

    CPUEZHState *env;   /* custom type */
    CPUState *cs;       /* QEMU-internal type*/

    target_long npc;    /* next program counter (pc) */
    uint32_t opcode;

    TCGv_i32 skip_var0;
    TCGv_i32 skip_var1;
    TCGCond skip_cond;
} DisasContext;

void ezh_cpu_tcg_init(void)
{
#define EZH_REG_OFFS(x) offsetof(CPUEZHState, x)
    cpu_GPO = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_GPO), "GPO");
    cpu_GPD = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_GPD), "GPD");
    cpu_GPI = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_GPI), "GPI");
    cpu_CFS = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_CFS), "CFS");
    cpu_CFM = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_CFM), "CFM");
    cpu_SP = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_SP), "SP");
    cpu_PC = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_PC), "PC");
    cpu_RA = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_RA), "RA");
    cpu_IC = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_cpu_IC), "IC");
    alu_EU = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_EU), "EU");
    alu_ZE = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_ZE), "ZE");
    alu_PO = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_PO), "PO");
    alu_NE = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_NE), "NE");
    alu_AZ = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_AZ), "AZ");
    alu_ZB = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_ZB), "ZB");
    alu_CA = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_CA), "CA");
    alu_NC = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_NC), "NC");
    alu_CZ = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(s_alu_CZ), "CZ");
    cpu_skip = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(skip), "skip");

    for (uint8_t i = 0; i < NUMBER_OF_CPU_REGISTERS; i++) {
        cpu_r[i] = tcg_global_mem_new_i32(tcg_env, EZH_REG_OFFS(r[i]),
                                          reg_names[i]);
    }
#undef AVR_REG_OFFS
}

static uint32_t next_insn(DisasContext *ctx)
{
    /* todo: change back to LE after testing */
    uint32_t insn = translator_ldl_end(ctx->env, &ctx->base, ctx->npc, MO_BE);
    ctx->npc += 4;

    return insn;
}

static bool decode_insn(DisasContext *ctx, uint32_t insn);
#include "decode-insn.c.inc"

static void gen_goto_tb(DisasContext *ctx, unsigned tb_slot_idx,
                        target_ulong dest)
{
    const TranslationBlock *tb = ctx->base.tb;

    if (translator_use_goto_tb(&ctx->base, dest)) {
        tcg_gen_goto_tb(tb_slot_idx);
        tcg_gen_movi_i32(cpu_PC, dest);
        tcg_gen_exit_tb(tb, tb_slot_idx);
    } else {
        tcg_gen_movi_i32(cpu_PC, dest);
        tcg_gen_lookup_and_goto_ptr();
    }
    ctx->base.is_jmp = DISAS_NORETURN;
}

/*
 * Read data from chip into EZH register (using immediate offset)
 */
static bool trans_E_LDR(DisasContext *ctx, arg_E_LDR *a)
{
    #if DEBUG == 1
    qemu_log("\ntrans_E_LDR called");
    qemu_log("\nrs:       0b%b", a->rs);
    qemu_log("\nrd:       0b%b", a->rd);
    qemu_log("\noffset8s: 0b%b", a->offset8s);
    #endif

    TCGv_i32 Rs = cpu_r[a->rs];
    TCGv_i32 Rd = cpu_r[a->rd];
    TCGv_i32 Rr = tcg_constant_i32(a->offset8s);

    /* update output registers */
    tcg_gen_add_i32(Rs, Rs, Rr);
    tcg_gen_qemu_ld_i32(Rd, Rs, 0, MO_UL);
    ctx->env->s_cpu_IC += 1;
    return true;
}

/*
 * Move immediate data to a register
 */
static bool trans_E_LOAD_IMM(DisasContext *ctx, arg_E_LOAD_IMM *a)
{
    #if DEBUG == 1
    qemu_log("\ntrans_E_LOAD_IMM called");
    qemu_log("\nrd:     0b%b", a->rd);
    qemu_log("\nimm11s: 0b%b", a->imm11s);
    #endif

    TCGv_i32 Rd = cpu_r[a->rd];
    TCGv_i32 Rr = tcg_constant_i32(a->imm11s);

    /* update output registers */
    tcg_gen_mov_tl(Rd, Rr);
    ctx->env->s_cpu_IC += 1;
    return true;
}

/* 
 * Used to branch to an immediate 32-bit address location. 
 * The RA register is updated with the return address to 
 * allow stacking and returning.
 */
static bool trans_E_GOSUB(DisasContext *ctx, arg_E_GOSUB *a)
{
    //todo
    #if DEBUG == 1
    qemu_log("\ntrans_E_GOSUB called");
    qemu_log("\naddr30imm: 0b%b", a->addr30imm);
    #endif
    gen_goto_tb(ctx, 0, a->addr30imm);
    ctx->env->s_cpu_IC += 1;
    return true;
}


/* 
 * Sets the requested bit in register to high
 */
static bool trans_E_BSET_IMM(DisasContext *ctx, arg_E_BSET_IMM *a)
{
    #if DEBUG == 1
    qemu_log("\ntrans_E_BSET_IMM called");
    qemu_log("\nrd:   0b%b", a->rd);
    qemu_log("\nplaceholder"); /* for some reason without this placeholder rd doesn't get printed. needs to be investigated */
    qemu_log("\rdata: 0b%b", a->rs);
    qemu_log("\nbit5: 0b%b", a->bit5);
    #endif

    TCGv_i32 one = tcg_constant_i32(1);
    TCGv_i32 shift = tcg_constant_i32(a->bit5);
    TCGv_i32 temp = tcg_temp_new_i32();
    tcg_gen_shl_i32(temp, one, shift);

    /* check to which register was requested */
    switch (a->rd){
    case EZH_GPO:
        tcg_gen_or_i32(cpu_GPO, cpu_GPO, temp);
        break;
    case EZH_GPI:
        tcg_gen_or_i32(cpu_GPI, cpu_GPI, temp);
        break;
    case EZH_GPD:
        tcg_gen_or_i32(cpu_GPD, cpu_GPD, temp);
        break;
    default:
        qemu_log("\ncould not identify which register to write to");
        break;
    }
    ctx->env->s_cpu_IC += 1;
    return true;
}

/* 
 * Clears the requested GPIO-Pin
 */
static bool trans_E_BCLR_IMM(DisasContext *ctx, arg_E_BCLR_IMM *a)
{
    #if DEBUG == 1
    qemu_log("\ntrans_E_BCLR_IMM called");
    qemu_log("\nrd:   0b%b", a->rd);
    qemu_log("\rdata: 0b%b", a->rs);
    qemu_log("\nbit5: 0b%b", a->bit5);
    #endif

    TCGv_i32 one = tcg_constant_i32(1);
    TCGv_i32 shift = tcg_constant_i32(a->bit5);
    TCGv_i32 temp = tcg_temp_new_i32();
    tcg_gen_shl_i32(temp, one, shift);

    /* check to which register was requested */
    switch (a->rd){
    case EZH_GPO:
        tcg_gen_xori_i32(temp, temp, 0xffffffff); /* invert number */
        tcg_gen_and_i32(cpu_GPO, cpu_GPO, temp);
        break;
    case EZH_GPI:
        tcg_gen_xori_i32(temp, temp, 0xffffffff); /* invert number */
        tcg_gen_and_i32(cpu_GPI, cpu_GPI, temp);
        break;
    case EZH_GPD:
        tcg_gen_xori_i32(temp, temp, 0xffffffff); /* invert number */
        tcg_gen_and_i32(cpu_GPD, cpu_GPD, temp);
        break;
    default:
        qemu_log("\ncould not identify which register to write to");
        break;
    }
    ctx->env->s_cpu_IC += 1;
    return true;
}

/* 
 * Toggles the requested GPIO-Pin from high -> low or from low -> high
 */
static bool trans_E_BTOG_IMM(DisasContext *ctx, arg_E_BTOG_IMM *a)
{
    #if DEBUG == 1
    qemu_log("\ntrans_E_BTOG_IMM called");
    qemu_log("\nrd:   0b%b", a->rd);
    qemu_log("\rdata: 0b%b", a->rs);
    qemu_log("\nbit5: 0b%b", a->bit5);
    #endif

    TCGv_i32 one = tcg_constant_i32(1);
    TCGv_i32 shift = tcg_constant_i32(a->bit5);
    TCGv_i32 temp = tcg_temp_new_i32();
    tcg_gen_shl_i32(temp, one, shift);

    /* check which register was requested */
    switch (a->rd){
    case EZH_GPO:
        tcg_gen_xor_i32(cpu_GPO, cpu_GPO, temp);
        break;
    case EZH_GPI:
        tcg_gen_xor_i32(cpu_GPI, cpu_GPI, temp);
        break;
    case EZH_GPD:
        tcg_gen_xor_i32(cpu_GPD, cpu_GPD, temp);
        break;
    default:
        qemu_log("\ncould not identify which register to write to");
        break;
    }

    ctx->env->s_cpu_IC += 1;
    return true;
}

/* 
 * Performs no operation
 */
static bool trans_E_NOP(DisasContext *ctx, arg_E_NOP *a)
{
    #if DEBUG == 1
    qemu_log("\ntrans_E_NOP called");
    #endif
    ctx->env->s_cpu_IC += 1;
    return true;
}

/* 
 * Used for branching using immediate addressing
 */
static bool trans_E_GOTO(DisasContext *ctx, arg_E_GOTO *a)
{
    // todo
    // gen_goto_tb(ctx, 0, a->imm);
    ctx->env->s_cpu_IC += 1;
    return true;
}

static void translate(DisasContext *ctx)
{
    uint32_t opcode = next_insn(ctx);
    qemu_log("opcode = 0x%08x\n", opcode);

    if (!decode_insn(ctx, opcode)) {
        gen_helper_unsupported(tcg_env);
        ctx->base.is_jmp = DISAS_NORETURN;
    }
}

/* Standardize the cpu_skip condition to NE.  */
static bool canonicalize_skip(DisasContext *ctx)
{
    switch (ctx->skip_cond) {
    case TCG_COND_NEVER:
        /* Normal case: cpu_skip is known to be false.  */
        return false;

    case TCG_COND_ALWAYS:
        /*
         * Breakpoint case: cpu_skip is known to be true, via TB_FLAGS_SKIP.
         * The breakpoint is on the instruction being skipped, at the start
         * of the TranslationBlock.  No need to update.
         */
        return false;

    case TCG_COND_NE:
        if (ctx->skip_var1 == NULL) {
            tcg_gen_mov_i32(cpu_skip, ctx->skip_var0);
        } else {
            tcg_gen_xor_i32(cpu_skip, ctx->skip_var0, ctx->skip_var1);
            ctx->skip_var1 = NULL;
        }
        break;

    default:
        /* Convert to a NE condition vs 0. */
        if (ctx->skip_var1 == NULL) {
            tcg_gen_setcondi_i32(ctx->skip_cond, cpu_skip, ctx->skip_var0, 0);
        } else {
            tcg_gen_setcond_i32(ctx->skip_cond, cpu_skip,
                               ctx->skip_var0, ctx->skip_var1);
            ctx->skip_var1 = NULL;
        }
        ctx->skip_cond = TCG_COND_NE;
        break;
    }
    ctx->skip_var0 = cpu_skip;
    return true;
}

static void ezh_tr_init_disas_context(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    uint32_t tb_flags = ctx->base.tb->flags;

    ctx->cs = cs;
    ctx->env = cpu_env(cs);
    ctx->npc = ctx->base.pc_first;

    ctx->skip_cond = TCG_COND_NEVER;
    if (tb_flags & TB_FLAGS_SKIP) {
        ctx->skip_cond = TCG_COND_ALWAYS;
        ctx->skip_var0 = cpu_skip;
    }

    if (tb_flags & TB_FLAGS_FULL_ACCESS) {
        /*
         * This flag is set by ST/LD instruction we will regenerate it ONLY
         * with mem/cpu memory access instead of mem access
         */
        ctx->base.max_insns = 1;
    }
}

static void ezh_tr_tb_start(DisasContextBase *db, CPUState *cs)
{
}

static void ezh_tr_insn_start(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);

    tcg_gen_insn_start(ctx->npc, 0, 0);
}

static void ezh_tr_translate_insn(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    TCGLabel *skip_label = NULL;

    /* Conditionally skip the next instruction, if indicated.  */
    if (ctx->skip_cond != TCG_COND_NEVER) {
        skip_label = gen_new_label();
        if (ctx->skip_var0 == cpu_skip) {
            /*
             * Copy cpu_skip so that we may zero it before the branch.
             * This ensures that cpu_skip is non-zero after the label
             * if and only if the skipped insn itself sets a skip.
             */
            ctx->skip_var0 = tcg_temp_new();
            tcg_gen_mov_i32(ctx->skip_var0, cpu_skip);
            tcg_gen_movi_i32(cpu_skip, 0);
        }
        if (ctx->skip_var1 == NULL) {
            tcg_gen_brcondi_i32(ctx->skip_cond, ctx->skip_var0, 0, skip_label);
        } else {
            tcg_gen_brcond_i32(ctx->skip_cond, ctx->skip_var0,
                              ctx->skip_var1, skip_label);
            ctx->skip_var1 = NULL;
        }
        ctx->skip_cond = TCG_COND_NEVER;
        ctx->skip_var0 = NULL;
    }

    translate(ctx);
    ctx->base.is_jmp = DISAS_TOO_MANY;

    ctx->base.pc_next = ctx->npc;

    if (skip_label) {
        canonicalize_skip(ctx);
        gen_set_label(skip_label);

        switch (ctx->base.is_jmp) {
        case DISAS_NORETURN:
            ctx->base.is_jmp = DISAS_CHAIN;
            break;
        case DISAS_NEXT:
            if (ctx->base.tb->flags & TB_FLAGS_SKIP) {
                ctx->base.is_jmp = DISAS_TOO_MANY;
            }
            break;
        default:
            break;
        }
    }

    if (ctx->base.is_jmp == DISAS_NEXT) {
        target_ulong page_first = ctx->base.pc_first & TARGET_PAGE_MASK;

        if ((ctx->base.pc_next - page_first) >= TARGET_PAGE_SIZE - 4) {
            ctx->base.is_jmp = DISAS_TOO_MANY;
        }
    }
}

static void ezh_tr_tb_stop(DisasContextBase *dcbase, CPUState *cs)
{
    DisasContext *ctx = container_of(dcbase, DisasContext, base);
    bool nonconst_skip = canonicalize_skip(ctx);
    /*
     * Because we disable interrupts while env->skip is set,
     * we must return to the main loop to re-evaluate afterward.
     */
    bool force_exit = ctx->base.tb->flags & TB_FLAGS_SKIP;

    switch (ctx->base.is_jmp) {
    case DISAS_NORETURN:
        assert(!nonconst_skip);
        break;
    case DISAS_NEXT:
    case DISAS_TOO_MANY:
    case DISAS_CHAIN:
        if (!nonconst_skip && !force_exit) {
            /* Note gen_goto_tb checks singlestep.  */
            gen_goto_tb(ctx, 1, ctx->npc);
            break;
        }
        tcg_gen_movi_i32(cpu_PC, ctx->npc);
        /* fall through */
    case DISAS_LOOKUP:
        if (!force_exit) {
            tcg_gen_lookup_and_goto_ptr();
            break;
        }
        /* fall through */
    case DISAS_EXIT:
        tcg_gen_exit_tb(NULL, 0);
        break;
    default:
        g_assert_not_reached();
    }
}

static const TranslatorOps ezh_tr_ops = {
    .init_disas_context = ezh_tr_init_disas_context,
    .tb_start           = ezh_tr_tb_start,
    .insn_start         = ezh_tr_insn_start,
    .translate_insn     = ezh_tr_translate_insn,
    .tb_stop            = ezh_tr_tb_stop,
};

void ezh_cpu_translate_code(CPUState *cs, TranslationBlock *tb,
                            int *max_insns, vaddr pc, void *host_pc)
{
    DisasContext dc = { };
    translator_loop(cs, tb, max_insns, pc, host_pc, &ezh_tr_ops, &dc.base, TCG_TYPE_I32);
}
