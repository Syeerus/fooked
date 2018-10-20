#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#if defined(linux) || defined(__unix__)
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "bf.h"

#define VERSION "1.0.1"
#define DEFAULT_MEM_SIZE 30000
#define MAX_INTERACTIVE_BUFFER_SIZE 2047

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

// Runs a string of code and prints error messages if necessary
void run_code(bf_env_t *env, char *source)
{
    bf_status_t status;
    bf_run(&status, source, env);
    switch (status.type)
    {
    case BF_STATUS_OK:
        break;
    case BF_STATUS_DATA_PTR_OUT_OF_BOUNDS:
        fprintf(stderr, "\nData pointer is out of bounds: line %lu, col %lu\n", (unsigned long)status.line, (unsigned long)status.column);
        break;
    case BF_STATUS_UNCLOSED_BRACKET:
        fprintf(stderr, "\nUnclosed bracked: line %lu, col %lu\n", (unsigned long)status.line, (unsigned long)status.column);
        break;
    case BF_STATUS_UNEXPECTED_CLOSING_BRACKET:
        fprintf(stderr, "\nUnexpected closing bracket: line %lu, col %lu\n", (unsigned long)status.line, (unsigned long)status.column);
    }
}

// Handles command line arguments, returns whether to exit or not
bool handle_cmd_line_args(int argc, char **argv, cmd_line_settings_t *settings)
{
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
                settings->flags |= CMD_LINE_ARG_INPUT;
                settings->input = arg;
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
                    settings->mem_size = mem_size;
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
            return true;
        }
        else if (str_match(arg, "-i") || str_match(arg, "--input"))
        {
            last_flag = CMD_LINE_ARG_INPUT;
        }
        else if (str_match(arg, "-I") || str_match(arg, "--interactive"))
        {
            settings->flags |= CMD_LINE_ARG_INTERACTIVE_MODE;
        }
        else if (str_match(arg, "-s") || str_match(arg, "--mem-size"))
        {
            last_flag = CMD_LINE_ARG_MEM_SIZE;
        }
        else if (str_match(arg, "-h") || str_match(arg, "--help"))
        {
            printf("\nFooked Brainfuck Interpreter\n");
            print_help(argv[0]);
            return true;
        }
        else if (settings->filename == NULL)
        {
            settings->filename = arg;
        }
    }

    return false;
}

// Input prompt for interactive mode
void get_interactive_input(bf_env_t *env, char *buffer, int max_length)
{
    printf("\np%lu v%d> ", (unsigned long)env->data_ptr_idx, env->data_cells[env->data_ptr_idx]);
    fgets(buffer, max_length, stdin);

    // Remove newline from buffer
    int pos = 0;
    int last_pos = max_length - 1;
    while (buffer[pos] != '\n' && pos < last_pos)
    {
        ++pos;
    }

    buffer[pos] = '\0';
}

int main(int argc, char **argv)
{
    cmd_line_settings_t settings;
    cmd_line_settings_init(&settings);
    if (handle_cmd_line_args(argc, argv, &settings))
    {
        return 0;
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

    if (settings.filename)
    {
        long file_size = 0;
        #if defined(linux) || defined(__unix__)
        struct stat buf;
        if (stat(settings.filename, &buf) == 0)
        {
            file_size = buf.st_size;
        }
        else
        {
            fprintf(stderr, "There was an error checking the size of the file '%s'.\n", settings.filename);
        }
        #elif defined(WIN32) || defined(_WIN32)
        struct _stat buf;
        if (_stat(settings.filename, &buf) == 0)
        {
            file_size = buf.st_size;
        }
        else
        {
            switch (errno)
            {
            case ENOENT:
                fprintf(stderr, "File '%s' not found.\n", settings.filename);
                break;
            case EINVAL:
                fprintf(stderr, "Invalid parameter to _stat.\n");
                break;
            default:
                fprintf(stderr, "Unexpected error for file '%s'.\n", settings.filename);
            }
        }
        #endif

        if (file_size > 0)
        {
            FILE *fp = fopen(settings.filename, "r");
            if (fp)
            {
                char *file_data = bf_malloc(file_size + 1);
                size_t pos = 0;
                int c = fgetc(fp);
                while (c != EOF && pos < file_size)
                {
                    file_data[pos] = (unsigned char)c;
                    ++pos;
                    c = fgetc(fp);
                }

                file_data[file_size] = '\0';

                if (fclose(fp) != 0)
                {
                    fprintf(stderr, "Failed to close file '%s'.\n", settings.filename);
                }

                run_code(&env, file_data);
                free(file_data);
            }
            else
            {
                fprintf(stderr, "There was an error opening the file '%s'.\n", settings.filename);
            }
        }
    }

    if (settings.flags & CMD_LINE_ARG_INTERACTIVE_MODE)
    {
        printf("\n\nInteractive Mode (type \"exit\" to quit)");
        char *buffer = bf_malloc(sizeof(char) * MAX_INTERACTIVE_BUFFER_SIZE);
        get_interactive_input(&env, buffer, MAX_INTERACTIVE_BUFFER_SIZE);
        while (!str_match(buffer, "exit"))
        {
            run_code(&env, buffer);
            get_interactive_input(&env, buffer, MAX_INTERACTIVE_BUFFER_SIZE);
        }

        free(buffer);
    }

    bf_env_destroy(&env);
    return 0;
}
