#include "command.h"

static const char* extended_help [] = {
    "breakpoint options include:\n  break clear <addr>|all -- clear one or all breakpoints\n  break list             -- list all breakpoints\n  break set <addr>       -- set a breakpoint\n",

    "list options include:\n  list               -- list instructions around PC\n  list <addr>        -- list instructions starting from an address or label\n  list <addr> <addr> -- list a range of instructions\n",

    "dump options include:\n  dump               -- dump memory around PC\n  dump <addr>        -- dump memory starting from an address or label\n  dump <addr> <addr> -- dump a range of memory\n"
};

static const char *sim_commands[] = {
    "file",
    "break",
    "continue",
    "finish",
    "next",
    "step",
    "list",
    "dump",
    "translate",
    "printregs",
    "memory",
    "register",
    "execute",
    "reset",
    "quit",
    "help",
    NULL
};

static char combuf[BUFSIZ];
static int subroutine_level, interrupt_level;
static bool halted = false, stopped = false, quit = false;

static lc3_uint RAM[UINT16_MAX], bps_count = 0;
// breakpoints
static char lc3_bps[UINT16_MAX];
static lc3_uint reg_file[8];
static lc3_uint PC, PSR;
//static lc3_uint *KBDR, *KBSR, *DDR, *DSR;

static label_list_t *label_list = NULL;
static char last_open_file[BUFSIZ] = {0};

static void cmd_help (const char* input);
static void cmd_break (const char* input);
static void cmd_step (const char *input);
static void cmd_next (const char *input);
static void cmd_finish (const char *input);
static void cmd_continue (const char *input);
static void cmd_dump (const char* input);
static void cmd_execute (const char* input);
static void cmd_file (const char* input);
static void cmd_list (const char* input);
static void cmd_memory (const char* input);
static void cmd_printregs (const char* input);
static void cmd_quit (const char* input);
static void cmd_register (const char* input);
static void cmd_reset (const char* input);
static void cmd_translate (const char* input);
static void cmd_quit (const char* input);

typedef void (*sim_func_t)(const char *);
static sim_func_t sim_cmd_functions[] = {
    cmd_file,
    cmd_break,
    cmd_continue,
    cmd_finish,
    cmd_next,
    cmd_step,
    cmd_list,
    cmd_dump,
    cmd_translate,
    cmd_printregs,
    cmd_memory,
    cmd_register,
    cmd_execute,
    cmd_reset,
    cmd_quit,
    cmd_help,
    NULL
};


static int sim_cmd_locate(const char *cmd);
static void load_object (const char* path);
static lc3_uint parse_value(const char *input);
static lc3_uint parse_location(const char *input);
static const char* strsplit(const char *s);


// ==================== Bit Hacks Begin =================
// http://stackoverflow.com/a/263738/4725840
// His naming is better than mine

/* a=target variable, b=bit number to act upon 0-n */
#define BIT_SET(a, b) ((a) | (1u << (b)))
#define BIT_CLEAR(a, b) ((a) & ~(1u << (b)))
#define BIT_FLIP(a, b) ((a) ^ (1u << (b)))
#define BIT_GET(a, b) ((a) & (1u << (b)))

// a=target, b=end pos, c=start pos
#define MAX_NBITS(n) ((1u << (n)) - 1)
#define BIT_RANGE(a, b, c) ((a) & ((MAX_NBITS((b) + 1)) & ~(MAX_NBITS(c))))

/* x=target variable,  y=mask */
#define BITMASK_SET(x, y) ((x) | (y))
#define BITMASK_CLEAR(x, y) ((x) & (~(y)))
#define BITMASK_FLIP(x, y) ((x) ^ (y))
#define BITMASK_GET(x, y) ((x) & (y))

// ==================== Bit Hacks End =================



// ==================== Endian Related Begin =================

#include <endian.h>
#if BYTE_ORDER == LITTLE_ENDIAN
// LC3 register is 16-bit width
#define fixend(n) (be16toh(0xFFFF & (n)))
#else
#define fixend(n) (n)
#endif

// ==================== Endian Related End =================


#define valid_val(val) ((val) < 0x10000 || (val) > 0xFFFF7FFF)
#define inc(a) (((a) + 1) & 0xFFFF)
#define RAM_init() (bzero(RAM, sizeof(RAM)))
/*#define RAM_set(loc, val) (RAM[(loc)] = ((val) & 0xFFFF))*/
#define RAM_get(loc) (RAM[(loc) & 0xFFFF])
#define RAM_set(loc, val) (RAM_get(loc) = (val))


