/*
 * QEMU EZH implementation
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
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

enum EZHPeripheral {
    POWER0, POWER1,
    GPIOA, GPIOB, GPIOC, GPIOD, GPIOE, GPIOF,
    GPIOG, GPIOH, GPIOI, GPIOJ, GPIOK, GPIOL,
    USART0, USART1, USART2, USART3,
    TIMER0, TIMER1, TIMER2, TIMER3, TIMER4, TIMER5,
    PERIFMAX
};

#define GPIO(n)     (n + GPIOA)
#define USART(n)    (n + USART0)
#define TIMER(n)    (n + TIMER0)
#define POWER(n)    (n + POWER0)

typedef struct {
    uint16_t addr;
    enum EZHPeripheral power_index;
    uint8_t power_bit;
    /* timer specific */
    uint16_t intmask_addr;
    uint16_t intflag_addr;
    bool is_timer16;
} peripheral_cfg;

struct EZHClass {
    /*< private >*/
    SysBusDeviceClass parent_class;
    /*< public >*/
    const char *uc_name;
    const char *cpu_type;
    size_t flash_size;
    size_t eeprom_size;
    size_t sram_size;
    size_t io_size;
    size_t gpio_count;
    const uint8_t *irq;
    const peripheral_cfg *dev;
};
typedef struct EZHClass EZHClass;

DECLARE_CLASS_CHECKERS(EZHClass, EZH,
                       TYPE_EZH)

static const peripheral_cfg dev168_328[PERIFMAX] = {
    [USART0]        = {  0xc0, POWER0, 1 },
    [TIMER2]        = {  0xb0, POWER0, 6, 0x70, 0x37, false },
    [TIMER1]        = {  0x80, POWER0, 3, 0x6f, 0x36, true },
    [POWER0]        = {  0x64 },
    [TIMER0]        = {  0x44, POWER0, 5, 0x6e, 0x35, false },
    [GPIOD]         = {  0x29 },
    [GPIOC]         = {  0x26 },
    [GPIOB]         = {  0x23 },
};

enum EZHIrq {
    USART0_RXC_IRQ, USART0_DRE_IRQ, USART0_TXC_IRQ,
    USART1_RXC_IRQ, USART1_DRE_IRQ, USART1_TXC_IRQ,
    USART2_RXC_IRQ, USART2_DRE_IRQ, USART2_TXC_IRQ,
    USART3_RXC_IRQ, USART3_DRE_IRQ, USART3_TXC_IRQ,
    TIMER0_CAPT_IRQ, TIMER0_COMPA_IRQ, TIMER0_COMPB_IRQ,
        TIMER0_COMPC_IRQ, TIMER0_OVF_IRQ,
    TIMER1_CAPT_IRQ, TIMER1_COMPA_IRQ, TIMER1_COMPB_IRQ,
        TIMER1_COMPC_IRQ, TIMER1_OVF_IRQ,
    TIMER2_CAPT_IRQ, TIMER2_COMPA_IRQ, TIMER2_COMPB_IRQ,
        TIMER2_COMPC_IRQ, TIMER2_OVF_IRQ,
    TIMER3_CAPT_IRQ, TIMER3_COMPA_IRQ, TIMER3_COMPB_IRQ,
        TIMER3_COMPC_IRQ, TIMER3_OVF_IRQ,
    TIMER4_CAPT_IRQ, TIMER4_COMPA_IRQ, TIMER4_COMPB_IRQ,
        TIMER4_COMPC_IRQ, TIMER4_OVF_IRQ,
    TIMER5_CAPT_IRQ, TIMER5_COMPA_IRQ, TIMER5_COMPB_IRQ,
        TIMER5_COMPC_IRQ, TIMER5_OVF_IRQ,
    IRQ_COUNT
};

#define USART_IRQ_COUNT     3
#define USART_RXC_IRQ(n)    (n * USART_IRQ_COUNT + USART0_RXC_IRQ)
#define USART_DRE_IRQ(n)    (n * USART_IRQ_COUNT + USART0_DRE_IRQ)
#define USART_TXC_IRQ(n)    (n * USART_IRQ_COUNT + USART0_TXC_IRQ)
#define TIMER_IRQ_COUNT     5
#define TIMER_CAPT_IRQ(n)   (n * TIMER_IRQ_COUNT + TIMER0_CAPT_IRQ)
#define TIMER_COMPA_IRQ(n)  (n * TIMER_IRQ_COUNT + TIMER0_COMPA_IRQ)
#define TIMER_COMPB_IRQ(n)  (n * TIMER_IRQ_COUNT + TIMER0_COMPB_IRQ)
#define TIMER_COMPC_IRQ(n)  (n * TIMER_IRQ_COUNT + TIMER0_COMPC_IRQ)
#define TIMER_OVF_IRQ(n)    (n * TIMER_IRQ_COUNT + TIMER0_OVF_IRQ)

