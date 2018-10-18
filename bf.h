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

#ifndef __BF_H__
#define __BF_H__

#include <stdlib.h>

typedef enum {
    BF_STATUS_OK,
    BF_STATUS_DATA_PTR_OUT_OF_BOUNDS,
    BF_STATUS_UNCLOSED_BRACKET,
    BF_STATUS_UNEXPECTED_CLOSING_BRACKET
} bf_status_type_t;

typedef enum {
    BF_CMD_NONE,
    BF_CMD_INC_DATA_PTR,    // >
    BF_CMD_DEC_DATA_PTR,    // <
    BF_CMD_INC_VALUE,       // +
    BF_CMD_DEC_VALUE,       // -
    BF_CMD_OUTPUT,          // .
    BF_CMD_INPUT,           // ,
    BF_CMD_JUMP_FORWARD,    // [
    BF_CMD_JUMP_BACK        // ]
} bf_cmd_type_t;

typedef struct {
    bf_status_type_t type;
    size_t line;        // For errors
    size_t column;      // For errors
} bf_status_t;

// This data structure is used similarly to a linked list
typedef struct bf_cmd {
    bf_cmd_type_t type;
    size_t value;                   // Used for increment and decrement optimization
    struct bf_cmd *next_cmd;
    struct bf_cmd *jump_cmd_target;      // Only used for the jump commands
} bf_cmd_t;

typedef struct bf_cmd_stack_item {
    bf_cmd_t *value;
    struct bf_cmd_stack_item *previous;
} bf_cmd_stack_item_t;

typedef struct {
    bf_cmd_stack_item_t *last;
    size_t length;
} bf_cmd_stack_t;

// Initializes a Brainfuck command stack
void bf_cmd_stack_init(bf_cmd_stack_t *stack);

// Frees and clears all of the items in a Brainfuck command stack
void bf_cmd_stack_destroy(bf_cmd_stack_t *stack);

// Pushes an item onto the stack
void bf_cmd_stack_push(bf_cmd_stack_t *stack, bf_cmd_t *cmd);

// Pops and returns an item from the stack
bf_cmd_t* bf_cmd_stack_pop(bf_cmd_stack_t *stack);

// Initializes a Brainfuck command struct
void bf_cmd_init(bf_cmd_t *cmd, bf_cmd_type_t type, size_t value);

// Frees the entire list of commands
// NOTE: Should only ever be used on the root command node
void bf_cmd_destroy(bf_cmd_t *root_cmd);

void bf_error(bf_status_t *status, bf_status_type_t type, size_t line, size_t column);

bf_cmd_t* bf_parse_str(bf_status_t *status, char *source);

void bf_run(bf_status_t *status, char *source);

#endif // __BF_H__
