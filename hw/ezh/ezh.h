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

/* refMan. page 47 */
#define FLASH_START_ADDR    (0x00000000)
#define RAM_START_ADDR      (0x20000000)
#define IO_START_ADDR       (0x40000000)

typedef struct EZHState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    EZHCPU cpu;
    MemoryRegion flash;
    MemoryRegion ram;
    MemoryRegion io;
} EZHState;

DECLARE_INSTANCE_CHECKER(EZHState, EZH, TYPE_EZH)

#endif /* EZH_H */
