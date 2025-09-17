#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include "commonui.h"


void parseFields(USER_DATA * data){
    int i = 0;
    while((data->buffer[i] != 0) && (i < 80))
    {
        if((data->buffer[i] >='a' && data->buffer[i] <= 'z') || (data->buffer[i] > 'A' && data->buffer[i] < 'Z'))
        {
            data->fieldPosition[data->fieldCount] = i;
            data->fieldType[data->fieldCount] = 'a';
            data->fieldCount++;
            while((data->buffer[i] >= 'a' && data->buffer[i] <= 'z') || (data->buffer[i] > 'A' && data->buffer[i] < 'Z') || ((data->buffer[i] >= '0') && (data->buffer[i] <= '9')))
            {
                i++;
            }
        }
        else if((data->buffer[i] >= '0') && (data->buffer[i] <= '9'))
        {
            data->fieldPosition[data->fieldCount] = i;
            data->fieldType[data->fieldCount] = 'n';
            data->fieldCount++;
            while((data->buffer[i] >= '0') && (data->buffer[i] <= '9')){
                i++;
            }
        }
        else
        {
            data->buffer[i] = 0;
            i++;
        }
    }
}


char* getFieldString(USER_DATA* data, uint8_t fieldNumber)
{
    if((fieldNumber <= data->fieldCount) && (data->fieldType[fieldNumber-1] == 'a'))
    {
        return &(data->buffer[data->fieldPosition[fieldNumber-1]]);
    }
    else
    {
        return NULL;
    }
}



int32_t getFieldInteger(USER_DATA* data, uint8_t fieldNumber)
{
    if((fieldNumber <= data->fieldCount) && (data->fieldType[fieldNumber-1] == 'n'))
    {
        return atoi(&(data->buffer[data->fieldPosition[fieldNumber-1]])); // Add atoi
    }
    else
    {
        return NULL;
    }
}


int strcmp1(char * str1, char * str2)
{
    int i = 0;
    while(true){
        if(str1[i] != str2[i])
        {
            return 1;
        }
        else if((str1[i] == '\0') && (str2[i] == '\0'))
        {
            return 0;
        }
        i++;
    }
}

int strcmp2(char * str1, const char * str2)
{
    int i = 0;
    while(true){
        if(str1[i] != str2[i])
        {
            return 1;
        }
        else if((str1[i] == '\0') && (str2[i] == '\0'))
        {
            return 0;
        }
        i++;
    }
}



bool isCommand(USER_DATA* data, const char strCommand[],uint8_t minArguments)
{
    if(!strcmp2(&(data->buffer[data->fieldPosition[0]]),strCommand) && (data->fieldCount == minArguments)){
        return true;
    }
    else{
        return false;
    }
}




