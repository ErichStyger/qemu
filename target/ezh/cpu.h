/*
 * QEMU EZH CPU
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

#ifndef QEMU_EZH_CPU_H
#define QEMU_EZH_CPU_H

#include "cpu-qom.h"
#include "exec/cpu-common.h"
#include "exec/cpu-defs.h"
#include "exec/cpu-interrupt.h"
#include "system/memory.h"

#define CPU_RESOLVING_TYPE TYPE_EZH_CPU

/*
 * AVR has two memory spaces, data & code.
 * e.g. both have 0 address
 * ST/LD instructions access data space
 * LPM/SPM and instruction fetching access code memory space
 */
#define MMU_CODE_IDX 0
#define MMU_DATA_IDX 1

#define EXCP_RESET 1
#define EXCP_INT(n) (EXCP_RESET + (n) + 1)

/* Number of CPU scratch registers */
#define NUMBER_OF_CPU_REGISTERS (8)

/* Flash program memory */
#define OFFSET_CODE               (0x00000000)
/* CPU registers, IO registers, and SRAM */
#define OFFSET_DATA               (0x00000001)

/* EZH controller base address (per-part variant) 
 * LPC5410x:                      (0x4004C000)
 * LPC54114/LPC51U68:             (0x4001D000)
 * LPC55(S)6x/LPC55(S)2x:         (0x4001D000)
 * LPC55(S)3x:                    (0x4001D000)
 * IMXRT500:                      (0x40027000)
 * MCXNx4x/MCXN23x:               (0x40033000)
 */
#define EZH_BASE_ADDR             (0x40033000)

#define EZHB_BOOT_ADDR            (EZH_BASE_ADDR + 0x20)
#define EZHB_CTRL_ADDR            (EZH_BASE_ADDR + 0x24)
#define EZHB_PC_ADDR              (EZH_BASE_ADDR + 0x28)
#define EZHB_SP_ADDR              (EZH_BASE_ADDR + 0x2C)
#define EZHB_BREAK_ADDR_ADDR      (EZH_BASE_ADDR + 0x30)
#define EZHB_BREAK_VECT_ADDR      (EZH_BASE_ADDR + 0x34)
#define EZHB_EMER_VECT_ADDR       (EZH_BASE_ADDR + 0x38)
#define EZHB_EMER_SEL_ADDR        (EZH_BASE_ADDR + 0x3C)
#define EZHB_ARM2EZH_ADDR         (EZH_BASE_ADDR + 0x40)
#define EZHB_EZH2ARM_ADDR         (EZH_BASE_ADDR + 0x44)
#define EZHB_PENDTRAP_ADDR        (EZH_BASE_ADDR + 0x48)

typedef struct CPUArchState {
    uint32_t s_cpu_GPO;
    uint32_t s_cpu_GPD;
    uint32_t s_cpu_GPI;
    uint32_t s_cpu_CFS;
    uint32_t s_cpu_CFM;
    uint32_t s_cpu_SP;
    uint32_t s_cpu_PC;
    uint32_t s_cpu_RA;
    uint32_t s_cpu_IC;
    
    uint32_t s_alu_EU;
    uint32_t s_alu_ZE;
    uint32_t s_alu_PO;
    uint32_t s_alu_NE;
    uint32_t s_alu_AZ;
    uint32_t s_alu_ZB;
    uint32_t s_alu_CA;
    uint32_t s_alu_NC;
    uint32_t s_alu_CZ;

    uint32_t r[NUMBER_OF_CPU_REGISTERS]; /* 32 bits each */

    uint32_t skip; /* if set skip instruction */

    uint64_t intsrc; /* interrupt sources */
    bool fullacc; /* CPU/MEM if true MEM only otherwise */

    uint64_t features;
} CPUEZHState;

/**
 *  EZHCPU:
 *  @env: #CPUEZHState
 */
struct ArchCPU {
    CPUState parent_obj;

    CPUEZHState env;

    MemoryRegion cpu_reg1;

    /* Initial value of stack pointer */
    uint32_t init_sp;
};

/**
 *  EZHCPUClass:
 *  @parent_realize: The parent class' realize handler.
 *  @parent_phases: The parent class' reset phase handlers.
 */
struct EZHCPUClass {
    CPUClass parent_class;

    DeviceRealize parent_realize;
    ResettablePhases parent_phases;
};

extern const struct VMStateDescription vms_ezh_cpu;

void ezh_cpu_do_interrupt(CPUState *cpu);
bool ezh_cpu_exec_interrupt(CPUState *cpu, int int_req);
hwaddr ezh_cpu_get_phys_page_debug(CPUState *cpu, vaddr addr);
int ezh_cpu_gdb_read_register(CPUState *cpu, GByteArray *buf, int reg);
int ezh_cpu_gdb_write_register(CPUState *cpu, uint8_t *buf, int reg);
int ezh_print_insn(bfd_vma addr, disassemble_info *info);
vaddr ezh_cpu_gdb_adjust_breakpoint(CPUState *cpu, vaddr addr);

void ezh_cpu_tcg_init(void);
void ezh_cpu_translate_code(CPUState *cs, TranslationBlock *tb,
                            int *max_insns, vaddr pc, void *host_pc);

enum {
    TB_FLAGS_FULL_ACCESS = 1,
    TB_FLAGS_SKIP = 2,
};

static inline int cpu_interrupts_enabled(CPUEZHState *env)
{
    // return env->sregI != 0;
    return 0;
}

bool ezh_cpu_tlb_fill(CPUState *cs, vaddr address, int size,
                      MMUAccessType access_type, int mmu_idx,
                      bool probe, uintptr_t retaddr);

extern const MemoryRegionOps avr_cpu_reg1;

#endif /* QEMU_EZH_CPU_H */
