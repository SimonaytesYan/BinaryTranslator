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
    x86_ADD     = 0x01,
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
    x86_SUB     = 0x29,
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

//==========================================FUNCTION PROTOTYPES===========================================

int  Translate(int* in_code, char* out_code, MyHeader* in_header, char* ram);
void Run(char* out_code);
int  GetFileSize(const char *file_name);
int  ParseCmdArgs(int argc, char* argv[], char* in_bin_filepath);

void MakeAddSub(char* code, size_t* ip, x86_COMMANDS command);
int  MakeCall(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char** in_command_out_command_match);
int  MakeHlt(char* code, size_t* ip);
void MakeMulDiv(char* code, size_t* ip, bool is_mul);
int  MakePop(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram);
int  MakePush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram);
int  MakeReturn(char* out_code, size_t* out_ip);

x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg);
void          MakeMovAbsInReg(char* code, size_t* ip, size_t number, x86_REGISTERS reg);
void          MakeIncDec(char* code, size_t* ip, x86_REGISTERS reg, x86_COMMANDS command);
void          MakePushPopReg(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg);
void          ParsePushPopArguments(int* in_code, size_t* in_ip, int* command, unsigned int* number, x86_REGISTERS* reg);
void          PutPrefixForTwoReg(char* code, size_t* ip, x86_REGISTERS *reg1, x86_REGISTERS *reg2);
void          PutPrefixForOneReg(char* code, size_t* ip, x86_REGISTERS* reg);
void          MakeMoveRegToReg(char* code, size_t* ip, x86_REGISTERS reg_to, x86_REGISTERS reg_from);

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
    
    char* out_code   = (char*)mmap(NULL, 4096,         PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    int*  in_code    = (int*) mmap(NULL, in_file_size, PROT_READ,              MAP_PRIVATE,                 in_bin_fd, 0);

    char* ram        = (char*)mmap(NULL, RAM_SIZE,        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    char* call_stack = (char*)mmap(NULL, CALL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    #ifdef DEBUG
        printf("RAM     = %p\n", ram);
        printf("CALL_ST = %p\n", call_stack);
    #endif

    printf("Translating...\n");

    size_t out_ip = 0;
    MakeMovAbsInReg(out_code, &out_ip, (size_t)call_stack, x86_RSI); //Put call stack pointer to rsi 
    for (int i = 0; i < out_ip; i++)
        printf("%X ", (unsigned char)out_code[i]);
    printf("\n");
    MakeMovAbsInReg(out_code, &out_ip, (size_t)ram,        x86_RDI); //Put ram pointer to rdi

    #ifdef DEBUG
        printf("OUT_CODE:\n");
        for (int i = 0; i < out_ip; i++)
            printf("%X ", (unsigned char)out_code[i]);
        printf("\n\n");
    #endif

    Translate((int*)((char*)in_code + sizeof(MyHeader)), &out_code[out_ip] , &in_bin_header, ram);

    printf("Running\n");

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

int Translate(int* in_code, char* out_code, MyHeader* in_header, char* ram)
{
    size_t in_ip  = 0;
    size_t out_ip = 0;
    char** in_command_out_command_match = (char**)calloc(in_header->commands_number, sizeof(char*));

    while (in_ip < in_header->commands_number)
    {
        int cmd = in_code[in_ip];
        in_command_out_command_match[in_ip] = &out_code[out_ip];

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
                
                MakePush(out_code, &out_ip, in_code, &in_ip, ram);
                break;
            }

            case CMD_POP:
            {
                #ifdef DEBUG
                    printf("POP\n");
                #endif
                
                MakePop(out_code, &out_ip, in_code, &in_ip, ram);
                break;
            }

            case CMD_CALL:
            {
                #ifdef DEBUG
                    printf("CALL\n");
                #endif

                in_ip++;
                MakeCall(out_code, &out_ip, in_code, &in_ip, in_command_out_command_match);
                break;
            }

            case CMD_RET:
            {
                #ifdef DEBUG
                    printf("RET\n");
                #endif

                in_ip++;
                MakeReturn(out_code, &out_ip);
                break;
            }

            default:
                break;
        }

        #ifdef DEBUG
            printf("cmd    = %b\n", cmd);

            printf("in_ip  = %zu\n", in_ip);
            printf("out_ip = %zu\n", out_ip);

            printf("IN_CODE:\n");
            for (int i = 0; i < in_ip; i++)
                printf("%X ", in_code[i]);
            printf("\n");

            printf("OUT_CODE:\n");
            for (int i = 0; i < out_ip; i++)
                printf("%X ", (unsigned char)out_code[i]);
            printf("\n\n");
        #endif
    }

    MakeHlt(out_code, &out_ip);

     #ifdef DEBUG
        printf("in_ip  = %zu\n", in_ip);
        printf("out_ip = %zu\n", out_ip);

        printf("IN_CODE:\n");
        for (int i = 0; i < in_ip; i++)
            printf("%X ", in_code[i]);
        printf("\n");

        printf("OUT_CODE:\n");
        for (int i = 0; i < out_ip; i++)
            printf("%X ", (unsigned char)out_code[i]);
        printf("\n\n");
    #endif

    return 0;
}

void MakeAddSubRegs(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg1, x86_REGISTERS reg2)
{
    PutPrefixForTwoReg(code, ip, &reg1, &reg2);
    code[(*ip)++] = command;
    code[(*ip)++] = 0xc0 | reg1 | (reg2 << 3);
}

void MakeAddSub(char* code, size_t* ip, x86_COMMANDS command)
{
    assert(code && ip);
    MakePushPopReg(code, ip, x86_POP, x86_R8);    //pop r8
    MakePushPopReg(code, ip, x86_POP, x86_R9);    //pop r9
    
    MakeAddSubRegs(code, ip, command, x86_R8, x86_R9);  //add r8, r9

    MakePushPopReg(code, ip, x86_PUSH, x86_R8);  //push r8
}

//!
//!is_mul = true  if mul
//!       = false if div
//!
void MakeMulDiv(char* code, size_t* ip, bool is_mul)
{
    assert(code && ip);

    MakePushPopReg(code, ip, x86_POP, x86_R8);        //pop r8
    MakePushPopReg(code, ip, x86_POP, x86_R9);       //pop r9

    MakeMoveRegToReg(code, ip, x86_R10, x86_RAX);           // mov r10, rax ;put old rax value in r10
    MakeMoveRegToReg(code, ip, x86_RAX, x86_R9);            //mov rax, r9

    x86_REGISTERS reg = x86_R8;
    PutPrefixForOneReg(code, ip, &reg);

    code[(*ip)++] = x86_MUL;
    if (is_mul)
        code[(*ip)++] = x86_MUL_WITH_REG | x86_R8;
    else
        code[(*ip)++] = x86_DIV_WITH_REG | x86_R8;


    MakePushPopReg  (code, ip, x86_PUSH, x86_RAX);          //push rax
    MakeMoveRegToReg(code, ip, x86_RAX, x86_R10);           //mov r10, rax  ;recover rax
}

int MakePush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram)
{
    int           command = 0;
    unsigned int  number  = 0;
    x86_REGISTERS reg     = x86_RAX;
    ParsePushPopArguments(in_code, in_ip, &command, &number, &reg);

    if (command & ARG_MEM)
    {
        out_code[(*out_ip)++] = x86_PUSH_MEM;

        if (command & ARG_REG)
        {
            out_code[(*out_ip)++] = 0xb4;                 //
            out_code[(*out_ip)++] = reg | x86_RDI << 3;   //push [reg + rdi] ([reg + rdi], because rdi - pointer to ram for this program)
        }
        else if (command & ARG_NUM)
            out_code[(*out_ip)++] = 0xb0 | x86_RDI;

        memcpy(&out_code[*out_ip], &number, sizeof(int));
        *out_ip += sizeof(int);
    }
    else if (command & ARG_REG)
    {
        MakePushPopReg(out_code, out_ip, x86_PUSH, reg);
    }
    else if (command & ARG_NUM)
    {
        out_code[(*out_ip)++] = x86_PUSH_N;

        memcpy(&out_code[*out_ip], &number, sizeof(int));   //
        *out_ip += sizeof(int);                             //Put number in 4 bytes
    }
    
    return 0;
}

