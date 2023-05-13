#include "CommandSystem.h"

//==========================================DEFINES===========================================

#define CHECK_DO(condition, true_branch) \
    if (condition)                       \
    {                                    \
        true_branch;                     \
    }

int InitMyHeader(MyHeader* header, int comands_number)
{
    CHECK_DO(header == nullptr, {fprintf(stderr, "Null header"); return -1;});

    header->signature        = SIGNATURE;
    header->version          = ASM_VERSION;
    header->comands_number   = comands_number;

    return 0;
}

int CheckMyHeaderFromFile(MyHeader *header, FILE* executable_file)
{
    CHECK_DO(executable_file == nullptr, fprintf(stderr, "Executable file = nullptr\n"); return -1); 

    fread(header, sizeof(*header), 1, executable_file);

    CHECK_DO(header->signature != SIGNATURE,   fprintf(stderr, "File isn`t executable\n");     return -1); 
    CHECK_DO(header->version   != ASM_VERSION, fprintf(stderr, "Wrong version of compiler\n"); return -1);
    return 0;
}