void lc3_run (void)
{
    char *cmd = NULL;
    int last_code = -1, code;
    while(! quit) {
        cmd = auto_gets();

        if (! cmd) break;

        if(*cmd) {
            sscanf(cmd, "%s", combuf);
            code = sim_cmd_locate(combuf);
        }
        else {
            code = last_code;
        }

        switch (code) {
            case 0 ... 15:
                sim_cmd_functions[code](cmd);
                last_code = code;
                break;
            default:
                printf("command %s not found\n", cmd);
                break;
        }
    }
}

void lc3_init_machine(const char *path)
{
    quit = halted = stopped= false;

    subroutine_level = 0;
    interrupt_level = 0;

    PC = 0;
    PSR = 0x4000;
    bps_count = 0;

    RAM_init();
    bzero(reg_file, sizeof(reg_file));
    bzero(lc3_bps, sizeof(lc3_bps));

    if(label_list) {
        destroy_label_list(label_list);
    }
    label_list = NULL;

    if(path) {
        (*path) ? load_object(path) : load_object(last_open_file);
    }
}

static bool lc3_legal_ir(lc3_uint instr)
{
    unsigned char opcode;
    opcode = BIT_RANGE(instr, 15, 12) >> 12;

    switch(opcode) {
        case 0x0:
            if(! (BIT_RANGE(instr, 11, 9) >> 9)) {
                return false;
            }
            break;

        case 0x1:
        case 0x5:
            if(! BIT_GET(instr, 5) && BIT_RANGE(instr, 5, 3)) {
                return false;;
            } 
            break;

        case 0x4:
        case 0xc:
            if(! BIT_GET(instr, 11) && (BIT_RANGE(instr, 5, 0) || BIT_RANGE(instr, 11, 9))) {
                return false;;
            } 
            break;

        case 0x2:
        case 0xa:
        case 0xe:
        case 0x3:
        case 0xb:

        case 0x6:
        case 0x7:
            break;

        case 0x9:
            if(BIT_RANGE(~BIT_RANGE(instr, 5, 0), 5, 0)) {
                return false;;
            } 
            break;

        case 0x8:
            if(BIT_RANGE(~BIT_RANGE(instr, 11, 0), 11, 0)) {
                return false;;
            } 
            break;

        case 0xf:
            if(BIT_RANGE(instr, 11, 8)) {
                return false;;
            } 
            switch (BIT_RANGE(instr, 7, 0)) {
                case 0x20 ... 0x25:
                    break;
                default:
                    return false;;
            }
            break;

        case 0xd:
        default:
            return false;;
    }
    return true;
}


static lc3_uint sign_extend(lc3_uint n, char width)
{
    lc3_uint res = UINT32_MAX;
    lc3_uint max, mask;
    switch (width) {
        case 5: case 6:
        case 9: case 11:
        case 16:
            max = (1u << width--);
            mask = (1u << width);
            break;
        default:
            return res;
    }
    if(n > max) return res;

    res = n;
    if(n & mask) {
        res |= ~(max - 1);
    }

    return res;
}


