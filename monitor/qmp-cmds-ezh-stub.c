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

/* this file is needed due to compatibility issues when 
 * trying to build multiple targets at the same time and 
 * qmp commands generally are defined globally */
#include "qemu/osdep.h"
#include "hw/ezh/ezh-qmp.h"

__attribute__((weak)) bool ezh_get_register(CPUState *cs, int64_t reg, int64_t *value) {
    return false;
}

__attribute__((weak)) bool ezh_set_register(CPUState *cs, int64_t reg, int64_t value) {
    return false;
}