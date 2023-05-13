#include <assert.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>

#include "CommandSystem/CommandSystem.h"

#define DEF_CMD(name, num, ...) \
    CMD_##name = num,

//==========================================DEFINES===========================================

#define CHECK_DO(condition, true_branch)   \
    if (condition)                      \
    {                                   \
        true_branch;                    \
    }

//==========================================CONSTANTS===========================================

const size_t kMaxFilepathSize = 256;

enum COMMANDS
{
    #include "MyLanguage/Libs/CPU/Cmd.h"
};

enum x86_COMMANDS
{
    x86_ADD    = 0x01,
    x86_SUB    = 0x29,
    x86_MUL    = 0xf7,
    x86_DIV    = 0xf7,
    x86_PUSH   = 0x50,
    x86_PUSH_N = 0x68,
    x86_POP    = 0x58,
};

enum x86_CMD_ARGUMENTS
{
    x86_MUL_WITH_REG  = 0xe0,
    x86_DIV_WITH_REG  = 0xf0,
    x86_STD_MODRM_MOD = 0b11,
};

enum x86_REGISTERS
{
    x86_AX = 0b000,
    x86_BX = 0b011,
    x86_CX = 0b001,
    x86_DX = 0b010,
}; 

//==========================================FUNCTION PROTOTYPES===========================================

int  ParseCmdArgs(int argc, char* argv[], char* in_bin_filepath);
int  Translate(int* in_code, char* out_code, MyHeader* in_header);
void Run(char* out_code);

void MakeAddSub(char* code, size_t* ip, x86_COMMANDS command);
void MakeMulDiv(char* code, size_t* ip, x86_COMMANDS command);
int  MakeHlt(char* code, size_t* ip);

char          MakePushPopRegOpcode(x86_COMMANDS command, x86_REGISTERS reg);
char          MakeMODRMArgument(char mod, x86_REGISTERS reg, char rm);
char          MakeMulDivWithRegArg(x86_CMD_ARGUMENTS arg_pref, x86_REGISTERS reg);
x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg);

//==========================================FUNCTION IMPLEMENTATION===========================================

int main(int argc, char* argv[])
{


    char in_bin_filepath[kMaxFilepathSize + 1] = {};

    CHECK_DO(ParseCmdArgs(argc, argv, in_bin_filepath) != 0, return -1;)    

    FILE* in_bin_fp = fopen(in_bin_filepath, "rb");
    CHECK_DO(in_bin_fp == nullptr, return -1;)

    MyHeader in_bin_header = {};
    CHECK_DO(CheckMyHeaderFromFile(&in_bin_header, in_bin_fp) != 0, return -1;);

    size_t in_file_size = 0;

    int in_bin_fd = open(in_bin_filepath, O_RDWR);
    
    char* out_code = (char*)mmap(NULL, 4096,         PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    int*  in_code  = (int*) mmap(NULL, in_file_size, PROT_READ,              MAP_PRIVATE,                 in_bin_fd, 
                                                                                                          sizeof(MyHeader));
    Translate(in_code, out_code , &in_bin_header);

    Run(out_code);
}

void Run(char* out_code)
{
    __asm__(
        ".intel_syntax noprefix\n\t"
        "jmp rdi\n\t"
        "ret\n\t"
        ".att_syntax prefix\n"
    );
}

void MakeAddSub(char* code, size_t* ip, x86_COMMANDS command)
{
    assert(code && ip);

    code[*ip++] = MakePushPopRegOpcode(x86_POP, x86_AX);                    //pop rax
    code[*ip++] = MakePushPopRegOpcode(x86_POP, x86_BX);                    //pop rbx
    
    code[*ip++] = command;                                                  //
    code[*ip++] = MakeMODRMArgument(x86_STD_MODRM_MOD, x86_AX, x86_BX);     //add rax, rbx

    code[*ip++] = MakePushPopRegOpcode(x86_PUSH, x86_AX);                   //push rax
}

void MakeMulDiv(char* code, size_t* ip, x86_COMMANDS command)
{
    assert(code && ip);
    assert(command == x86_MUL || command == x86_DIV);

    code[*ip++] = MakePushPopRegOpcode(x86_POP, x86_AX);         //pop rax
    code[*ip++] = MakePushPopRegOpcode(x86_POP, x86_BX);         //pop rbx
    
    code[*ip++] = command;
    if (command == x86_MUL)
        code[*ip++] = MakeMulDivWithRegArg(x86_MUL_WITH_REG, x86_BX);
    else // command == x86_DIV
        code[*ip++] = MakeMulDivWithRegArg(x86_DIV_WITH_REG, x86_BX);
    
    code[*ip++] = MakePushPopRegOpcode(x86_PUSH, x86_AX);        //push rax
}

int MakePush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip)
{
    char command = in_code[*in_ip++];
    if (command & ARG_MEM)
    {
        out_code[*out_ip++] = 0xff;
        
        char* command_opcode = &out_code[*out_ip++];

        if (command & ARG_IMMED)
        {
            int number = in_code[*in_ip++];
            if (command & ARG_REG)
            {
                if (number > 127)
                    *command_opcode |= 0b01000000;
                else
                    *command_opcode |= 0b10000000;
            }
            else
            {
                out_code[*out_ip++] = 0x34;
                out_code[*out_ip++] = 0x25;
            }

            memcpy(&out_code[*out_ip], &out_code[*out_ip], sizeof(int));
            *out_ip += sizeof(int);
        }
        if (command & ARG_REG)
        {
            *command_opcode |= 0b00110000;
            *command_opcode |= ConvertMyRegInx86Reg((REGISTERS)in_code[*(in_ip)++]);
        }
    }
    else if (command & ARG_REG)
    {
        x86_REGISTERS reg = ConvertMyRegInx86Reg((REGISTERS)in_code[*(in_ip)++]);
        out_code[*out_ip++] = MakePushPopRegOpcode(x86_PUSH, reg);
    }
    else if (command & ARG_IMMED)
    {
        int argument = in_code[*in_ip++];
        if (argument < 0b10000000)
        {
            out_code[*out_ip++] = x86_PUSH_N | 0b10;
            out_code[*out_ip++] = (char)argument;
        }
        else
        {
            out_code[*out_ip++] = x86_PUSH_N;

            memcpy(&out_code[*out_ip], &argument, sizeof(int));
            *out_ip += sizeof(int);
        }
    }
    
    return 0;
}


