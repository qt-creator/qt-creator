#define FOR_EACH_INSTR(V) \
    V(ADD) \
    V(SUB)

#define DECLARE_INSTR(op) #op,
#define DECLARE_OP_INSTR(op) op_##op,

enum op_code {
    FOR_EACH_INSTR(DECLARE_OP_INSTR)
};

static const char *names[] = {
FOR_EACH_INSTR(DECLARE_INSTR)
};
