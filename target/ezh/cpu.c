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
#include "qapi/error.h"
#include "qemu/qemu-print.h"
#include "exec/translation-block.h"
#include "system/address-spaces.h"
#include "cpu.h"
#include "disas/dis-asm.h"
#include "tcg/debug-assert.h"
#include "hw/core/qdev-properties.h"
#include "accel/tcg/cpu-ops.h"
#include "exec/log.h"

static void ezh_cpu_set_pc(CPUState *cs, vaddr value)
{
    EZHCPU *cpu = EZH_CPU(cs);
    cpu->env.s_cpu_PC = value;
}

static vaddr ezh_cpu_get_pc(CPUState *cs)
{
    EZHCPU *cpu = EZH_CPU(cs);
    return cpu->env.s_cpu_PC;
}

static bool ezh_cpu_has_work(CPUState *cs)
{
    return cpu_test_interrupt(cs, CPU_INTERRUPT_HARD | CPU_INTERRUPT_RESET)
            && cpu_interrupts_enabled(cpu_env(cs));
}

static int ezh_cpu_mmu_index(CPUState *cs, bool ifetch)
{
    return 0; /* no MMU available => return 0 */
}

static TCGTBCPUState ezh_get_tb_cpu_state(CPUState *cs)
{
    EZHCPUState *env = cpu_env(cs);
    uint32_t flags = 0;

    if (env->fullacc) {
        flags |= TB_FLAGS_FULL_ACCESS;
    }
    if (env->skip) {
        flags |= TB_FLAGS_SKIP;
    }

    return (TCGTBCPUState){ .pc = env->s_cpu_PC, .flags = flags };
}

static void ezh_cpu_synchronize_from_tb(CPUState *cs,
                                        const TranslationBlock *tb)
{
    tcg_debug_assert(!tcg_cflags_has(cs, CF_PCREL));
    cpu_env(cs)->s_cpu_PC = tb->pc;
}

static void ezh_restore_state_to_opc(CPUState *cs,
                                     const TranslationBlock *tb,
                                     const uint64_t *data)
{
    cpu_env(cs)->s_cpu_PC = data[0];
}

/* reset cpu and set registers to defined state */
static void ezh_cpu_reset_hold(Object *obj, ResetType type)
{
    CPUState *cs = CPU(obj);
    EZHCPU *cpu = EZH_CPU(cs);
    EZHCPUClass *mcc = EZH_CPU_GET_CLASS(obj);
    EZHCPUState *env = &cpu->env;

    if (mcc->parent_phases.hold) {
        mcc->parent_phases.hold(obj, type);
    }
    env->s_alu_AZ = 0;
    env->s_alu_CA = 0;
    env->s_alu_CZ = 0;
    env->s_alu_EU = 0;
    env->s_alu_NC = 0;
    env->s_alu_NE = 0;
    env->s_alu_PO = 0;
    env->s_alu_ZB = 0;
    env->s_alu_ZE = 0;
    env->s_cpu_CFM = 0;
    env->s_cpu_CFS = 0;
    env->s_cpu_GPD = 0;
    env->s_cpu_GPI = 0;
    env->s_cpu_GPO = 0;
    env->s_cpu_IC = 0;
    env->s_cpu_PC = 0;
    env->s_cpu_RA = 0;
    env->s_cpu_SP = 0;

    env->skip = 0;

    memset(env->r, 0, sizeof(env->r));
}

static void ezh_cpu_disas_set_info(const CPUState *cpu, disassemble_info *info)
{
    info->endian = BFD_ENDIAN_BIG;
    info->mach = bfd_arch_unknown;
    info->print_insn = ezh_print_insn;
}

static void ezh_cpu_realizefn(DeviceState *dev, Error **errp)
{
    CPUState *cs = CPU(dev);
    EZHCPUState *env = cpu_env(cs);
    EZHCPU *cpu = env_archcpu(env);
    EZHCPUClass *mcc = EZH_CPU_GET_CLASS(dev);
    Error *local_err = NULL;

    cpu_exec_realizefn(cs, &local_err);
    if (local_err != NULL) {
        error_propagate(errp, local_err);
        return;
    }
    qemu_init_vcpu(cs);
    cpu_reset(cs);

    mcc->parent_realize(dev, errp);

    memory_region_init_io(&cpu->cpu_reg, OBJECT(cpu), &ezh_cpu_reg, env,
                          "ezh-cpu-reg", 32);
}

static void ezh_cpu_set_int(void *opaque, int irq, int level)
{
    EZHCPU *cpu = opaque;
    EZHCPUState *env = &cpu->env;
    CPUState *cs = CPU(cpu);
    uint64_t mask = (1ull << irq);

    if (level) {
        env->intsrc |= mask;
        cpu_interrupt(cs, CPU_INTERRUPT_HARD);
    } else {
        env->intsrc &= ~mask;
        if (env->intsrc == 0) {
            cpu_reset_interrupt(cs, CPU_INTERRUPT_HARD);
        }
    }
}

static void ezh_cpu_initfn(Object *obj)
{
    EZHCPU *cpu = EZH_CPU(obj);

    /* Set the number of interrupts supported by the CPU. */
    qdev_init_gpio_in(DEVICE(cpu), ezh_cpu_set_int,
                      sizeof(cpu->env.intsrc) * 8);
}

