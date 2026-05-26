/*
 * QEMU EZH implementation
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 * Copyright (c) 2026 Stefano Nicora
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/module.h"
#include "qemu/units.h"
#include "qapi/error.h"
#include "exec/target_page.h"
#include "system/memory.h"
#include "system/address-spaces.h"
#include "system/system.h"
#include "hw/core/qdev-properties.h"
#include "hw/core/sysbus.h"
#include "qom/object.h"
#include "hw/misc/unimp.h"
#include "migration/vmstate.h"
#include "ezh.h"

typedef struct EZHClass {
    /*< private >*/
    SysBusDeviceClass parent_class;
    /*< public >*/
    const char *uc_name;
    const char *cpu_type;
    size_t flash_size;
    size_t ram_size;
    size_t io_size;
} EZHClass;

DECLARE_CLASS_CHECKERS(EZHClass, EZH, TYPE_EZH)

static uint64_t ezh_io_read(void *opaque, hwaddr addr, unsigned size) {
    return 0;
}

static void ezh_io_write(void *opaque, hwaddr addr, uint64_t value, unsigned size) {
}

static const MemoryRegionOps ezh_io_ops = {
    .read = ezh_io_read,
    .write = ezh_io_write,
    .endianness = DEVICE_NATIVE_ENDIAN,
};  

static void ezh_realize(DeviceState *dev, Error **errp)
{
    EZHState *s = EZH(dev);
    const EZHClass *mc = EZH_GET_CLASS(dev);

    /* CPU initialization */
    object_initialize_child(OBJECT(dev), "cpu", &s->cpu, mc->cpu_type);
    qdev_realize(DEVICE(&s->cpu), NULL, &error_abort);

    MemoryRegion *sysmem = get_system_memory();

    /* Flash memory region */
    memory_region_init_rom(&s->flash, OBJECT(dev), "ezh.flash", mc->flash_size, &error_fatal);
    memory_region_add_subregion(sysmem, FLASH_START_ADDR, &s->flash);

    /* RAM memory region */
    memory_region_init_ram(&s->ram, OBJECT(dev), "ezh.ram", mc->ram_size, &error_abort);
    memory_region_add_subregion(sysmem, RAM_START_ADDR, &s->ram);

    /* I/O memory region */
    memory_region_init_io(&s->io, OBJECT(dev), &ezh_io_ops, s, "ezh.io", mc->io_size);
    memory_region_add_subregion(sysmem, IO_START_ADDR, &s->io);
}

static void ezh_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    EZHClass *mc = EZH_CLASS(oc);

    dc->realize = ezh_realize;
    mc->cpu_type = TYPE_EZH_CPU;
    mc->flash_size = RAM_START_ADDR - FLASH_START_ADDR ;
    mc->ram_size = IO_START_ADDR - RAM_START_ADDR;
    mc->io_size = 0xBFFFFFFF - IO_START_ADDR;               /* refMan. page 47 */
}

/* constructor | TypeInfo contains all neccessary information about the machine */
static const TypeInfo ezh_type_info = {
    .name           = TYPE_EZH,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(EZHState),
    .class_size     = sizeof(EZHClass),
    .class_init     = ezh_class_init,
};

static void ezh_register_types(void)
{
    type_register_static(&ezh_type_info);
}

type_init(ezh_register_types);