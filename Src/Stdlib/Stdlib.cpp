#include <stdio.h>

#include "Stdlib.h"

const size_t kBufferSize = 256;

void OutputNumber10(long long number)
{
    char Buffer[kBufferSize + 1] = {};

    int  index  = kBufferSize - 1;
    bool is_neg = false;

    if (number < 0)
    {
        is_neg = true;
        number *= -1;
    }

    do 
    {
        Buffer[index] = (number % 10) + '0';
        number /= 10;
        index--;
    }
    while(number > 0);

    if (is_neg)
        Buffer[index--] = '-';
    
    puts(&Buffer[index + 1]);
}

long long InputNumber10()
{
    long long number = 0;
    long long digit  = 0;
    bool      is_neg = false;

    while((digit = getc(stdin)) != '\n' && digit != EOF && digit != ' ')
    {
        if(digit == '-')
        {
            is_neg = true;
            continue;
        }

        number *= 10;
        number += digit - '0';
    }

    if (is_neg)
        number *= -1;
    return number;
}

long long SqrtInt(long long x)
{
    long long y = 0;
    while (y*y <= x)
        y++;
    y--;
    return y;
}