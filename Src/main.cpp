#include <assert.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>

#include "Translation/Translation.h"
#include "Stdlib/Stdlib.h"
#include "Filework.h"

const size_t kMaxFilepathSize = 256;

void Build(long long cell_type, long long x, long long y)
{
    printf("Build(cell_type = %d, x = %d, y = %d)\n", cell_type, x, y);
}

long long Get(long long x, long long y)
{
    printf("Get(x = %d, y = %d) = %d\n", x, y, 10);
    return 10;
}

void LoadRes (long long food, long long water, 
              long long wood, long long population, 
              long long free_population, long long stone)
{
	printf("food 			= %lld\n", food);
	printf("water			= %lld\n", water);
	printf("wood 			= %lld\n", wood);
	printf("population 		= %lld\n", population);
	printf("free_population = %lld\n", free_population);
	printf("stone 			= %lld\n", stone);
    printf("LoadRes()\n");
}

int main(int argc, char* argv[])
{
    char in_bin_filepath[kMaxFilepathSize + 1] = {};
    CHECK(ParseCmdArgs(argc, argv, in_bin_filepath) != 0, return -1;)    

    FILE* in_bin_fp = fopen(in_bin_filepath, "rb");
    CHECK(in_bin_fp == nullptr, return -1;)

    MyHeader in_bin_header = {};
    CHECK(CheckMyHeaderFromFile(&in_bin_header, in_bin_fp) != 0, return -1;);

    size_t in_file_size = GetFileSize(in_bin_filepath);

    TranslateAndRun(in_bin_filepath, in_file_size, in_bin_header, Build, Get, LoadRes);

    printf("Successfully end JIT compilation and execution\n");
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
