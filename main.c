#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#define PROMPT ">> "

static uint16_t RAM[UINT16_MAX];
static uint16_t reg_file[8];
static uint16_t PC;

uint16_t sign_extend(uint16_t n, char width)
{
    uint16_t res = UINT16_MAX;
    uint16_t max, mask;
    switch (width) {
        case 5: case 6:
        case 9: case 11:
            max = (uint16_t)(1 << width--);
            mask = (uint16_t)(1 << width);
            break;
        default:
            return res;
    }
    if(n > max) return res;

    res = n;
    if(n & mask) {
        mask = ~(max - 1);
        res |= mask;
    }

    return res;
}

void step(uint16_t instr)
{
    unsigned char opcode, dr, sr1, sr2;
    uint16_t offset6, PCoffset9, PCoffset11, imm5;
    
    instr = ntohs(instr);
    dr = (instr & 0x0d00) >> 9;
    sr1 = (instr & 0x01c0) >> 6;
    opcode = (instr & 0xF000) >> 12;

    imm5 = sign_extend(instr & 0x001F, 5);
    offset6 = sign_extend(instr & 0x003F, 6);
    PCoffset9 = sign_extend(instr & 0x01FF, 9);
    PCoffset11 = sign_extend(instr & 0x07FF, 11);
    switch(opcode) {
        // ADD
        case 0x1:
            if(instr & 0x0010) {
                reg_file[dr] = reg_file[sr1] + imm5;
            }
            else {
                sr2 = (instr & 0x0007);
                reg_file[dr] = reg_file[sr1] + reg_file[sr2];
            }
            break;
        // AND
        case 0x5:
            if(instr & 0x0010) {
                reg_file[dr] = reg_file[sr1] & imm5;
            }
            else {
                sr2 = (instr & 0x0007);
                reg_file[dr] = reg_file[sr1] & reg_file[sr2];
            }
            break;
        // ST
        case 0x3:
            RAM[PC + PCoffset9] = reg_file[dr];
            break;
        // JSR
        case 0x4:
            if(instr & 0x0800)
                PC += PCoffset11;
            else
                PC += reg_file[sr1];
            break;
        // LDR
        case 0x6:
            reg_file[dr] = RAM[reg_file[sr1] + offset6];
            break;
        // STR
        case 0x7:
            RAM[reg_file[sr1] + offset6] = reg_file[dr];
            break;
        // RTI
        case 0x8:
            break;
        // NOT
        case 0x9:
            reg_file[dr] = ~reg_file[sr1];
            break;
        // LDI
        case 0xa:
            reg_file[dr] = RAM[RAM[PC + PCoffset9]];
            break;
        // STI
        case 0xb:
            RAM[RAM[PC + PCoffset9]] = reg_file[dr];
            break;
        // JMP
        case 0xc:
            PC = reg_file[sr1];
            break;
        // not used
        case 0xd:
            break;
        // LEA
        case 0xe:
            reg_file[dr] = PC + PCoffset9;
            break;
        // TRAP
        case 0xf:
            break;
        default:
            break;
    }
}

int main(int argc, char *argv[]){
    const char *commands[] = {
        "help",
        "run",
        "step",
        "continue",
        "break",
        "quit",
    };

    FILE *obj_file;
    uint16_t instr;

    switch(argc) {
        case 2:
            break;
        default:
            exit(1);
    }
    obj_file = fopen(argv[1], "r");
    if(! obj_file) {
        exit(1);
    }

    printf("%s", PROMPT);

    static char buf[BUFSIZ], cmd[BUFSIZ];
    uint16_t init_loc;

    if(fread(&instr, sizeof(uint16_t), 1, obj_file) == 1) {
        init_loc = PC = ntohs(instr);
    }
    else {
        exit(1);
    }
    while(fread(&instr, sizeof(uint16_t), 1, obj_file) == 1) {
        instr = ntohs(instr);
        if((0xF000 & instr) == 0) {
            RAM[PC] = (instr & 0x00FF);
        }

        ++PC;
    }

    PC = init_loc;
    rewind(obj_file);
    fread(&instr, sizeof(uint16_t), 1, obj_file);

    uint16_t i;
    char cur_state = 0, last_state = 0;
    while(fgets(buf, BUFSIZ, stdin)) {

    }
    while(fread(&instr, sizeof(uint16_t), 1, obj_file) == 1) {
        ++PC;

        step(instr);
    }
    return 0;
}

