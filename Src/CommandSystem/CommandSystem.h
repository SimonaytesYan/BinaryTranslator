#ifndef __COMAND_SYSTEM_SYM__
#define __COMAND_SYSTEM_SYM__

#include <stdio.h>

const int REG_N = 4;

enum REGISTERS
{
    RAX = 1,
    RBX = 2,
    RCX = 3,
    RDX = 4, 
};

enum CMD_MASKS
{
    CMD_MASK  = 0xFF,
    ARG_NUM = 0x100,  //< command has number argument      flag  
    ARG_REG   = 0x200,  //< command has register argument    flag 
    ARG_MEM   = 0x400   //< command has memory area argument flag
};

const int SIGNATURE   = 'S' * 256 + 'Y';
const int ASM_VERSION = 3;

const int RAM_SIZE     = 230400;
const int FI_BYTE      = 0xFF;
const int WINDOW_HIGHT = 360;
const int WINDOW_WIDTH = 636;

const int MAX_LABELS    = 64;
const int MAX_LABEL_LEN = 20;

struct Label 
{
    char name[MAX_LABEL_LEN] = "";
    int  cmd_to              = -1;
};

struct MyHeader
{
    int    signature      = -1;
    int    version        = -1;
    size_t commands_number = (size_t)-1;
};

int  InitMyHeader(MyHeader* header, int commands_number);
void DumpLabels(Label* labels);
int  CheckMyHeaderFromFile(MyHeader *header, FILE* executable_file);

#endif //__COMAND_SYSTEM_SYM__