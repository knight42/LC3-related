#ifndef LC3_LABEL_H
#define LC3_LABEL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define DELIM " \t\v\f\r\n"

#define LABEL_MAX_LEN 30
typedef struct Label {
    char name[LABEL_MAX_LEN + 1];
    lc3_uint location;
    struct Label *next;
} label_t;

typedef struct label_list {
    label_t *first;
} label_list_t;


extern label_t* new_label(const char *name, lc3_uint loc) ;
// return values:
// label_t* success
// NULL     failed


extern label_list_t* new_label_list(void) ;
// return values:
// label_list_t*    success
// NULL             failed



extern int insert_label(label_list_t *list, const char *name, lc3_uint loc) ;
// return values:
// 0  success
// 1  already exists
// 2  invalid args


extern lc3_uint label2loc(label_list_t *list, const char *name) ;
// return values:
// location             success
// lc3_ERR(UINT32_MAX)  not found / illegal args



extern char* loc2label(label_list_t *list, lc3_uint loc) ;
// return values:
// label name         success
// NULL               not found / illegal args


extern int destroy_label_list(label_list_t *list) ;
// return values:
// 0         success
// 1         illegal param

extern int parse_labels_from_file(const char *path, label_list_t **_list) ;
// return values:
// 0                success
// 1                no such file
// 2                failed to alloc memory
// 3                illegal params

#endif
