#include <stdint.h>

#define NUM_OPS 29

#define  OP_ADD 1
#define  OP_AND 2
#define  OP_BR  3
#define  OP_JMP 4
#define  OP_JSR 5
#define  OP_JSRR 6
#define  OP_LD 7
#define  OP_LDI 8
#define  OP_LDR 9
#define  OP_LEA 10
#define  OP_NOT 11
#define  OP_RTI 12
#define  OP_ST 13
#define  OP_STI 14
#define  OP_STR 15
#define  OP_TRAP 16

#define  TRAP_GETC 17
#define  TRAP_HALT 18
#define  TRAP_IN 19
#define  TRAP_OUT 20
#define  TRAP_PUTS 21
#define  TRAP_PUTSP 22

#define  DIRE_FILL 23
#define  OP_RET 24
#define  DIRE_STRING 25

#define  DIRE_BLKW 26
#define  DIRE_END 27
#define  DIRE_ORIG 28

#define LABEL_MLEN 30


static const char* const opnames[NUM_OPS] = {
    /* no opcode seen (yet) */
    "missing opcode",

    /* real instruction opcodes */
    "ADD", "AND", "BR", "JMP", "JSR", "JSRR", "LD", "LDI", "LDR", "LEA",
    "NOT", "RTI", "ST", "STI", "STR", "TRAP",

    /* trap pseudo-ops */
    "GETC", "HALT", "IN", "OUT", "PUTS", "PUTSP",

    /* non-trap pseudo-ops */
    ".FILL", "RET", ".STRINGZ",

    /* directives */
    ".BLKW", ".END", ".ORIG"
};

typedef struct Label {
    char name[21];
    int16_t location;
    struct Label *next;
} label_t;

typedef struct label_list {
    int length;
    label_t *first;
} label_list_t;
