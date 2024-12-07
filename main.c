#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DBG 0

#if DBG
#define dbg(s, ...) printf(s, ##__VA_ARGS__)
#else
#define dbg(s, ...)
#endif

int main(int argc, char *argv[]) {
    if (argc < 2) { // no file given
        fprintf(stderr, "Usage: fvm my_binary.fvm\n");
        exit(EXIT_FAILURE);
    }
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        fprintf(stderr, "Error: could not open file %s\n", argv[1]);
        exit(EXIT_FAILURE);
    }
    char code[4096];
    int code_len = fread(code, sizeof(char), sizeof(code), file);
    fclose(file);

    struct Value *value_stack = malloc(4096 * sizeof(struct Value));
    int value_stack_size = 0;
    struct Value *env_stack = malloc(4096 * sizeof(struct Value));
    int env_stack_size = 0;
    struct Value *captures = malloc(4096 * sizeof(struct Value));
    int captures_size = 0;

    int pc = 0;
    while (pc < code_len) {
        for (int i = pc; i < code_len; i++) {
            switch(code[i]) {
                case LAM: dbg("LAM %d ", code[++i]); break;
                case APP: dbg("APP "); break;
                case LIT: dbg("LIT %d ", code[++i]); break;
                case CAP: dbg("CAP %d ", code[++i]); break;
                case RET: dbg("RET "); break;
                case VAR: dbg("VAR %d ", code[++i]); break;
                case OWN: dbg("OWN %d ", code[++i]); break;
                default: dbg("%d ", code[i]); break;
            }
        }
        dbg("\n");
        dbg("pc=%d op=%d\n", pc, code[pc]);
        print_value_stack(value_stack, value_stack_size);
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
                pc += skip + 2;
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
                struct Value old = env_stack[env_stack_size - index - 1];
                value_stack[value_stack_size] = old;
                if (value_stack[value_stack_size].type == 0) {
                    value_stack[value_stack_size].closure = malloc(sizeof(struct Closure));
                    value_stack[value_stack_size].closure->f = old.closure->f;
                    value_stack[value_stack_size].closure->env = malloc(sizeof(struct Value) * env_stack_size);
                    value_stack[value_stack_size].closure->env_size = env_stack_size;
                    memcpy(value_stack[value_stack_size].closure->env, old.closure->env, env_stack_size);
                }
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
                captures_size = 0;
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
                continuation->env = malloc(sizeof(struct Value) * captures_size);
                continuation->env_size = captures_size;
                memcpy(continuation->env, captures, captures_size);
                captures_size = 0;
                struct Value arg = value_stack[--value_stack_size];
                struct Value closure = value_stack[--value_stack_size];
                dbg("%d\n", continuation->f);
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
            case OWN: {
                // a version of VAR that doesn't clone the referent
                // Use with caution!
                // Conservative static analysis can make sure it's used safely.
                pc++;
                char index = code[pc++];
                value_stack[value_stack_size++] = env_stack[env_stack_size - index - 1];
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
        dbg("value_stack[%d]: ", i);
        switch (value_stack[i].type) {
            case 0:
                dbg("closure(f=%d)\n", value_stack[i].closure->f);
                break;
            case 1:
                dbg("%d\n", value_stack[i].integer);
        }
    }
}