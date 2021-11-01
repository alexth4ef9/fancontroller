/* SPDX-License-Identifier: GPL-3.0-only
   fan controller - Copyright (C) 2021 alexth4ef9
*/

/**
 * @brief   Enable shell command history
 */
#define SHELL_USE_HISTORY TRUE

/**
 * @brief   Enable shell command completion
 */
#define SHELL_USE_COMPLETION TRUE

/**
 * @brief   Enable shell escape sequence processing
 */
#define SHELL_USE_ESC_SEQ TRUE

/**
 * @brief   Prompt string
 */
#define SHELL_PROMPT_STR "fc> "

/**
 * @brief   Builtin commands
 */
#define SHELL_CMD_EXIT_ENABLED     FALSE
#define SHELL_CMD_INFO_ENABLED     TRUE
#define SHELL_CMD_ECHO_ENABLED     FALSE
#define SHELL_CMD_SYSTIME_ENABLED  TRUE
#define SHELL_CMD_MEM_ENABLED      TRUE
#define SHELL_CMD_THREADS_ENABLED  TRUE
#define SHELL_CMD_TEST_ENABLED     FALSE
