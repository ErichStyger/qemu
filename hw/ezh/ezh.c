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
    size_t sram_size;
    size_t io_size;
} EZHClass;

DECLARE_CLASS_CHECKERS(EZHClass, EZH, TYPE_EZH)

static void ezh_realize(DeviceState *dev, Error **errp)
{
    EZHState *s = EZH(dev);
    const EZHClass *mc = EZH_GET_CLASS(dev);

    /* CPU */
    object_initialize_child(OBJECT(dev), "cpu", &s->cpu, mc->cpu_type);
    qdev_realize(DEVICE(&s->cpu), NULL, &error_abort);

    /* SRAM */
    int sram_io_size = TARGET_PAGE_SIZE - mc->io_size;
    void *sram_io_mem = g_malloc0(sram_io_size);

    memory_region_init_ram_device_ptr(&s->sram_io, OBJECT(dev), "sram-as-io", sram_io_size, sram_io_mem);
    memory_region_add_subregion(get_system_memory(), 0 + mc->io_size, &s->sram_io);
    vmstate_register_ram(&s->sram_io, dev);
    memory_region_init_ram(&s->sram, OBJECT(dev), "sram", mc->sram_size - sram_io_size, &error_abort);
    memory_region_add_subregion(get_system_memory(), 0 + TARGET_PAGE_SIZE, &s->sram);

    /* Flash */
    memory_region_init_rom(&s->flash, OBJECT(dev),
                           "flash", mc->flash_size, &error_fatal);
    memory_region_add_subregion(get_system_memory(), 0, &s->flash);

    /* I/O */
    s->io = qdev_new(TYPE_UNIMPLEMENTED_DEVICE);
    qdev_prop_set_string(s->io, "name", "I/O");
    qdev_prop_set_uint64(s->io, "size", mc->io_size);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(s->io), &error_fatal);
    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(s->io), 0, 0, -1234);
}

static void ezh_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    EZHClass *mc = EZH_CLASS(oc);

    dc->realize = ezh_realize;
    dc->user_creatable = false;
    mc->cpu_type = TYPE_EZH_CPU;
    mc->flash_size = 2000 * KiB;
    mc->sram_size = 512 * KiB;
    mc->io_size = 256;
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