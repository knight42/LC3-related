#include <string.h>
#include <ctype.h>

char* strtrim(char *s)
{
    if(!s) return NULL;
    char *p;
    s += strspn(s, " \f\n\r\t\v");
    if(*s == ';') {
        *s = 0; return s;
    }
    p = strrchr(s, ';');
    if (!p) {
        if(! (p = strrchr(s, '\n'))) {
            p = strrchr(s, '\0');
        }
        *p = 0;
    }
    while(isspace(*--p));
    *(p + 1) = 0;
    return s;
}
