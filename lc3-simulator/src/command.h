#ifndef LC3_CMD_H
#define LC3_CMD_H

#include <ctype.h>
#include <stdbool.h>
#include <signal.h>
#include "common.h"
#include "label.h"
#include "rl.h"

extern void lc3_run (void);
extern void lc3_init_machine (const char *path);

#endif
