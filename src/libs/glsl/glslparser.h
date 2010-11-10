
#include "glslparsertable_p.h"
#include "glsllexer.h"
#include "glslast.h"
#include <vector>
#include <stack>

namespace GLSL {

class Parser: public GLSLParserTable
{
public:
    Parser(const char *source, unsigned size, int variant);
    ~Parser();

    bool parse();

private:
    inline int consumeToken() { return _index++; }
    inline const Token &tokenAt(int index) const { return _tokens.at(index); }
    inline int tokenKind(int index) const { return _tokens.at(index).kind; }
    void dump(AST *ast);

private:
    int _tos;
    int _index;
    std::vector<int> _stateStack;
    std::vector<int> _locationStack;
    std::vector<AST *> _astStack;
    std::vector<Token> _tokens;
};

} // end of namespace GLSL
