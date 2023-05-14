#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "CommandSystem/CommandSystem.h"

#define DEF_CMD(name, num, ...) \
    CMD_##name = num,

//==========================================DEFINES===========================================

#define DEBUG

#define CHECK(condition, true_branch)   \
    if (condition)                      \
    {                                   \
        true_branch;                    \
    }

//==========================================CONSTANTS===========================================

const size_t kMaxFilepathSize = 256;

enum COMMANDS
{
    #include "CommandSystem/Cmd.h"
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
    x86_NEXT_IS_INT   = 0x25,
    x86_POP_MEM       = 0x8f,
    x86_PUSH_MEM      = 0xff,
};

enum x86_REGISTERS
{
    x86_ERROR_REG = 0b1111,
    x86_AX        = 0b000,
    x86_BX        = 0b011,
    x86_CX        = 0b001,
    x86_DX        = 0b010,
}; 

//==========================================FUNCTION PROTOTYPES===========================================

int  ParseCmdArgs(int argc, char* argv[], char* in_bin_filepath);
int  Translate(int* in_code, char* out_code, MyHeader* in_header);
void Run(char* out_code);

void MakeAddSub(char* code, size_t* ip, x86_COMMANDS command);
void MakeMulDiv(char* code, size_t* ip, bool is_mul);
int  MakePop(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip);
int  MakePush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip);
int  MakeHlt(char* code, size_t* ip);

char          MakePushPopRegOpcode(x86_COMMANDS command, x86_REGISTERS reg);
char          MakeMODRMArgument(char mod, x86_REGISTERS reg, char rm);
char          MakeMulDivWithRegArg(x86_CMD_ARGUMENTS arg_pref, x86_REGISTERS reg);
x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg);
void          ParsePushPopArguments(int* in_code, size_t* in_ip, int* command, int* number, x86_REGISTERS* reg);
int           GetFileSize(const char *file_name);

//==========================================FUNCTION IMPLEMENTATION===========================================

int main(int argc, char* argv[])
{
    char in_bin_filepath[kMaxFilepathSize + 1] = {};
    CHECK(ParseCmdArgs(argc, argv, in_bin_filepath) != 0, return -1;)    

    FILE* in_bin_fp = fopen(in_bin_filepath, "rb");
    CHECK(in_bin_fp == nullptr, return -1;)

    MyHeader in_bin_header = {};
    CHECK(CheckMyHeaderFromFile(&in_bin_header, in_bin_fp) != 0, return -1;);

    size_t in_file_size = GetFileSize(in_bin_filepath);

    int in_bin_fd = open(in_bin_filepath, O_RDWR);
    
    char* out_code = (char*)mmap(NULL, 4096,         PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1,        0);
    int*  in_code  = (int*) mmap(NULL, in_file_size, PROT_READ,              MAP_PRIVATE,                 in_bin_fd, 0);

    Translate((int*)((char*)in_code + sizeof(MyHeader)), out_code , &in_bin_header);

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
    code[(*ip)++] = MakePushPopRegOpcode(x86_POP, x86_AX);                    //pop rax
    code[(*ip)++] = MakePushPopRegOpcode(x86_POP, x86_BX);                    //pop rbx
    
    code[(*ip)++] = command;                                                  //
    code[(*ip)++] = MakeMODRMArgument(x86_STD_MODRM_MOD, x86_AX, x86_BX);     //add rax, rbx

    code[(*ip)++] = MakePushPopRegOpcode(x86_PUSH, x86_AX);                   //push rax
}

//!
//!is_mul = true  if mul
//!       = false if div
//!
void MakeMulDiv(char* code, size_t* ip, bool is_mul)
{
    assert(code && ip);

    code[(*ip)++] = MakePushPopRegOpcode(x86_POP, x86_AX);         //pop rax
    code[(*ip)++] = MakePushPopRegOpcode(x86_POP, x86_BX);         //pop rbx
    
    code[(*ip)++] = x86_MUL;
    if (is_mul)
    {
        code[(*ip)++] = MakeMulDivWithRegArg(x86_MUL_WITH_REG, x86_BX);
    }
    else // command == x86_DIV
    {
        code[(*ip)++] = MakeMulDivWithRegArg(x86_DIV_WITH_REG, x86_BX);
    }
    
    code[(*ip)++] = MakePushPopRegOpcode(x86_PUSH, x86_AX);        //push rax
}

int MakePush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip)
{
    int           command = 0;
    int           number  = 0;
    x86_REGISTERS reg     = x86_AX;
    ParsePushPopArguments(in_code, in_ip, &command, &number, &reg);

    if (command & ARG_MEM)
    {
        out_code[(*out_ip)++] = x86_PUSH_MEM;
        
        char* command_opcode = &out_code[(*out_ip)++];

        if (command & ARG_NUM)
        {
            if (command & ARG_REG)
            {
                if (number > 127)
                    *command_opcode = (char)0xb0;
                else
                    *command_opcode = (char)0x70;
            }
            else
            {
                *command_opcode = 0x34;
                out_code[(*out_ip)++] = x86_NEXT_IS_INT;
            }

            if (command & ARG_REG && number < 128)
                out_code[(*out_ip)++] = (char)number;
            else
            {
                memcpy(&out_code[*out_ip], &number, sizeof(int));
                *out_ip += sizeof(int);
            }
        }
        if (command & ARG_REG)
        {
            *command_opcode |= 0b00110000;
            *command_opcode |= (char)reg;
        }
    }
    else if (command & ARG_REG)
    {
        out_code[(*out_ip)++] = MakePushPopRegOpcode(x86_PUSH, reg);
    }
    else if (command & ARG_NUM)
    {
        if (number < 128)
        {
            out_code[(*out_ip)++] = x86_PUSH_N | 0b10;
            out_code[(*out_ip)++] = (char)number;               //Put number in one byte
        }
        else
        {
            out_code[(*out_ip)++] = x86_PUSH_N;

            memcpy(&out_code[*out_ip], &number, sizeof(int)); //
            *out_ip += sizeof(int);                             //Put number in 4 bytes
        }
    }
    
    return 0;
}


