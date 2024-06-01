#include "helpfunc.h"
#include <stdlib.h>

// This file contains itoa and atoi implementations, because I wasn't sure if
// the standard implementations are available in linux lab.

char* my_itoa (int value)
{
    char buffer[11]; // Biggest int is 10 digits + the NULL character
    int i;
    for(i = 0; i < 11; i++)
    {
        int digit;
        digit = value % 10;

        if(i == 0) // Always copy the first digit
            buffer[0] = 48 + digit; //48 is the  ascii code for 0
        else if (value != 0) // If the digit isn't zero
            buffer[i] = 48 + digit;
        else{ // End of the number
            buffer[i] = '\0';
            break;
        }
        value = value/10;
         
    }
    char* repr = malloc(sizeof(char)*i);
    // Reverting the string
    for(int j = 0; j < i; j++)
    {
        repr[j] = buffer[(i-1)-j];
    }
    repr[i] = buffer[i]; // transfering the null byte
    return repr;
}

int my_atoi (char* string)
{
    int value = 0;
    int i = 0;
    int pow = 1;
    // for every digit
    while(string[i] != '\0')
    {
        // multiply by its power and add it to the sum
        value += (string[i] - 48) * pow;
        // update power for the next digit
        pow = pow*10;
        i++;
    }
    return value;
}