#include "lc3as.h"

#define diehere(format, ...) fprintf(stderr, "[line: %d]: " format, __VA_ARGS__)
#define log(format, ...) fprintf(stderr, "%s: " format, __FUNCTION__, __VA_ARGS__)

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
    fprintf(sym, "//\t%-16s  %04hX\n", name, loc);
    return 0;
}

int lc3as_locate(const char *s)
{
    int i;
    size_t l1, l2;
    if(!s || !*s) return 0;

    l1 = strlen(s);
    if(strncasecmp(s, "BR", 2) == 0) {
        s += 2;
        if(tolower(*s) == 'n') ++s;
        if(tolower(*s) == 'z') ++s;
        if(tolower(*s) == 'p') ++s;
        if(! *s) return OP_BR;
        else return 0;
    }

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

int32_t parse_int32(const char *s)
{
    int32_t n = INT32_MAX;
    if(! s) return n;
    switch (*s) {
        case 'r':
        case 'R':
        case '#':
            sscanf(s + 1, "%d", &n);
            break;
        case 'x':
            sscanf(s + 1, "%x", &n);
            break;
        case '-':
        case '0' ... '9':
            sscanf(s, "%d", &n);
    }
    return n;
}

int16_t parse_int16(const char *s)
{
    int16_t n = INT16_MAX;
    if(! s) return n;
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

bool is_valid_label(const char *s)
{
    if(strlen(s) > LABEL_MLEN) return false;
    if(isalpha(*s++)) {
        while(*s && (isalpha(*s) || isdigit(*s) || *s == '_')) ++s;
        if(*s) return false;
        else return true;
    }
    return false;
}

bool is_valid_reg(const char *s)
{
    return (s && strlen(s) == 2 && (*s == 'r' || *s == 'R') && (*(s + 1) >= '0' && *(s + 1) < '8'));
}

bool is_valid_strz(const char *s)
{
    if(!s) return false;
    const char *p = s + strlen(s) - 1;
    if(*s++ != '"' || *p != '"') return false;
    while(s != p) {
        if(*s == '"' && *(s - 1) != '\\')
            return false;
        ++s;
    }
    return true;
}

char* strtrim(char *s)
{
    if(!*s || !s) return s;
    char *p, *q;
    s += strspn(s, " \f\n\r\t\v");
    if(*s == ';') {
        *s = 0; return s;
    }

    p = strchr(s, '"');
    q = strchr(s, ';');
/*
 *    if(! p) {
 *        if(! q) q = s + strlen(s);
 *
 *        while(isspace(*--q));
 *        *(1 + q) = 0;
 *    }
 *    else if(q) {
 *        if(q < p) {
 *            while(isspace(*--q));
 *            *(1 + q) = 0;
 *        }
 *        else if((p = strchr(p + 1, '"'))){
 *            *(1 + p) = 0;
 *        }
 *    }
 *    else {
 *        q = s + strlen(s);
 *        while(isspace(*--q));
 *        *(1 + q) = 0;
 *    }
 */

    if(! q) {
        q = s + strlen(s);
    }
    else if(p && q > p && (p = strchr(p + 1, '"'))) {
        *(1 + p) = 0;
        return s;
    }
    while(isspace(*--q));
    *(1 + q) = 0;
    return s;
}

void dump_ir(FILE *obj, uint16_t ir)
{
    if(obj) {
        ir = ntohs(ir);
        fwrite(&ir, sizeof(uint16_t), 1, obj);
    }
    else {
        log("%s\n", "Cannot open file to write.");
        exit(1);
    }
}

int16_t get_offset(label_list_t *table, int prog_loc, int lineno, const char *s, char width)
{
    int16_t offset = INT16_MAX;
    int16_t max, min, mask;
    switch (width--) {
        case 6:
        case 9:
        case 11:
            min = - (int16_t)(1 << width);
            max = (int16_t)(1 << width) - 1;
            mask = max | (int16_t)(1 << width);
            break;
        default:
            return offset;
    }
    static char fmt[100];
    sprintf(fmt, "[line: %%d]: Offset out of range: x%%0%dhX\n", width + 1);

    offset = label2loc(table, s);
    if(offset != -1) {
        offset -= prog_loc;
    }
    else if((offset = parse_int16(s)) == INT16_MAX) {
        diehere("Cannot find label: %s\n", lineno, s);
        exit(1);
    }

    if(offset > max || offset < min){
        fprintf(stderr, fmt, lineno, offset);
        exit(1);
    }

    return (mask & offset);
}

static FILE *stripped;
#define _tmpfile "__tmp__XXXXXXX.txt"

label_list_t* gen_symbols(FILE *in, FILE *sym_file)
{
    static char buf[BUFSIZ], opcode[30], operand[BUFSIZ];
    char *beg, *arg1, *arg2;
    int16_t prog_loc = 0x3000, imm;
    int lineno = 1;
    int read, code, ret;
    bool saw_end = false, saw_orig = false;

    fputs("// Symbol table\n", sym_file);
    fputs("// Scope level 0:\n", sym_file);
    fputs("//\tSymbol Name       Page Address\n", sym_file);
    fputs("//\t----------------  ------------\n", sym_file);

    stripped = fopen(_tmpfile, "w+");
    label_list_t *table = new_label_list();
    for(; !saw_end && fgets(buf, BUFSIZ, in); ++lineno) {
        beg = strtrim(buf);

        if(! *beg) {
            fputc('\n', stripped);
            continue;
        }

        sscanf(beg, "%s%n", opcode, &read);
        code = lc3as_locate(opcode);
        if(! saw_orig) {
            if(code == DIRE_ORIG) {
                beg += read;
                saw_orig = true;
                sscanf(beg, "%s%n", operand, &read);
                ret = parse_int32(operand);
                if(ret == INT32_MAX) {
                    diehere("Invalid operand for .ORIG\n", lineno);
                    exit(1);
                }
                else if(ret > 0xFFFF || ret < 0) {
                    diehere("Out of range: %s\n", lineno, operand);
                    exit(2);
                }
                table->init_loc = prog_loc = (int16_t)(0xFFFF & ret);
            }
            else {
                diehere("Invalid text before .ORIG\n", lineno);
                exit(2);
            }
        }
        else if(! saw_end) {
            ret = 0;
            while(code == 0) {
                if(is_valid_label(opcode)) {
                    ret = insert_label(sym_file, table, opcode, prog_loc);
                    if(ret == 1) {
                        diehere("label: %s already exists!\n", lineno, opcode);
                        exit(1);
                    }
                    beg += read;
                    ret = sscanf(beg, "%s%n", opcode, &read);
                    if(ret < 0) break;
                    code = lc3as_locate(opcode);
                }
                else {
                    diehere("Invalid label: %s\n", lineno, opcode);
                    exit(2);
                }
            }
            // line contains only labels
            if(ret < 0) {
                fputc('\n', stripped);
                continue;
            }

            if(code == DIRE_END) {
                saw_end = true;
                break;
            }

            while(isspace(*beg)) ++beg;

            if(code == DIRE_BLKW) {
                arg1 = beg;
                while(! isspace(*arg1)) ++arg1;
                while(isspace(*arg1)) ++arg1;
                imm = parse_int16(arg1);
                if(imm == INT16_MAX) {
                    diehere("Invalid operand: %d for .BLKW\n", lineno, imm);
                    exit(1);
                }
                prog_loc += imm - 1;
            }
            else if (code == DIRE_STRING) {
                arg1 = beg + read;
                while(isspace(*arg1)) ++arg1;

                if(! is_valid_strz(arg1)) {
                    diehere("Invalid operand: %s for .STRINGZ\n", lineno, arg1);
                    exit(1);
                }

                arg2 = arg1 + strlen(arg1) - 1;
                while(++arg1 != arg2) {
                    if(*arg1 == '\\') ++arg1;
                    ++prog_loc;
                }
            }
            fprintf(stripped, "%s\n", beg);
            ++prog_loc;
        }
    }

    if(!saw_orig) {
        diehere("Missing .ORIG!\n", lineno);
        exit(1);
    }
    if(!saw_end) {
        diehere("Missing .END!\n", lineno);
        exit(1);
    }

    fputc('\n', sym_file);
    rewind(stripped);
    return table;
}

void gen_obj(label_list_t *table, FILE *obj_file)
{
    int code, lineno = 1;
    int16_t prog_loc = table->init_loc;
    static char raw[BUFSIZ];
    uint16_t IR = 0;
    char *arg1, *arg2, *arg3, *opcode;
    static const char *delim = " \f\n\r\t\v,";


    arg1 = arg2 = arg3 = NULL;
    dump_ir(obj_file, (uint16_t)prog_loc);

    while(fgets(raw, BUFSIZ, stripped)) {
        ++lineno;

        if(*raw == '\n') continue;

        opcode = strtok(raw, delim);
        code = lc3as_locate(opcode);
        if(code != DIRE_STRING && (arg1 = strtok(NULL, delim)) && (arg2 = strtok(NULL, delim)))
        {
            arg3 = strtok(NULL, delim);
        }

        switch (code) {
            case OP_ADD:
                IR = 0x1000;
                break;
            case OP_AND:
                IR = 0x5000;
                break;
            case OP_BR:
                IR = 0x0000;
                break;
            case OP_JMP:
                IR = 0xC000;
                break;
            case OP_JSR:
                IR = 0x4800;
                break;
            case OP_JSRR:
                IR = 0x4000;
                break;
            case OP_LD:
                IR = 0x2000;
                break;
            case OP_LDI:
                IR = 0xA000;
                break;
            case OP_LDR:
                IR = 0x6000;
                break;
            case OP_LEA:
                IR = 0xE000;
                break;
            case OP_NOT:
                IR = 0x903F;
                break;
            case OP_ST:
                IR = 0x3000;
                break;
            case OP_STI:
                IR = 0xB000;
                break;
            case OP_STR:
                IR = 0x7000;
                break;
            case OP_TRAP:
                IR = 0xF000;
                break;

            case OP_RET:
            case OP_RTI:

            case TRAP_GETC:
            case TRAP_OUT:
            case TRAP_PUTS:
            case TRAP_IN:
            case TRAP_PUTSP:
            case TRAP_HALT:

            case DIRE_FILL:
            case DIRE_BLKW:
            case DIRE_STRING:
                break;

            default:
                diehere("Unknown opcode: %s\n", lineno, opcode);
                exit(2);
        }

        int16_t dr, sr1, sr2, imm, offset;
        int i;
        char *beg;
        switch (code) {
            case OP_ADD:
            case OP_AND:
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR |= dr << 9;
                    sr1 = parse_int16(arg2);
                    IR |= sr1 << 6;
                    if(is_valid_reg(arg3)) {
                        sr2 = parse_int16(arg3);
                        IR |= sr2;
                        dump_ir(obj_file, IR);
                    }
                    else {
                        imm = parse_int16(arg3);
                        if(imm > 0xF || imm < -0x10) {
                            diehere("Out of range: %s\n", lineno, arg3);
                            exit(2);
                        }
                        IR |= (0x1F & imm) | (1 << 5);
                        dump_ir(obj_file, IR);
                    }
                }
                else {
                    diehere("Invalid operands\n", lineno);
                    exit(2);
                }
                break;

            case OP_LD:
            case OP_ST:
            case OP_LDI:
            case OP_STI:
            case OP_LEA:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR |= (dr << 9);

                    offset = get_offset(table, prog_loc + 1, lineno, arg2, 9);
                    IR |= offset;
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand\n", lineno);
                    exit(1);
                }
                break;

            case OP_LDR:
            case OP_STR:
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR |= (dr << 9);
                    sr1 = parse_int16(arg2);
                    IR |= (sr1 << 6);

                    offset = get_offset(table, prog_loc + 1, lineno, arg3, 6);
                    IR |= offset;
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operands\n", lineno);
                    exit(2);
                }
                break;


            case OP_JMP:
            case OP_JSRR:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR |= (dr << 6);
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand\n", lineno);
                    exit(2);
                }
                break;

            case OP_BR:
                beg = opcode + 2;
                // BR = BRnzp
                if(!(*beg)) {
                    IR |= (0x7 << 9);
                }
                else {
                    if(tolower(*beg) == 'n') {
                        ++beg;
                        IR |= (0x1 << 11);
                    }
                    if(tolower(*beg) == 'z') {
                        ++beg;
                        IR |= (0x1 << 10);
                    }
                    if(tolower(*beg) == 'p') {
                        ++beg;
                        IR |= (0x1 << 9);
                    }
                    if(*beg) {
                        diehere("Invalid opcode\n", lineno);
                        exit(1);
                    }
                }
                offset = get_offset(table, prog_loc + 1, lineno, arg1, 9);
                IR |= offset;
                dump_ir(obj_file, IR);

                break;

            case OP_JSR:
                offset = get_offset(table, prog_loc + 1, lineno, arg1, 11);
                IR |= offset;
                dump_ir(obj_file, IR);
                break;

            case OP_NOT:
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR |= (dr << 9);
                    sr1 = parse_int16(arg2);
                    IR |= (sr1 << 6);
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operands!\n", lineno);
                    exit(2);
                }
                break;

            case OP_TRAP:
                imm = parse_int16(arg1);
                if(imm == INT16_MAX) {
                    diehere("Invalid operand!\n", lineno);
                    exit(2);
                }
                switch(imm) {
                    case 0x20:
                    case 0x21:
                    case 0x22:
                    case 0x23:
                    case 0x24:
                    case 0x25:
                        IR |= (imm & 0xFF);
                        dump_ir(obj_file, IR);
                        break;
                    default:
                        diehere("Invalid operand!\n", lineno);
                        exit(1);
                }
                break;

            case OP_RTI:
                dump_ir(obj_file, 0x8000);
                break;
            case OP_RET:
                dump_ir(obj_file, 0xC1C0);
                break;
            case TRAP_GETC:
                dump_ir(obj_file, 0xF020);
                break;
            case TRAP_OUT:
                dump_ir(obj_file, 0xF021);
                break;
            case TRAP_PUTS:
                dump_ir(obj_file, 0xF022);
                break;
            case TRAP_IN:
                dump_ir(obj_file, 0xF023);
                break;
            case TRAP_PUTSP:
                dump_ir(obj_file, 0xF024);
                break;
            case TRAP_HALT:
                dump_ir(obj_file, 0xF025);
                break;

            case DIRE_FILL:
                if((i = parse_int32(arg1)) != INT32_MAX) {
                    if(i > 0xFFFF || i < -0x8000) {
                        diehere("Out of range: x%X\n", lineno, i);
                        exit(2);
                    }
                    dump_ir(obj_file, (0xFFFF & i));
                }
                else if(is_valid_label(arg1)) {
                    if((imm = label2loc(table, arg1)) == -1) {
                        diehere("Cannot find label: %s\n", lineno, arg1);
                        exit(2);
                    }
                    dump_ir(obj_file, (uint16_t)imm);
                }
                else {
                    diehere("Invalid operand: %s\n", lineno, arg1);
                    exit(2);
                }
                break;

            case DIRE_STRING:
                arg1 = opcode + strlen(opcode) + 1;
                while(isspace(*arg1)) ++arg1;
                strtok(arg1, "\r\n");
                arg2 = arg1 + strlen(arg1) - 1;
                while(++arg1 != arg2) {
                    if(*arg1 == '\\') {
                        switch(*++arg1) {
                            case 't':
                                dump_ir(obj_file, 0x9);
                                break;
                            case 'n':
                                dump_ir(obj_file, 0xA);
                                break;
                            case 'r':
                                dump_ir(obj_file, 0xD);
                                break;
                            default:
                                dump_ir(obj_file, (0xFF & *arg1));
                                break;
                        }
                    }
                    else {
                        dump_ir(obj_file, (0xFF & *arg1));
                    }
                    ++prog_loc;
                }
                dump_ir(obj_file, 0);
                break;

            case DIRE_BLKW:
                imm = parse_int16(arg1);
                if(imm == INT16_MAX || imm <= 0) {
                    diehere("Invalid operand %s for .BLKW\n", lineno, arg1);
                    exit(2);
                }
                for (i = 0; i < imm; i++) {
                    dump_ir(obj_file, 0);
                }
                prog_loc += imm - 1;
                break;

            default:
                diehere("Unkown opcode: %s\n", lineno, opcode);
                exit(2);
        }
        ++prog_loc;
    }
    fclose(stripped);
    unlink(_tmpfile);
}
