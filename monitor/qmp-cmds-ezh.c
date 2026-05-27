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
#include "qapi/error.h"
#include "qapi/qapi-commands-ezh.h"
#include "hw/core/cpu.h"
#include "hw/ezh/ezh-qmp.h"

static CPUState *get_cpu(Error **errp)
{
    CPUState *cs = first_cpu;

    if (!cs) {
        error_setg(errp, "No CPU found");
        return NULL;
    }

    return cs;
}

EzhGetRegisterReturn *qmp_ezh_get_register(int64_t reg, Error **errp)
{
    CPUState *cs = get_cpu(errp);
    int64_t value;
    EzhGetRegisterReturn *ret;

    if (!cs) {
        return NULL;
    }

    if (!ezh_get_register(cs, reg, &value)) {
        error_setg(errp, "Invalid EZH register or CPU type");
        return NULL;
    }

    ret = g_new0(EzhGetRegisterReturn, 1);
    ret->value = value;
    return ret;
}

void qmp_ezh_set_register(int64_t reg, int64_t value, Error **errp)
{
    CPUState *cs = get_cpu(errp);

    if (!cs) {
        return;
    }

    if (!ezh_set_register(cs, reg, value)) {
        error_setg(errp, "Invalid EZH register or CPU type");
    }
}