#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "qmlexpressionundercursor.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlExpressionUnderCursor::QmlExpressionUnderCursor()
{
}

void QmlExpressionUnderCursor::operator()(const QTextCursor &cursor)
{
    _pos = cursor.position();
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::Block *ast)
{
    _scopes.push(ast);

    return true;
}

void QmlExpressionUnderCursor::endVisit(QmlJS::AST::Block *)
{
    _scopes.pop();
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::UiObjectBinding *ast)
{
    _scopes.push(ast);

    return true;
}

void QmlExpressionUnderCursor::endVisit(QmlJS::AST::UiObjectBinding *)
{
    _scopes.pop();
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    _scopes.push(ast);

    return true;
}

void QmlExpressionUnderCursor::endVisit(QmlJS::AST::UiObjectDefinition *)
{
    _scopes.pop();
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::UiScriptBinding *ast)
{
    if (ast->firstSourceLocation().offset <= _pos && _pos <= ast->lastSourceLocation().end()) {
        _expressionNode = ast;
        _expressionScopes = _scopes;
    }

    return false;
}