static int lc3_disassemble(lc3_uint vPC)
{
    unsigned char opcode, dr, sr1, sr2;

    vPC &= 0xFFFF;
    lc3_uint instr = RAM_get(vPC);
    opcode = BIT_RANGE(instr, 15, 12) >> 12;

    putchar((lc3_bps[vPC]) ? 'B' : ' ');

    char *p;
    p = loc2label(label_list, vPC);
    printf("%*s ", 20, (p) ? p : " ");

    printf("x%04X x%04X ", vPC, instr);

    if(! lc3_legal_ir(instr)) {
        if(isprint((int)instr)) {
            printf(".FILL '%c'\n", instr);
        }
        else {
            switch (instr) {
                case 0x7:
                    puts(".FILL '\\a'");
                    break;
                case 0x8:
                    puts(".FILL '\\b'");
                    break;
                case 0x9:
                    puts(".FILL '\\t'");
                    break;
                case 0xa:
                    puts(".FILL '\\n'");
                    break;
                case 0xb:
                    puts(".FILL '\\v'");
                    break;
                case 0xc:
                    puts(".FILL '\\f'");
                    break;
                case 0xd:
                    puts(".FILL '\\r'");
                    break;
                default:
                    printf(".FILL x%X\n", instr);
                    break;
            }
        }

        return -1;
    }

    dr = BIT_RANGE(instr, 11, 9) >> 9;
    sr1 = BIT_RANGE(instr, 8, 6) >> 6;
    sr2 = BIT_RANGE(instr, 2, 0);

    switch(opcode) {
        case 0x1:
            printf("ADD ");
            break;
        case 0x5:
            printf("AND ");
            break;
        case 0x0:
            printf("BR");
            break;
        case 0xc:
            if(sr1 == 7) {
                puts("RET");
                return 0;
            }
            printf("JMP ");
            break;
        case 0x4:
            // JSRR
            if(BIT_GET(instr, 11)) {
                printf("JSR ");
            }
            else {
                printf("JSRR ");
            }
            break;
        case 0x2:
            printf("LD ");
            break;
        case 0xa:
            printf("LDI ");
            break;
        case 0x6:
            printf("LDR ");
            break;
        case 0xe:
            printf("LEA ");
            break;
        case 0x9:
            printf("NOT ");
            break;
        case 0x8:
            puts("RTI");
            return 0;
        case 0x3:
            printf("ST ");
            break;
        case 0xb:
            printf("STI ");
            break;
        case 0x7:
            printf("STR ");
            break;
        // case 0xd:
        case 0xf:
            switch (BIT_RANGE(instr, 7, 0)) {
                case 0x20:
                    puts("GETC");
                    break;
                case 0x21:
                    puts("OUT");
                    break;
                case 0x22:
                    puts("PUTS");
                    break;
                case 0x23:
                    puts("IN");
                    break;
                case 0x24:
                    puts("PUTSP");
                    break;
                case 0x25:
                    puts("HALT");
                    break;
            }
            return 0;
    }

    lc3_uint offset6, PCoffset9, PCoffset11, imm5;
    imm5 = sign_extend(BIT_RANGE(instr, 4, 0), 5);
    offset6 = sign_extend(BIT_RANGE(instr, 5, 0), 6);
    PCoffset9 = sign_extend(BIT_RANGE(instr, 8, 0), 9);
    PCoffset11 = sign_extend(BIT_RANGE(instr, 10, 0), 11);

    switch(opcode) {
        case 0x0:
            if(BIT_GET(instr, 11)) putchar('n');
            if(BIT_GET(instr, 10)) putchar('z');
            if(BIT_GET(instr, 9)) putchar('p');
            p = loc2label(label_list, vPC + 1 + PCoffset9);
            if(p)
                printf(" %s", p);
            else
                printf(" #%d", PCoffset9);
            break;

        case 0x1:
        case 0x5:
            if(BIT_GET(instr, 5)) {
                printf("R%u, R%u, #%d", dr, sr1, imm5);
            }
            else {
                printf("R%u, R%u, R%u", dr, sr1, sr2);
            }
            break;

        case 0x4:
        case 0xc:
            if(BIT_GET(instr, 11)) {
                p = loc2label(label_list, vPC + 1 + PCoffset9);
                if(p)
                    printf(" %s", p);
                else
                    printf(" #%d", PCoffset11);
            }
            else {
                printf("R%u", sr1);
            }
            break;

        case 0x2:
        case 0xa:
        case 0xe:
        case 0x3:
        case 0xb:
            printf("R%u, ", dr);
            p = loc2label(label_list, vPC + 1 + PCoffset9);
            if(p)
                printf("%s", p);
            else
                printf("#%d", PCoffset9);
            break;

        case 0x6:
        case 0x7:
            printf("R%u, R%u, #%d", dr, sr1, offset6);
            break;

        case 0x9:
            printf("R%u, R%u", dr, sr1);
            break;

        // case 0xd:
        // case 0x8:
        // case 0xf:
    }
    putchar('\n');
    return 0;
}

static int sim_cmd_locate(const char *cmd)
{
    int i;
    if(! cmd) return -1;
    for (i = 0; sim_commands[i]; ++i)
    {
        if(strncasecmp(cmd, sim_commands[i], strlen(cmd)) == 0)
            return i;
    }
    return -1;
}

