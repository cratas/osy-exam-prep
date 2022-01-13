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
    char* input_file_name = argv[1];
    char* output_file_name = argv[2];

    char* input_text;
    FILE* fd;

    fd = fopen(input_file_name, "rb");
    if(!fd)
    {
        printf("Invalid input file. \n");
        return 1;
    }

    fseek(fd, 0, SEEK_END);
    int size_of_file = ftell(fd);
    rewind(fd);

    input_text = (char*)malloc(size_of_file + 1);

    fread(input_text, size_of_file, 1, fd);
    fclose(fd);

    char* words[64];
    int counter = 0;

    char* token = strtok(input_text, " ");


    while(token != NULL)
    {
        words[counter++] = token;
        token = strtok(NULL, " ");
    }
    
    fd = fopen(output_file_name, "w");
    if(!fd)
    {
        printf("Invalid output file. \n");
        return 1;
    }

    for(int i = 0; i < counter; i++)
    {
        fprintf(fd, "%d. %s\n", i+1, words[i]);
    }

    fclose(fd);
    free(token);

    return 0;
}