#ifndef QMLRESOLVEEXPRESSION_H
#define QMLRESOLVEEXPRESSION_H

#include "qmljsastvisitor_p.h"
#include "qmllookupcontext.h"
#include "qmlsymbol.h"

namespace DuiEditor {
namespace Internal {

class QmlResolveExpression: protected QmlJS::AST::Visitor
{
public:
    QmlResolveExpression(const QmlLookupContext &context);
    ~QmlResolveExpression();

    QmlSymbol *operator()(QmlJS::AST::Node *node)
    { return typeOf(node); }

protected:
    using QmlJS::AST::Visitor::visit;

    QmlSymbol *typeOf(QmlJS::AST::Node *node);
    QmlSymbol *switchValue(QmlSymbol *symbol);

    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);

private:
    QmlPropertyDefinitionSymbol *createPropertyDefinitionSymbol(QmlJS::AST::UiPublicMember *ast);
    QmlSymbolFromFile *createSymbolFromFile(QmlJS::AST::UiObjectMember *ast);

private:
    QmlLookupContext _context;
    QList<QmlSymbol*> _temporarySymbols;
    QmlSymbol *_value;
};

} // namespace Internal
} // namespace DuiEditor

#endif // QMLRESOLVEEXPRESSION_H
