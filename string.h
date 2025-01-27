/*
 * string.h
 *
 *  Created on: Oct 19, 2024
 *      Author: bhatt
 */

#ifndef STRING_H_
#define STRING_H_


#include "uart0.h"
#include <inttypes.h>
#define MAX_CHARS 80
#define MAX_FIELDS 5

typedef struct USER_DATA
{
char buffer[MAX_CHARS+1];
uint8_t fieldCount;
uint8_t fieldPosition[MAX_FIELDS];
char fieldType[MAX_FIELDS];
} USER_DATA;

char* hexToString(uint32_t hex);
void getsUart0(USER_DATA *data);
void parseFields(USER_DATA *data);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
bool compare_string(char str1[], char str2[]);
void itoa(uint32_t num, char itoa_str[]);
bool isCommand(USER_DATA* data, const char strCommand[], uint8_t minArguments);
char* hexToString(uint32_t hex);
void manualStringCopy(char* dest, const char* src, uint8_t maxLength);
uint8_t StringLength(const char* str);


#endif /* STRING_H_ */
