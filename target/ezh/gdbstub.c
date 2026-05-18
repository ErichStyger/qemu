/*
 * QEMU AVR gdbstub
 *
 * Copyright (c) 2016-2020 Michael Rolnik
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
#include "gdbstub/helpers.h"
#include "cpu.h"

int ezh_cpu_gdb_read_register(CPUState *cs, GByteArray *mem_buf, int n)
{
    CPUEZHState *env = cpu_env(cs);

    if (n < 8) {
        return gdb_get_reg32(mem_buf, env->r[n]);
    } else if ( n == 8){
        return gdb_get_reg32(mem_buf, env->s_cpu_GPO);
    } else if ( n == 9){
        return gdb_get_reg32(mem_buf, env->s_cpu_GPD);
    } else if ( n == 10){
        return gdb_get_reg32(mem_buf, env->s_cpu_GPI);
    } else if ( n == 11){
        return gdb_get_reg32(mem_buf, env->s_cpu_CFS);
    } else if ( n == 12){
        return gdb_get_reg32(mem_buf, env->s_cpu_CFM);
    } else if ( n == 13){
        return gdb_get_reg32(mem_buf, env->s_cpu_SP);
    } else if ( n == 14){
        return gdb_get_reg32(mem_buf, env->s_cpu_PC);
    } else if ( n == 15){
        return gdb_get_reg32(mem_buf, env->s_cpu_RA);
    } else if ( n == 16){
        return gdb_get_reg32(mem_buf, env->s_cpu_IC);
    }
    return 0;
}

int ezh_cpu_gdb_write_register(CPUState *cs, uint8_t *mem_buf, int n)
{
    CPUEZHState *env = cpu_env(cs);

    if (n < 8) {
        env->r[n] = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 8){
        env->s_cpu_GPO = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 9){
        env->s_cpu_GPD = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 10){
        env->s_cpu_GPI = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 11){
        env->s_cpu_CFS = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 12){
        env->s_cpu_CFM = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 13){
        env->s_cpu_SP = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 14){
        env->s_cpu_PC = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 15){
        env->s_cpu_RA = ldl_be_p(mem_buf);
        return 1;
    } else if ( n == 16){
        env->s_cpu_IC = ldl_be_p(mem_buf);
        return 1;
    }

    return 0;
}

vaddr ezh_cpu_gdb_adjust_breakpoint(CPUState *cpu, vaddr addr)
{
    /*
     * This is due to some strange GDB behavior
     * Let's assume main has address 0x100:
     * b main   - sets breakpoint at address 0x00000100 (code)
     * b *0x100 - sets breakpoint at address 0x00800100 (data)
     *
     * Force all breakpoints into code space.
     */
    // return addr % OFFSET_DATA;
    return addr;
}
