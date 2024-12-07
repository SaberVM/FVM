#define LAM 0
#define APP 1
#define VAR 2
#define RET 3
#define LIT 4
#define CAP 5
#define OWN 6
#define ARR 7
#define GET 8
#define SET 9
#define VAL 10

struct Value {
    int type;
    union {
        struct {
            int f;
            struct Value *env;
            int env_size;
        } closure;
        int integer;
        struct {
            int size;
            struct Value *values;
        } array;
    };
};

struct Value clone(struct Value dest, struct Value *src);

void print_value_stack(struct Value *value_stack, int value_stack_size);