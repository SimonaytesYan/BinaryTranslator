#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <math.h>

#include "Translation.h"
#include "../Stdlib/Stdlib.h"
#include "../CommandSystem/CommandSystem.h"
#include "../Stopwatch.h"

const size_t kTestNumber = 1;
const size_t kRunsInTest = 1;

//#define DEBUG
#define GET_TIME

//==========================================FUNCTION PROTOTYPES===========================================

int  Translate(int* in_code, char* out_code, MyHeader* in_header, char* ram);
void Run(char* out_code);

void  EmitAddSub(char* code, size_t* ip, x86_COMMANDS command);
int   EmitCall(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char** in_command_out_command_match);
void  EmitConditionalJmp(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, COMMANDS command, char** in_command_out_command_match);
void  EmitIn(char* code, size_t* ip);
int   EmitHlt(char* code, size_t* ip);
void  EmitJmp(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char** in_command_out_command_match);
void  EmitMulDiv(char* code, size_t* ip, bool is_mul);
void  EmitOut(char* code, size_t* ip);
int   EmitPop(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram);
int   EmitPush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram);
void  EmitReturn(char* out_code, size_t* out_ip);
void  EmitSqrt(char* code, size_t* ip);

x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg);
x86_COMMANDS  ConditionalJmpConversion(COMMANDS command);
void          CommandParse(COMMANDS cmd, int* in_code, size_t* in_ip, char* out_code, size_t* out_ip, char* ram, char** in_command_out_command_match);
void          EmitMovAbsInReg(char* code, size_t* ip, size_t number, x86_REGISTERS reg);
void          EmitIncDec(char* code, size_t* ip, x86_REGISTERS reg, x86_COMMANDS command);
void          EmitPushPopReg(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg);
void          ParsePushPopArguments(int* in_code, size_t* in_ip, int* command, unsigned int* number, x86_REGISTERS* reg);
void          PutPrefixForTwoReg(char* code, size_t* ip, x86_REGISTERS *reg1, x86_REGISTERS *reg2);
void          PutPrefixForOneReg(char* code, size_t* ip, x86_REGISTERS* reg);
void          EmitMoveRegToReg(char* code, size_t* ip, x86_REGISTERS reg_to, x86_REGISTERS reg_from);
void          DumpInOutCode(int* in_code, size_t in_ip, char* out_code, size_t out_ip);
void          EmitAddSubRegs(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg1, x86_REGISTERS reg2);
void          EmitSyscall(char* code, size_t* ip);
void          NullifyReg(char* code, size_t* ip, x86_REGISTERS reg);
void          EmitAddSubNumWithReg(char* code, size_t* ip, x86_REGISTERS reg, int number, x86_COMMANDS command);
void          EmitJmpToReg(char* code, size_t* ip, x86_REGISTERS reg);
void          EmitCmpTwoReg(char* code, size_t* ip, x86_REGISTERS reg1, x86_REGISTERS reg2);
void          EmitPushAllRegs(char* code, size_t* ip);
void          EmitPopAllRegs(char* code, size_t* ip);
void          EmitCqo(char* code, size_t* ip);
double        CalcAndPrintfStdDeviation(const double data[], const size_t number_meas);

//TODO Струкртура контекста 

//==========================================FUNCTION IMPLEMENTATION===========================================

