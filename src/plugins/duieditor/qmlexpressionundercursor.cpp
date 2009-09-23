#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "qmlexpressionundercursor.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlExpressionUnderCursor::QmlExpressionUnderCursor()
    : _expressionNode(0),
      _pos(0)
{
}

void QmlExpressionUnderCursor::operator()(const QTextCursor &cursor,
                                          QmlJS::AST::UiProgram *program)
{
    _pos = cursor.position();
    _expressionNode = 0;
    _expressionOffset = -1;
    _expressionLength = -1;
    _scopes.clear();

    if (program)
        program->accept(this);
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

bool QmlExpressionUnderCursor::visit(QmlJS::AST::FieldMemberExpression *ast)
{
    if (ast->identifierToken.offset <= _pos && _pos <= ast->identifierToken.end()) {
        _expressionNode = ast;
        _expressionOffset = ast->identifierToken.offset;
        _expressionLength = ast->identifierToken.length;
        _expressionScopes = _scopes;
    }

    return true;
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::IdentifierExpression *ast)
{
    if (ast->firstSourceLocation().offset <= _pos && _pos <= ast->lastSourceLocation().end()) {
        _expressionNode = ast;
        _expressionOffset = ast->firstSourceLocation().offset;
        _expressionLength = ast->lastSourceLocation().end() - _expressionOffset;
        _expressionScopes = _scopes;
    }

    return false;
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