int MakeHlt(char* code, size_t* ip)
{
    char   end_program[]   = { (char)0x48, (char)0xC7, (char)0xC0, (char)0x3C, (char)0x00, (char)0x00, 
                               (char)0x00, (char)0x48, (char)0x31, (char)0xFF, (char)0x0F, (char)0x05 };
    size_t end_program_len = 12;

    memcpy(&code[*ip], end_program, end_program_len);

    *ip += end_program_len;

    return 0;
}

int Translate(int* in_code, char* out_code, MyHeader* in_header)
{
    size_t in_ip  = 0;
    size_t out_ip = 0;
    while (in_ip < in_header->comands_number)
    {
        int cmd = in_code[in_ip];

        switch (cmd & CMD_MASK)
        {
            case CMD_ADD:
            {
                MakeAddSub(out_code, &out_ip, x86_ADD);
                break;
            }

            case CMD_SUB:
            {
                MakeAddSub(out_code, &out_ip, x86_SUB);
                break;                
            }

            case CMD_MUL:
            {   
                MakeMulDiv(out_code, &out_ip, x86_MUL);
                break;
            }

            case CMD_DIV:
            {
                MakeMulDiv(out_code, &out_ip, x86_DIV);
                break;
            }

            case CMD_HLT:
            {
                MakeHlt(out_code, &out_ip);
                break;
            }

            case CMD_PUSH:
            {
                MakePush(out_code, &out_ip, in_code, &in_ip);
                break;
            }

            default:
                break;
        }
    }

    MakeHlt(out_code, &out_ip);

    return 0;
}

x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg)
{
    switch (reg)
    {
        case RAX:
            return x86_AX;
        case RBX:
            return x86_BX;
        case RCX:
            return x86_CX;
        case RDX:
            return x86_DX;
    }
}

//!
//!\param [in] mod - 2 bits
//!\param [in] reg - 3 bits
//!\param [in] rm  - 3 bits
//!
char MakeMODRMArgument(char mod, x86_REGISTERS reg, char rm)
{
    return (char)(mod << 6) | (char)(reg << 3) | rm;  
}

char MakePushPopRegOpcode(x86_COMMANDS command, x86_REGISTERS reg)
{
    return (char)command | (char)reg;
}

char MakeMulDivWithRegArg(x86_CMD_ARGUMENTS arg_pref, x86_REGISTERS reg)
{
    return (char)arg_pref | (char)reg;
}

int ParseCmdArgs(int argc, char* argv[], char* in_bin_filepath)
{
    if (argc != 2)
    {
        printf("Wrong input format\n");
        return -1;
    }

    if (strlen(argv[1]) > kMaxFilepathSize)
    {
        printf("Too long filename\n");
        return -1;
    }
    
    strcpy(in_bin_filepath, argv[1]);

    return 0;
}