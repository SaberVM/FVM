#include "main.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>

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
                case LEN: dbg("LEN "); break;
                case ADD: dbg("ADD "); break;
                case SUB: dbg("SUB "); break;
                case MUL: dbg("MUL "); break;
                case DIV: dbg("DIV "); break;
                case MOD: dbg("MOD "); break;
                case EQL: dbg("EQL "); break;
                case GRT: dbg("GRT "); break;
                case AND: dbg("AND "); break;
                case NOT: dbg("NOT "); break;
                case JIF: dbg("JIF %d ", code[++i]); break;
                case REP: dbg("REP "); break;
                case BRK: dbg("BRK %d ", code[++i]); break;
                case CNT: dbg("CNT %d ", code[++i]); break;
                case PAR: dbg("PAR %d %d ", code[i + 1], code[i + 2]); i += 2; break;
                case SYS: dbg("SYS %d ", code[++i]); break;
                case CUT: dbg("CUT %d ", code[++i]); break;
                default: dbg("?%d ", code[i]);
            }
        }
        dbg("\n");
        switch (code[pc]) {
            case LAM: {
                char skip = code[pc + 1];
                struct Value *val = value_stack + value_stack_size++;
                dbg("* caps: ");
                print_value_stack(captures, captures_size);
                val->type = 0;
                val->closure.f = pc + 2;
                val->closure.env = malloc(sizeof(struct Value) * captures_size);
                val->closure.env_size = captures_size;
                memcpy(val->closure.env, captures, sizeof(struct Value) * captures_size);
                captures_size = 0;
                dbg("* lam: ");
                print_value_stack(val->closure.env, val->closure.env_size);
                pc += skip + 2;
                break;
            }
            case CAP: {
                pc++;
                char index = code[pc++];
                clone(captures + captures_size++, env_stack + env_stack_size - index - 1);
                break;
            }
            case VAR: {
                pc++;
                char index = code[pc++];
                clone(value_stack + value_stack_size++, env_stack + env_stack_size - index - 1);
                break;
            }
            case RET: {
                struct Value *cont = value_stack + value_stack_size - 2;
                struct Value *cont_env = cont->closure.env;
                int cont_env_size = cont->closure.env_size;
                pc = cont->closure.f;
                free_all_except(env_stack, env_stack_size, value_stack + value_stack_size - 1);
                memcpy(env_stack, cont_env, sizeof(struct Value) * cont_env_size);
                env_stack_size = cont_env_size;
                free(cont_env);
                value_stack[value_stack_size - 2] = value_stack[value_stack_size - 1];
                value_stack_size--;
                captures_size = 0;
                break;
            }
            case LIT: {
                pc++;
                char val = code[pc++];
                value_stack[value_stack_size].type = 1;
                value_stack[value_stack_size].number = (double)val;
                value_stack_size++;
                break;
            }
            case APP: {
                struct Value *closure = value_stack + value_stack_size - 2;
                struct Value *closure_env = closure->closure.env;
                int closure_env_size = closure->closure.env_size;
                int old_pc = pc;
                pc = closure->closure.f;
                free_all(env_stack, env_stack_size);
                memcpy(env_stack, closure_env, sizeof(struct Value) * closure_env_size);
                env_stack_size = closure_env_size;
                free(closure_env);
                struct Value *cont = value_stack + value_stack_size - 2;
                *cont = (struct Value){
                    .type = 0, 
                    .closure = {
                        .f = old_pc + 1,
                        .env = malloc(sizeof(struct Value) * captures_size),
                        .env_size = captures_size,
                    }
                };
                memcpy(cont->closure.env, captures, sizeof(struct Value) * captures_size);
                captures_size = 0;
                clone(env_stack + env_stack_size++, value_stack + value_stack_size - 1);
                value_stack_size--;
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
                double param = value_stack[--value_stack_size].number; // TODO: type check
                if ((int)param < param) {
                    fprintf(stderr, "Error: Invalid array size: i=%f\n", param);
                    exit(EXIT_FAILURE);
                }
                int size = (int)param;
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
                double param = value_stack[--value_stack_size].number; // TODO: type check
                if ((int)param < param) {
                    fprintf(stderr, "Error: Invalid array access: i=%f\n", param);
                    exit(EXIT_FAILURE);
                }
                int index = (int)param;
                struct Value arr = value_stack[value_stack_size - 1];
                if (arr.type == 2 && index >= 0 && index < arr.array.size) {
                    clone(value_stack + value_stack_size++, arr.array.values + index);
                } else {
                    fprintf(stderr, "Error: Invalid array access: t=%d, i=%d\n", arr.type, index);
                    exit(EXIT_FAILURE);
                }
                break;
            }
            case SET: {
                pc++;
                double param = value_stack[--value_stack_size].number; // TODO: type check
                if ((int)param < param) {
                    fprintf(stderr, "Error: Invalid array access: i=%f\n", param);
                    exit(EXIT_FAILURE);
                }
                int index = (int)param;
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
            case LEN: {
                pc++;
                struct Value arr = value_stack[value_stack_size - 1];
                if (arr.type == 2) {
                    value_stack[value_stack_size++] = (struct Value){.type = 1, .number = (double)arr.array.size};
                } else {
                    fprintf(stderr, "Error: Invalid array access: t=%d\n", arr.type);
                    exit(EXIT_FAILURE);
                }
            }
            case ADD: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = a + b};
                break;
            }
            case SUB: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = b - a};
                break;
            }
            case MUL: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = a * b};
                break;
            }
            case DIV: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = b / a};
                break;
            }
            case MOD: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = fmod(b, a)};
                break;
            }
            case EQL: {
                pc++;
                struct Value a = value_stack[--value_stack_size];
                struct Value b = value_stack[--value_stack_size];
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = (double)equal(&a, &b)};
                break;
            }
            case GRT: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = b > a};
                break;
            }
            case AND: {
                pc++;
                double a = value_stack[--value_stack_size].number;
                double b = value_stack[--value_stack_size].number;
                value_stack[value_stack_size++] = (struct Value){.type = 1, .number = a && b};
                break;
            }
            case NOT: {
                pc++;
                double x = value_stack[value_stack_size - 1].number;
                value_stack[value_stack_size - 1].number = !x;
                break;
            }
            case JIF: {
                pc++;
                char dist = code[pc];
                double cond = value_stack[--value_stack_size].number;
                if (cond) pc += dist;
            }
            case REP: {
                // a no-op; 
                // static analysis checks that CNT jumps to REP
                // in a balanced-parentheses kinda way
                pc++;
                break;
            }
            case BRK: {
                // static analysis checks that BRK jumps to CNT
                pc++;
                char dist = code[pc++];
                pc += dist;
                break;
            }
            case CNT: {
                char dist = code[pc + 1];
                pc -= dist;
                break;
            }
            case PAR: {
                // in advanced implementations,
                // this would run the next n instructions
                // concurrently with the following m instructions
                // like a fork/join
                // but running them sequentially is semantically valid
                pc += 3;
                break;
            }
            case SYS: {
                pc++;
                char which = code[pc++];
                switch (which) {
                    case 0: {
                        // print
                        double val = value_stack[--value_stack_size].number;
                        printf("%.0f\n", val);
                        break;
                    }
                }
                break;
            }
            case CUT: {
                pc++;
                char n = code[pc++];
                env_stack_size -= n;
                break;
            }
        }
        dbg("- env: ");
        print_value_stack(env_stack, env_stack_size);
        dbg("- vals: ");
        print_value_stack(value_stack, value_stack_size);
        dbg("- captures: ");
        print_value_stack(captures, captures_size);
    }
}

