#ifndef SYM_BIN_TRANSLATION
#define SYM_BIN_TRANSLATION

#include <stdio.h>

#include "../CommandSystem/CommandSystem.h"

//==========================================DEFINES===========================================

#define DEF_CMD(name, num, ...) \
    CMD_##name = num,

//#define DEBUG

//==========================================CONSTANTS===========================================

enum COMMANDS
{
    #include "../MyLanguage/Libs/CPU/Cmd.h"
};

enum x86_COMMANDS
{
    x86_ERROR_CMD = 0x00,

    x86_ADD     = 0x01,
    x86_OR      = 0x09,
    x86_AND     = 0x21,
    x86_CALL    = 0xd0,
    x86_CMP     = 0x39,
    x86_DEC     = 0xc8,
    x86_DIV     = 0xf7,
    x86_INC     = 0xc0,
    x86_JMP     = 0xe0,
    x86_MUL     = 0xf7,
    x86_MOV_ABS = 0xb8,
    x86_MOV     = 0x89,
    x86_PUSH    = 0x50,
    x86_PUSH_N  = 0x68,
    x86_POP     = 0x58,
    x86_RET     = 0xc3,
    x86_SUB     = 0x29,
    x86_XOR     = 0x31,

    x86_JE      = 0x84,
    x86_JNE     = 0x85,
    x86_JL      = 0x8c,
    x86_JGE     = 0x8d,
    x86_JLE     = 0x8e,
    x86_JG      = 0x8f,
};

enum x86_CMD_ARGUMENTS
{
    x86_DIV_WITH_REG  = 0xf0,
    x86_MUL_WITH_REG  = 0xe0,
    x86_POP_MEM       = 0x8f,
    x86_PUSH_MEM      = 0xff,
};

enum x86_REGISTERS
{
    x86_ERROR_REG  = 0b1111,

    x86_RAX = 0b000,        //User register
    x86_RBX = 0b011,        //User register
    x86_RCX = 0b001,        //User register
    x86_RDX = 0b010,        //User register

    x86_RSI = 0b0110,       //System register
    x86_RDI = 0b0111,       //System register
    x86_R8  = 0b1000,       //System register
    x86_R9  = 0b1001,       //System register
    x86_R10 = 0b1010,       //System register
    x86_R11 = 0b1011,       //System register
    x86_R12 = 0b1100,       //System register
    x86_R13 = 0b1101,       //System register
    x86_R14 = 0b1110,       //System register
    x86_R15 = 0b1111,       //System register
};

//==========================================FUNCTION PROTOTYPES============================

void TranslateAndRun(const char* in_bin_filepath, size_t in_file_size, MyHeader in_bin_header);

#endif