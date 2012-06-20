#define FOR_EACH_INSTR(V) \
    V(ADD) \
    V(SUB)

#define OTHER_FOR_EACH(V) \
    V(DIV) \
    V(MUL)

#define DECLARE_INSTR(op) #op,
#define DECLARE_OP_INSTR(op) op_##op,

enum op_code {
    FOR_EACH_INSTR(DECLARE_OP_INSTR)
    OTHER_FOR_EACH(DECLARE_OP_INSTR)
};


static const char *names[] = {
FOR_EACH_INSTR(DECLARE_INSTR)
OTHER_FOR_EACH(DECLARE_INSTR)
};