int equal(struct Value *a, struct Value *b) { // shallow equality
    if (a->type != b->type) return 0;
    switch (a->type) {
        case 0: return a->closure.f == b->closure.f;
        case 1: return fabs(a->number - b->number) < DBL_EPSILON * fabs(a->number + b->number);
        case 2: return a->array.values == b->array.values;
    }
    return 0; // shut up the compiler
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

void free_all_except(struct Value *env, int env_size, struct Value *except) {
    dbg("* free: ");
    print_value_stack(env, env_size);
    dbg("* except: ");
    print_value(except);
    dbg("\n");
    for (int i = 0; i < env_size; i++) {
        if (equal(env + sizeof(struct Value) * i, except)) continue;
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

void clone(struct Value *dest, struct Value *src) {
    dest->type = src->type;
    switch (src->type) {
        case 0:
            dest->closure.f = src->closure.f;
            dest->closure.env_size = src->closure.env_size;
            dest->closure.env = malloc(sizeof(struct Value) * src->closure.env_size);
            memcpy(dest->closure.env, src->closure.env, sizeof(struct Value) * src->closure.env_size);
            break;
        case 1:
            dest->number = src->number;
            break;
        case 2:
            dest->array.size = src->array.size;
            dest->array.values = malloc(sizeof(struct Value) * src->array.size);
            memcpy(dest->array.values, src->array.values, sizeof(struct Value) * src->array.size);
            break;
    }
}

void print_value_stack(struct Value *value_stack, int value_stack_size) {
    for (int i = value_stack_size - 1; i >= 0; i--) {
        print_value(value_stack + i);
        dbg(", ");
    }
    dbg("\n");
}

void print_value(struct Value *value) {
    switch (value->type) {
        case 0:
            dbg("closure(f=%d,env=%llu)", value->closure.f, value->closure.env);
            break;
        case 1:
            dbg("%f", value->number);
            break;
        case 2:
            dbg("array(size=%d)", value->array.size);
            break;
        default:
            dbg("unknown(%d)", value->type);
    }
}