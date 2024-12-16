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
#define FST 10
#define SND 11
#define LET 12
#define LEN 13
#define ADD 14
#define SUB 15
#define MUL 16
#define DIV 17
#define MOD 18
#define EQL 19
#define GRT 20
#define AND 21
#define NOT 23
#define JIF 24
#define REP 25
#define BRK 26
#define CNT 27
#define PAR 28
#define SYS 29
#define CUT 30

struct Value {
    int type;
    union {
        struct {
            int f;
            struct Value *env;
            int env_size;
        } closure;
        double number;
        struct {
            int size;
            struct Value *values;
        } array;
    };
};

int equal(struct Value *a, struct Value *b);

void free_all(struct Value *env, int env_size, struct Value *except);

struct Value clone(struct Value dest, struct Value *src);

void print_value_stack(struct Value *value_stack, int value_stack_size);