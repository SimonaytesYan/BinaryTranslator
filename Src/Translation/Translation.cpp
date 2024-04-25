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

#define DEBUG
// #define GET_TIME

struct Context
{
    int*   in_code  = nullptr;
    char*  out_code = nullptr;
    size_t out_ip   = 0;
    size_t in_ip    = 0;

    char* ram       = nullptr;
};

//==========================================FUNCTION PROTOTYPES===========================================

int              Translate(int* in_code, char* out_code, MyHeader* in_header, char* ram);
extern "C" void  Run(char* out_code);

void  EmitAddSub(Context* ctx, x86_COMMANDS command);
int   EmitCall(Context* ctx, char** in_command_out_command_match);
void  EmitConditionalJmp(Context* ctx, COMMANDS command, char** in_command_out_command_match);
void  EmitIn(Context* ctx);
int   EmitHlt(Context* ctx);
void  EmitJmp(Context* ctx, char** in_command_out_command_match);
void  EmitMulDiv(Context* ctx, bool is_mul);
void  EmitOut(Context* ctx);
int   EmitPop(Context* ctx);
int   EmitPush(Context* ctx);
void  EmitReturn(Context* ctx);
void  EmitSqrt(Context* ctx);

x86_REGISTERS ConvertMyRegInx86Reg(REGISTERS reg);
x86_COMMANDS  ConditionalJmpConversion(COMMANDS command);
int           CommandParse(COMMANDS cmd, Context* ctx, char** in_command_out_command_match);
void          EmitMovAbsInReg(Context* ctx, size_t number, x86_REGISTERS reg);
void          EmitIncDec(Context* ctx, x86_REGISTERS reg, x86_COMMANDS command);
void          EmitPushPopReg(Context* ctx, x86_COMMANDS command, x86_REGISTERS reg);
void          ParsePushPopArguments(Context* ctx, int* command, unsigned int* number, x86_REGISTERS* reg);
void          PutPrefixForTwoReg(Context* ctx, x86_REGISTERS *reg1, x86_REGISTERS *reg2);
void          PutPrefixForOneReg(Context* ctx, x86_REGISTERS* reg);
void          EmitMoveRegToReg(Context* ctx, x86_REGISTERS reg_to, x86_REGISTERS reg_from);
void          DumpInOutCode(int* in_code, size_t in_ip, char* out_code, size_t out_ip);
void          EmitAddSubAndOrRegs(Context* ctx, x86_COMMANDS command, x86_REGISTERS reg1, x86_REGISTERS reg2);
void          EmitSyscall(Context* ctx);
void          NullifyReg(Context* ctx, x86_REGISTERS reg);
void          EmitAddSubNumWithReg(Context* ctx, x86_REGISTERS reg, int number, x86_COMMANDS command);
void          EmitJmpToReg(Context* ctx, x86_REGISTERS reg);
void          EmitCmpTwoReg(Context* ctx, x86_REGISTERS reg1, x86_REGISTERS reg2);
void          EmitPushAllRegs(Context* ctx);
void          EmitPopAllRegs(Context* ctx);
void          EmitCqo(Context* ctx);
void            EmitCondJmpInstruction(Context* ctx, COMMANDS command, const int offset);

double        CalcAndPrintfStdDeviation(const double data[], const size_t number_meas);
void ContextCtor(Context* ctx, int* in_code, char* out_code, size_t in_ip, size_t out_ip, char* ram);

//==========================================FUNCTION IMPLEMENTATION===========================================