static void cmd_break (const char* input)
{
    if(! input) return;
    const char *p = strsplit(input);
    if(! p) {
        puts(extended_help[0]);
        return;
    }
    strcpy(combuf, p);
    strtok(combuf, DELIM);
    lc3_uint loc;
    if(strcmp(combuf, "list") == 0) {
        if(bps_count == 0)
            puts("No breakpoints are set.");
        else {
            lc3_uint i;
            puts("The following instructions are set as breakpoints:");
            for (i = 0; i < UINT16_MAX; i++) {
                if(lc3_bps[i])
                    lc3_disassemble(i);
            }
        }
    }
    else if(strcmp(combuf, "clear") == 0) {
        p = strtok(NULL, DELIM);
        if(strcasecmp(p, "all") == 0) {
            bps_count = 0;
            bzero(lc3_bps, sizeof(lc3_bps));
            puts("Cleared all breakpoints.");
        }
        else {
            loc = parse_location(p);
            if(loc != lc3_ERR) {
                if(lc3_bps[loc]) {
                    printf("Cleared breakpoint at x%04X\n", loc & 0xFFFF);
                    lc3_bps[loc] = 0;
                    --bps_count;
                }
                else {
                    puts("No such breakpoint was set.");
                }
            }
            else
                printf("%s\n", extended_help[0]);
        }
    }
    else if(strcmp(combuf, "set") == 0) {
        p = strtok(NULL, DELIM);
        loc = parse_location(p);
        if(loc != lc3_ERR) {
            if(lc3_bps[loc])
                puts("That breakpoint is already set.");
            else {
                printf("Set breakpoint at x%04X\n", loc);
                lc3_bps[loc] = 1;
                ++bps_count;
            }
        }
        else
            printf("%s\n", extended_help[0]);
    }
    else {
        printf("%s", extended_help[0]);
    }
}