void TranslateAndRun(char* in_bin_filepath, size_t in_file_size, MyHeader in_bin_header)
{
    assert(in_bin_filepath);

    //TODO asserts

    printf("Translating...\n");
    int in_bin_fd = open(in_bin_filepath, O_RDWR);

    char* out_code   = (char*)mmap(NULL, 4096,         PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    int*  in_code    = (int*) mmap(NULL, in_file_size, PROT_READ,              MAP_PRIVATE,                 in_bin_fd, 0);

    char* ram        = (char*)mmap(NULL, RAM_SIZE,        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    char* call_stack = (char*)mmap(NULL, CALL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);


    size_t out_ip = 0;
    EmitMovAbsInReg(out_code, &out_ip, (size_t)call_stack, x86_RSI);    //Put call stack pointer to rsi 
    EmitMovAbsInReg(out_code, &out_ip, (size_t)ram,        x86_RDI);    //Put ram pointer to rdi
    
    NullifyReg(out_code, &out_ip, x86_RAX);                             //
    NullifyReg(out_code, &out_ip, x86_RBX);                             //
    NullifyReg(out_code, &out_ip, x86_RCX);                             //Nullify all user registers
    NullifyReg(out_code, &out_ip, x86_RDX);                             //

    Translate((int*)((char*)in_code + sizeof(MyHeader)), &out_code[out_ip] , &in_bin_header, ram);

    printf("Running\n");

    #ifdef GET_TIME
        InitTimer();
        double times[kTestNumber] = {};
    #endif

    for (int i = 0; i < kTestNumber; i++)
    {
        #ifdef GET_TIME
            StartTimer();
        #endif

        #ifdef DEBUG
            printf("i = %d\n", i);
        #endif

        for (int j = 0; j < kRunsInTest; j++)
            Run(out_code);

        #ifdef GET_TIME
            StopTimer();
            times[i] = GetTimerMicroseconds();
        #endif

        fseek(stdin, 0, SEEK_SET);
    }

    #ifdef GET_TIME
        CalcAndPrintfStdDeviation(times, kTestNumber);
    #endif
}

double CalcAndPrintfStdDeviation(const double data[], const size_t number_meas)
{
    double sum = 0;
    for (int i = 0; i < number_meas; i++)
        sum += data[i];

    double average       = sum/number_meas;
    double std_deviation = 0;

    for (int i = 0; i < number_meas; i++)
    {
        std_deviation += (data[i] - average) * (data[i] - average);
    }
    if (number_meas != 1)
        std_deviation /= (double)(number_meas - 1);

    std_deviation = sqrt(std_deviation);
    printf("average time  = %lg +- %lg \n", average, std_deviation / average);

    return std_deviation;
}

void Run(char* out_code)
{
    __asm__ volatile(
        ".intel_syntax noprefix\n\t"
        "push rax\n"
        "push rbx\n"
        "push rcx\n"
        "push rdx\n"
        "push r8\n"
        "push r9\n"
        "push r10\n"
        "push r11\n"
        "push r12\n"
        "push r13\n"
        "push r14\n"
        "push r15\n"

        "call rdi\n\t"

        "pop r15\n"
        "pop r14\n"
        "pop r13\n"
        "pop r12\n"
        "pop r11\n"
        "pop r10\n"
        "pop r9\n"
        "pop r8\n"
        "pop rdx\n"
        "pop rcx\n"
        "pop rbx\n"
        "pop rax\n"
        ".att_syntax prefix\n"
    );  //TODO гарантированное помещение out_code в rdi
        //TODO добавить дестройный список, вместо push и pop

    return;
}

int Translate(int* in_code, char* out_code, MyHeader* in_header, char* ram)
{
    size_t in_ip  = 0;
    size_t out_ip = 0;
    char** in_command_out_command_match = (char**)calloc(in_header->commands_number + 1, sizeof(char*));

    //==========================FIRST PASS==============================
    printf("First compilation...\n");
    while (in_ip < in_header->commands_number)
    {
        int cmd = in_code[in_ip];
        in_command_out_command_match[in_ip] = &out_code[out_ip];

        CommandParse((COMMANDS)cmd, in_code, &in_ip, out_code, &out_ip, ram, in_command_out_command_match);

        #ifdef DEBUG
            printf("cmd    = %d\n", cmd & CMD_MASK);
            printf("cmd    = %b\n", cmd);
            DumpInOutCode(in_code, in_ip, out_code, out_ip);            
        #endif
    }
    in_command_out_command_match[in_ip] = &out_code[out_ip];
    EmitHlt(out_code, &out_ip);     //end of program
    //==================================================================
    
    //==========================SECOND PASS==============================
    printf("Second compilation...\n");
    in_ip  = 0;
    out_ip = 0;
    while (in_ip < in_header->commands_number)
    {
        int cmd = in_code[in_ip];
        CommandParse((COMMANDS)cmd, in_code, &in_ip, out_code, &out_ip, ram, in_command_out_command_match);
    }
    EmitHlt(out_code, &out_ip);     //end of program
    //==================================================================


    #ifdef DEBUG
        DumpInOutCode(in_code, in_ip, out_code, out_ip);    
    #endif

    free(in_command_out_command_match);

    return 0;
}

void CommandParse(COMMANDS cmd, int* in_code, size_t* in_ip, char* out_code, size_t* out_ip, char* ram, char** in_command_out_command_match)
{
    switch ((int)cmd & CMD_MASK)
    {
        case CMD_ADD:
        {
            #ifdef DEBUG
                printf("ADD\n");
            #endif

            (*in_ip)++;
            EmitAddSub(out_code, out_ip, x86_ADD);
            break;
        }

        case CMD_SUB:
        {
            #ifdef DEBUG
                printf("SUB\n");
            #endif
                
            (*in_ip)++;
            EmitAddSub(out_code, out_ip, x86_SUB);
            break;                
        }

        case CMD_MUL:
        {  
            #ifdef DEBUG
                printf("MUL\n");
            #endif
                
            (*in_ip)++; 
            EmitMulDiv(out_code, out_ip, true);
            break;
        }

        case CMD_DIV:
        {
            #ifdef DEBUG
                printf("DIV\n");
            #endif
                
            (*in_ip)++;
            EmitMulDiv(out_code, out_ip, false);
            break;
        }

        case CMD_HLT:
        {
            #ifdef DEBUG
                printf("HLT\n");
            #endif
                
            (*in_ip)++;
            EmitHlt(out_code, out_ip);
            break;
        }

        case CMD_PUSH:
        {
            #ifdef DEBUG
                printf("PUSH\n");
            #endif
            
            EmitPush(out_code, out_ip, in_code, in_ip, ram);
            break;
        }

        case CMD_POP:
        {
            #ifdef DEBUG
                printf("POP\n");
            #endif
                
            EmitPop(out_code, out_ip, in_code, in_ip, ram);
            break;
        }

        case CMD_CALL:
        {
            #ifdef DEBUG
                printf("CALL\n");
            #endif

            (*in_ip)++;
            EmitCall(out_code, out_ip, in_code, in_ip, in_command_out_command_match);
            break;
        }

        case CMD_RET:
        {
            #ifdef DEBUG
                printf("RET\n");
            #endif

            (*in_ip)++;
            EmitReturn(out_code, out_ip);
            break;
        }

        case CMD_JMP:
        {
            #ifdef DEBUG
                printf("JMP\n");
            #endif

            (*in_ip)++;
            EmitJmp(out_code, out_ip, in_code, in_ip, in_command_out_command_match);
            break;
        }

        case CMD_JA:
        case CMD_JAE:
        case CMD_JB:
        case CMD_JBE:
        case CMD_JE:
        case CMD_JNE:
        {
            #ifdef DEBUG
                printf("COND JMP\n");
            #endif

            (*in_ip)++;
            EmitConditionalJmp(out_code, out_ip, in_code, in_ip, cmd, in_command_out_command_match);
            break;
        }

        case CMD_IN:
        {
            #ifdef DEBUG
                printf("IN\n");
            #endif
            
            (*in_ip)++;
            EmitIn(out_code, out_ip);
            break;
        }
        case CMD_OUT:
        {
            #ifdef DEBUG
                printf("OUT\n");
            #endif
            (*in_ip)++;
            EmitOut(out_code, out_ip);
            break;
        }

        case CMD_SQRT:
        {
            #ifdef DEBUG
                printf("SQRT\n");
            #endif
            (*in_ip)++;
            EmitSqrt(out_code, out_ip);
            break;
        }

        default:
            break;
    }    
}

void EmitCallReg(char* code, size_t* ip, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        code[(*ip)++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }

    code[(*ip)++] = 0xff;
    code[(*ip)++] = x86_CALL | reg;
}

void EmitIn(char* code, size_t* ip)
{
    EmitPushAllRegs(code, ip);

    EmitMovAbsInReg(code, ip, (size_t)InputNumber10, x86_RAX);  //
    EmitCallReg(code, ip, x86_RAX);                             //call OutputNum10
    
    EmitMoveRegToReg(code, ip, x86_R9, x86_RAX);

    EmitPopAllRegs(code, ip);
    
    EmitPushPopReg(code, ip, x86_PUSH, x86_R9);
}

void EmitOut(char* code, size_t* ip)
{
    EmitPushPopReg(code, ip, x86_POP, x86_R10);
    EmitPushAllRegs(code, ip);

    EmitMoveRegToReg(code, ip, x86_RDI, x86_R10);               //argument for OutNumber10

    EmitMovAbsInReg(code, ip, (size_t)OutputNumber10, x86_RAX); //
    EmitCallReg(code, ip, x86_RAX);                             //call OutputNum10

    EmitPopAllRegs(code, ip);
}

void EmitJmp(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char** in_command_out_command_match)
{
    size_t in_code_label = in_code[(*in_ip)++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];

    EmitMovAbsInReg(out_code, out_ip, label, x86_R8);       //movabs r8, label
    EmitJmpToReg(out_code, out_ip, x86_R8);                 //jmp r8
}

void EmitConditionalJmp(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, COMMANDS command, char** in_command_out_command_match)
{
    size_t in_code_label = in_code[(*in_ip)++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];

    EmitPushPopReg(out_code, out_ip, x86_POP, x86_R9);  //pop r9
    EmitPushPopReg(out_code, out_ip, x86_POP, x86_R8);  //pop r8

    EmitCmpTwoReg(out_code, out_ip, x86_R8, x86_R9);    //cmp r8, r9

    x86_COMMANDS jmp_cond = ConditionalJmpConversion(command);
    long long offset = 0;
    if (label < ((size_t)&out_code[*out_ip]))
        offset = (long long)label - ((long long)&out_code[*out_ip]);
    else
        offset = (long long)label - (long long)((size_t)&out_code[*out_ip] + 2 + sizeof(int));

    out_code[(*out_ip)++] = 0xf;
    out_code[(*out_ip)++] = jmp_cond;

    memcpy(&out_code[*out_ip], &offset, sizeof(int));
    (*out_ip) += 4;
}

void EmitAddSub(char* code, size_t* ip, x86_COMMANDS command)
{
    assert(code && ip);

    EmitPushPopReg(code, ip, x86_POP, x86_R9);    //pop r9
    EmitPushPopReg(code, ip, x86_POP, x86_R8);    //pop r8
    
    EmitAddSubRegs(code, ip, command, x86_R8, x86_R9);  //add(sub) r8, r9

    EmitPushPopReg(code, ip, x86_PUSH, x86_R8);  //push r8
}

//!
//!is_mul = true  if mul
//!       = false if div
//!
void EmitMulDiv(char* code, size_t* ip, bool is_mul)
{
    assert(code && ip);

    EmitPushPopReg(code, ip, x86_POP, x86_R8);        // pop r8
    EmitPushPopReg(code, ip, x86_POP, x86_R9);        // pop r9

    EmitMoveRegToReg(code, ip, x86_R10, x86_RAX);           // mov r10, rax ;put old rax value in r10
    EmitMoveRegToReg(code, ip, x86_R11, x86_RDX);           // save rdx value to r11
    
    EmitMoveRegToReg(code, ip, x86_RAX, x86_R9);            // mov rax, r9

    EmitCqo(code, ip);                                      //cqo 

    x86_REGISTERS reg = x86_R8;
    PutPrefixForOneReg(code, ip, &reg);

    code[(*ip)++] = x86_MUL;
    if (is_mul)
        code[(*ip)++] = x86_MUL_WITH_REG | x86_R8;
    else
        code[(*ip)++] = x86_DIV_WITH_REG | x86_R8;

    EmitPushPopReg  (code, ip, x86_PUSH, x86_RAX);          // push rax
    EmitMoveRegToReg(code, ip, x86_RAX, x86_R10);           // mov r10, rax  ;recover rax
    EmitMoveRegToReg(code, ip, x86_RDX, x86_R11);           // recover rdx
}

int EmitPush(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram)
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
            out_code[(*out_ip)++] = 0xb4;                 // push ...
            out_code[(*out_ip)++] = reg | x86_RDI << 3;   //      ...[reg + rdi] ([reg + rdi], because rdi - pointer to ram for this program)
        }
        else if (command & ARG_NUM)
        {
            number *= sizeof(size_t);
            out_code[(*out_ip)++] = 0xb0 | x86_RDI;
        }

        memcpy(&out_code[*out_ip], &number, sizeof(int));
        *out_ip += sizeof(int);
    }
    else if (command & ARG_REG)
    {
        EmitPushPopReg(out_code, out_ip, x86_PUSH, reg);
    }
    else if (command & ARG_NUM)
    {
        out_code[(*out_ip)++] = x86_PUSH_N;

        memcpy(&out_code[*out_ip], &number, sizeof(int));   //TODO обёртка
        *out_ip += sizeof(int);                             //Put number in 4 bytes
    }
    
    return 0;
}

