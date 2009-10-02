#include "qmljsast_p.h"
#include "qmljsengine_p.h"
#include "qmlresolveexpression.h"

using namespace QmlEditor;
using namespace QmlEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlResolveExpression::QmlResolveExpression(const QmlLookupContext &context)
    : _context(context), _value(0)
{
}

QmlSymbol *QmlResolveExpression::typeOf(Node *node)
{
    QmlSymbol *previousValue = switchValue(0);
    if (node)
        node->accept(this);
    return switchValue(previousValue);
}

QList<QmlSymbol*> QmlResolveExpression::visibleSymbols(Node *node)
{
    QList<QmlSymbol*> result;

    QmlSymbol *symbol = typeOf(node);
    if (symbol) {
        if (symbol->isIdSymbol())
            symbol = symbol->asIdSymbol()->parentNode();
        result.append(symbol->members());

        // TODO: also add symbols from super-types
    } else {
        result.append(_context.visibleTypes());
    }

    if (node) {
        foreach (QmlIdSymbol *idSymbol, _context.document()->ids().values())
            result.append(idSymbol);
    }

    result.append(_context.visibleSymbolsInScope());

    return result;
}

QmlSymbol *QmlResolveExpression::switchValue(QmlSymbol *value)
{
    QmlSymbol *previousValue = _value;
    _value = value;
    return previousValue;
}

bool QmlResolveExpression::visit(IdentifierExpression *ast)
{
    const QString name = ast->name->asString();
    _value = _context.resolve(name);
    return false;
}

static inline bool matches(UiQualifiedId *candidate, const QString &wanted)
{
    if (!candidate)
        return false;

    if (!(candidate->name))
        return false;

    if (candidate->next)
        return false; // TODO: verify this!

    return wanted == candidate->name->asString();
}

bool QmlResolveExpression::visit(FieldMemberExpression *ast)
{
    const QString memberName = ast->name->asString();

    QmlSymbol *base = typeOf(ast->base);
    if (!base)
        return false;

    if (base->isIdSymbol())
        base = base->asIdSymbol()->parentNode();

    if (!base)
        return false;

    foreach (QmlSymbol *memberSymbol, base->members())
        if (memberSymbol->name() == memberName)
            _value = memberSymbol;

    return false;
}

bool QmlResolveExpression::visit(QmlJS::AST::UiQualifiedId *ast)
{
    _value = _context.resolveType(ast);

    return false;
}