// @return:
// 0    success
// 1    halted
// 2    illegal
static int lc3_execute(void)
{
    if(halted) {
        return 1;
    }

    lc3_uint IR = RAM_get(PC);
    if(! lc3_legal_ir(IR)) {
        return 2;
    }

    PC = inc(PC);
    int c;

    lc3_uint offset6, PCoffset9, PCoffset11, imm5, res, addr;
    unsigned char opcode, dr, sr1, sr2;
    dr = BIT_RANGE(IR, 11, 9) >> 9;
    sr1 = BIT_RANGE(IR, 8, 6) >> 6;
    sr2 = BIT_RANGE(IR, 2, 0);

    imm5 = sign_extend(BIT_RANGE(IR, 4, 0), 5);
    offset6 = sign_extend(BIT_RANGE(IR, 5, 0), 6);
    PCoffset9 = sign_extend(BIT_RANGE(IR, 8, 0), 9);
    PCoffset11 = sign_extend(BIT_RANGE(IR, 10, 0), 11);

    res = lc3_ERR;

    opcode = BIT_RANGE(IR, 15, 12) >> 12;
    switch(opcode) {
        // BR
        case 0x0:
            if((BIT_GET(IR, 11) && BIT_GET(PSR, 2)) || \
               (BIT_GET(IR, 10) && BIT_GET(PSR, 1)) || \
               (BIT_GET(IR, 9) && BIT_GET(PSR, 0)))
                PC += PCoffset9;
            break;
        // ADD
        case 0x1:
            if(BIT_GET(IR, 5)) {
                res = reg_file[dr] = reg_file[sr1] + imm5;
            }
            else {
                res = reg_file[dr] = reg_file[sr1] + reg_file[sr2];
            }
            break;
        // AND
        case 0x5:
            if(BIT_GET(IR, 5)) {
                res = reg_file[dr] = reg_file[sr1] & imm5;
            }
            else {
                res = reg_file[dr] = reg_file[sr1] & reg_file[sr2];
            }
            break;
        // LD
        case 0x2:
            res = reg_file[dr] = RAM_get(PC + PCoffset9);
            break;
        // ST
        case 0x3:
            res = RAM_set(PC + PCoffset9, reg_file[dr]);
            break;
        // JSR
        case 0x4:
            if(BIT_GET(IR, 11)) {
                reg_file[7] = PC;
                PC += PCoffset11;
            }
            else {
                // JSRR
                PCoffset11 = PC; // tmp
                PC = reg_file[sr1];
                reg_file[7] = PCoffset11;
            }
            ++subroutine_level;
            break;
        // LDR
        case 0x6:
            res = reg_file[dr] = RAM_get(reg_file[sr1] + offset6);
            break;
        // STR
        case 0x7:
            res = RAM_set(reg_file[sr1] + offset6, reg_file[dr]);
            break;
        // RTI
        case 0x8:
            break;
        // NOT
        case 0x9:
            res = reg_file[dr] = ~reg_file[sr1];
            break;
        // LDI
        case 0xa:
            res = reg_file[dr] = RAM_get(RAM_get(PC + PCoffset9));
            break;
        // STI
        case 0xb:
            res = RAM_set(RAM_get(PC + PCoffset9), reg_file[dr]);
            break;
        // JMP
        case 0xc:
            PC = reg_file[sr1];
            if(sr1 == 7) {
                --subroutine_level;
            }
            break;
        // not used
        case 0xd:
            break;
        // LEA
        case 0xe:
            res = reg_file[dr] = PC + PCoffset9;
            break;
        // TRAP
        case 0xf:
            switch (BIT_RANGE(IR, 7, 0)) {
                case 0x20:
                    c = getchar();
                    reg_file[0] = c & 0xFF;
                    break;
                case 0x21:
                    c = reg_file[0] & 0xFF;
                    putchar(c);
                    fflush(stdout);
                    break;
                case 0x22:
                    addr = reg_file[0];
                    while(RAM_get(addr) != 0) {
                        putchar(0xFF & RAM_get(addr));
                        fflush(stdout);
                        ++addr;
                    }
                    break;
                case 0x23:
                case 0x24:
                    break;
                case 0x25:
                    halted = true;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }
    if (res != lc3_ERR) {
        res &= 0xFFFF;
        // set CC
        PSR = 0;
        if(BIT_GET(res, 15))
            PSR = BIT_SET(PSR, 2);
        else if (res == 0)
            PSR = BIT_SET(PSR, 1);
        else
            PSR = BIT_SET(PSR, 0);

        PSR |= 0x4000;
    }

    return 0;
}

static int try_execute(void)
{
    switch (lc3_execute()) {
        case 0:
            break;
        case 1:
            puts("\n\n======== LC3 is halted =========\n\n");
            return 1;
        case 2:
            printf("Illegal instruction at x%04X!\n", PC);
            return 2;
    }
    return 0;
}

static void cmd_step (const char *input)
{
    try_execute();
    cmd_printregs(NULL);
    lc3_disassemble(PC);
}

static void step_until_dest_level(int dest_lv)
{
    do{
        if(try_execute() != 0)
            break;
    } while(! halted && ! stopped && dest_lv != subroutine_level);
}

static void step_until_breakpoint(void)
{
    do{
        if(try_execute() != 0)
            break;
    } while(! halted && ! stopped && ! lc3_bps[PC]);
}

static void cmd_next (const char *input)
{
    step_until_dest_level(subroutine_level);
    cmd_printregs(NULL);
    lc3_disassemble(PC);
}

static void cmd_finish (const char *input)
{
    step_until_dest_level(subroutine_level - 1);
    cmd_printregs(NULL);
    lc3_disassemble(PC);
}

static void cmd_continue (const char *input)
{
    step_until_breakpoint();
    cmd_printregs(NULL);
    lc3_disassemble(PC);
}

static void load_object (const char* path)
{
    FILE *obj_file;
    obj_file = fopen(path, "r");
    if(! obj_file) {
        printf("No such file: %s\n", path);
        return;
    }
    strcpy(last_open_file, path);
    static char buf[128];
    strncpy(buf, path, 128);
    strcpy(strrchr(buf, '.'), ".sym");
    switch (parse_labels_from_file(buf, &label_list)) {
        case 0:
            break;
        case 1:
            fprintf(stderr, "%s not found...\n", buf);
            break;
        case 2:
            fprintf(stderr, "Failed to alloc memory...\n");
            return;
        default:
            fprintf(stderr, "Error occured...\n");
            return;
    }

    lc3_uint init_loc;
    lc3_uint instr;

    if(fread(&instr, sizeof(uint16_t), 1, obj_file) != 1) {
        fprintf(stderr, "The object file seemed to be empty\n");
        return;
    }

    init_loc = PC = fixend(instr);

    for(; fread(&instr, sizeof(uint16_t), 1, obj_file) == 1; PC++) 
    {
        RAM_set(PC, fixend(instr));
    }

    PC = init_loc;
    halted = false;
    cmd_printregs(NULL);
    lc3_disassemble(init_loc);
    printf("Loaded \"%s\" and set PC to x%04X\n", path, PC);
}

static void cmd_file (const char* input)
{
    if(! input) {
        return;
    }

    if(*input) {
        if(sscanf(input, "%*s%100s", combuf) != 1) {
            puts("syntax: file <file to load>");
            return;
        }
        else {
            load_object(combuf);
            return;
        }
    }

    load_object(last_open_file);
}

static void cmd_printregs(const char *input)
{
    static const char *CC_code[] = {
        "?", "P", "Z", "ZP", "N", "NP", "NZ", "NZP"
    };
    printf("PC=x%04X\tIR=x%04X\tPSR=x%04X\tCC: %s\n", 0xFFFF & PC,
           RAM_get(PC), 0xFFFF & PSR, 
           CC_code[BIT_RANGE(PSR, 2, 0)]);

    int i = 0;
    printf("R%d=x%04X", i, 0xFFFF & reg_file[i]);
    for (i = 1; i < 8; i++) {
        printf("  R%d=x%04X", i, 0xFFFF & reg_file[i]);
    }
    putchar('\n');
}

static void cmd_reset(const char *input)
{
    *combuf = 0;
    lc3_init_machine(combuf);
}

static lc3_uint parse_value(const char *input)
{
    static char _arg[128];
    if(sscanf(input, "%127s", _arg) != 1) {
        return lc3_ERR;
    }

    lc3_uint val;
    if( (*_arg == 'x' && (sscanf(_arg + 1, "%x", &val) == 1) && valid_val(val)) || \
        (*_arg == '#' && (sscanf(_arg + 1, "%u", &val) == 1) && valid_val(val)) || \
        ((sscanf(_arg, "%u", &val) == 1) && valid_val(val))) {
        return val;
    }
    return lc3_ERR;
}

static lc3_uint parse_location(const char *input)
{
    static char _arg[128];
    if(sscanf(input, "%127s", _arg) != 1) {
        return lc3_ERR;
    }
    
    lc3_uint loc;
    if((loc = label2loc(label_list, _arg)) != lc3_ERR || \
            (((loc = parse_value(_arg)) != lc3_ERR) && loc < 0x10000))
    {
        return loc;
    }

    return lc3_ERR;
}

const char* strsplit(const char *s)
{
    if(! s) return NULL;
    const char *p;
    (p = strchr(s, ' ')) || ( p = strchr(s, '\t'));
    if(! p) return NULL;

    while(*p && isspace(*p)) ++p;

    if(! *p) return NULL;
    return p;
}

static void cmd_register(const char *input)
{
    if(! input) return;

    const char *t = NULL, *t2;
    bool err = false;
    lc3_uint *p = NULL, n = 0, val = 0;

    if(sscanf(input, "%*s%s", combuf) != 1) {
        puts("syntax: register <reg> <value>");
        return;
    }

    if(strcmp(combuf, "PC") == 0) {
        p = &PC;
        t = "PC";
    }
    else if(strcmp(combuf, "PSR") == 0) {
        p = &PSR;
        t = "PSR";
    }
    else if(*combuf == 'R' || *combuf == 'r') {
        sscanf(combuf + 1, "%d", &n);
        if(n > 7)
            err = true;
        else {
            p = reg_file + n;
            t = "R";
        }
    }
    else {
        err = true;
    }

    if(! err) {
        t2 = strsplit(strsplit(input));
        val = parse_value(t2);
        if(val != lc3_ERR)
            *p = val;
        else
            err = true;
    }

    if(! err) {
        printf("Set %s", t);
        if(*t == 'R') printf("%d", n);
        printf(" to x%04X\n", val & 0xFFFF);
    }
    else {
        puts("syntax: register <reg> <value>");
    }
}

static void cmd_dump (const char* input)
{
    lc3_uint loc = 0, end = 0;
    static lc3_uint last_loc = 0;

    if(! input) {
        last_loc = 0;
        return;
    }

    const char *s1, *s2;
    if(! *input) {
        loc = last_loc;
        end = loc + 10;
    }
    else {
        s1 = strsplit(input);
        loc = (s1) ? parse_location(s1) : PC;
        if(loc == lc3_ERR) {
            printf("%s\n", extended_help[2]);
            return;
        }

        s2 = strsplit(s1);
        end = (s2) ? parse_location(s2) : (loc + 10);
        if(end == lc3_ERR)
        {
            printf("%s\n", extended_help[2]);
            return;
        }
    }

    for(end = inc(end), last_loc = end;
        loc != end;
        loc = inc(loc))
    {
        printf("x%04X: x%04X\n", loc, RAM_get(loc) & 0xFFFF);
    }
}

static void cmd_list (const char *input)
{
    lc3_uint loc = 0, end = 0;
    static lc3_uint last_loc = 0;

    if(! input) {
        last_loc = 0;
        return;
    }

    const char *s1, *s2;
    if(! *input) {
        loc = last_loc;
        end = loc + 10;
    }
    else {
        s1 = strsplit(input);
        loc = (s1) ? parse_location(s1) : PC;
        if(loc == lc3_ERR) {
            printf("%s\n", extended_help[1]);
            return;
        }

        s2 = strsplit(s1);
        end = (s2) ? parse_location(s2) : (loc + 10);
        if(end == lc3_ERR)
        {
            printf("%s\n", extended_help[1]);
            return;
        }
    }

    for(end = inc(end), last_loc = end;
        loc != end;
        loc = inc(loc))
    {
        lc3_disassemble(loc);
    }
}

static void cmd_memory (const char *input)
{
    if(! input) return;

    lc3_uint loc, val = 0;
    const char *arg1, *arg2;
    arg1 = strsplit(input);
    arg2 = strsplit(arg1);
    if(!arg1 || !arg2) {
        puts("syntax: memory <addr> <value>");
        return;
    }
    loc = parse_location(arg1);
    val = parse_value(arg2);
    if(loc == lc3_ERR || val == lc3_ERR) {
        puts("syntax: memory <addr> <value>");
        return;
    }
    RAM_set(loc, val);
    printf("Wrote x%04X to address x%04x\n", loc, val & 0xFFFF);
}

static void cmd_translate (const char* input)
{
    sscanf(input, "%*s%s", combuf);
    lc3_uint loc = label2loc(label_list, combuf), val = 0;
    if(loc != lc3_ERR || \
            (*combuf == 'x' && (sscanf(combuf + 1, "%x", &loc) == 1) && loc <= 0xFFFF)) {
        val = RAM_get(loc);
        printf("Address x%04X has value x%04X\n", loc, val & 0xFFFF);
    }
    else
        puts("Addresses must be labels or values in the range x0000 to xFFFF.");
}

static void cmd_execute (const char* input)
{
    const char *path = strsplit(input);
    set_script_path(path);
}

static void cmd_quit (const char* input)
{
    quit = true;
}

static void cmd_help (const char* input)
{
    if(! input) return;

    const char *help_msg[] = {
        "file <file>           -- file load (also sets PC to start of file)\n",
        "break ...             -- breakpoint management\n",
        "continue              -- continue execution\n",
        "finish                -- execute to end of current subroutine\n",
        "next                  -- execute next instruction (full subroutine/trap)\n",
        "step                  -- execute one step (into subroutine/trap)\n",
        "list ...              -- list instructions at the PC, an address, a label\n",
        "dump ...              -- dump memory at the PC, an address, a label\n",
        "translate <addr>      -- show the value of a label and print the contents\n",
        "printregs             -- print registers and current instruction\n",
        "memory <addr> <val>   -- set the value held in a memory location\n",
        "register <reg> <val>  -- set a register to a value\n",
        "execute <file name>   -- execute a script file\n",
        "reset                 -- reset LC-3 and reload last file\n",
        "quit                  -- quit the simulator\n",
        "help                  -- print this help\n",
        "All commands except quit can be abbreviated.",
        NULL
    };
    int i, code;
    if(! *input) {
        for (i = 0; help_msg[i]; i++) {
            puts(help_msg[i]);
        }
    }
    else {
        code = sim_cmd_locate(strsplit(input));
        switch (code) {
            case 0 ... 15:
                puts(help_msg[code]);
                switch(code) {
                    case 1:
                        puts(extended_help[0]);
                        break;
                    case 6:
                        puts(extended_help[1]);
                        break;
                    case 7:
                        puts(extended_help[2]);
                        break;
                }
                break;
            default:
                for (i = 0; help_msg[i]; i++) {
                    puts(help_msg[i]);
                }
        }
    }
}
