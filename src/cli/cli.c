/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

#include "ch.h"
#include "hal.h"

#include "chprintf.h"

#include "cli.h"
#include "fs.h"
#include "led.h"
#include "shell.h"
#include "usbcfg.h"
#include "util.h"

#include <stdlib.h>
#include <string.h>

static void cmd_reset(BaseSequentialStream *chp, int argc, char *argv[])
{
    (void)argv;
    if (argc > 0) {
        chprintf(chp, "Usage: reset" SHELL_NEWLINE_STR);
        return;
    }

    NVIC_SystemReset();
}

static void cmd_identity(BaseSequentialStream *chp, int argc, char *argv[])
{
    static const char filename[] = "identity";
    static const char filename_tmp[] = "identity.tmp";
    static const char *items[] = {
        "Vendor ID:",
        "Product:  ",
        "Revision: ",
        "S/N:      ",
    };

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
        chprintf(chp,
                 "Usage: identity set vendor_id product_id revision "
                 "serial" SHELL_NEWLINE_STR);
        chprintf(chp, "       identity show" SHELL_NEWLINE_STR);
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
        if (fsWrite(filename_tmp, buffer, sizeof(buffer)) == sizeof(buffer)) {
            if (fsRename(filename_tmp, filename) == 0) {
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
        if (fsRead(filename, buffer, sizeof(buffer)) == sizeof(buffer)) {
            for (unsigned i = 0; i < COUNTOF(items); i++) {
                chprintf(
                    chp, "  %s 0x%08X" SHELL_NEWLINE_STR, items[i], buffer[i]);
            }
        } else {
            for (unsigned i = 0; i < COUNTOF(items); i++) {
                chprintf(chp, "  %s -" SHELL_NEWLINE_STR, items[i]);
            }
        }
    }
}

static const ShellCommand commands[] = {
    {"reset", cmd_reset},
    {"identity", cmd_identity},
    {NULL, NULL},
};

#if (SHELL_USE_HISTORY == TRUE)
static char histbuf[SHELL_MAX_HIST_BUFF];
#endif
#if (SHELL_USE_COMPLETION == TRUE)
static char *completion[SHELL_MAX_COMPLETIONS];
#endif

static const ShellConfig shellcfg = {
    (BaseSequentialStream *)&SDU1,
    commands,
#if (SHELL_USE_HISTORY == TRUE)
    histbuf,
    SHELL_MAX_HIST_BUFF,
#endif
#if (SHELL_USE_COMPLETION == TRUE)
    completion,
#endif
};

enum CLI_STATE { DISCONNECTED, CONNECTING, CONNECTED, RUNNING };

static enum CLI_STATE cli_state;

void cliInit(void)
{
    ledSet(LED1, 1, 19);

    sduObjectInit(&SDU1);

    shellInit();

    cli_state = DISCONNECTED;
}

static thread_t *shellThrd = NULL;
static THD_WORKING_AREA(waThreadShell, 1024);

static systime_t start_time;

void cliCheck(void)
{
    switch (cli_state) {
    case DISCONNECTED:
        if (palReadPad(PORT_USBDET, PAD_USBDET)) {
            sduStart(&SDU1, &serusbcfg1);
            usbDisconnectBus(serusbcfg1.usbp);
            start_time = chVTGetSystemTimeX();
            cli_state = CONNECTING;
        };
        break;
    case CONNECTING:
        if ((chVTGetSystemTimeX() - start_time) > TIME_MS2I(1500)) {
            usbStart(serusbcfg1.usbp, &usbcfg);
            usbConnectBus(serusbcfg1.usbp);
            cli_state = CONNECTED;
        }
        break;
    case CONNECTED:
        if (SDU1.config->usbp->state == USB_ACTIVE) {
            shellThrd = chThdCreateStatic(waThreadShell,
                                          sizeof(waThreadShell),
                                          NORMALPRIO,
                                          shellThread,
                                          (void *)&shellcfg);
            ledSet(LED1, 9, 1);
            cli_state = RUNNING;
        }
        break;
    case RUNNING:
        if (!palReadPad(PORT_USBDET, PAD_USBDET)) {
            usbStop(serusbcfg1.usbp);
            sduStop(&SDU1);
            chThdTerminate(shellThrd);
            chThdRelease(shellThrd);
            shellThrd = NULL;
            ledSet(LED1, 1, 19);
            cli_state = DISCONNECTED;
        }
        break;
    }
}
