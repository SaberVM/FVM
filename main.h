#define LAM 0
#define APP 1
#define VAR 2
#define RET 3
#define LIT 4
#define CAP 5

struct Closure {
    int f;
    void *env;
    int env_size;
};

struct Value {
    int type;
    union {
        struct Closure *closure;
        int integer;
    };
};

void print_value_stack(struct Value *value_stack, int value_stack_size);