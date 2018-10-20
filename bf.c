/**
 * Copyright (c) 2018 Syeerus
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdio.h>
#include <stdbool.h>
#include <string.h>

#include "bf.h"

void* bf_malloc(size_t size)
{
    void *ptr = malloc(size);
    if (ptr == NULL && size != 0)
    {
        fprintf(stderr, "Memory allocation error.");
        abort();
    }

    return ptr;
}

void bf_cmd_init(bf_cmd_t *cmd, bf_cmd_type_t type, size_t value, size_t line, size_t column)
{
    cmd->type = type;
    cmd->value = value;
    cmd->line = line;
    cmd->column = column;
    cmd->next_cmd = NULL;
    cmd->jump_cmd_target = NULL;
}

void bf_cmd_destroy(bf_cmd_t *root_cmd)
{
    bf_cmd_t *current_cmd = root_cmd;
    while (current_cmd != NULL)
    {
        bf_cmd_t *next_cmd = current_cmd->next_cmd;
        free(current_cmd);
        current_cmd = next_cmd;
    }
}

void bf_cmd_stack_init(bf_cmd_stack_t *stack)
{
    stack->last = NULL;
    stack->length = 0;
}

void bf_cmd_stack_destroy(bf_cmd_stack_t *stack)
{
    bf_cmd_stack_item_t *current_item = stack->last;
    while (current_item != NULL)
    {
        bf_cmd_stack_item_t *previous_item = current_item->previous;
        free(current_item);
        current_item = previous_item;
    }
}

void bf_cmd_stack_push(bf_cmd_stack_t *stack, bf_cmd_t *cmd)
{
    static const size_t SIZE_OF_CMD_STACK_ITEM_TYPE = sizeof(bf_cmd_stack_item_t);

    bf_cmd_stack_item_t *item = bf_malloc(SIZE_OF_CMD_STACK_ITEM_TYPE);
    item->previous = stack->last;
    item->value = cmd;
    stack->last = item;
    ++(stack->length);
}

bf_cmd_t* bf_cmd_stack_pop(bf_cmd_stack_t *stack)
{
    if (stack->length > 0)
    {
        bf_cmd_stack_item_t *item = stack->last;
        stack->last = item->previous;
        --(stack->length);
        bf_cmd_t *cmd = item->value;
        free(item);

        return cmd;
    }

    return NULL;
}

void bf_env_init(bf_env_t *env, size_t num_of_data_cells, char *input)
{
    env->data_cells = bf_malloc(sizeof(char) * num_of_data_cells);
    env->num_of_data_cells = num_of_data_cells;
    env->data_ptr_idx = 0;
    env->input = input;
    env->input_idx = 0;

    size_t i;
    for (i=0; i<num_of_data_cells; ++i)
    {
        env->data_cells[i] = 0;
    }
}

void bf_env_destroy(bf_env_t *env)
{
    free(env->data_cells);
    env->data_cells = NULL;
    env->num_of_data_cells = 0;
    env->data_ptr_idx = 0;
    env->input = NULL;
}

void bf_error(bf_status_t *status, bf_status_type_t type, size_t line, size_t column)
{
    status->type = type;
    status->line = line;
    status->column = column;
}

void bf_run(bf_status_t *status, char *source, bf_env_t *env)
{
    bf_cmd_t *root_cmd = bf_parse_str(status, source);
    if (status->type == BF_STATUS_OK)
    {
        bf_cmd_t *cmd = root_cmd;
        while (cmd != NULL)
        {
            switch (cmd->type)
            {
            case BF_CMD_NONE:
                break;
            case BF_CMD_INC_DATA_PTR:
                if ((env->data_ptr_idx + cmd->value) >= env->num_of_data_cells)
                {
                    bf_error(status, BF_STATUS_DATA_PTR_OUT_OF_BOUNDS, cmd->line, cmd->column);
                    return;
                }

                env->data_ptr_idx += cmd->value;
                break;
            case BF_CMD_DEC_DATA_PTR:
                if (cmd->value > env->data_ptr_idx)
                {
                    bf_error(status, BF_STATUS_DATA_PTR_OUT_OF_BOUNDS, cmd->line, cmd->column);
                    return;
                }

                env->data_ptr_idx -= cmd->value;
                break;
            case BF_CMD_INC_VALUE:
                env->data_cells[env->data_ptr_idx] += cmd->value;
                break;
            case BF_CMD_DEC_VALUE:
                env->data_cells[env->data_ptr_idx] -= cmd->value;
                break;
            case BF_CMD_OUTPUT:
                printf("%c", env->data_cells[env->data_ptr_idx]);
                break;
            case BF_CMD_INPUT:
                if (env->input)
                {
                    env->data_cells[env->data_ptr_idx] = env->input[env->input_idx];
                    if (env->input[env->input_idx] != '\0' && env->input[env->input_idx + 1] != '\0')
                    {
                        ++(env->input_idx);
                    }
                }
                break;
            case BF_CMD_JUMP_FORWARD:
                if (env->data_cells[env->data_ptr_idx] == 0)
                {
                    cmd = cmd->jump_cmd_target;
                    continue;
                }
                break;
            case BF_CMD_JUMP_BACK:
                if (env->data_cells[env->data_ptr_idx] != 0)
                {
                    cmd = cmd->jump_cmd_target;
                    continue;
                }
            }

            cmd = cmd->next_cmd;
        }

        bf_cmd_destroy(root_cmd);
    }
}

bf_cmd_t* bf_parse_str(bf_status_t *status, char *source)
{
    static const size_t SIZE_OF_CMD_TYPE = sizeof(bf_cmd_t);

    bf_cmd_t *root_cmd = NULL;
    bf_cmd_t *prev_cmd = NULL;

    bf_cmd_type_t current_type = BF_CMD_NONE;
    bf_cmd_type_t prev_type = BF_CMD_NONE;

    bf_cmd_stack_t jump_stack;
    bf_cmd_stack_init(&jump_stack);

    size_t pos = 0;
    size_t line_offset = 0;
    size_t line = 1;
    while (source[pos] != '\0')
    {
        char c = source[pos];
        bool is_optimized_cmd = false;
        switch (c)
        {
        case '>':
            current_type = BF_CMD_INC_DATA_PTR;
            is_optimized_cmd = true;
            break;
        case '<':
            current_type = BF_CMD_DEC_DATA_PTR;
            is_optimized_cmd = true;
            break;
        case '+':
            current_type = BF_CMD_INC_VALUE;
            is_optimized_cmd = true;
            break;
        case '-':
            current_type = BF_CMD_DEC_VALUE;
            is_optimized_cmd = true;
            break;
        case '.':
            current_type = BF_CMD_OUTPUT;
            break;
        case ',':
            current_type = BF_CMD_INPUT;
            break;
        case '[':
            current_type = BF_CMD_JUMP_FORWARD;
            break;
        case ']':
            current_type = BF_CMD_JUMP_BACK;
            break;
        case '\n':
            if (current_type != BF_CMD_NONE)
            {
                current_type = BF_CMD_NONE;
            }

            line_offset = pos + 1;
            ++line;
        default:
            if (current_type != BF_CMD_NONE)
            {
                current_type = BF_CMD_NONE;
            }
        }

        if (current_type != BF_CMD_NONE)
        {
            bf_cmd_t *cmd = NULL;
            if (!is_optimized_cmd)
            {
                cmd = bf_malloc(SIZE_OF_CMD_TYPE);
                bf_cmd_init(cmd, current_type, 0, line, (pos - line_offset) + 1);
                if (current_type == BF_CMD_JUMP_FORWARD)
                {
                    bf_cmd_stack_push(&jump_stack, cmd);
                }
                else if (current_type == BF_CMD_JUMP_BACK)
                {
                    if (jump_stack.length > 0)
                    {
                        cmd->jump_cmd_target = bf_cmd_stack_pop(&jump_stack);
                    }
                    else
                    {
                        // Error
                        bf_cmd_destroy(root_cmd);
                        bf_cmd_stack_destroy(&jump_stack);
                        bf_error(status, BF_STATUS_UNEXPECTED_CLOSING_BRACKET, line, (pos - line_offset) + 1);
                        return NULL;
                    }
                }
            }
            else
            {
                if (current_type != prev_type)
                {
                    cmd = bf_malloc(SIZE_OF_CMD_TYPE);
                    bf_cmd_init(cmd, current_type, 1, line, (pos - line_offset) + 1);
                }
                else
                {
                    ++(prev_cmd->value);
                }
            }

            if (cmd != NULL)
            {
                if (prev_type == BF_CMD_JUMP_BACK && prev_cmd->jump_cmd_target)
                {
                    prev_cmd->jump_cmd_target->jump_cmd_target = cmd;
                    prev_cmd->jump_cmd_target = prev_cmd->jump_cmd_target->next_cmd;
                }

                if (prev_cmd)
                {
                    prev_cmd->next_cmd = cmd;
                }

                if (root_cmd == NULL)
                {
                    root_cmd = cmd;
                }

                prev_cmd = cmd;
            }

            prev_type = current_type;
        }

        ++pos;
    }

    if (jump_stack.length > 0)
    {
        // Error
        bf_cmd_destroy(root_cmd);
        bf_cmd_stack_destroy(&jump_stack);
        bf_error(status, BF_STATUS_UNCLOSED_BRACKET, line, (pos - line_offset) + 1);
        return NULL;
    }

    status->type = BF_STATUS_OK;
    return root_cmd;
}