int EmitPop(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char* ram)
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
        {
            number *= sizeof(size_t);
            out_code[(*out_ip)++] = 0x80 | x86_RDI;
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
        memcpy(&out_code[*out_ip], &number, sizeof(int));   //
        *out_ip += sizeof(int);                             //Put number in 4 bytes
    }

    return 0;
}

void EmitReturn(char* code, size_t* ip)
{
    EmitAddSubNumWithReg(code, ip, x86_RSI, 8, x86_SUB); // sub rsi, 8

    code[(*ip)++] = 0xff;                                //
    code[(*ip)++] = 0x20 | x86_RSI;                      //jmp [rsi]
}

void EmitJmpToReg(char* code, size_t* ip, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        code[(*ip)++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }
    
    code[(*ip)++] = 0xff;                                           //
    code[(*ip)++] = x86_JMP | reg;                                  //jmp rax
}

int EmitCall(char* out_code, size_t* out_ip, int* in_code, size_t* in_ip, char** in_command_out_command_match)
{
    size_t in_code_label = in_code[(*in_ip)++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];

    EmitMovAbsInReg(out_code, out_ip, 0, x86_R8);           //movabs r8, return_address (return_address will be put here later) <-------
                                                                                                                        //             |
    size_t* ret_address = (size_t*)&out_code[(*out_ip) - 8];                                                            //             |
                                                                                                                        //             |
    x86_REGISTERS reg2 = x86_R8;                            //                                                          //             |
    x86_REGISTERS reg1 = x86_RSI;                           //                                                          //             |
    PutPrefixForTwoReg(out_code, out_ip, &reg1, &reg2);     //                                                          //             |
    out_code[(*out_ip)++] = x86_MOV;                        //                                                          //             |
    out_code[(*out_ip)++] = (char)(reg1 | (reg2 << 3));     //mov [rsi], r8                                             //             |
                                                                                                                        //             |
    EmitAddSubNumWithReg(out_code, out_ip, x86_RSI, 8, x86_ADD);// add rsi, 8                                           //             |
    EmitMovAbsInReg(out_code, out_ip, label, x86_R8);           //movabs r8, label                                      //             |
    EmitJmpToReg(out_code, out_ip, x86_R8);                     //jmp r8                                                //             |
                                                                                                                        //             |
    size_t curr_address = (size_t)&out_code[*out_ip];     //put return_address --------------------------------------------------------|
    memcpy(ret_address, &curr_address, sizeof(size_t));

    return 0;
}