int MakePop(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram)
{
    int           command = 0;
    unsigned int  number  = 0;
    x86_REGISTERS reg     = x86_RAX;

    ParsePushPopArguments(in_code, in_ip, &command, &number, &reg);

    if (command & ARG_MEM)
    {
        out_code[(*out_ip)++] = x86_POP_MEM;

        if (command & ARG_REG)
        {
            out_code[(*out_ip)++] = 0x84;
            out_code[(*out_ip)]   = x86_RDI << 3; // Dont increment out_ip, because we should make or this byte with register.
                                                  // Program will do it later
        }
        else if (command & ARG_NUM)
            out_code[(*out_ip)++] = 0x80 | x86_RDI;

    }
    if (command & ARG_REG)
    {
        if (!(command & ARG_MEM))
            out_code[(*out_ip)] = x86_POP;
        out_code[(*out_ip)++] |= (char)reg;
    }
    if (command & ARG_NUM)
    {
        memcpy(&out_code[*out_ip], &number, sizeof(int));   //
        *out_ip += sizeof(int);                             //Put number in 4 bytes
    }

    return 0;
}

int MakeReturn(char* out_code, size_t* out_ip)
{
    out_code[(*out_ip)++] = 0xff;                                   //
    out_code[(*out_ip)++] = 0x20 | x86_RSI;                         //jmp [rsi]

    MakeIncDec(out_code, out_ip, x86_RSI, x86_INC);
    return 0;
}

