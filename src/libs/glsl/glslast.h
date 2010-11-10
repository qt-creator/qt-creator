#ifndef GLSLAST_H
#define GLSLAST_H

#include <vector>

namespace GLSL {

class AST;
class Operand;
class Operator;

class AST
{
public:
    AST();
    virtual ~AST();

    virtual Operand *asOperand() { return 0; }
    virtual Operator *asOperator() { return 0; }
};

class Operand: public AST
{
public:
    Operand(int location)
        : location(location) {}

    virtual Operand *asOperand() { return this; }

public: // attributes
    int location;
};

class Operator: public AST, public std::vector<AST *>
{
    typedef std::vector<AST *> _Base;

public:
    template <typename It>
    Operator(int ruleno, It begin, It end)
        : _Base(begin, end), ruleno(ruleno) {}

    virtual Operator *asOperator() { return this; }

public: // attributes
    int ruleno;
};

} // namespace GLSL

#endif // GLSLAST_H
