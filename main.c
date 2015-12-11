#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "lc3as.h"

void manual(const char *prog) {
    puts("Usage:");
    printf("%s <source>.asm\n", prog);
}

#define diehere(format, ...) fprintf(stderr, "%s: " format, __FUNCTION__, __VA_ARGS__)

int main(int argc, char *argv[]){
    switch (argc) {
        case 1:
            puts("Please tell me the path of your asm file.\nExit...");
            manual(argv[0]);
            exit(1);
        case 2:
            break;
        default:
            puts("Too much arguments!\nExit...");
            manual(argv[0]);
            exit(2);
            
    }
    FILE *tmp = NULL;
    FILE *asm_file = NULL;
    FILE *sym_file = NULL;
    FILE *obj_file = NULL;
    char filepath[BUFSIZ];
    size_t len;
    char *ext;

    len = strlen(argv[1]);
    strncpy(filepath, argv[1], BUFSIZ);

    /* Check for .asm extension; if not found, add it. */
    if ((ext = strrchr (filepath, '.')) == NULL || strcmp (ext, ".asm") != 0) {
        ext = filepath + len;
        strcpy (ext, ".asm");
    }

    asm_file = fopen(filepath, "r");
    if(! asm_file) {
        perror(argv[1]);
        puts("Exit...");
        exit(1);
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

    sprintf(filepath, "%s", "tempXXXXX.txt");
    tmp = fopen(filepath, "w+");
    if(! tmp) {
        perror("tmpfile");
        exit(1);
    }

    fputs("// Symbol table\n", sym_file);
    fputs("// Scope level 0:\n", sym_file);
    fputs("//\tSymbol Name       Page Address\n", sym_file);
    fputs("//\t----------------  ------------\n", sym_file);

    int lineno = 0, read;
    int16_t prog_loc = 0x3000, prog_init_loc = 0x3000;
    static char buf[BUFSIZ], operand[BUFSIZ], opcode[BUFSIZ];
    char *beg;
    int code, ret;
    bool saw_orig = false, saw_end = false;

    int16_t IR;
    int16_t offset, imm, i;
    int8_t dr, sr1, sr2;
    static const char *delim = " \f\n\r\t\v,"; 
    char *arg1 = NULL, *arg2 = NULL, *arg3 = NULL;

    label_list_t *list = new_label_list();

    while(!saw_end && fgets(buf, BUFSIZ, asm_file)) {
        ++lineno;

        /*beg = strupper(strtrim(buf));*/
        beg = strtrim(buf);
        if(! *beg) continue;

        sscanf(beg, "%s%n", opcode, &read);
        code = lc3as_locate(opcode);
        if(! saw_orig) {
            if(code == DIRE_ORIG) {
                beg += read;
                saw_orig = true;
                sscanf(beg, "%s%n", operand, &read);
                prog_loc = parse_int16(operand);
                if(prog_loc == -1) {
                    diehere("%d: Invalid operand for .ORIG", lineno);
                    exit(2);
                }
                prog_init_loc = prog_loc;
            }
        }
        else if(! saw_end) {
            ret = 0;
            while(code == 0) {
                if(lc3as_label_valid(opcode)) {
                    ret = insert_label(sym_file, list, opcode, prog_loc);
                    if(ret == 1) {
                        diehere("label: %s already exists!\n", opcode);
                        exit(1);
                    }
                    beg += read;
                    ret = sscanf(beg, "%s%n", opcode, &read);
                    if(ret < 0) break;
                    code = lc3as_locate(opcode);
                }
                else {
                    diehere("Invalid label: %s\n", opcode);
                    exit(2);
                }
            }
            // line contains only labels
            if(ret < 0) continue;

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
                if(imm < 0) {
                    diehere("Invalid operand: %d for .BLKW\n", imm);
                    exit(1);
                }
                prog_loc += imm - 1;
            }
            else if (code == DIRE_STRING) {
                arg1 = beg + read;
                while(isspace(*arg1)) ++arg1;
                
                ret = parse_strz(arg1);
                if(ret != 0) {
                    diehere("Invalid operand: %s for .STRINGZ\n", arg1);
                    exit(1);
                }

                arg2 = arg1 + strlen(arg1) - 1;
                while(++arg1 != arg2) {
                    if(*arg1 == '\\') ++arg1;
                    ++prog_loc;
                }
            }
            fprintf(tmp, "%s\n", beg);
            ++prog_loc;
        }
    }

    if(!saw_orig) {
        diehere("%s", "Missing .ORIG!\nExit...");
        exit(1);
    }
    if(!saw_end) {
        diehere("%s", "Missing .END!\nExit...");
        exit(1);
    }
    fputc('\n', sym_file);
    fclose(sym_file);
    fclose(asm_file);


    rewind(tmp);
    prog_loc = prog_init_loc;

    dump_ir(obj_file, prog_init_loc);

    // second pass
    while(fgets(buf, BUFSIZ, tmp)) {

        ++prog_loc;
        sscanf(buf, "%s%n", opcode, &read);
        code = lc3as_locate(opcode);
        arg1 = buf + read;
        arg2 = arg3 = NULL;
        if(code != DIRE_STRING) {
            arg1 = strtok(arg1, delim);
            if(arg1) {
                arg2 = strtok(NULL, delim);
                if(arg2) {
                    arg3 = strtok(NULL, delim);
                }
            }
        }
        IR = 0;
        switch (code) {
            case OP_ADD:
                IR |= 0x1;
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR <<= 3;
                    IR |= dr;
                    sr1 = parse_int16(arg2);
                    IR <<= 3;
                    IR |= sr1;
                    if(is_valid_reg(arg3)) {
                        sr2 = parse_int16(arg3);
                        IR <<= 6;
                        IR |= sr2;
                        dump_ir(obj_file, IR);
                    }
                    else {
                        imm = parse_int16(arg3);
                        IR = (int16_t)(IR << 1) | 1;
                        IR <<= 5;
                        IR |= (0x1F & imm);
                        dump_ir(obj_file, IR);
                    }
                }
                else {
                    diehere("Invalid operands: %s, %s", arg1, arg2);
                    exit(2);
                }
                break;
            case OP_AND:
                IR |= 5;
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR <<= 3;
                    IR |= dr;
                    sr1 = parse_int16(arg2);
                    IR <<= 3;
                    IR |= sr1;
                    if(is_valid_reg(arg3)) {
                        sr2 = parse_int16(arg3);
                        IR <<= 6;
                        IR |= sr2;
                        dump_ir(obj_file, IR);
                    }
                    else {
                        imm = parse_int16(arg3);
                        IR = (int16_t)(IR << 1) | 1;
                        IR <<= 5;
                        IR |= (0x1F & imm);
                        dump_ir(obj_file, IR);
                    }
                }
                else {
                    diehere("Invalid operands: %s, %s", arg1, arg2);
                    exit(2);
                }
                break;
            case OP_BR:
                beg = opcode + 2;
                // BR = BRnzp
                if(!(*beg)) {
                    IR |= 7;
                }
                else {
                    if(tolower(*beg) == 'n') {
                        ++beg;
                        IR |= 1;
                    }
                    IR <<= 1;
                    if(tolower(*beg) == 'z') {
                        ++beg;
                        IR |= 1;
                    }
                    IR <<= 1;
                    if(tolower(*beg) == 'p') {
                        ++beg;
                        IR |= 1;
                    }
                    if(*beg) {
                        diehere("Invalid opcode: %s\n", opcode);
                        exit(1);
                    }
                }
                IR <<= 9;
                offset = label2loc(list, arg1);
                if(offset != -1) {
                    IR |= (0x1FF & (offset - prog_loc));
                }
                else {
                    offset = parse_int16(arg1);
                    IR |= (0x1FF & offset);
                }
                dump_ir(obj_file, IR);
                break;

            case OP_JMP:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR = 0xC0 << 2;
                    IR |= dr;
                    IR <<= 6;
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand: %s for JMP\n", arg1);
                    exit(1);
                }
                break;

            case OP_JSR:
                offset = parse_int16(arg1);
                IR = 0x48 << 8;
                IR |= (offset & 0x7FF);
                break;

            case OP_JSRR:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR = 0x40 << 2;
                    IR |= dr;
                    IR <<= 6;
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand: %s for JSRR\n", arg1);
                    exit(1);
                }
                break;

            case OP_LD:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR = 0x2 << 3;
                    IR |= dr;
                    IR <<= 9;
                    offset = label2loc(list, arg2);
                    /*printf("offset: %X\nloc: %X\n", offset, prog_loc);*/
                    if(offset != -1) {
                        IR |= (0x1FF & (offset - prog_loc));
                    }
                    else {
                        offset = parse_int16(arg2);
                        IR |= (0x1FF & offset);
                    }
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand: %s for LD\n", arg1);
                    exit(1);
                }
                break;
            case OP_LDI:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR = 0xA << 3;
                    IR |= dr;
                    IR <<= 9;
                    offset = label2loc(list, arg2);
                    if(offset != -1) {
                        IR |= (0x1FF & (offset - prog_loc));
                    }
                    else {
                        offset = parse_int16(arg2);
                        IR |= (0x1FF & offset);
                    }
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand: %s for LDI\n", arg1);
                    exit(1);
                }
                break;
            case OP_LDR:
                IR = 0x6;
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR <<= 3;
                    IR |= dr;
                    sr1 = parse_int16(arg2);
                    IR <<= 3;
                    IR |= sr1;

                    offset = label2loc(list, arg3);
                    IR <<= 6;
                    if(offset != -1) {
                        IR |= (0x3F & (prog_loc - offset));
                    }
                    else {
                        offset = parse_int16(arg3);
                        IR |= offset;
                    }
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operands: %s, %s", arg1, arg2);
                    exit(2);
                }
                break;

            case OP_ST:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR = 0x3 << 3;
                    IR |= dr;
                    IR <<= 9;
                    offset = label2loc(list, arg2);
                    if(offset != -1) {
                        IR |= (0x1FF & (offset - prog_loc));
                    }
                    else {
                        offset = parse_int16(arg2);
                        IR |= (0x1FF & offset);
                    }
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand: %s for LD\n", arg1);
                    exit(1);
                }
                break;
            case OP_STI:
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR = 0xB << 3;
                    IR |= dr;
                    IR <<= 9;
                    offset = label2loc(list, arg2);
                    if(offset != -1) {
                        IR |= (0x1FF & (offset - prog_loc));
                    }
                    else {
                        offset = parse_int16(arg2);
                        IR |= (0x1FF & offset);
                    }
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operand: %s for STI\n", arg1);
                    exit(1);
                }
                break;
            case OP_STR:
                IR = 0x7;
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    IR <<= 3;
                    IR |= dr;
                    sr1 = parse_int16(arg2);
                    IR <<= 3;
                    IR |= sr1;

                    offset = label2loc(list, arg3);
                    IR <<= 6;
                    if(offset != -1) {
                        IR |= (0x3F & (prog_loc - offset));
                    }
                    else {
                        offset = parse_int16(arg3);
                        IR |= offset;
                    }
                    dump_ir(obj_file, IR);
                }
                else {
                    diehere("Invalid operands: %s, %s", arg1, arg2);
                    exit(2);
                }
                break;

            case OP_LEA:
                IR = 0xE << 3;
                if(is_valid_reg(arg1)) {
                    dr = parse_int16(arg1);
                    IR |= dr;
                }
                else {
                    diehere("Invalid operand: %s for LEA\n", arg1);
                    exit(1);
                }
                IR <<= 9;
                offset = label2loc(list, arg2);
                if(offset != -1) {
                    IR |= (0x1FF & (offset - prog_loc));
                }
                else {
                    offset = parse_int16(arg2);
                    IR |= (0x1FF & offset);
                }
                dump_ir(obj_file, IR);
                break;


            case OP_NOT:
                IR = 0x9 << 3;
                if(is_valid_reg(arg1) && is_valid_reg(arg2)) {
                    dr = parse_int16(arg1);
                    sr1 = parse_int16(arg2);
                    IR |= dr;
                    IR <<= 3;
                    IR |= sr1;
                    IR = (int16_t)(IR << 6) | (-1 & 0x3F);
                }
                else {
                    diehere("Invalid operand: %s, %s for NOT\n", arg1, arg2);
                    exit(1);
                }
                dump_ir(obj_file, IR);
                break;

            case OP_TRAP:
                imm = parse_int16(arg1);
                IR = (int16_t)0xF000;
                IR |= (imm & 0xFF);
                dump_ir(obj_file, IR);
                break;

            case OP_RET:
                dump_ir(obj_file, (int16_t)0xC1C0);
                break;
            case OP_RTI:
                dump_ir(obj_file, (int16_t)0x8000);
                break;

            case TRAP_GETC:
                dump_ir(obj_file, (int16_t)0xF020);
                break;
            case TRAP_OUT:
                dump_ir(obj_file, (int16_t)0xF021);
                break;
            case TRAP_PUTS:
                dump_ir(obj_file, (int16_t)0xF022);
                break;
            case TRAP_IN:
                dump_ir(obj_file, (int16_t)0xF023);
                break;
            case TRAP_PUTSP:
                dump_ir(obj_file, (int16_t)0xF024);
                break;
            case TRAP_HALT:
                dump_ir(obj_file, (int16_t)0xF025);
                break;


            case DIRE_FILL:
                imm = label2loc(list, arg1);
                if(imm == -1) {
                    imm = parse_int16(arg1);
                }
                dump_ir(obj_file, imm);
                break;
            case DIRE_BLKW:
                imm = parse_int16(arg1);
                for (i = 0; i < imm; i++) {
                    dump_ir(obj_file, 0);
                }
                prog_loc += imm - 1;
                break;
            case DIRE_STRING:
                while(isspace(*arg1)) ++arg1;
                strtok(arg1, "\r\n");
                arg2 = arg1 + strlen(arg1) - 1;
                while(++arg1 != arg2) {
                    if(*arg1 == '\\') {
                        switch(*++arg1) {
                            case 'n':
                                dump_ir(obj_file, 0xA);
                                break;
                            case 'r':
                                dump_ir(obj_file, 0xD);
                                break;
                            default:
                                dump_ir(obj_file, *arg1);
                                break;
                        }
                    }
                    else {
                        dump_ir(obj_file, *arg1);
                    }
                    ++prog_loc;
                }
                dump_ir(obj_file, 0);
                break;
            default:
                diehere("unknown opcode: %s \n", opcode);
                exit(1);
        }
    }
    fclose(tmp);
    unlink(filepath);
    fclose(obj_file);
    return 0;
}
