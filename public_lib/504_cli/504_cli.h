#ifndef __504_CLI_H__ 
#define __504_CLI_H__   

#include <stdarg.h>
#include <string.h>

#define CLI_504_USED_TERMINAL 0

#ifndef MAX_CLI_OUTPUT_BUFFER_SIZE
#define MAX_CLI_OUTPUT_BUFFER_SIZE 128
#endif // !MAX_CLI_OUTPUT_BUFFER_SIZE

#ifndef MAX_CLI_INPUT_SIZE
#define MAX_CLI_INPUT_SIZE 32
#endif // !MAX_CLI_INPUT_SIZE



typedef struct
{
    char* cmd;
    void (*handler)(int argc, char** argv);
    char help[MAX_CLI_OUTPUT_BUFFER_SIZE];
}cli_504_t;


void cli_504_init(cli_504_t* cli,size_t cmd_count);
void cli_504_parse(char* line);

#endif // !__504_CLI_H__