int MakeCall(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char** in_command_out_command_match)
{
    size_t in_code_label = in_code[(*in_ip)++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];                
    size_t ret_address   = (size_t)&out_code[*out_ip];
    
    MakeMovAbsInReg(out_code, out_ip, ret_address, x86_R8);                     //movabs r8, ret_address

    x86_REGISTERS reg2 = x86_R8;                                                //
    x86_REGISTERS reg1 = x86_RSI;                                               //            
    PutPrefixForTwoReg(out_code, out_ip, &reg1, &reg2);                         //
    out_code[(*out_ip)++] = x86_MOV;                                            //
    out_code[(*out_ip)++] = (char)(reg1 | (reg2 << 3));                         //mov [rsi], r8

    MakeIncDec(out_code, out_ip, x86_RSI, x86_INC);                             //inc rsi

    MakeMovAbsInReg(out_code, out_ip, label, x86_RAX);                          //movabs rax, label

    out_code[(*out_ip)++] = 0xff;                                               //
    out_code[(*out_ip)++] = x86_JMP | x86_RAX;                                  //jmp rax

    return 0;
}

int MakeHlt(char* code, size_t* ip)
{
    MakeMovAbsInReg(code, ip, 0x3c, x86_RAX);   //mov rax, 0x3c
    
    size_t end_program_len = 5;
    char end_program[] = {
        (char)0x48, (char)0x31, (char)0xFF,                                                 //xor rdi, rdi
        (char)0x0F, (char)0x05 };                                                           //syscall

    memcpy(&code[*ip], end_program, end_program_len);
    *ip += end_program_len;

    return 0;
}

x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg)
{
    switch (reg)
    {
        case RAX:
            return x86_RAX;
        case RBX:
            return x86_RBX;
        case RCX:
            return x86_RCX;
        case RDX:
            return x86_RDX;
        default:
            return x86_ERROR_REG;
    }

    return x86_ERROR_REG;
}

//!
//!Function parse push and pop commands in to command opcode + number(if any) + register(if any)
//!and move in_ip to the next command
//!
void ParsePushPopArguments(int* in_code, size_t* in_ip, int* command, unsigned int* number, x86_REGISTERS* reg)
{
    *command = in_code[(*in_ip)++];
    if (*command & ARG_NUM)
        *number  = in_code[(*in_ip)++];
    if (*command & ARG_REG)
        *reg     = ConvertMyRegInx86Reg((REGISTERS)in_code[(*in_ip)++]);
}

void PutPrefixForOneReg(char* code, size_t* ip, x86_REGISTERS* reg)
{
    code[(*ip)] = 0x48;
    if (*reg >= x86_R8)
    {
        code[(*ip)] |= 0b1;
        *reg = (x86_REGISTERS)(*reg & 0b111);
    }
    (*ip)++;
}

void PutPrefixForTwoReg(char* code, size_t* ip, x86_REGISTERS *reg1, x86_REGISTERS *reg2)
{
    code[(*ip)] = 0x48;
    if (*reg1 >= x86_R8)
    {
        code[(*ip)] |= 0b1;
        *reg1 = (x86_REGISTERS)(*reg1 & 0b111);
    }
    if (*reg2 >= x86_R8)
    {
        code[(*ip)] |= 0b100;
        *reg2 = (x86_REGISTERS)(*reg2 & 0b111);        
    }
    (*ip)++;
}

void MakeMoveRegToReg(char* code, size_t* ip, x86_REGISTERS reg_to, x86_REGISTERS reg_from)
{
    PutPrefixForTwoReg(code, ip, &reg_to, &reg_from);

    code[(*ip)++] = x86_MOV;
    code[(*ip)++] = 0xc0 | reg_to | (reg_from << 3);
}

void MakePushPopReg(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        code[(*ip)++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }
    code[(*ip)++] = (char)command | (char)reg;
}

void MakeMovAbsInReg(char* code, size_t* ip, size_t number, x86_REGISTERS reg)
{
    PutPrefixForOneReg(code, ip, &reg);
    
    code[(*ip)++] = x86_MOV_ABS | reg;                      //
    memcpy(&code[*ip], &number, sizeof(size_t));            //
    *ip += sizeof(size_t);                                  //mov reg, number
}

void MakeIncDec(char* code, size_t* ip, x86_REGISTERS reg, x86_COMMANDS command)
{
    PutPrefixForOneReg(code, ip, &reg);
    
    
    code[(*ip)++] = 0xff;
    code[(*ip)++] = command | reg;
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
