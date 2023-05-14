#ifndef NO_ARGS
    #define NO_ARGS     0
#endif

#ifndef COMMON_ARGS
    #define COMMON_ARGS 1
#endif

#ifndef JMP_ARGS 
    #define JMP_ARGS    2
#endif

#ifndef meow
    #define meow printf("*meow*\n");
#endif

DEF_CMD(HLT, 0, NO_ARGS,                                                    \
{                                                                           \
    return;                                                                 \
})

DEF_CMD(PUSH, 1, COMMON_ARGS,                                               \
{                                                                           \
    int arg = 0;                                                            \
    if (GetPushArg(&arg, cmd, cpu) != 0)                                    \
        return;                                                             \
    PUSH(arg);                                                              \
})

DEF_CMD(ADD, 2, NO_ARGS,                                                    \
{                                                                           \
    Elem a1 = 0, a2 = 0;                                                    \
    POP(a1);                                                                \
    POP(a2);                                                                \
    PUSH(a2 + a1);                                                          \
})

DEF_CMD(SUB, 3, NO_ARGS,                                                    \
{                                                                           \
    Elem a1 = 0, a2 = 0;                                                    \
    POP(a1);                                                                \
    POP(a2);                                                                \
    PUSH(a2 - a1);                                                          \
})

DEF_CMD(MUL, 4, NO_ARGS,                                                    \
{                                                                           \
    Elem a1 = 0, a2 = 0;                                                    \
    POP(a1);                                                                \
    POP(a2);                                                                \
    PUSH(a2 * a1);                                                          \
})

DEF_CMD(DIV, 5, NO_ARGS,                                                    \
{                                                                           \
    Elem a1 = 0, a2 = 0;                                                    \
    POP(a1);                                                                \
    POP(a2);                                                                \
    CHECK(a1 == 0, "Divison by zero\n", (void)-1);                          \
    PUSH(a2 / a1);                                                          \
})

DEF_CMD(OUT, 6, NO_ARGS,                                                    \
{                                                                           \
    Elem a1 = 0;                                                            \
    POP(a1);                                                                \
    printf("%d\n", a1);                                                     \
    meow                                                                    \
})

DEF_CMD(POP, 7, COMMON_ARGS,                                                \
{                                                                           \
    Elem a1 = 0;                                                            \
    POP(a1);                                                                \
    int arg = 0;                                                            \
    int write_to = 0;                                                       \
    if (GetPopArg(&arg, &write_to, cmd, cpu) != 0)                          \
        return;                                                             \
                                                                            \
    if (write_to == ARG_MEM)                                                \
    {                                                                       \
        cpu->ram[arg] = a1;                                                 \
    }                                                                       \
    if (write_to == ARG_REG)                                                \
    {                                                                       \
        cpu->regs[arg] = a1;                                                \
    }
})

DEF_CMD(DUMP, 8, NO_ARGS,                                                   \
{                                                                           \
    DumpCPU(cpu);                                                           \
})

DEF_CMD(JMP, 9, JMP_ARGS,                                                   \
{                                                                           \
    cpu->pc = cpu->code[cpu->pc++];                                         \
})

#define DEF_JMP_IF(name, num, oper)                                         \
DEF_CMD(name, num, JMP_ARGS, {                                              \
    Elem a1 = 0, a2 = 0;                                                    \
    POP(a1);                                                                \
    POP(a2);                                                                \
    if (a2 oper a1)                                                         \
    {                                                                       \
        cpu->pc++;                                                          \
    }                                                                       \
    else                                                                    \
        cpu->pc = cpu->code[cpu->pc];                                     \
})

DEF_JMP_IF(JA,  10, >)
DEF_JMP_IF(JAE, 11, >=)
DEF_JMP_IF(JB,  12, <)
DEF_JMP_IF(JBE, 13, <=)
DEF_JMP_IF(JE,  14, ==)
DEF_JMP_IF(JNE, 15, !=)

#undef DEF_JMP_IF

DEF_CMD(CALL, 16, JMP_ARGS,                                                 \
{                                                                           \
    StackPush(&cpu->call_stack, cpu->pc + 1);                               \
    cpu->pc = cpu->code[cpu->pc++];                                         \
})

DEF_CMD(RET, 17, NO_ARGS,                                                   \
{                                                                           \
    size_t  error = 0;                                                      \
    Elem    a     = StackPop(&(cpu->call_stack), &error);                   \
    CHECK(error != NO_ERROR, "Error during push in call stack", (void)0);   \
    cpu->pc = a;                                                            \
})

DEF_CMD(SQRT, 18, NO_ARGS,                                                  \
{                                                                           \
    Elem a = 0;                                                             \
    POP(a)                                                                  \
    PUSH(sqrt(a));                                                          \
})

DEF_CMD(IN, 19, NO_ARGS,                                                    \
{                                                                           \
    Elem val = 0;                                                           \
    scanf("%d", &val);                                                      \
    PUSH(val);                                                              \
})

DEF_CMD(DRAW, 20, NO_ARGS,                                                  \
{                                                                           \
    DrawRam(cpu);                                                           \
})           

#define LogicalOp(name, num, op)                                            \
    DEF_CMD(name, num, NO_ARGS,                                             \
    {                                                                       \
        Elem a1 = 0, a2 = 0;                                                \
        POP(a1);                                                            \
        POP(a2);                                                            \
        PUSH(a2 op a1);                                                     \
    })

LogicalOp(IS_EQ, 21, ==)
LogicalOp(IS_NE, 22, !=)
LogicalOp(IS_B,  23, >)
LogicalOp(IS_BE, 24, >=)
LogicalOp(IS_S,  25, <)
LogicalOp(IS_SE, 26, <=)
LogicalOp(AND,   27, &&)
LogicalOp(OR,    28, ||)

DEF_CMD(POW, 29, NO_ARGS,
{
    Elem a1, a2 = 0;
    POP(a1);
    POP(a2);
    PUSH(pow(a1, a2));
})

DEF_CMD(NOT, 30, NO_ARGS,
{
    Elem a1 = 0;
    POP(a1);
    PUSH(!a1);
})

DEF_CMD(SIN, 31, NO_ARGS,
{
    Elem a1 = 0;
    POP(a1);
    PUSH(sin(a1));
})

#undef NO_ARGS
#undef COMMON_ARGS
#undef JMP_ARGS
#undef meow