/*
 * QEMU ATmega MCU
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef EZH_H
#define EZH_H

#include "hw/char/avr_usart.h"
#include "hw/timer/avr_timer16.h"
#include "hw/misc/avr_power.h"
#include "target/ezh/cpu.h"
#include "qom/object.h"

#define TYPE_EZH     "ezh"

#define TIMER_MAX 6
#define GPIO_MAX 12

typedef struct EZHState {
    /*< private >*/
    SysBusDevice parent_obj;
    /*< public >*/

    EZHCPU cpu;
    MemoryRegion flash;
    MemoryRegion eeprom;
    MemoryRegion sram;
    MemoryRegion sram_io;
    DeviceState *io;
    AVRTimer16State timer[TIMER_MAX];
    uint64_t xtal_freq_hz;
} EZHState;

DECLARE_INSTANCE_CHECKER(EZHState, EZH,
                         TYPE_EZH)

#endif /* EZH_H */
