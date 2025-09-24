#ifndef COMMONUI_H_
#define COMMONUI_H_

#include <stdint.h>
#include <stdbool.h>
//-----------------------------------------------------------------------------
// Subroutines
//-----------------------------------------------------------------------------

#define MAX_CHARS 80
#define MAX_FIELDS 5
typedef struct _USER_DATA
{
    char buffer[MAX_CHARS+1];
    uint8_t fieldCount;
    uint8_t fieldPosition[MAX_FIELDS];
    char fieldType[MAX_FIELDS];
} USER_DATA;

void parseFields(USER_DATA * data);
bool isCommand(USER_DATA* data, const char strCommand[],uint8_t minArguments);
int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber);
char* getFieldString(USER_DATA* data, uint8_t fieldNumber);
int strcmp1(char * str1, char * str2);

#endif