int MakePop(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip)
{
    int           command = 0;
    int           number  = 0;
    x86_REGISTERS reg     = x86_AX;

    ParsePushPopArguments(in_code, in_ip, &command, &number, &reg);

    if (command & ARG_MEM)
    {
        out_code[(*out_ip)++] = x86_POP_MEM;

        if (command & ARG_NUM)
        {
            if (command & ARG_REG)
            {
                if (number < 128)
                    out_code[*out_ip] = (char)0x40;
                else
                    out_code[*out_ip] = (char)0x80;
            }
            else
            {
                out_code[(*out_ip)++] = 0x04;
                out_code[(*out_ip)++] = x86_NEXT_IS_INT;
            }
        }
    }
    if (command & ARG_REG)
    {
        if (!(command & ARG_MEM))
            out_code[(*out_ip)] = x86_POP;
        out_code[(*out_ip)++] |= (char)reg;
    }
    if (command & ARG_NUM)
    {
        if (command & ARG_REG && number < 128)
            out_code[(*out_ip)++] = (char)number;                 //Put number in one byte
        else
        {
            printf("=====\nARG_NUM\n======\n");
            memcpy(&out_code[*out_ip], &number, sizeof(int));   //
            *out_ip += sizeof(int);                              //Put number in 4 bytes
        }
    }

    return 0;
}

int MakeHlt(char* code, size_t* ip)
{
    char end_program[] = { 
        (char)0x48, (char)0xC7, (char)0xC0, (char)0x3C, (char)0x00, (char)0x00, (char)0x00, //mov rax, 0x3c
        (char)0x48, (char)0x31, (char)0xFF,                                                 //xor rdi, rdi
        (char)0x0F, (char)0x05 };                                                           //syscall

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
                #ifdef DEBUG
                    printf("ADD\n");
                #endif

                in_ip++;
                MakeAddSub(out_code, &out_ip, x86_ADD);
                break;
            }

            case CMD_SUB:
            {
                #ifdef DEBUG
                    printf("SUB\n");
                #endif
                
                in_ip++;
                MakeAddSub(out_code, &out_ip, x86_SUB);
                break;                
            }

            case CMD_MUL:
            {  
                #ifdef DEBUG
                    printf("MUL\n");
                #endif
                
                in_ip++; 
                MakeMulDiv(out_code, &out_ip, true);
                break;
            }

            case CMD_DIV:
            {
                #ifdef DEBUG
                    printf("DIV\n");
                #endif
                
                in_ip++;
                MakeMulDiv(out_code, &out_ip, false);
                break;
            }

            case CMD_HLT:
            {
                #ifdef DEBUG
                    printf("HLT\n");
                #endif
                
                in_ip++;
                MakeHlt(out_code, &out_ip);
                break;
            }

            case CMD_PUSH:
            {
                #ifdef DEBUG
                    printf("PUSH\n");
                #endif
                
                MakePush(out_code, &out_ip, in_code, &in_ip);
                break;
            }

            case CMD_POP:
            {
                #ifdef DEBUG
                    printf("POP\n");
                #endif
                
                MakePop(out_code, &out_ip, in_code, &in_ip);
                break;
            }

            default:
                break;
        }

        #ifdef DEBUG
            printf("cmd    = %0x\n", cmd);

            printf("in_ip  = %zu\n", in_ip);
            printf("out_ip = %zu\n", out_ip);

            printf("IN_CODE:\n");
            for (int i = 0; i < in_ip; i++)
                printf("%x ", in_code[i]);
            printf("\n");

            printf("OUT_CODE:\n");
            for (int i = 0; i < out_ip; i++)
                printf("%x ", (unsigned char)out_code[i]);
            printf("\n\n");
        #endif
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
        default:
            return x86_ERROR_REG;
    }

    return x86_ERROR_REG;
}


//!
//!Function parse push and pop commands in to command opcode + number(if any) + register(if any)
//!and move in_ip to the next command
//!
void ParsePushPopArguments(int* in_code, size_t* in_ip, int* command, int* number, x86_REGISTERS* reg)
{
    *command = in_code[(*in_ip)++];
    if (*command & ARG_NUM)
        *number  = in_code[(*in_ip)++];
    if (*command & ARG_REG)
        *reg     = ConvertMyRegInx86Reg((REGISTERS)in_code[(*in_ip)++]);
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

//!-----------------------------------------------------
//! Function to measure file size in bytes
//! \param  [in] file_name Name of file whose size you need to know
//! \return File size in bytes
//!
//! ----------------------------------------------------
int GetFileSize(const char *file_name)
{
    assert(file_name);

    struct stat buff = {};
    stat(file_name, &buff);
    return buff.st_size;
}