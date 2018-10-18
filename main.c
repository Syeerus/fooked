#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>

#include "bf.h"

#define VERSION "0.0.1"
#define DEFAULT_MEM_SIZE 30000

typedef enum {
    CMD_LINE_ARG_NONE = 0x00,
    CMD_LINE_ARG_INPUT = 0x01,
    CMD_LINE_ARG_INTERACTIVE_MODE = 0x02,
    CMD_LINE_ARG_MEM_SIZE = 0x04,
} cmd_line_flag_t;

typedef struct {
    unsigned int flags;
    size_t mem_size;
    char *filename;
    char *input;
} cmd_line_settings_t;

// Initializes a settings structure
void cmd_line_settings_init(cmd_line_settings_t *settings)
{
    settings->flags = 0;
    settings->mem_size = DEFAULT_MEM_SIZE;
    settings->filename = NULL;
    settings->input = NULL;
}

// Safe string matching function
bool str_match(const char *str1, const char *str2)
{
    if (str1 == NULL && str2 == NULL)
    {
        return true;
    }

    if (str1 == NULL || str2 == NULL)
    {
        return false;
    }

    size_t str1_size = strlen(str1);
    if (str1_size != strlen(str2))
    {
        return false;
    }

    return (strncmp(str1, str2, str1_size) == 0);
}

// Prints the help message to the screen
void print_help(const char *prog_name)
{
    printf("\nUsage:\n");
    printf("  %s [file_name] [-i <input> | --input <input>] [-s <size> | --mem-size <size>] [-I | --interactive]\n", prog_name);
    printf("  %s -v | --version\n", prog_name);
    printf("  %s -h | --help\n", prog_name);
    printf("\nOptions:\n");
    printf("  -i --input          Passes an input string.\n");
    printf("  -s --mem-size       Sets the memory size.\n");
    printf("  -I --interactive    Enables interactive mode.\n");
    printf("  -v --version        Prints the version and exits.\n");
    printf("  -h --help           Prints this help message.\n");
}

int main(int argc, char **argv)
{
    cmd_line_settings_t settings;
    cmd_line_settings_init(&settings);
    cmd_line_flag_t last_flag = CMD_LINE_ARG_NONE;
    unsigned long long mem_size;
    size_t i;
    for (i=1; i<argc; ++i)
    {
        char *arg = argv[i];
        if (last_flag != CMD_LINE_ARG_NONE)
        {
            switch (last_flag)
            {
            case CMD_LINE_ARG_NONE:     // Suppress warning
            case CMD_LINE_ARG_INTERACTIVE_MODE:
                break;
            case CMD_LINE_ARG_INPUT:
                settings.flags |= CMD_LINE_ARG_INPUT;
                settings.input = arg;
                break;
            case CMD_LINE_ARG_MEM_SIZE:
                mem_size = strtoull(arg, NULL, 10);
                if (errno == ERANGE)
                {
                    mem_size = SIZE_MAX;
                    errno = 0;
                }

                if (mem_size > SIZE_MAX)
                {
                    mem_size = SIZE_MAX;
                }
                else if (mem_size > 0)
                {
                    settings.mem_size = mem_size;
                }
                else
                {
                    // Error
                    fprintf(stderr, "Invalid memory size, using default.\n");
                }
            }

            last_flag = CMD_LINE_ARG_NONE;
        }
        else if (str_match(arg, "-v") || str_match(arg, "--version"))
        {
            printf("%s version %s\n", argv[0], VERSION);
            return 0;
        }
        else if (str_match(arg, "-i") || str_match(arg, "--input"))
        {
            last_flag = CMD_LINE_ARG_INPUT;
        }
        else if (str_match(arg, "-I") || str_match(arg, "--interactive"))
        {
            settings.flags |= CMD_LINE_ARG_INTERACTIVE_MODE;
        }
        else if (str_match(arg, "-s") || str_match(arg, "--mem-size"))
        {
            last_flag = CMD_LINE_ARG_MEM_SIZE;
        }
        else if (str_match(arg, "-h") || str_match(arg, "--help"))
        {
            printf("\nFooked Brainfuck Interpreter\n");
            print_help(argv[0]);
            return 0;
        }
        else
        {
            settings.filename = arg;
        }
    }

    if (settings.filename == NULL && !(settings.flags & CMD_LINE_ARG_INTERACTIVE_MODE))
    {
        // Error
        fprintf(stderr, "No filename or flag for interactive mode provided.\n");
        print_help(argv[0]);
        return 0;
    }

    bf_env_t env;
    bf_env_init(&env, settings.mem_size, settings.input);
    // TODO: Parse and run file
    // TODO: Interactive mode
    bf_env_destroy(&env);
    return 0;
}
