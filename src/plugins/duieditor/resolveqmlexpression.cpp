#include "qmljsast_p.h"
#include "qmljsengine_p.h"
#include "resolveqmlexpression.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

ResolveQmlExpression::ResolveQmlExpression(const QmlLookupContext &context)
    : _context(context), _value(0)
{
}

QmlLookupContext::Symbol *ResolveQmlExpression::typeOf(Node *node)
{
    QmlLookupContext::Symbol *previousValue = switchValue(0);
    if (node)
        node->accept(this);
    return switchValue(previousValue);
}


QmlLookupContext::Symbol *ResolveQmlExpression::switchValue(QmlLookupContext::Symbol *value)
{
    QmlLookupContext::Symbol *previousValue = _value;
    _value = value;
    return previousValue;
}

bool ResolveQmlExpression::visit(IdentifierExpression *ast)
{
    const QString name = ast->name->asString();
    _value = _context.resolve(name);
    return false;
}

bool ResolveQmlExpression::visit(FieldMemberExpression *ast)
{
    const QString memberName = ast->name->asString();

    if (QmlLookupContext::Symbol *base = typeOf(ast->base)) {
        UiObjectMemberList *members = 0;

        if (UiObjectBinding *uiObjectBinding = cast<UiObjectBinding *>(base)) {
            if (uiObjectBinding->initializer)
                members = uiObjectBinding->initializer->members;
        } else if (UiObjectDefinition *uiObjectDefinition = cast<UiObjectDefinition *>(base)) {
            if (uiObjectDefinition->initializer)
                members = uiObjectDefinition->initializer->members;
        }

        for (UiObjectMemberList *it = members; it; it = it->next) {
            UiObjectMember *member = it->member;

            if (UiPublicMember *publicMember = cast<UiPublicMember *>(member)) {
                if (publicMember->name->asString() == memberName) {
                    _value = publicMember;
                    break; // we're done.
                }
            }
        }
    }

    return false;
}