static const uint8_t irq168_328[IRQ_COUNT] = {
    [TIMER2_COMPA_IRQ]      = 8,
    [TIMER2_COMPB_IRQ]      = 9,
    [TIMER2_OVF_IRQ]        = 10,
    [TIMER1_CAPT_IRQ]       = 11,
    [TIMER1_COMPA_IRQ]      = 12,
    [TIMER1_COMPB_IRQ]      = 13,
    [TIMER1_OVF_IRQ]        = 14,
    [TIMER0_COMPA_IRQ]      = 15,
    [TIMER0_COMPB_IRQ]      = 16,
    [TIMER0_OVF_IRQ]        = 17,
    [USART0_RXC_IRQ]        = 19,
    [USART0_DRE_IRQ]        = 20,
    [USART0_TXC_IRQ]        = 21,
};

// static void connect_peripheral_irq(const EZHClass *k,
//                                    SysBusDevice *dev, int dev_irqn,
//                                    DeviceState *cpu,
//                                    unsigned peripheral_index)
// {
//     int cpu_irq = k->irq[peripheral_index];

//     if (!cpu_irq) {
//         return;
//     }
//     /* FIXME move that to ezh_cpu_set_int() once 'sample' board is removed */
//     assert(cpu_irq >= 2);
//     cpu_irq -= 2;

//     sysbus_connect_irq(dev, dev_irqn, qdev_get_gpio_in(cpu, cpu_irq));
// }

// static void connect_power_reduction_gpio(EZHState *s,
//                                          const EZHClass *k,
//                                          DeviceState *cpu,
//                                          unsigned peripheral_index)
// {
//     unsigned power_index = k->dev[peripheral_index].power_index;
//     assert(k->dev[power_index].addr);
//     sysbus_connect_irq(SYS_BUS_DEVICE(&s->pwr[power_index - POWER0]),
//                        k->dev[peripheral_index].power_bit,
//                        qdev_get_gpio_in(cpu, 0));
// }

