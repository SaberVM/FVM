#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
    char code[16] = {LAM, 10, CAP, 0, LAM, 4, VAR, 0, RET, RET, LIT, 7, APP, LIT, 9, APP};
    int code_len = 16;

    struct Value *value_stack = malloc(4096 * sizeof(struct Value));
    int value_stack_size = 0;
    struct Value *env_stack = malloc(4096 * sizeof(struct Value));
    int env_stack_size = 0;
    struct Value *captures = malloc(4096 * sizeof(struct Value));
    int captures_size = 0;

    int pc = 0;
    while (pc < code_len) {
        switch (code[pc]) {
            case LAM: {
                char skip = code[pc + 1];
                value_stack[value_stack_size].type = 0;
                value_stack[value_stack_size].closure = malloc(sizeof(struct Closure));
                value_stack[value_stack_size].closure->f = pc + 2;
                value_stack[value_stack_size].closure->env = malloc(sizeof(struct Value) * captures_size);
                value_stack[value_stack_size].closure->env_size = captures_size;
                memcpy(value_stack[value_stack_size].closure->env, captures, captures_size);
                value_stack_size++;
                captures_size = 0;
                pc += skip;
                break;
            }
            case CAP: {
                pc++;
                char index = code[pc++];
                captures[captures_size++] = env_stack[env_stack_size - index - 1];
                break;
            }
            case VAR: {
                pc++;
                char index = code[pc++];
                value_stack[value_stack_size] = env_stack[env_stack_size - index - 1];
                value_stack_size++;
                break;
            }
            case RET: {
                struct Value val = value_stack[--value_stack_size];
                struct Value continuation = value_stack[--value_stack_size];
                pc = continuation.closure->f;
                env_stack_size = continuation.closure->env_size;
                memcpy(env_stack, continuation.closure->env, env_stack_size);
                free(continuation.closure->env);
                free(continuation.closure);
                value_stack[value_stack_size++] = val;
                break;
            }
            case LIT: {
                pc++;
                char val = code[pc++];
                value_stack[value_stack_size].type = 1;
                value_stack[value_stack_size].integer = val;
                value_stack_size++;
                break;
            }
            case APP: {
                struct Closure *continuation = malloc(sizeof(struct Closure));
                continuation->f = pc + 1;
                // TODO: we should use the captures stack instead of cloning the whole env
                continuation->env = malloc(sizeof(struct Value) * env_stack_size);
                continuation->env_size = env_stack_size;
                memcpy(continuation->env, env_stack, env_stack_size);
                struct Value arg = value_stack[--value_stack_size];
                struct Value closure = value_stack[--value_stack_size];
                value_stack[value_stack_size++] = (struct Value){
                    .type = 0, 
                    .closure = continuation
                };
                pc = closure.closure->f;
                env_stack_size = closure.closure->env_size;
                memcpy(env_stack, closure.closure->env, env_stack_size);
                free(closure.closure->env);
                free(closure.closure);
                env_stack[env_stack_size++] = arg;
                break;
            }
        }
    }
    if (value_stack[value_stack_size - 1].type == 1) {
        printf("%d\n", value_stack[value_stack_size - 1].integer);
    }
}

void print_value_stack(struct Value *value_stack, int value_stack_size) {
    for (int i = value_stack_size - 1; i >= 0; i--) {
        printf("value_stack[%d]: ", i);
        switch (value_stack[i].type) {
            case 0:
                printf("closure(f=%d)\n", value_stack[i].closure->f);
                break;
            case 1:
                printf("%d\n", value_stack[i].integer);
        }
    }
}