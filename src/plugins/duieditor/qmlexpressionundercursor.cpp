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
    Node::accept(ast->qualifiedId, this);
    Node::accept(ast->qualifiedTypeNameId, this);

    _scopes.push(ast);

    Node::accept(ast->initializer, this);

    return false;
}

void QmlExpressionUnderCursor::endVisit(QmlJS::AST::UiObjectBinding *)
{
    _scopes.pop();
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::UiObjectDefinition *ast)
{
    Node::accept(ast->qualifiedTypeNameId, this);

    _scopes.push(ast);

    Node::accept(ast->initializer, this);

    return false;
}

void QmlExpressionUnderCursor::endVisit(QmlJS::AST::UiObjectDefinition *)
{
    _scopes.pop();
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::UiQualifiedId *ast)
{
    if (ast->identifierToken.offset <= _pos) {
        for (UiQualifiedId *iter = ast; iter; iter = iter->next) {
            if (_pos <= iter->identifierToken.end()) {
                // found it
                _expressionNode = ast;
                _expressionOffset = ast->identifierToken.offset;

                for (UiQualifiedId *iter2 = ast; iter2; iter2 = iter2->next) {
                    _expressionLength = iter2->identifierToken.end() - _expressionOffset;
                }

                _expressionScopes = _scopes;
                break;
            }
        }
    }

    return false;
}

bool QmlExpressionUnderCursor::visit(QmlJS::AST::UiImport * /*ast*/)
{
    return false;
}
