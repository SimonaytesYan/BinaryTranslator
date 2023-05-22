#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "Translation/Translation.h"

const size_t kMaxFilepathSize = 256;

#define CHECK(condition, true_branch)   \
    if (condition)                      \
    {                                   \
        true_branch;                    \
    }

int  GetFileSize(const char *file_name);
int  ParseCmdArgs(int argc, char* argv[], char* in_bin_filepath);

int main(int argc, char* argv[])
{
    char in_bin_filepath[kMaxFilepathSize + 1] = {};
    CHECK(ParseCmdArgs(argc, argv, in_bin_filepath) != 0, return -1;)    

    FILE* in_bin_fp = fopen(in_bin_filepath, "rb");
    CHECK(in_bin_fp == nullptr, return -1;)

    MyHeader in_bin_header = {};
    CHECK(CheckMyHeaderFromFile(&in_bin_header, in_bin_fp) != 0, return -1;);

    size_t in_file_size = GetFileSize(in_bin_filepath);

    TranslateAndRun(in_bin_filepath, in_file_size, in_bin_header);
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