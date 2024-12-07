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
                case ARR: dbg("ARR "); break;
                case GET: dbg("GET "); break;
                case SET: dbg("SET "); break;
                case FST: dbg("FST"); break;
                case SND: dbg("SND"); break;
                case LET: dbg("LET %d ", code[++i]); break;
                default: dbg("?%d ", code[i]); break;
            }
        }
        dbg("\n");
        print_value_stack(value_stack, value_stack_size);
        switch (code[pc]) {
            case LAM: {
                char skip = code[pc + 1];
                value_stack[value_stack_size].type = 0;
                value_stack[value_stack_size].closure.f = pc + 2;
                value_stack[value_stack_size].closure.env = malloc(sizeof(struct Value) * captures_size);
                value_stack[value_stack_size].closure.env_size = captures_size;
                memcpy(value_stack[value_stack_size].closure.env, captures, captures_size);
                value_stack_size++;
                captures_size = 0;
                pc += skip + 2;
                break;
            }
            case CAP: {
                pc++;
                char index = code[pc++];
                struct Value old = env_stack[env_stack_size - index - 1];
                captures[captures_size++] = clone(old, &old);
                break;
            }
            case VAR: {
                pc++;
                char index = code[pc++];
                struct Value old = env_stack[env_stack_size - index - 1];
                value_stack[value_stack_size++] = clone(old, &old);
                break;
            }
            case RET: {
                struct Value val = value_stack[--value_stack_size];
                struct Value continuation = value_stack[--value_stack_size];
                pc = continuation.closure.f;
                free_all(env_stack, env_stack_size);
                memcpy(env_stack, continuation.closure.env, continuation.closure.env_size);
                env_stack_size = continuation.closure.env_size;
                free(continuation.closure.env);
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
                struct Value arg = value_stack[--value_stack_size];
                struct Value closure = value_stack[--value_stack_size];
                int old_pc = pc;
                pc = closure.closure.f;
                free_all(env_stack, env_stack_size);
                memcpy(env_stack, closure.closure.env, closure.closure.env_size);
                env_stack_size = closure.closure.env_size;
                free(closure.closure.env);
                value_stack[value_stack_size] = (struct Value){
                    .type = 0, 
                    .closure = {
                        .f = old_pc + 1,
                        .env = malloc(sizeof(struct Value) * captures_size),
                        .env_size = captures_size,
                    }
                };
                memcpy(value_stack[value_stack_size].closure.env, captures, sizeof(struct Value) * captures_size);
                value_stack_size++;
                captures_size = 0;
                env_stack[env_stack_size++] = arg;
                break;
            }
            case OWN: {
                pc++;
                char index = code[pc++];
                value_stack[value_stack_size++] = env_stack[env_stack_size - index - 1];
                break;
            }
            case ARR: {
                pc++;
                int size = value_stack[--value_stack_size].integer; // TODO: type check
                value_stack[value_stack_size++] = (struct Value){
                    .type = 2,
                    .array = {
                        .size = size,
                        .values = calloc(size, sizeof(struct Value))
                    }
                };
                break;
            }
            case GET: {
                pc++;
                int index = value_stack[--value_stack_size].integer; // TODO: type check
                struct Value arr = value_stack[value_stack_size - 1];
                if (arr.type == 2 && index >= 0 && index < arr.array.size) {
                    struct Value val = arr.array.values[index];
                    value_stack[value_stack_size++] = clone(val, &val);
                } else {
                    fprintf(stderr, "Error: Invalid array access: t=%d, i=%d\n", arr.type, index);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case SET: {
                pc++;
                int index = value_stack[--value_stack_size].integer; // TODO: type check
                struct Value arr = value_stack[--value_stack_size];
                struct Value val = value_stack[--value_stack_size];
                if (arr.type == 2 && index >= 0 && index < arr.array.size) {
                    arr.array.values[index] = val;
                    value_stack[value_stack_size++] = arr;
                } else {
                    fprintf(stderr, "Error: Invalid array access: t=%d, i=%d, s=%d\n", arr.type, index, arr.array.size);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case FST: {
                pc++;
                value_stack_size--;
                break;
            }
            case SND: {
                pc++;
                value_stack[value_stack_size - 2] = value_stack[value_stack_size - 1];
                value_stack_size--;
                break;
            }
            case LET: {
                pc++;
                char index = code[pc++];
                env_stack[env_stack_size++] = value_stack[value_stack_size-- - index - 1];
                break;
            }
        }
    }
    if (value_stack[value_stack_size - 1].type == 1) {
        printf("%d\n", value_stack[value_stack_size - 1].integer);
    }
}

void free_all(struct Value *env, int env_size) {
    for (int i = 0; i < env_size; i++) {
        switch (env[i].type) {
            case 0:
                free(env[i].closure.env);
                break;
            case 2:
                free(env[i].array.values);
                break;
        }
    }
}

struct Value clone(struct Value dest, struct Value *src) {
    switch (dest.type) {
        case 0:
            dest.closure.env = malloc(sizeof(struct Value) * dest.closure.env_size);
            memcpy(dest.closure.env, src->closure.env, dest.closure.env_size);
            break;
        case 1:
            break;
        case 2:
            dest.array.values = malloc(sizeof(struct Value) * dest.array.size);
            memcpy(dest.array.values, src->array.values, dest.array.size);
            break;
    } 
    return dest;
}

void print_value_stack(struct Value *value_stack, int value_stack_size) {
    for (int i = value_stack_size - 1; i >= 0; i--) {
        dbg("value_stack[%d]: ", i);
        switch (value_stack[i].type) {
            case 0:
                dbg("closure(f=%d)\n", value_stack[i].closure.f);
                break;
            case 1:
                dbg("%d\n", value_stack[i].integer);
                break;
            case 2:
                dbg("array(size=%d)\n", value_stack[i].array.size);
                break;
            default:
                dbg("unknown(%d)\n", value_stack[i].type);
        }
    }
}