int EmitHlt(char* code, size_t* ip)
{
    code[(*ip)++] = x86_RET;
    return 0;
}

void EmitSqrt(char* code, size_t* ip)
{
    EmitPushPopReg(code, ip, x86_POP, x86_R10);
    EmitPushAllRegs(code, ip);

    EmitMoveRegToReg(code, ip, x86_RDI, x86_R10);               //argument for sqrt

    EmitMovAbsInReg(code, ip, (size_t)SqrtInt, x86_RAX);        //
    EmitCallReg(code, ip, x86_RAX);                             //call sqrt

    EmitMoveRegToReg(code, ip, x86_R10, x86_RAX);
    EmitPopAllRegs(code, ip);

    EmitPushPopReg(code, ip, x86_PUSH, x86_R10);
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

void EmitPushAllRegs(char* code, size_t* ip)
{
    EmitPushPopReg(code, ip, x86_PUSH, x86_RAX);
    EmitPushPopReg(code, ip, x86_PUSH, x86_RBX);
    EmitPushPopReg(code, ip, x86_PUSH, x86_RCX);
    EmitPushPopReg(code, ip, x86_PUSH, x86_RDX);
    EmitPushPopReg(code, ip, x86_PUSH, x86_RSI);
    EmitPushPopReg(code, ip, x86_PUSH, x86_RDI);
}

void EmitPopAllRegs(char* code, size_t* ip)
{
    EmitPushPopReg(code, ip, x86_POP, x86_RDI);
    EmitPushPopReg(code, ip, x86_POP, x86_RSI);
    EmitPushPopReg(code, ip, x86_POP, x86_RDX);
    EmitPushPopReg(code, ip, x86_POP, x86_RCX);
    EmitPushPopReg(code, ip, x86_POP, x86_RBX);
    EmitPushPopReg(code, ip, x86_POP, x86_RAX);
}

void DumpInOutCode(int* in_code, size_t in_ip, char* out_code, size_t out_ip)
{
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
}

void EmitCmpTwoReg(char* code, size_t* ip, x86_REGISTERS reg1, x86_REGISTERS reg2)
{
    PutPrefixForTwoReg(code, ip, &reg1, &reg2);

    code[(*ip)++] = x86_CMP;
    code[(*ip)++] = 0xc0 | reg1 | (reg2 << 3);
}

x86_COMMANDS ConditionalJmpConversion(COMMANDS command)
{
    switch (command)        //reverb because soft CPU architecture
    {
        case CMD_JA:
            return x86_JLE;
        case CMD_JAE:
            return x86_JL;
        case CMD_JB:
            return x86_JGE;
        case CMD_JBE:
            return x86_JG;
        case CMD_JE:
            return x86_JNE;
        case CMD_JNE:
            return x86_JE;
            
        default:
            return x86_ERROR_CMD;
    }
}

void EmitSyscall(char* code, size_t* ip)
{
    code[(*ip)++] = 0x0F;
    code[(*ip)++] = 0x05;
}

void EmitAddSubNumWithReg(char* code, size_t* ip, x86_REGISTERS reg, int number, x86_COMMANDS command)
{
    PutPrefixForOneReg(code, ip, &reg);

    code[(*ip)++] = 0x81;

    if (command == x86_ADD)
        code[(*ip)++] = 0xc0 | reg;
    else
        code[(*ip)++] = 0xe8 | reg;

    memcpy(&code[*ip], &number, sizeof(int));
    *ip += sizeof(int);
}

void NullifyReg(char* code, size_t* ip, x86_REGISTERS reg)
{
    x86_REGISTERS reg1 = reg;
    x86_REGISTERS reg2 = reg;
    PutPrefixForTwoReg(code, ip, &reg1, &reg2);

    code[(*ip)++] = x86_XOR;
    code[(*ip)++] = 0xc0 | (reg << 3) | reg;    
}

void EmitAddSubRegs(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg1, x86_REGISTERS reg2)
{
    PutPrefixForTwoReg(code, ip, &reg1, &reg2);
    code[(*ip)++] = command;
    code[(*ip)++] = 0xc0 | reg1 | (reg2 << 3);
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

void EmitMoveRegToReg(char* code, size_t* ip, x86_REGISTERS reg_to, x86_REGISTERS reg_from)
{
    PutPrefixForTwoReg(code, ip, &reg_to, &reg_from);

    code[(*ip)++] = x86_MOV;
    code[(*ip)++] = 0xc0 | reg_to | (reg_from << 3);
}

void EmitPushPopReg(char* code, size_t* ip, x86_COMMANDS command, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        code[(*ip)++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }
    code[(*ip)++] = (char)command | (char)reg;
}

void EmitMovAbsInReg(char* code, size_t* ip, size_t number, x86_REGISTERS reg)
{
    PutPrefixForOneReg(code, ip, &reg);
    
    code[(*ip)++] = x86_MOV_ABS | reg;                      //
    memcpy(&code[*ip], &number, sizeof(size_t));            //
    *ip += sizeof(size_t);                                  //mov reg, number
}

void EmitIncDec(char* code, size_t* ip, x86_REGISTERS reg, x86_COMMANDS command)
{
    PutPrefixForOneReg(code, ip, &reg);
    
    code[(*ip)++] = 0xff;
    code[(*ip)++] = command | reg;
}

void EmitCqo(char* code, size_t* ip)
{
    code[(*ip)++] = 0x48;
    code[(*ip)++] = 0x99; 
}