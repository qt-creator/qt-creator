#ifndef RESOLVEQMLEXPRESSION_H
#define RESOLVEQMLEXPRESSION_H

#include "qmljsastvisitor_p.h"

namespace DuiEditor {
namespace Internal {

class ResolveQmlExpression: protected QmlJS::AST::Visitor
{
public:
    ResolveQmlExpression();

protected:

private:
};

} // namespace Internal
} // namespace DuiEditor

#endif // RESOLVEQMLEXPRESSION_H
