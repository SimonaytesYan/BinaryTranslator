#pragma once

#include <sys/stat.h>
#include <assert.h>

#define CHECK(condition, true_branch)   \
    if (condition)                      \
    {                                   \
        true_branch;                    \
    }

int  GetFileSize(const char *file_name);
int  ParseCmdArgs(int argc, char* argv[], char* in_bin_filepath);


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