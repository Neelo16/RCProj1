#include <stdio.h>
#include <stdlib.h>

char* getcoisa(char buffer[])
{
    int i = 5;
    char* language = malloc(sizeof(char)*20);

    while( buffer[i] != "\0")
    {
        language[i-5] = buffer[i];
        i++;
    }
    printf("language -> %s", language);
    return language;
}


int main(int argc, const char *argv[])
{
    char buffer[20] = "ULQ portugues";
    char* language;

    language = getcoisa(buffer);
    return 0;
}
