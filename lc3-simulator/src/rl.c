#include "rl.h"
#define PROMPT ">> "

// http://web.mit.edu/gnu/doc/html/rlman_2.html
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

char *lstrip(char *s)
{
    if(! s) return NULL;
    while(*s && isspace(*s)) ++s;
    return s;
}

char* trimspace(char *s)
{
    if(! s) return NULL;
    char *end;
    //s = lstrip(s);
    while(*s && isspace(*s)) ++s;
    end = s + strlen(s) - 1;
    while((end > s) && isspace(*end)) --end;
    *(end + 1) = 0;
    return s;
}

#define SCRIPT_STACK_SIZE 19
static int st_ptr = 0;
static FILE *script_stack[SCRIPT_STACK_SIZE] = {0};
static FILE *script_file = NULL;
static char input_buf[BUFSIZ];

int add_script(const char *path)
{
    // 0: Success
    // 1: Stack full
    // 2: No such file
    if(st_ptr == SCRIPT_STACK_SIZE) {
        // clear stack
        while(st_ptr-- > 0) fclose(script_stack[st_ptr]);
        st_ptr = 0;
        fputs("Cannot execute more than 20 levels of scripts!\n", stderr);
        return 1;
    }

    FILE *p = fopen(path, "r");

    if(! p) {
        fprintf(stderr, "No such file: %s\n", path);
        return 2;
    }
    else if(script_file) {
        script_stack[st_ptr++] = script_file;
    }
    script_file = p;
    return 0;
}

char* auto_gets()
{
    if(! script_file) {
        return trimspace(rl_gets());
    }
    else if(! fgets(input_buf, BUFSIZ, script_file)) {
        while(st_ptr > 0) {
            script_file = script_stack[--st_ptr];
            if(fgets(input_buf, BUFSIZ, script_file))
                return trimspace(input_buf);
            else
                fclose(script_file);
        }
        script_file = NULL;
        return trimspace(rl_gets());
    }
    else {
        return trimspace(input_buf);
    }
}