static ObjectClass *ezh_cpu_class_by_name(const char *cpu_model)
{
    return object_class_by_name(cpu_model);
}

static void ezh_cpu_dump_state(CPUState *cs, FILE *f, int flags)
{
    EZHCPUState *env = cpu_env(cs);
    int i;

    qemu_fprintf(f, "\n");
    qemu_fprintf(f, "PC:  0x%032X\n", env->s_cpu_PC);
    qemu_fprintf(f, "SP:  0x%032X\n", env->s_cpu_SP);
    qemu_fprintf(f, "RA:  0x%032X\n", env->s_cpu_RA);
    qemu_fprintf(f, "IC:  0x%032X\n", env->s_cpu_IC);
    qemu_fprintf(f, "GPO: 0b%032b\n", env->s_cpu_GPO);
    qemu_fprintf(f, "GPI: 0b%032b\n", env->s_cpu_GPI);
    qemu_fprintf(f, "GPD: 0b%032b\n", env->s_cpu_GPD);
    qemu_fprintf(f, "CFS: 0b%032b\n", env->s_cpu_CFS);
    qemu_fprintf(f, "CFM: 0b%032b\n", env->s_cpu_CFM);
    qemu_fprintf(f, "ALU: [  %s %s %s %s %s %s %s %s %s  ]\n",
                 env->s_alu_AZ ? "AZ" : "--",
                 env->s_alu_CA ? "CA" : "--",
                 env->s_alu_CZ ? "CZ" : "--",
                 env->s_alu_EU ? "EU" : "--",
                 env->s_alu_NC ? "NC" : "--",
                 env->s_alu_NE ? "NE" : "--",
                 env->s_alu_PO ? "PO" : "--",
                 env->s_alu_ZB ? "ZB" : "--",
                 env->s_alu_ZE ? "ZE" : "--");

    qemu_fprintf(f, "\n");
    for (i = 0; i < ARRAY_SIZE(env->r); i++) {
        qemu_fprintf(f, "R[%02d]:  %02x   ", i, env->r[i]);

        if ((i % 8) == 7) {
            qemu_fprintf(f, "\n");
        }
    }
    qemu_fprintf(f, "\n");
}

#include "hw/core/sysemu-cpu-ops.h"

static const struct SysemuCPUOps ezh_sysemu_ops = {
    .has_work = ezh_cpu_has_work,
    .get_phys_addr_debug = ezh_cpu_get_phys_page_debug,
};

/* Live in all translation blocks, and corresponds to memory location that is within CPUArchState */
static const TCGCPUOps ezh_tcg_ops = {
    .guest_default_memory_order = 0,
    .mttcg_supported = false,
    .initialize = ezh_cpu_tcg_init,
    .translate_code = ezh_cpu_translate_code,
    .get_tb_cpu_state = ezh_get_tb_cpu_state,
    .synchronize_from_tb = ezh_cpu_synchronize_from_tb,
    .restore_state_to_opc = ezh_restore_state_to_opc,
    .mmu_index = ezh_cpu_mmu_index,
    .cpu_exec_interrupt = ezh_cpu_exec_interrupt,
    .cpu_exec_halt = ezh_cpu_has_work,
    .cpu_exec_reset = cpu_reset,
    .tlb_fill = ezh_cpu_tlb_fill,
    .do_interrupt = ezh_cpu_do_interrupt,
    .pointer_wrap = cpu_pointer_wrap_uint32,
};

static void ezh_cpu_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    CPUClass *cc = CPU_CLASS(oc);                   /* defined in hw/core/cpu.h */
    EZHCPUClass *mcc = EZH_CPU_CLASS(oc);           /* automatically generated with call in cpu-qom.h */
    ResettableClass *rc = RESETTABLE_CLASS(oc);

    device_class_set_parent_realize(dc, ezh_cpu_realizefn, &mcc->parent_realize);

    resettable_class_set_parent_phases(rc, NULL, ezh_cpu_reset_hold, NULL,
                                       &mcc->parent_phases);

    cc->class_by_name = ezh_cpu_class_by_name;

    cc->dump_state = ezh_cpu_dump_state;
    cc->set_pc = ezh_cpu_set_pc;
    cc->get_pc = ezh_cpu_get_pc;
    //dc->vmsd = &vms_ezh_cpu; /* unimplemented machine.c */
    cc->sysemu_ops = &ezh_sysemu_ops;
    cc->disas_set_info = ezh_cpu_disas_set_info;
    cc->gdb_read_register = ezh_cpu_gdb_read_register;
    cc->gdb_write_register = ezh_cpu_gdb_write_register;
    cc->gdb_adjust_breakpoint = ezh_cpu_gdb_adjust_breakpoint;
    cc->gdb_core_xml_file = "ezh-cpu.xml";
    cc->tcg_ops = &ezh_tcg_ops;
}

static const TypeInfo cpu_type_info = {
    .name          = TYPE_EZH_CPU,
    .parent        = TYPE_CPU,
    .instance_size = sizeof(EZHCPU),
    .instance_align = __alignof(EZHCPU),
    .instance_init = ezh_cpu_initfn,
    .class_size = sizeof(EZHCPUClass),
    .class_init = ezh_cpu_class_init,
};

/* tell qemu that it supports the following devices from now on */
static void cpu_register_types(void)
{
    type_register_static(&cpu_type_info);
}

type_init(cpu_register_types);
