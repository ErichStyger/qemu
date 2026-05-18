/*
 * QEMU Arduino boards
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

/* TODO: Implement the use of EXTRAM */

#include "qemu/osdep.h"
#include "qapi/error.h"
#include "ezh.h"
#include "boot.h"
#include "qom/object.h"

typedef struct MCXN947MachineState {
    /*< private >*/
    MachineState parent_obj;
    /*< public >*/
    EZHState mcu;
} MCXN947MachineState;

typedef struct MCXN947MachineClass {
    /*< private >*/
    MachineClass parent_class;
    /*< public >*/
    const char *mcu_type;
    uint64_t xtal_hz;
}MCXN947MachineClass;

#define TYPE_MCXN947_MACHINE MACHINE_TYPE_NAME("mcxn947")       /* name that is used to invoke the machine via the command prompt */
DECLARE_OBJ_CHECKERS(MCXN947MachineState, MCXN947MachineClass,
                     MCXN947_MACHINE, TYPE_MCXN947_MACHINE)

static void mcxn947_machine_init(MachineState *machine)
{
    MCXN947MachineClass *amc = MCXN947_MACHINE_GET_CLASS(machine);
    MCXN947MachineState *ams = MCXN947_MACHINE(machine);

    object_initialize_child(OBJECT(machine), "mcu", &ams->mcu, amc->mcu_type);  /* create mcxn947 board */
    // object_property_set_uint(OBJECT(&ams->mcu), "xtal-frequency-hz",
    //                          amc->xtal_hz, &error_abort);
    sysbus_realize(SYS_BUS_DEVICE(&ams->mcu), &error_abort);    /* connect ezh core to mcxn947 board */

    if (machine->firmware) {
        if (!ezh_load_firmware(&ams->mcu.cpu, machine,
                               &ams->mcu.flash, machine->firmware)) {
            exit(1);
        }
    }
}

static void mcxn947_machine_class_init(ObjectClass *oc, const void *data)
{
    MachineClass *mc = MACHINE_CLASS(oc);
    MCXN947MachineClass *cmc = MCXN947_MACHINE_CLASS(oc);

    mc->init = mcxn947_machine_init;
    mc->desc        = "MCXN947";
    mc->default_cpus = 1;
    mc->min_cpus = mc->default_cpus;
    mc->max_cpus = mc->default_cpus;
    mc->no_floppy = 1;
    mc->no_cdrom = 1;
    mc->no_parallel = 1;
    mc->default_cpu_type = TYPE_EZH_CPU;
    cmc->mcu_type   = TYPE_EZH;
    cmc->xtal_hz    = 16 * 1000 * 1000;
}

static const TypeInfo mcxn947_type_info = {
    .name           = TYPE_MCXN947_MACHINE,
    .parent         = TYPE_MACHINE,
    .instance_size  = sizeof(MCXN947MachineState),
    .class_size     = sizeof(MCXN947MachineClass),
    .class_init     = mcxn947_machine_class_init,
};

static void mcxn947_register_types(void)
{
    type_register_static(&mcxn947_type_info);
}

type_init(mcxn947_register_types);
