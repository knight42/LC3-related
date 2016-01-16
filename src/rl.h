#ifndef LC3_RL_H
#define LC3_RL_H

// ==================== GNU readline Begin =================

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <readline/readline.h>
#include <readline/history.h>

extern char* auto_gets(void);
extern int add_script(const char *path);

// ==================== Readline End =================

#endif
