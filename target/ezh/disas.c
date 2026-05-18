/*
 * AVR disassembler
 *
 * Copyright (c) 2019-2020 Richard Henderson <rth@twiddle.net>
 * Copyright (c) 2019-2020 Michael Rolnik <mrolnik@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "qemu/osdep.h"
#include "cpu.h"

typedef struct {
    disassemble_info *info;
    uint16_t next_word;
    bool next_word_used;
} DisasContext;

/* Include the auto-generated decoder.  */
static bool decode_insn(DisasContext *ctx, uint32_t insn);
#include "decode-insn.c.inc"

#define output(mnemonic, format, ...) \
    (pctx->info->fprintf_func(pctx->info->stream, "%-9s " format, \
                              mnemonic, ##__VA_ARGS__))

int ezh_print_insn(bfd_vma addr, disassemble_info *info)
{
    DisasContext ctx = { info };
    DisasContext *pctx = &ctx;
    bfd_byte buffer[4];
    uint16_t insn;
    int status;

    status = info->read_memory_func(addr, buffer, 2, info);
    if (status != 0) {
        info->memory_error_func(status, addr, info);
        return -1;
    }
    insn = bfd_getl16(buffer);

    status = info->read_memory_func(addr + 2, buffer + 2, 2, info);
    if (status == 0) {
        ctx.next_word = bfd_getl16(buffer + 2);
    }

    if (!decode_insn(&ctx, insn)) {
        output(".db", "0x%02x, 0x%02x", buffer[0], buffer[1]);
    }

    if (!ctx.next_word_used) {
        return 2;
    } else if (status == 0) {
        return 4;
    }
    info->memory_error_func(status, addr + 2, info);
    return -1;
}


#define INSN(opcode, format, ...)                                       \
static bool trans_##opcode(DisasContext *pctx, arg_##opcode * a)        \
{                                                                       \
    output(#opcode, format, ##__VA_ARGS__);                             \
    return true;                                                        \
}

INSN(E_LOAD_IMM, "r%d, r%d", a->rd, a->imm11s)
INSN(E_NOP, "")
INSN(E_GOSUB, "r%d", a->addr30imm)
INSN(E_LDR, "r%d, r%d, r%d", a->rd, a->rs, a->offset8s)
INSN(E_GOTO, "r%d", a->a21)
INSN(E_BTOG_IMM, "r%d, r%d, r%d", a->rd, a->rs, a->bit5)
INSN(E_BCLR_IMM, "r%d, r%d, r%d", a->rd, a->rs, a->bit5)
INSN(E_BSET_IMM, "r%d, r%d, r%d", a->rd, a->rs, a->bit5)