void TranslateAndRun(char* in_bin_filepath, size_t in_file_size, MyHeader in_bin_header)
{
    assert(in_bin_filepath);

    printf("Translating...\n");
    int in_bin_fd = open(in_bin_filepath, O_RDWR);

    char* out_code   = (char*)mmap(NULL, 4096,         PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    int*  in_code    = (int*) mmap(NULL, in_file_size, PROT_READ,              MAP_PRIVATE,                 in_bin_fd, 0);

    char* ram        = (char*)mmap(NULL, RAM_SIZE,        PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
    char* call_stack = (char*)mmap(NULL, CALL_STACK_SIZE, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);

    Context ctx = {};
    ContextCtor(&ctx, in_code, out_code, 0, 0, ram);

    EmitMovAbsInReg(&ctx, (size_t)call_stack, x86_RSI);    //Put call stack pointer to rsi 
    EmitMovAbsInReg(&ctx, (size_t)ram,        x86_RDI);    //Put ram pointer to rdi
    
    NullifyReg(&ctx, x86_RAX);                             //
    NullifyReg(&ctx, x86_RBX);                             //
    NullifyReg(&ctx, x86_RCX);                             //Nullify all user registers
    NullifyReg(&ctx, x86_RDX);                             //

    Translate((int*)((char*)in_code + sizeof(MyHeader)), &out_code[ctx.out_ip] , &in_bin_header, ram);

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

extern "C" void Run(char* out_code)
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

void ContextCtor(Context* ctx, int* in_code, char* out_code, size_t in_ip, size_t out_ip, char* ram)
{
    assert(ctx);

    ctx->in_code  = in_code;
    ctx->in_ip    = in_ip;
    ctx->out_code = out_code;
    ctx->out_ip   = out_ip;
    ctx->ram      = ram;
}

int Translate(int* in_code, char* out_code, MyHeader* in_header, char* ram)
{
    char** in_command_out_command_match = (char**)calloc(in_header->commands_number + 1, sizeof(char*));

    Context ctx  = {};
    ContextCtor(&ctx, in_code, out_code, 0, 0, ram);

    //==========================FIRST PASS==============================
    printf("First compilation...\n");
    while (ctx.in_ip < in_header->commands_number)
    {
        int cmd = in_code[ctx.in_ip];
        in_command_out_command_match[ctx.in_ip] = &out_code[ctx.out_ip];
        
        #ifdef DEBUG
            printf("cmd = %d\n", cmd & CMD_MASK);
            printf("cmd = %b\n", cmd);
        #endif

        assert(CommandParse((COMMANDS)cmd, &ctx, in_command_out_command_match) == 0);
        #ifdef DEBUG
            DumpInOutCode(ctx.in_code, ctx.in_ip, ctx.out_code, ctx.out_ip); 
        #endif           
    }
    in_command_out_command_match[ctx.in_ip] = &out_code[ctx.out_ip];
    EmitHlt(&ctx);     //end of program
    //==================================================================
    
    //==========================SECOND PASS==============================
    printf("Second compilation...\n");
    ctx.in_ip  = 0;
    ctx.out_ip = 0;
    while (ctx.in_ip < in_header->commands_number)
    {
        int cmd = in_code[ctx.in_ip];
        CommandParse((COMMANDS)cmd, &ctx, in_command_out_command_match);
    }
    EmitHlt(&ctx);     //end of program
    //==================================================================


    #ifdef DEBUG
        DumpInOutCode(ctx.in_code, ctx.in_ip, ctx.out_code, ctx.out_ip);    
    #endif

    free(in_command_out_command_match);

    return 0;
}

COMMANDS ConvertLogicalOpToCondJmp(COMMANDS cmd)
{
    switch (cmd)
    {
        case CMD_IS_EQ:
            return CMD_JE;
        case CMD_IS_NE:
            return CMD_JNE;
        case CMD_IS_B:
            return CMD_JA;
        case CMD_IS_BE:
            return CMD_JAE;
        case CMD_IS_S:
            return CMD_JB;
        case CMD_IS_SE:
            return CMD_JBE;
        case CMD_AND:
        case CMD_OR:
        default:
            break;
    }

    return (COMMANDS)-1;
}

void DecodeAndEmitLogicalOperator(Context* ctx, COMMANDS cmd)
{
    EmitPushPopReg(ctx, x86_POP, x86_R8);   // pop r8
    EmitPushPopReg(ctx, x86_POP, x86_R9);   // pop r9
    if (cmd == CMD_OR || cmd == CMD_AND)
    {
        EmitMovAbsInReg(ctx, 0, x86_R10);           // mov r10, 0

        EmitCmpTwoReg(ctx, x86_R8, x86_R10);        // cmp r8, r10
        EmitCondJmpInstruction(ctx, CMD_JE , 7);    // je label
        EmitMovAbsInReg(ctx, 1, x86_R8);            // mov r8, 1
                                                    // label:
        
        EmitCmpTwoReg(ctx, x86_R9, x86_R10);        // cmp r9, r10
        EmitCondJmpInstruction(ctx, CMD_JE , 7);    // je label1
        EmitMovAbsInReg(ctx, 1, x86_R9);            // mov r9, 1
                                                    // label1:
        
        if (cmd == CMD_OR)
            EmitAddSubAndOrRegs(ctx, x86_OR, x86_R8, x86_R9);   // or r8, r9
        else if (cmd == CMD_AND)
            EmitAddSubAndOrRegs(ctx, x86_AND, x86_R8, x86_R9);  // and r8, r9
        
        EmitPushPopReg(ctx, x86_PUSH, x86_R8);          // push r8
    }
    else
    {
        EmitCmpTwoReg(ctx, x86_R8, x86_R9);     // cmp r8, r9

        EmitMovAbsInReg(ctx, 1, x86_R8);        // mov r8, 1
        EmitCondJmpInstruction(ctx, ConvertLogicalOpToCondJmp(cmd), 7);  // cond_jmp label

        EmitMovAbsInReg(ctx, 0, x86_R8);        // mov r8, 0
                                                // label:
    }
}

int CommandParse(COMMANDS cmd, Context* ctx, char** in_command_out_command_match)
{
    const int cmd_mask_command = (int)cmd & CMD_MASK;
    switch (cmd_mask_command)
    {
        case CMD_ADD:
        {
            #ifdef DEBUG
                printf("ADD\n");
            #endif

            ctx->in_ip++;
            EmitAddSub(ctx, x86_ADD);
            break;
        }

        case CMD_SUB:
        {
            #ifdef DEBUG
                printf("SUB\n");
            #endif
                
            ctx->in_ip++;
            EmitAddSub(ctx, x86_SUB);
            break;                
        }

        case CMD_MUL:
        {  
            #ifdef DEBUG
                printf("MUL\n");
            #endif
                
            ctx->in_ip++; 
            EmitMulDiv(ctx, true);
            break;
        }

        case CMD_DIV:
        {
            #ifdef DEBUG
                printf("DIV\n");
            #endif
                
            ctx->in_ip++;
            EmitMulDiv(ctx, false);
            break;
        }

        case CMD_HLT:
        {
            #ifdef DEBUG
                printf("HLT\n");
            #endif
                
            ctx->in_ip++;
            EmitHlt(ctx);
            break;
        }

        case CMD_PUSH:
        {
            #ifdef DEBUG
                printf("PUSH\n");
            #endif
            
            EmitPush(ctx);
            break;
        }

        case CMD_POP:
        {
            #ifdef DEBUG
                printf("POP\n");
            #endif
                
            EmitPop(ctx);
            break;
        }

        case CMD_CALL:
        {
            #ifdef DEBUG
                printf("CALL\n");
            #endif

            ctx->in_ip++;
            EmitCall(ctx, in_command_out_command_match);
            break;
        }

        case CMD_RET:
        {
            #ifdef DEBUG
                printf("RET\n");
            #endif

            ctx->in_ip++;
            EmitReturn(ctx);
            break;
        }

        case CMD_JMP:
        {
            #ifdef DEBUG
                printf("JMP\n");
            #endif

            ctx->in_ip++;
            EmitJmp(ctx, in_command_out_command_match);
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

            ctx->in_ip++;
            EmitConditionalJmp(ctx, cmd, in_command_out_command_match);
            break;
        }

        case CMD_IN:
        {
            #ifdef DEBUG
                printf("IN\n");
            #endif
            
            ctx->in_ip++;
            EmitIn(ctx);
            break;
        }
        case CMD_OUT:
        {
            #ifdef DEBUG
                printf("OUT\n");
            #endif
            ctx->in_ip++;
            EmitOut(ctx);
            break;
        }

        case CMD_SQRT:
        {
            #ifdef DEBUG
                printf("SQRT\n");
            #endif
            ctx->in_ip++;
            EmitSqrt(ctx);
            break;
        }

        case CMD_IS_EQ:
        case CMD_IS_NE:
        case CMD_IS_B:
        case CMD_IS_BE:
        case CMD_IS_S:
        case CMD_IS_SE:
        case CMD_AND:
        case CMD_OR:
        {
            ctx->in_ip++;
            DecodeAndEmitLogicalOperator(ctx, cmd);
            break;
        }

        default:
            printf("Unknown instruction %d(%b)\n", cmd_mask_command, cmd_mask_command);
            return -1;
    }    

    return 0;
}

void EmitCallReg(Context* ctx, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        ctx->out_code[ctx->out_ip++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }

    ctx->out_code[ctx->out_ip++] = 0xff;
    ctx->out_code[ctx->out_ip++] = x86_CALL | reg;
}

void EmitIn(Context* ctx)
{
    EmitPushAllRegs(ctx);

    EmitMovAbsInReg(ctx, (size_t)InputNumber10, x86_RAX);  //
    EmitCallReg(ctx, x86_RAX);                             //call OutputNum10
    
    EmitMoveRegToReg(ctx, x86_R9, x86_RAX);

    EmitPopAllRegs(ctx);
    
    EmitPushPopReg(ctx, x86_PUSH, x86_R9);
}

void EmitOut(Context* ctx)
{
    EmitPushPopReg(ctx, x86_POP, x86_R10);
    EmitPushAllRegs(ctx);

    EmitMoveRegToReg(ctx, x86_RDI, x86_R10);               //argument for OutNumber10

    EmitMovAbsInReg(ctx, (size_t)OutputNumber10, x86_RAX); //
    EmitCallReg(ctx, x86_RAX);                             //call OutputNum10

    EmitPopAllRegs(ctx);
}

void EmitJmp(Context* ctx, char** in_command_out_command_match)
{
    size_t in_code_label = (int)ctx->in_code[ctx->in_ip++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];

    EmitMovAbsInReg(ctx, label, x86_R8);       //movabs r8, label
    EmitJmpToReg(ctx, x86_R8);                 //jmp r8
}

void EmitCondJmpInstruction(Context* ctx, COMMANDS command, const int offset)
{
    ctx->out_code[ctx->out_ip++] = 0xf;
    ctx->out_code[ctx->out_ip++] = ConditionalJmpConversion(command);

    memcpy(&ctx->out_code[ctx->out_ip], &offset, sizeof(int));
    ctx->out_ip += 4;
}

void EmitConditionalJmp(Context* ctx, COMMANDS command, char** in_command_out_command_match)
{
    size_t in_code_label = ctx->in_code[ctx->in_ip++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];

    #ifdef DEBUG
        printf("in_code_label = %zu\n", in_code_label);
        printf("label = %zu\n", label);
    #endif

    EmitPushPopReg(ctx, x86_POP, x86_R9);  //pop r9
    EmitPushPopReg(ctx, x86_POP, x86_R8);  //pop r8

    EmitCmpTwoReg(ctx, x86_R8, x86_R9);    //cmp r8, r9

    long long offset = 0;
    if (label != 0)                                             //unknown label for now (in first compilation pass)
    {
        if (label < ((size_t)&(ctx->out_code[ctx->out_ip])))
            offset = (long long)label - ((long long)&ctx->out_code[ctx->out_ip]);
        else
            offset = (long long)label - (long long)((size_t)ctx->out_code[ctx->out_ip] + 2 + sizeof(int));
    }

    EmitCondJmpInstruction(ctx, command, offset);
}

void EmitAddSub(Context* ctx, x86_COMMANDS command)
{
    assert(ctx);

    EmitPushPopReg(ctx, x86_POP, x86_R9);    //pop r9
    EmitPushPopReg(ctx, x86_POP, x86_R8);    //pop r8
    
    EmitAddSubAndOrRegs(ctx, command, x86_R8, x86_R9);  //add(sub) r8, r9

    EmitPushPopReg(ctx, x86_PUSH, x86_R8);  //push r8
}

//!
//!is_mul = true  if mul
//!       = false if div
//!
void EmitMulDiv(Context* ctx, bool is_mul)
{
    assert(ctx);

    EmitPushPopReg(ctx, x86_POP, x86_R8);        // pop r8
    EmitPushPopReg(ctx, x86_POP, x86_R9);        // pop r9

    EmitMoveRegToReg(ctx, x86_R10, x86_RAX);           // mov r10, rax ;put old rax value in r10
    EmitMoveRegToReg(ctx, x86_R11, x86_RDX);           // save rdx value to r11
    
    EmitMoveRegToReg(ctx, x86_RAX, x86_R9);            // mov rax, r9

    EmitCqo(ctx);                                      //cqo 

    x86_REGISTERS reg = x86_R8;
    PutPrefixForOneReg(ctx, &reg);

    ctx->out_code[ctx->out_ip++] = x86_MUL;
    if (is_mul)
        ctx->out_code[ctx->out_ip++] = x86_MUL_WITH_REG | x86_R8;
    else
        ctx->out_code[ctx->out_ip++] = x86_DIV_WITH_REG | x86_R8;

    EmitPushPopReg  (ctx, x86_PUSH, x86_RAX);          // push rax
    EmitMoveRegToReg(ctx, x86_RAX, x86_R10);           // mov r10, rax  ;recover rax
    EmitMoveRegToReg(ctx, x86_RDX, x86_R11);           // recover rdx
}

int EmitPush(Context* ctx)
{
    int           command = 0;
    unsigned int  number  = 0;
    x86_REGISTERS reg     = x86_RAX;
    ParsePushPopArguments(ctx, &command, &number, &reg);

    if (command & ARG_MEM)
    {
        ctx->out_code[ctx->out_ip++] = x86_PUSH_MEM;

        if (command & ARG_REG)
        {
            ctx->out_code[ctx->out_ip++] = 0xb4;                 // push ...
            ctx->out_code[ctx->out_ip++] = reg | x86_RDI << 3;   //      ...[reg + rdi] ([reg + rdi], because rdi - pointer to ram for this program)
        }
        else if (command & ARG_NUM)
        {
            number *= sizeof(size_t);
            ctx->out_code[ctx->out_ip++] = 0xb0 | x86_RDI;
        }

        memcpy(&ctx->out_code[ctx->out_ip], &number, sizeof(int));
        ctx->out_ip += sizeof(int);
    }
    else if (command & ARG_REG)
    {
        EmitPushPopReg(ctx, x86_PUSH, reg);
    }
    else if (command & ARG_NUM)
    {
        ctx->out_code[ctx->out_ip++] = x86_PUSH_N;

        memcpy(&ctx->out_code[ctx->out_ip], &number, sizeof(int));   //TODO обёртка
        ctx->out_ip += sizeof(int);                             //Put number in 4 bytes
    }
    
    return 0;
}

int EmitPop(Context* ctx)
{
    int           command = 0;
    unsigned int  number  = 0;
    x86_REGISTERS reg     = x86_RAX;

    ParsePushPopArguments(ctx, &command, &number, &reg);

    if (command & ARG_MEM)
    {
        ctx->out_code[ctx->out_ip++] = x86_POP_MEM;

        if (command & ARG_REG)
        {
            ctx->out_code[ctx->out_ip++] = 0x84;
            ctx->out_code[ctx->out_ip]   = x86_RDI << 3; // Dont increment out_ip, because we should make or this byte with register.
                                                  // Program will do it later
        }
        else if (command & ARG_NUM)
        {
            number *= sizeof(size_t);
            ctx->out_code[ctx->out_ip++] = 0x80 | x86_RDI;
        }

    }
    if (command & ARG_REG)
    {
        if (!(command & ARG_MEM))
            ctx->out_code[ctx->out_ip] = x86_POP;
        ctx->out_code[ctx->out_ip++] |= (char)reg;
    }
    if (command & ARG_NUM)
    {
        memcpy(&ctx->out_code[ctx->out_ip], &number, sizeof(int));   //
        ctx->out_ip += sizeof(int);                             //Put number in 4 bytes
    }

    return 0;
}

void EmitReturn(Context* ctx)
{
    EmitAddSubNumWithReg(ctx, x86_RSI, 8, x86_SUB); // sub rsi, 8

    ctx->out_code[ctx->out_ip++] = 0xff;                                //
    ctx->out_code[ctx->out_ip++] = 0x20 | x86_RSI;                      //jmp [rsi]
}

void EmitJmpToReg(Context* ctx, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        ctx->out_code[ctx->out_ip++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }
    
    ctx->out_code[ctx->out_ip++] = 0xff;                                           //
    ctx->out_code[ctx->out_ip++] = x86_JMP | reg;                                  //jmp rax
}

int EmitCall(Context* ctx, char** in_command_out_command_match)
{
    size_t in_code_label = ctx->in_code[ctx->in_ip++];
    size_t label         = (size_t)in_command_out_command_match[in_code_label];

    EmitMovAbsInReg(ctx, 0, x86_R8);                                //movabs r8, return_address (return_address will be put here later) <-------
                                                                                                                                //              |
    size_t* ret_address = (size_t*)&ctx->out_code[ctx->out_ip - 8];                                                             //              |
                                                                                                                                //              |
    x86_REGISTERS reg2 = x86_R8;                                    //                                                          //              |
    x86_REGISTERS reg1 = x86_RSI;                                   //                                                          //              |
    PutPrefixForTwoReg(ctx, &reg1, &reg2);                          //                                                          //              |
    ctx->out_code[ctx->out_ip++] = x86_MOV;                         //                                                           //             |
    ctx->out_code[ctx->out_ip++] = (char)(reg1 | (reg2 << 3));      // mov [rsi], r8                                             //             |
                                                                                                                                //              |
    EmitAddSubNumWithReg(ctx, x86_RSI, 8, x86_ADD);                 // add rsi, 8                                                               |
    EmitMovAbsInReg(ctx, label, x86_R8);                            // movabs r8, label                                      //                 |
    EmitJmpToReg(ctx, x86_R8);                                      // jmp r8                                                //                 |
                                                                                                                                //              |
    size_t curr_address = (size_t)&ctx->out_code[ctx->out_ip];      //put return_address -------------------------------------------------------|
    memcpy(ret_address, &curr_address, sizeof(size_t));

    return 0;
}

int EmitHlt(Context* ctx)
{
    ctx->out_code[ctx->out_ip++] = x86_RET;
    return 0;
}

void EmitSqrt(Context* ctx)
{
    EmitPushPopReg(ctx, x86_POP, x86_R10);
    EmitPushAllRegs(ctx);

    EmitMoveRegToReg(ctx, x86_RDI, x86_R10);               //argument for sqrt

    EmitMovAbsInReg(ctx, (size_t)SqrtInt, x86_RAX);        //
    EmitCallReg(ctx, x86_RAX);                             //call sqrt

    EmitMoveRegToReg(ctx, x86_R10, x86_RAX);
    EmitPopAllRegs(ctx);

    EmitPushPopReg(ctx, x86_PUSH, x86_R10);
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

void EmitPushAllRegs(Context* ctx)
{
    EmitPushPopReg(ctx, x86_PUSH, x86_RAX);
    EmitPushPopReg(ctx, x86_PUSH, x86_RBX);
    EmitPushPopReg(ctx, x86_PUSH, x86_RCX);
    EmitPushPopReg(ctx, x86_PUSH, x86_RDX);
    EmitPushPopReg(ctx, x86_PUSH, x86_RSI);
    EmitPushPopReg(ctx, x86_PUSH, x86_RDI);
}

void EmitPopAllRegs(Context* ctx)
{
    EmitPushPopReg(ctx, x86_POP, x86_RDI);
    EmitPushPopReg(ctx, x86_POP, x86_RSI);
    EmitPushPopReg(ctx, x86_POP, x86_RDX);
    EmitPushPopReg(ctx, x86_POP, x86_RCX);
    EmitPushPopReg(ctx, x86_POP, x86_RBX);
    EmitPushPopReg(ctx, x86_POP, x86_RAX);
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

void EmitCmpTwoReg(Context* ctx, x86_REGISTERS reg1, x86_REGISTERS reg2)
{
    PutPrefixForTwoReg(ctx, &reg2, &reg1);

    ctx->out_code[ctx->out_ip++] = x86_CMP;
    ctx->out_code[ctx->out_ip++] = 0xc0 | reg2 | (reg1 << 3);
}

x86_COMMANDS ConditionalJmpConversion(COMMANDS command)
{
    switch (command)        //reverb because soft CPU architecture
    {
        case CMD_JA:
            return x86_JG;
        case CMD_JAE:
            return x86_JGE;
        case CMD_JB:
            return x86_JL;
        case CMD_JBE:
            return x86_JLE;
        case CMD_JE:
            return x86_JE;
        case CMD_JNE:
            return x86_JNE;
            
        default:
            return x86_ERROR_CMD;
    }
}

void EmitSyscall(Context* ctx)
{
    ctx->out_code[ctx->out_ip++] = 0x0F;
    ctx->out_code[ctx->out_ip++] = 0x05;
}

void EmitAddSubNumWithReg(Context* ctx, x86_REGISTERS reg, int number, x86_COMMANDS command)
{
    PutPrefixForOneReg(ctx, &reg);

    ctx->out_code[ctx->out_ip++] = 0x81;

    if (command == x86_ADD)
        ctx->out_code[ctx->out_ip++] = 0xc0 | reg;
    else
        ctx->out_code[ctx->out_ip++] = 0xe8 | reg;

    memcpy(&ctx->out_code[ctx->out_ip], &number, sizeof(int));
    ctx->out_ip += sizeof(int);
}

void NullifyReg(Context* ctx, x86_REGISTERS reg)
{
    x86_REGISTERS reg1 = reg;
    x86_REGISTERS reg2 = reg;
    PutPrefixForTwoReg(ctx, &reg1, &reg2);

    ctx->out_code[ctx->out_ip++] = x86_XOR;
    ctx->out_code[ctx->out_ip++] = 0xc0 | (reg << 3) | reg;    
}

void EmitAddSubAndOrRegs(Context* ctx, x86_COMMANDS command, x86_REGISTERS reg1, x86_REGISTERS reg2)
{
    PutPrefixForTwoReg(ctx, &reg1, &reg2);
    ctx->out_code[ctx->out_ip++] = command;
    ctx->out_code[ctx->out_ip++] = 0xc0 | reg1 | (reg2 << 3);
}

//!
//!Function parse push and pop commands in to command opcode + number(if any) + register(if any)
//!and move in_ip to the next command
//!
void ParsePushPopArguments(Context* ctx, int* command, unsigned int* number, x86_REGISTERS* reg)
{
    *command = ctx->in_code[ctx->in_ip++];
    if (*command & ARG_NUM)
        *number  = ctx->in_code[ctx->in_ip++];
    if (*command & ARG_REG)
        *reg     = ConvertMyRegInx86Reg((REGISTERS)ctx->in_code[ctx->in_ip++]);
}

void PutPrefixForOneReg(Context* ctx, x86_REGISTERS* reg)
{
    ctx->out_code[ctx->out_ip] = 0x48;
    if (*reg >= x86_R8)
    {
        ctx->out_code[ctx->out_ip] |= 0b1;
        *reg = (x86_REGISTERS)(*reg & 0b111);
    }
    ctx->out_ip++;
}

void PutPrefixForTwoReg(Context* ctx, x86_REGISTERS *reg1, x86_REGISTERS *reg2)
{
    ctx->out_code[ctx->out_ip] = 0x48;
    if (*reg1 >= x86_R8)
    {
        ctx->out_code[ctx->out_ip] |= 0b1;
        *reg1 = (x86_REGISTERS)(*reg1 & 0b111);
    }
    if (*reg2 >= x86_R8)
    {
        ctx->out_code[ctx->out_ip] |= 0b100;
        *reg2 = (x86_REGISTERS)(*reg2 & 0b111);        
    }
    ctx->out_ip++;
}

void EmitMoveRegToReg(Context* ctx, x86_REGISTERS reg_to, x86_REGISTERS reg_from)
{
    PutPrefixForTwoReg(ctx, &reg_to, &reg_from);

    ctx->out_code[ctx->out_ip++] = x86_MOV;
    ctx->out_code[ctx->out_ip++] = 0xc0 | reg_to | (reg_from << 3);
}

void EmitPushPopReg(Context* ctx, x86_COMMANDS command, x86_REGISTERS reg)
{
    if (reg >= x86_R8)
    {
        ctx->out_code[ctx->out_ip++] = 0x41;
        reg = (x86_REGISTERS)(reg & 0b111);
    }
    ctx->out_code[ctx->out_ip++] = (char)command | (char)reg;
}

void EmitMovAbsInReg(Context* ctx, size_t number, x86_REGISTERS reg)
{
    PutPrefixForOneReg(ctx, &reg);
    
    ctx->out_code[ctx->out_ip++] = x86_MOV_ABS | reg;               //
    memcpy(&ctx->out_code[ctx->out_ip], &number, sizeof(size_t));   //
    ctx->out_ip += sizeof(size_t);                                  //mov reg, number
}

void EmitIncDec(Context* ctx, x86_REGISTERS reg, x86_COMMANDS command)
{
    PutPrefixForOneReg(ctx, &reg);
    
    ctx->out_code[ctx->out_ip++] = 0xff;
    ctx->out_code[ctx->out_ip++] = command | reg;
}

void EmitCqo(Context* ctx)
{
    ctx->out_code[ctx->out_ip++] = 0x48;
    ctx->out_code[ctx->out_ip++] = 0x99; 
}

double CalcAndPrintfStdDeviation(const double data[], const size_t number_meas)
{
    assert(data);
    
    double sum = 0;
    for (int i = 0; i < number_meas; i++)
        sum += data[i];

    double average       = sum / (double)number_meas;
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