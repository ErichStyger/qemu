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

#ifndef HW_EZH_QMP_H
#define HW_EZH_QMP_H

#include "hw/core/cpu.h"
#include <stdbool.h>
#include <stdint.h>

bool ezh_cpu_get_register(CPUState *cs, int64_t reg, int64_t *value);

bool ezh_cpu_set_register(CPUState *cs, int64_t reg, int64_t value);

#endif