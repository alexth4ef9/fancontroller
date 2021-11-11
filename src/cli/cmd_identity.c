/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "chprintf.h"
#include "shell.h"

#include "fs.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

extern thread_t *_threadFsSettings;

static const char filename[] = "identity";
static const char filename_tmp[] = "identity.tmp";
static const char *items[] = {
    "Vendor ID:",
    "Product:  ",
    "Revision: ",
    "S/N:      ",
};

static void cmd_identity_usage(BaseSequentialStream *chp)
{
    chprintf(chp,
             "Usage: identity set vendor_id product_id revision "
             "serial" SHELL_NEWLINE_STR);
    chprintf(chp, "       identity show" SHELL_NEWLINE_STR);
}

void cmd_identity(BaseSequentialStream *chp, int argc, char *argv[])
{
    if (argc > 0) {
        if (strcmp(argv[0], "set") == 0 && argc != 5) {
            chprintf(chp,
                     "Usage: identity set vendor_id product_id revision "
                     "serial" SHELL_NEWLINE_STR);
            return;
        } else if (strcmp(argv[0], "show") == 0 && argc != 1) {
            chprintf(chp, "Usage: identity show" SHELL_NEWLINE_STR);
            return;
        }
    } else {
        cmd_identity_usage(chp);
        return;
    }

    if (strcmp(argv[0], "set") == 0) {
        uint32_t buffer[COUNTOF(items)];
        for (unsigned i = 0; i < COUNTOF(items); i++) {
            char *endptr;
            buffer[i] = strtoul(argv[i + 1], &endptr, 0);
            if (buffer[i] == 0 && *endptr != '\0') {
                chprintf(chp, "invalid parameter" SHELL_NEWLINE_STR);
                return;
            }
        }
        if (fsWrite(_threadFsSettings, filename_tmp, buffer, sizeof(buffer)) ==
            sizeof(buffer)) {
            if (fsRename(_threadFsSettings, filename_tmp, filename) == 0) {
                chprintf(chp, "set identity to:" SHELL_NEWLINE_STR);
                for (unsigned i = 0; i < COUNTOF(items); i++) {
                    chprintf(chp,
                             "  %s 0x%08X" SHELL_NEWLINE_STR,
                             items[i],
                             buffer[i]);
                }
            }
        } else {
            chprintf(chp, "failed to set identity" SHELL_NEWLINE_STR);
        }
    } else if (strcmp(argv[0], "show") == 0) {
        uint32_t buffer[COUNTOF(items)];
        chprintf(chp, "Identity:" SHELL_NEWLINE_STR);
        if (fsRead(_threadFsSettings, filename, buffer, sizeof(buffer)) ==
            sizeof(buffer)) {
            for (unsigned i = 0; i < COUNTOF(items); i++) {
                chprintf(
                    chp, "  %s 0x%08X" SHELL_NEWLINE_STR, items[i], buffer[i]);
            }
        } else {
            for (unsigned i = 0; i < COUNTOF(items); i++) {
                chprintf(chp, "  %s -" SHELL_NEWLINE_STR, items[i]);
            }
        }
    } else {
        cmd_identity_usage(chp);
    }
}
