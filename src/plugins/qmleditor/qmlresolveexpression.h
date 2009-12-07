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

    Qml::QmlSymbol *typeOf(QmlJS::AST::Node *node);
    QList<Qml::QmlSymbol*> visibleSymbols(QmlJS::AST::Node *node);

protected:
    using QmlJS::AST::Visitor::visit;

    Qml::QmlSymbol *switchValue(Qml::QmlSymbol *symbol);

    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);

private:
    QmlLookupContext _context;
    Qml::QmlSymbol *_value;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLRESOLVEEXPRESSION_H
