#include "label.h"

label_t* new_label(const char *name, lc3_uint loc)
{
    label_t *p = (label_t*)calloc(1, sizeof(label_t));
    if(! p) return NULL;

    strncpy(p->name, name, LABEL_MAX_LEN);
    p->location = loc;
    return p;
}

int destroy_label_list(label_list_t *list)
{
    if(! list) return 1;
    label_t *p, *h;
    for(p = list->first; p; p = h)
    {
        h = p->next;
        free(p);
    }
    free(list);
    return 0;
}

label_list_t* new_label_list(void)
{
    return (label_list_t*)calloc(1, sizeof(struct label_list));
}

int insert_label(label_list_t *list, const char *name, lc3_uint loc)
{
    if(!list || !name) return 2;
    label_t **p;
    for(p = &list->first; *p; p = &(*p)->next) {
        if(strncasecmp((*p)->name, name, LABEL_MAX_LEN) == 0)
            return 1;
    }
    *p = new_label(name, loc);
    return 0;
}

lc3_uint label2loc(label_list_t *list, const char *name)
{
    if(!list || !name) return lc3_ERR;
    label_t *p;
    for(p = list->first; p; p = p->next) {
        if(strncasecmp(p->name, name, LABEL_MAX_LEN) == 0)
            return p->location;
    }
    return lc3_ERR;
}

char* loc2label(label_list_t *list, lc3_uint loc)
{
    if(!list) return NULL;
    label_t *p;
    for(p = list->first; p; p = p->next) {
        if(p->location == loc)
            return p->name;
    }
    return NULL;
}

int parse_labels_from_file(const char *path, label_list_t **_list)
{
    if(! path || ! _list) return 3;

    FILE *sym_file;
    sym_file = fopen(path, "r");
    if(! sym_file) return 1;

    if(! *_list) {
        *_list = new_label_list();;
    }
    #ifdef DEBUG
    printf("%s\n", path);
    #endif
    fflush(stdout);
    label_list_t *list = *_list;
    lc3_uint loc;
    int ret;
    char *p;
    char lname[LABEL_MAX_LEN * 2];
    char *buf = (char*)malloc(BUFSIZ * sizeof(char));
    if(! buf) return 2;

    // skip 4 lines
    for (loc = 0; loc < 4; loc++) {
        fgets(buf, BUFSIZ, sym_file);
    }

    while(fgets(buf, BUFSIZ, sym_file)) {
        p = buf + strspn(buf, "/" DELIM);
        if(! *p) continue;

        // parse label
        sscanf(p, "%30s", lname);
        strtok(p, DELIM);
        p = strtok(NULL, DELIM);
        // parse address
        sscanf(p, "%x", &loc);
        #ifdef DEBUG
        printf("%s, %04X\n", lname, loc);
        #endif
        if ((ret = insert_label(list, lname, loc)) != 0)
        {
            switch (ret) {
                case 1:
                    #ifdef DEBUG
                    fprintf(stderr, "%s already exists, ignored.\n", lname);
                    #endif
                    break;
                case 2:
                    fprintf(stderr, "Invaild params.\n");
                    return 3;
            }
        }
    }
    free(buf);
    return 0;
}
