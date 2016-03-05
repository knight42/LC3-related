#include <stdio.h>
#include <string.h>
#include "lc3as.h"

void manual(const char *prog) {
    puts("Usage:");
    printf("%s <source>.asm\n", prog);
}

int main(int argc, char *argv[]){
    switch (argc) {
        case 1:
            puts("Please tell me the path of your asm file.\nExit...");
            manual(argv[0]);
            return 1;
        case 2:
            break;
        default:
            puts("Too much arguments!\nExit...");
            manual(argv[0]);
            return 2;
    }
    FILE *asm_file = NULL;
    FILE *sym_file = NULL;
    FILE *obj_file = NULL;
    char filepath[BUFSIZ];
    char *ext;

    strncpy(filepath, argv[1], BUFSIZ);

    if ((ext = strrchr (filepath, '.')) == NULL) {
        ext = filepath + strlen(argv[1]);
    }

    asm_file = fopen(filepath, "r");
    if(! asm_file) {
        perror(argv[1]);
        puts("Exit...");
        return 1;
    }

    /* Open output files. */
    strcpy (ext, ".obj");
    if ((obj_file = fopen (filepath, "w")) == NULL) {
        fprintf (stderr, "Could not open %s for writing.\n", filepath);
        return 2;
    }

    strcpy(ext, ".sym");
    if ((sym_file = fopen (filepath, "w")) == NULL) {
        fprintf (stderr, "Could not open %s for writing.\n", filepath);
        return 2;
    }

    label_list_t *table;

    // first pass
    table = gen_symbols(asm_file, sym_file);
    puts("Symbols have been generated.");

    fclose(sym_file);
    fclose(asm_file);

    // second pass
    gen_obj(table, obj_file);
    fclose(obj_file);
    puts("Object file has been generated.");

    free(table);
    return 0;
}
