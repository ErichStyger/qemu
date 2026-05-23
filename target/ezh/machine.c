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

#include "qemu/osdep.h"
#include "cpu.h"
#include "migration/cpu.h"

/* currently unimplemented
 * needed if QEMU needs to be portable
 * e.g. move instance from one phyisical device
 * to another
 */
// static int get_segment(QEMUFile *f, void *opaque, size_t size,
//                        const VMStateField *field)
// {
//     uint32_t *ramp = opaque;
//     uint8_t temp;

//     temp = qemu_get_byte(f);
//     *ramp = ((uint32_t)temp) << 16;
//     return 0;
// }

// static int put_segment(QEMUFile *f, void *opaque, size_t size,
//                        const VMStateField *field, JSONWriter *vmdesc)
// {
//     uint32_t *ramp = opaque;
//     uint8_t temp = *ramp >> 16;

//     qemu_put_byte(f, temp);
//     return 0;
// }

// static const VMStateInfo vms_rampD = {
//     .name = "rampD",
//     .get = get_segment,
//     .put = put_segment,
// };

// const VMStateDescription vms_ezh_cpu = {
//     .name = "cpu",
//     .version_id = 1,
//     .minimum_version_id = 1,
//     .fields = (const VMStateField[]) {
//         // VMSTATE_UINT32(env.pc_w, EZHCPU),

//         VMSTATE_UINT32_ARRAY(env.r, EZHCPU, NUMBER_OF_CPU_REGISTERS),

//         VMSTATE_END_OF_LIST()
//     }
// };
