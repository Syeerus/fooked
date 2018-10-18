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

#include <stdbool.h>

#include "bf.h"

void bf_cmd_init(bf_cmd_t *cmd, bf_cmd_type_t type, size_t value)
{
    cmd->type = type;
    cmd->value = value;
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

void bf_stack_init(bf_stack_t *stack)
{
    stack->last = NULL;
    stack->length = 0;
}

void bf_stack_destroy(bf_stack_t *stack)
{
    bf_stack_item_t *current_item = stack->last;
    while (current_item != NULL)
    {
        bf_stack_item_t *previous_item = current_item->previous;
        free(current_item);
        current_item = previous_item;
    }
}

void bf_stack_push(bf_stack_t *stack, bf_cmd_t *cmd)
{
    static const size_t SIZE_OF_STACK_ITEM_TYPE = sizeof(bf_stack_item_t);

    bf_stack_item_t *item = malloc(SIZE_OF_STACK_ITEM_TYPE);
    item->previous = stack->last;
    item->value = cmd;
    stack->last = item;
    ++(stack->length);
}

bf_cmd_t* bf_stack_pop(bf_stack_t *stack)
{
    if (stack->length > 0)
    {
        bf_stack_item_t *item = stack->last;
        stack->last = item->previous;
        --(stack->length);
        bf_cmd_t *cmd = item->value;
        free(item);

        return cmd;
    }

    return NULL;
}

void bf_error(bf_status_t *status, bf_status_type_t type, size_t line, size_t column)
{
    status->type = type;
    status->line = line;
    status->column = column;
}

void bf_run(bf_status_t *status, char *source)
{
    bf_cmd_t *root_cmd = bf_parse_str(status, source);
    if (status->type == BF_STATUS_OK)
    {
        bf_cmd_t *cmd = root_cmd;
        while (cmd != NULL)
        {
            cmd = cmd->next_cmd;
        }
    }
}

bf_cmd_t* bf_parse_str(bf_status_t *status, char *source)
{
    static const size_t SIZE_OF_CMD_TYPE = sizeof(bf_cmd_t);

    bf_cmd_t *root_cmd = NULL;
    bf_cmd_t *prev_cmd = NULL;

    bf_cmd_type_t current_type = BF_CMD_NONE;
    bf_cmd_type_t prev_type = BF_CMD_NONE;

    bf_stack_t jump_stack;
    bf_stack_init(&jump_stack);

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

            line_offset = pos;
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
                cmd = malloc(SIZE_OF_CMD_TYPE);
                bf_cmd_init(cmd, current_type, 0);
                if (current_type == BF_CMD_JUMP_FORWARD)
                {
                    bf_stack_push(&jump_stack, cmd);
                }
                else if (current_type == BF_CMD_JUMP_BACK)
                {
                    if (jump_stack.length > 0)
                    {
                        cmd->jump_cmd_target = bf_stack_pop(&jump_stack);
                    }
                    else
                    {
                        // Error
                        bf_cmd_destroy(root_cmd);
                        bf_stack_destroy(&jump_stack);
                        bf_error(status, BF_STATUS_UNEXPECTED_CLOSING_BRACKET, line, pos - line_offset);
                        return NULL;
                    }
                }
            }
            else
            {
                if (current_type != prev_type)
                {
                    cmd = malloc(SIZE_OF_CMD_TYPE);
                    bf_cmd_init(cmd, current_type, 1);
                }
                else
                {
                    ++(prev_cmd->value);
                }
            }

            if (cmd != NULL)
            {
                if (prev_type == BF_CMD_JUMP_BACK && prev_cmd->jump_cmd_target != NULL)
                {
                    bf_cmd_t *jump_target = prev_cmd->jump_cmd_target;
                    jump_target->jump_cmd_target = cmd;
                }

                if (prev_cmd != NULL)
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
        bf_stack_destroy(&jump_stack);
        bf_error(status, BF_STATUS_UNCLOSED_BRACKET, line, pos - line_offset);
        return NULL;
    }

    status->type = BF_STATUS_OK;
    return root_cmd;
}