static void ezh_realize(DeviceState *dev, Error **errp)
{
    EZHState *s = EZH(dev);
    const EZHClass *mc = EZH_GET_CLASS(dev);
    // DeviceState *cpudev;
    // SysBusDevice *sbd;
    char *devname;
    size_t i;

    // if (!s->xtal_freq_hz) {
    //     error_setg(errp, "\"xtal-frequency-hz\" property must be provided.");
    //     return;
    // }

    /* CPU */
    object_initialize_child(OBJECT(dev), "cpu", &s->cpu, mc->cpu_type);

    // object_property_set_uint(OBJECT(&s->cpu), "init-sp",
    //                          mc->io_size + mc->sram_size - 1, &error_abort);

    qdev_realize(DEVICE(&s->cpu), NULL, &error_abort);
    // cpudev = DEVICE(&s->cpu);

    /*
     * SRAM
     *
     * Softmmu is not able mix i/o and ram on the same page.
     * Therefore in all cases, the first page exclusively contains i/o.
     *
     * If the MCU's i/o region matches the page size, then we can simply
     * allocate all ram starting at the second page.  Otherwise, we must
     * allocate some ram as i/o to complete the first page.
     */
    assert(mc->io_size == 0x100 || mc->io_size == 0x200);
    if (mc->io_size >= TARGET_PAGE_SIZE) {
        memory_region_init_ram(&s->sram, OBJECT(dev), "sram", mc->sram_size,
                               &error_abort);
        memory_region_add_subregion(get_system_memory(),
                                    OFFSET_DATA + mc->io_size, &s->sram);
    } else {
        int sram_io_size = TARGET_PAGE_SIZE - mc->io_size;
        void *sram_io_mem = g_malloc0(sram_io_size);

        memory_region_init_ram_device_ptr(&s->sram_io, OBJECT(dev), "sram-as-io",
                                          sram_io_size, sram_io_mem);
        memory_region_add_subregion(get_system_memory(),
                                    OFFSET_DATA + mc->io_size, &s->sram_io);
        vmstate_register_ram(&s->sram_io, dev);

        memory_region_init_ram(&s->sram, OBJECT(dev), "sram",
                               mc->sram_size - sram_io_size, &error_abort);
        memory_region_add_subregion(get_system_memory(),
                                    OFFSET_DATA + TARGET_PAGE_SIZE, &s->sram);
    }

    /* Flash */
    memory_region_init_rom(&s->flash, OBJECT(dev),
                           "flash", mc->flash_size, &error_fatal);
    memory_region_add_subregion(get_system_memory(), OFFSET_CODE, &s->flash);

    /*
     * I/O
     *
     * 0x00 - 0x1f: Registers
     * 0x20 - 0x5f: I/O memory
     * 0x60 - 0xff: Extended I/O
     */
    s->io = qdev_new(TYPE_UNIMPLEMENTED_DEVICE);
    qdev_prop_set_string(s->io, "name", "I/O");
    qdev_prop_set_uint64(s->io, "size", mc->io_size);
    sysbus_realize_and_unref(SYS_BUS_DEVICE(s->io), &error_fatal);
    sysbus_mmio_map_overlap(SYS_BUS_DEVICE(s->io), 0, OFFSET_DATA, -1234);

    /* GPIO */
    for (i = 0; i < GPIO_MAX; i++) {
        int idx = GPIO(i);
        if (!mc->dev[idx].addr) {
            continue;
        }
        devname = g_strdup_printf("ezh-gpio-%c", 'a' + (char)i);
        create_unimplemented_device(devname,
                                    OFFSET_DATA + mc->dev[idx].addr, 3);
        g_free(devname);
    }

    // /* USART */
    // for (i = 0; i < USART_MAX; i++) {
    //     int idx = USART(i);
    //     if (!mc->dev[idx].addr) {
    //         continue;
    //     }
    //     devname = g_strdup_printf("usart%zu", i);
    //     object_initialize_child(OBJECT(dev), devname, &s->usart[i],
    //                             TYPE_AVR_USART);
    //     qdev_prop_set_chr(DEVICE(&s->usart[i]), "chardev", serial_hd(i));
    //     sbd = SYS_BUS_DEVICE(&s->usart[i]);
    //     sysbus_realize(sbd, &error_abort);
    //     sysbus_mmio_map(sbd, 0, OFFSET_DATA + mc->dev[USART(i)].addr);
    //     connect_peripheral_irq(mc, sbd, 0, cpudev, USART_RXC_IRQ(i));
    //     connect_peripheral_irq(mc, sbd, 1, cpudev, USART_DRE_IRQ(i));
    //     connect_peripheral_irq(mc, sbd, 2, cpudev, USART_TXC_IRQ(i));
    //     // connect_power_reduction_gpio(s, mc, DEVICE(&s->usart[i]), idx);
    //     g_free(devname);
    // }

    /* Timer */
    // for (i = 0; i < TIMER_MAX; i++) {
    //     int idx = TIMER(i);
    //     if (!mc->dev[idx].addr) {
    //         continue;
    //     }
    //     // if (!mc->dev[idx].is_timer16) {
    //     //     create_unimplemented_device("avr-timer8",
    //     //                                 OFFSET_DATA + mc->dev[idx].addr, 5);
    //     //     create_unimplemented_device("avr-timer8-intmask",
    //     //                                 OFFSET_DATA
    //     //                                 + mc->dev[idx].intmask_addr, 1);
    //     //     create_unimplemented_device("avr-timer8-intflag",
    //     //                                 OFFSET_DATA
    //     //                                 + mc->dev[idx].intflag_addr, 1);
    //     //     continue;
    //     // }
    //     devname = g_strdup_printf("timer%zu", i);
    //     object_initialize_child(OBJECT(dev), devname, &s->timer[i],
    //                             TYPE_AVR_TIMER16);
    //     // object_property_set_uint(OBJECT(&s->timer[i]), "cpu-frequency-hz",
    //     //                          s->xtal_freq_hz, &error_abort);
    //     sbd = SYS_BUS_DEVICE(&s->timer[i]);
    //     sysbus_realize(sbd, &error_abort);
    //     sysbus_mmio_map(sbd, 0, OFFSET_DATA + mc->dev[idx].addr);
    //     sysbus_mmio_map(sbd, 1, OFFSET_DATA + mc->dev[idx].intmask_addr);
    //     sysbus_mmio_map(sbd, 2, OFFSET_DATA + mc->dev[idx].intflag_addr);
    //     connect_peripheral_irq(mc, sbd, 0, cpudev, TIMER_CAPT_IRQ(i));
    //     connect_peripheral_irq(mc, sbd, 1, cpudev, TIMER_COMPA_IRQ(i));
    //     connect_peripheral_irq(mc, sbd, 2, cpudev, TIMER_COMPB_IRQ(i));
    //     connect_peripheral_irq(mc, sbd, 3, cpudev, TIMER_COMPC_IRQ(i));
    //     connect_peripheral_irq(mc, sbd, 4, cpudev, TIMER_OVF_IRQ(i));
    //     // connect_power_reduction_gpio(s, mc, DEVICE(&s->timer[i]), idx);
    //     g_free(devname);
    // }
}

// static const Property ezh_props[] = {
//     DEFINE_PROP_UINT64("xtal-frequency-hz", EZHState,
//                        xtal_freq_hz, 0),
// };

static void ezh_class_init(ObjectClass *oc, const void *data)
{
    DeviceClass *dc = DEVICE_CLASS(oc);
    EZHClass *mc = EZH_CLASS(oc);

    dc->realize = ezh_realize;

    // device_class_set_props(dc, ezh_props);
    dc->user_creatable = false;
    mc->cpu_type = TYPE_EZH_CPU;
    mc->flash_size = 32 * KiB;
    mc->eeprom_size = 1 * KiB;
    mc->sram_size = 2 * KiB;
    mc->io_size = 256;
    mc->gpio_count = 23;
    mc->irq = irq168_328;
    mc->dev = dev168_328;
}

/* constructor | TypeInfo contains all neccessary information about the machine */
static const TypeInfo ezh_type_info = {
    .name           = TYPE_EZH,
    .parent         = TYPE_SYS_BUS_DEVICE,
    .instance_size  = sizeof(EZHState),
    .class_size     = sizeof(EZHClass),
    .class_init     = ezh_class_init,
};

/* tell qemu that it supports the following devices from now on */
static void ezh_register_types(void)
{
    type_register_static(&ezh_type_info);
}

type_init(ezh_register_types);