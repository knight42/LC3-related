#ifndef LC3_RL_H
#define LC3_RL_H

#include "rl.h"
#define PROMPT ">> "

static char *line_read = (char *)NULL;

/* Read a string, and return a pointer to it.  Returns NULL on EOF. */
char * rl_gets ()
{
    /* If the buffer has already been allocated, return the memory
       to the free pool. */
    if (line_read)
    {
        free (line_read);
        line_read = (char *)NULL;
    }

    /* Get a line from the user. */
    line_read = readline (PROMPT);

    /* If the line has any text in it, save it on the history. */
    if (line_read && *line_read)
        add_history (line_read);

    return (line_read);
}

#endif
