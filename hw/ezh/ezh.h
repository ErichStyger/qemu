/*
 * QEMU EZH
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 * Copyright (c) 2026 Stefano Nicora
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef EZH_H
#define EZH_H

#include "target/ezh/cpu.h"
#include "hw/core/sysbus.h"     /* SysBusDevice */
#include "qom/object.h"

#define TYPE_EZH     "ezh"

typedef struct EZHState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    EZHCPU cpu;
    MemoryRegion flash;
    MemoryRegion sram;
    MemoryRegion sram_io;
    DeviceState *io;        /* hw/core/qdev.h */
} EZHState;

DECLARE_INSTANCE_CHECKER(EZHState, EZH, TYPE_EZH)

#endif /* EZH_H */
