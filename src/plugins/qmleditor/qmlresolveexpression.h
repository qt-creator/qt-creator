#ifndef QMLRESOLVEEXPRESSION_H
#define QMLRESOLVEEXPRESSION_H

#include "qmllookupcontext.h"

#include <qml/parser/qmljsastvisitor_p.h>
#include <qml/qmlsymbol.h>

namespace QmlEditor {
namespace Internal {

class QmlResolveExpression: protected QmlJS::AST::Visitor
{
public:
    QmlResolveExpression(const QmlLookupContext &context);

    QmlSymbol *typeOf(QmlJS::AST::Node *node);
    QList<QmlSymbol*> visibleSymbols(QmlJS::AST::Node *node);

protected:
    using QmlJS::AST::Visitor::visit;

    QmlSymbol *switchValue(QmlSymbol *symbol);

    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);

private:
    QmlLookupContext _context;
    QmlSymbol *_value;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLRESOLVEEXPRESSION_H
