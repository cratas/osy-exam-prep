#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <stdarg.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <mqueue.h>


int main(int argc, char** argv )
{

    FILE* fv = fopen(argv[1], "r");

    char buffer[255];

    fseek(fv, 0, SEEK_END);
    int file_size = ftell(fv);
    rewind(fv);

    int err = fread(buffer, 1, file_size, fv);

    char* token = strtok(buffer, " ");
    
    int counter = 0;
    fv = fopen(argv[2], "w");

    while(token != NULL)
    {
        fprintf(fv, "%d. %s\n", ++counter, token);
        token = strtok(NULL, " ");
    }
    fclose(fv);

    for(int i=0; i<10;i++) 
    {}

    return 0;
}