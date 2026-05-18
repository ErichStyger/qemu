/*
 * AVR loader helpers
 *
 * Copyright (c) 2019-2020 Philippe Mathieu-Daudé
 *
 * This work is licensed under the terms of the GNU GPLv2 or later.
 * See the COPYING file in the top-level directory.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "qemu/osdep.h"
#include "qemu/datadir.h"
#include "hw/core/loader.h"
#include "elf.h"
#include "boot.h"
#include "qemu/error-report.h"

bool ezh_load_firmware(EZHCPU *cpu, MachineState *ms,
                       MemoryRegion *program_mr, const char *firmware)
{
    g_autofree char *filename = NULL;
    int bytes_loaded;
    uint64_t entry;
    uint32_t e_flags;

    filename = qemu_find_file(QEMU_FILE_TYPE_BIOS, firmware);
    if (filename == NULL) {
        error_report("Unable to find %s", firmware);
        return false;
    }

    bytes_loaded = load_elf_as(filename, NULL, NULL, NULL,
                               &entry, NULL, NULL,
                               &e_flags, ELFDATA2LSB, EM_AVR, 0, 0, NULL);
    if (bytes_loaded >= 0) {
        /* If ELF file is provided, determine CPU type reading ELF e_flags. */
        if (entry) {
            error_report("BIOS entry_point must be 0x0000 "
                         "(ELF image '%s' has entry_point 0x%04" PRIx64 ")",
                         firmware, entry);
            return false;
        }
    } else {
        bytes_loaded = load_image_mr(filename, program_mr);
    }
    if (bytes_loaded < 0) {
        error_report("Unable to load firmware image %s as ELF or raw binary",
                     firmware);
        return false;
    }
    return true;
}
