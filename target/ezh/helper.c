/*
 * QEMU EZH CPU helpers
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
#include "qemu/log.h"
#include "qemu/error-report.h"
#include "cpu.h"
#include "accel/tcg/cpu-ops.h"
#include "exec/cputlb.h"
#include "exec/page-protection.h"
#include "exec/target_page.h"
#include "accel/tcg/cpu-ldst.h"
#include "exec/helper-proto.h"
#include "qemu/plugin.h"

#define EXCP_RESET 1
#define EXCP_INT(n) (EXCP_RESET + (n) + 1)

bool ezh_cpu_exec_interrupt(CPUState *cs, int interrupt_request)
{
    EZHCPUState *env = cpu_env(cs);

    /*
     * We cannot separate a skip from the next instruction,
     * as the skip would not be preserved across the interrupt.
     * Separating the two insn normally only happens at page boundaries.
     */
    if (env->skip) {
        return false;
    }

    if (interrupt_request & CPU_INTERRUPT_RESET) {
        if (cpu_interrupts_enabled(env)) {
            cs->exception_index = EXCP_RESET;
            ezh_cpu_do_interrupt(cs);

            cpu_reset_interrupt(cs, CPU_INTERRUPT_RESET);
            return true;
        }
    }
    if (interrupt_request & CPU_INTERRUPT_HARD) {
        if (cpu_interrupts_enabled(env) && env->intsrc != 0) {
            int index = ctz64(env->intsrc);
            cs->exception_index = EXCP_INT(index);
            ezh_cpu_do_interrupt(cs);

            env->intsrc &= env->intsrc - 1; /* clear the interrupt */
            if (!env->intsrc) {
                cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
            }
            return true;
        }
    }
    return false;
}

void ezh_cpu_do_interrupt(CPUState *cs)
{
    EZHCPUState *env = cpu_env(cs);
    uint32_t ret = env->s_cpu_PC;
    int vector = 0;
    int base = 0;

    if (cs->exception_index == EXCP_RESET) {
        vector = 0;
    } else if (env->intsrc != 0) {
        vector = ctz64(env->intsrc) + 1;
    }

    cpu_stb_mmuidx_ra(env, env->s_cpu_SP--, ret, 0, 0);

    env->s_cpu_PC = base + vector;

    cs->exception_index = -1;

    qemu_plugin_vcpu_interrupt_cb(cs, ret);
}

hwaddr ezh_cpu_get_phys_page_debug(CPUState *cs, vaddr addr)
{
    return addr; /* I assume 1:1 address correspondence */
}

bool ezh_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr)
{
    int prot;
    uint32_t paddr;

    address &= TARGET_PAGE_MASK;

    paddr = address;
    prot = PAGE_READ | PAGE_WRITE;

    tlb_set_page(cs, address, paddr, prot, mmu_idx, TARGET_PAGE_SIZE);
    return true;
}

void helper_unsupported(EZHCPUState *env)
{
    CPUState *cs = env_cpu(env);

    /*
     *  I count not find what happens on the real platform, so
     *  it's EXCP_DEBUG for meanwhile
     */
    cs->exception_index = EXCP_DEBUG;
    if (qemu_loglevel_mask(LOG_UNIMP)) {
        qemu_log("UNSUPPORTED\n");
        cpu_dump_state(cs, stderr, 0);
    }
    cpu_loop_exit(cs);
}

/*
 * The first 32 bytes of the data space are mapped to the cpu regs.
 * We cannot write these from normal store operations because TCG
 * does not expect global temps to be modified -- a global may be
 * live in a host cpu register across the store.  We can however
 * read these, as TCG does make sure the global temps are saved
 * in case the load operation traps.
 */

static uint64_t ezh_cpu_reg_read(void *opaque, hwaddr addr, unsigned size)
{
    EZHCPUState *env = opaque;

    assert(addr < 32);
    return env->r[addr];
}

static void ezh_cpu_trap_write(void *opaque, hwaddr addr,
                               uint64_t data64, unsigned size)
{
    EZHCPUState *env = opaque;
    CPUState *cs = env_cpu(env);

    env->fullacc = true;
    cpu_loop_exit_restore(cs, cs->mem_io_pc);
}

const MemoryRegionOps ezh_cpu_reg = {
    .read = ezh_cpu_reg_read,
    .write = ezh_cpu_trap_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
    .valid.min_access_size = 1,
    .valid.max_access_size = 1,
};
