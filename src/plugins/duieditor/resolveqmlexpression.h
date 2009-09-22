#ifndef RESOLVEQMLEXPRESSION_H
#define RESOLVEQMLEXPRESSION_H

#include "qmljsastvisitor_p.h"
#include "qmllookupcontext.h"

namespace DuiEditor {
namespace Internal {

class ResolveQmlExpression: protected QmlJS::AST::Visitor
{
public:
    ResolveQmlExpression(const QmlLookupContext &context);

    QmlLookupContext::Symbol *operator()(QmlJS::AST::Node *node)
    { return typeOf(node); }

protected:
    using QmlJS::AST::Visitor::visit;

    QmlLookupContext::Symbol *typeOf(QmlJS::AST::Node *node);
    QmlLookupContext::Symbol *switchValue(QmlLookupContext::Symbol *symbol);

    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);

private:
    QmlLookupContext _context;
    QmlLookupContext::Symbol *_value;
};

} // namespace Internal
} // namespace DuiEditor

#endif // RESOLVEQMLEXPRESSION_H
