/*
 * QEMU EZH QMP IMPLEMENTATION
 *
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
#include "cpu.h"
#include "cpu-qom.h"
#include "hw/ezh/ezh-qmp.h"

#define EZH_R0   0x00
#define EZH_R1   0x01
#define EZH_R2   0x02
#define EZH_R3   0x03
#define EZH_R4   0x04
#define EZH_R5   0x05
#define EZH_R6   0x06
#define EZH_R7   0x07
#define EZH_GPO  0x08
#define EZH_GPD  0x09
#define EZH_CFS  0x0a
#define EZH_CFM  0x0b
#define EZH_SP   0x0c
#define EZH_PC   0x0d
#define EZH_GPI  0x0e
#define EZH_RA   0x0f

bool ezh_get_register(CPUState *cs, int64_t reg, int64_t *value) {
    if (!object_dynamic_cast(OBJECT(cs), TYPE_EZH_CPU)) {
        return false;
    }

    EZHCPU *cpu;
    cpu = EZH_CPU(cs);
    CPUEZHState *env = &cpu->env;

    switch (reg) {
    case EZH_R0:
    case EZH_R1:
    case EZH_R2:
    case EZH_R3:
    case EZH_R4:
    case EZH_R5:
    case EZH_R6:
    case EZH_R7:  *value = env->r[reg]; return true;
    case EZH_GPO: *value = env->s_cpu_GPO; return true;
    case EZH_GPD: *value = env->s_cpu_GPD; return true;
    case EZH_GPI: *value = env->s_cpu_GPI; return true;
    case EZH_CFS: *value = env->s_cpu_CFS; return true;
    case EZH_CFM: *value = env->s_cpu_CFM; return true;
    case EZH_SP:  *value = env->s_cpu_SP;  return true;
    case EZH_PC:  *value = env->s_cpu_PC;  return true;
    case EZH_RA:  *value = env->s_cpu_RA;  return true;
    default: return false;
    }
}

bool ezh_set_register(CPUState *cs, int64_t reg, int64_t value) {
    if (!object_dynamic_cast(OBJECT(cs), TYPE_EZH_CPU)) {
        return false;
    }

    EZHCPU *cpu;
    cpu = EZH_CPU(cs);
    CPUEZHState *env = &cpu->env;

    switch (reg) {
    case EZH_R0:
    case EZH_R1:
    case EZH_R2:
    case EZH_R3:
    case EZH_R4:
    case EZH_R5:
    case EZH_R6:
    case EZH_R7:  env->r[reg] = value; return true;
    case EZH_GPO: env->s_cpu_GPO = value; return true;
    case EZH_GPD: env->s_cpu_GPD = value; return true;
    case EZH_GPI: env->s_cpu_GPI = value; return true;
    case EZH_CFS: env->s_cpu_CFS = value; return true;
    case EZH_CFM: env->s_cpu_CFM = value; return true;
    case EZH_SP:  env->s_cpu_SP = value;  return true;
    case EZH_PC:  env->s_cpu_PC = value;  return true;
    case EZH_RA:  env->s_cpu_RA = value;  return true;
    default: return false;
    }
}