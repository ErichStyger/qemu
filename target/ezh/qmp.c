#include "qemu/osdep.h"
#include "qapi/error.h"
#include "qapi/qapi-commands-ezh.h"
#include "cpu.h"
#include "target/ezh/cpu.h"

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

static EZHCPU *get_ezh_cpu(Error **errp)
{
    CPUState *cs = first_cpu;

    if (!cs) {
        error_setg(errp, "No CPU found");
        return NULL;
    }

    return EZH_CPU(cs);
}

EzhGetRegisterReturn *qmp_ezh_get_register(int64_t reg, Error **errp)
{
    EZHCPU *cpu = get_ezh_cpu(errp);
    if (!cpu) {
        return 0;
    }

    CPUEZHState *env = &cpu->env;
    EzhGetRegisterReturn *ret = g_new0(EzhGetRegisterReturn, 1);

    switch(reg){
        case EZH_R0:  ret->value = env->r[reg];     break;
        case EZH_R1:  ret->value = env->r[reg];     break;
        case EZH_R2:  ret->value = env->r[reg];     break;
        case EZH_R3:  ret->value = env->r[reg];     break;
        case EZH_R4:  ret->value = env->r[reg];     break;
        case EZH_R5:  ret->value = env->r[reg];     break;
        case EZH_R6:  ret->value = env->r[reg];     break;
        case EZH_R7:  ret->value = env->r[reg];     break;
        case EZH_GPO: ret->value = env->s_cpu_GPO;  break;
        case EZH_GPD: ret->value = env->s_cpu_GPD;  break;
        case EZH_GPI: ret->value = env->s_cpu_GPI;  break;
        case EZH_CFS: ret->value = env->s_cpu_CFS;  break;
        case EZH_CFM: ret->value = env->s_cpu_CFM;  break;
        case EZH_SP:  ret->value = env->s_cpu_SP;   break;
        case EZH_PC:  ret->value = env->s_cpu_PC;   break;
        case EZH_RA:  ret->value = env->s_cpu_RA;   break;
        default: error_setg(errp, "Invalid register index: %ld", reg); return 0;
    }

    return ret;
}

void qmp_ezh_set_register(int64_t reg, int64_t value, Error **errp)
{
    EZHCPU *cpu = get_ezh_cpu(errp);
    if (!cpu) {
        return;
    }

    CPUEZHState *env = &cpu->env;

    switch(reg){
        case EZH_R0:  env->r[reg] = value;     break;
        case EZH_R1:  env->r[reg] = value;     break;
        case EZH_R2:  env->r[reg] = value;     break;
        case EZH_R3:  env->r[reg] = value;     break;
        case EZH_R4:  env->r[reg] = value;     break;
        case EZH_R5:  env->r[reg] = value;     break;
        case EZH_R6:  env->r[reg] = value;     break;
        case EZH_R7:  env->r[reg] = value;     break;
        case EZH_GPO: env->s_cpu_GPO = value;  break;
        case EZH_GPD: env->s_cpu_GPD = value;  break;
        case EZH_GPI: env->s_cpu_GPI = value;  break;
        case EZH_CFS: env->s_cpu_CFS = value;  break;
        case EZH_CFM: env->s_cpu_CFM = value;  break;
        case EZH_SP:  env->s_cpu_SP  = value;  break;
        case EZH_PC:  env->s_cpu_PC  = value;  break;
        case EZH_RA:  env->s_cpu_RA  = value;  break;
        default: error_setg(errp, "Invalid register index: %ld", reg); return;
    }

    env->r[reg] = value;
}