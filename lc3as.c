#include "lc3as.h"

#define diehere(format, ...) fprintf(stderr, "%s: " format, __FUNCTION__, __VA_ARGS__)

label_t* new_label(const char *name, int16_t loc)
{
    label_t *p = (label_t*)calloc(1, sizeof(label_t));
    strcpy(p->name, name);
    p->location = loc;
    return p;
}
label_list_t* new_label_list(void)
{
    struct label_list *list = (struct label_list*)calloc(1, sizeof(struct label_list));
    return list;
}
int insert_label(FILE *sym, label_list_t *list, const char *name, int16_t loc)
{
    if(!list || !name) return -1;
    label_t **p;
    for(p = &list->first; *p; p = &(*p)->next) {
        if(strncasecmp((*p)->name, name, LABEL_MLEN) == 0) return 1;
    }
    *p = new_label(name, loc);
    fprintf(sym, "//\t%-18s%04hX\n", name, loc);
    return 0;
}

int lc3as_locate(const char *s)
{
    int i;
    size_t l1, l2;
    l1 = strlen(s);
    if(strncasecmp(s, "BR", 2) == 0 && l1 < 6)
        return OP_BR;

    for (i = 1; i < NUM_OPS; ++i)
    {
        l2 = strlen(opnames[i]);
        if((l1 == l2 && strncasecmp(s, opnames[i], l2) == 0))
            return i;
    }
    return 0;
}

int16_t label2loc(label_list_t *list, const char *name)
{
    label_t *p;
    for(p = list->first; p; p = p->next) {
        if(strncasecmp(p->name, name, LABEL_MLEN) == 0)
            return p->location;
    }
    return -1;
}

bool lc3as_label_valid(const char *s){
    if(strlen(s) > LABEL_MLEN) return false;
    if(isalpha(*s++)) {
        while(*s && (isalpha(*s) || isdigit(*s) || *s == '_')) ++s;
        if(*s) return false;
        else return true;
    }
    return false;
}

int16_t parse_int16(const char *s)
{
    int16_t n = -1;
    if(! s) return -1;
    switch (*s) {
        case 'r':
        case 'R':
        case '#':
            sscanf(s + 1, "%hd", &n);
            break;
        case 'x':
            sscanf(s + 1, "%hx", &n);
            break;
        case '0' ... '9':
            sscanf(s, "%hd", &n);
    }
    return n;
}

bool is_valid_reg(const char *s)
{
    return (s && strlen(s) == 2 && (*s == 'r' || *s == 'R') && (*(s + 1) >= '0' && *(s + 1) <= '7'));
}

int  parse_strz(const char *s)
{
    if(!s) return -1;
    const char *p = s + strlen(s) - 1;
    if(*s++ != '"' || *p != '"') return 1;
    while(s != p) {
        if(*s == '"' && *(s - 1) != '\\')
            return 1;
        ++s;
    }
    return 0;
}

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

void dump_ir(FILE *obj, int16_t ir)
{
    if(obj) {
        ir = ntohs(ir);
        fwrite(&ir, sizeof(int16_t), 1, obj);
    }
}
