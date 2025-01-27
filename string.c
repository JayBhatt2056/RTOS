/*
 * string.c
 *
 *  Created on: Oct 19, 2024
 *      Author: bhatt
 */
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include "clock.h"
#include "uart0.h"
#include "kernel.h"
#include "string.h"
#include "tm4c123gh6pm.h"



uint8_t StringLength(const char* str)
{
    uint8_t length = 0;
    while (str[length] != '\0') // Traverse until the null terminator
    {
        length++;
    }
    return length;
}
void manualStringCopy(char* dest, const char* src, uint8_t maxLength)
{
    uint8_t i = 0;
    while (i < maxLength - 1 && src[i] != '\0') // Ensure null termination within bounds
    {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0'; // Null-terminate the destination string
}

void getsUart0(USER_DATA *data)
{
    uint8_t count = 0;
    char c;

    while (true)
    {
        if(kbhitUart0())
        {
            c = getcUart0();

            if (c == 8 || c == 127) //8 or 127 for backspace
            {
                if (count > 0)
                {
                    count--;
                }
            }
            else if (c == 13) //13 for enter
            {
                data->buffer[count] = '\0';
                return;
            }
            else if (c >= 32) //32 for character
            {
                if (count < MAX_CHARS)
                {
                    data->buffer[count] = c;
                    count++;
                }
                else
                {
                    data->buffer[count] = '\0';
                    return;
                }
            }
        }
        else
        {
            yield();
        }
    }
}

void parseFields(USER_DATA *data)
{
    char PType = 'd'; // Previous type initialized to delimiter
    data->fieldCount = 0; // Reset field count
    uint8_t i = 0;

    while (true)
    {
        char currentChar = data->buffer[i]; // Get current character

        // Break at end of string
        if (currentChar == '\0')
            break;

        char CType = 'd'; // Current character type, default to delimiter

        // Check if character is alphabetic
        if ((currentChar >= 'A' && currentChar <= 'Z') || (currentChar >= 'a' && currentChar <= 'z'))
        {
            CType = 'a'; // Alphabetic
        }
        // Check if character is numeric
        else if (currentChar >= '0' && currentChar <= '9')
        {
            CType = 'n'; // Numeric
        }
        // Non-alphanumeric characters are treated as delimiters
        else
        {
            data->buffer[i] = '\0'; // Replace with null terminator
        }

        // If type changes, record a new field
        if (CType != PType)
        {
            if (data->fieldCount < MAX_FIELDS && CType != 'd') // Ensure within limit
            {
                // Record the field's type and position
                data->fieldType[data->fieldCount] = CType;
                data->fieldPosition[data->fieldCount] = i;
                data->fieldCount++;
            }
            PType = CType; // Update previous type
        }

        i++; // Move to next character
    }
}

char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{

    if (fieldNumber <= data->fieldCount)
    {
        return &data->buffer[data->fieldPosition[fieldNumber]];
    }
    else
    {
        return NULL;
    }
}
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if (fieldNumber < data->fieldCount && data->fieldType[fieldNumber] == 'n')
    {

        int32_t fieldValue = atoi(&data->buffer[data->fieldPosition[fieldNumber]]);

        return fieldValue;
    }
    else
    {
        return 0;
    }
}

bool compare_string(char str1[], char str2[])
{
    uint8_t i = 0;

    // Compare each character
    while (str1[i] != '\0' && str2[i] != '\0')
    {
        if (str1[i] != str2[i])
            return false; // Mismatch found
        i++;
    }

    // Check if both strings terminated at the same point
    return (str1[i] == '\0' && str2[i] == '\0');
}

void itoa(uint32_t num, char itoa_str[])
{
    uint8_t i = 0;
    uint8_t j = 0;


    do {
        itoa_str[i++] = num % 10 + '0';
        num /= 10;
    } while (num > 0);

    // Reverse the string
    for (j = 0; j < i / 2; j++) {
        char temp = itoa_str[j];
        itoa_str[j] = itoa_str[i - j - 1];
        itoa_str[i - j - 1] = temp;
    }
    itoa_str[i] = '\0';
}


bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments)
{
    if (data->fieldCount >= minArguments + 1)
    {
        if(compare_string(strCommand, &data->buffer[data->fieldPosition[0]]))
        {
            return true;
        }
    }

    return false;
}

char* hexToString(uint32_t hex) {
    static char hexStr[9];  // Static buffer, 8 characters for hex + 1 for null terminator
    char hexDigits[] = "0123456789ABCDEF";  // Hexadecimal digits

    // Fill the string with hexadecimal digits, starting from the most significant nibble
    uint16_t i = 7;
    while (1)
    {
       hexStr[i] = hexDigits[hex & 0xF];  // Get the last 4 bits (1 hex digit)
       hex >>= 4;  // Shift the hex value right by 4 bits to get the next nibble

       if (i == 0)
       {  // If we're at the first character, break the loop
           break;
       }
       i--;

    }
    hexStr[8] = '\0';

   return hexStr;  // Return the pointer to the static buffer
}

