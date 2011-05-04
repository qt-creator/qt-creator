#include "qmljsscopeastpath.h"

#include "parser/qmljsast_p.h"

using namespace QmlJS;
using namespace AST;

ScopeAstPath::ScopeAstPath(Document::Ptr doc)
    : _doc(doc)
{
}

QList<Node *> ScopeAstPath::operator()(quint32 offset)
{
    _result.clear();
    _offset = offset;
    if (_doc)
        Node::accept(_doc->ast(), this);
    return _result;
}

void ScopeAstPath::accept(Node *node)
{ Node::acceptChild(node, this); }

bool ScopeAstPath::preVisit(Node *node)
{
    if (Statement *stmt = node->statementCast()) {
        return containsOffset(stmt->firstSourceLocation(), stmt->lastSourceLocation());
    } else if (ExpressionNode *exp = node->expressionCast()) {
        return containsOffset(exp->firstSourceLocation(), exp->lastSourceLocation());
    } else if (UiObjectMember *ui = node->uiObjectMemberCast()) {
        return containsOffset(ui->firstSourceLocation(), ui->lastSourceLocation());
    }
    return true;
}

bool ScopeAstPath::visit(UiObjectDefinition *node)
{
    _result.append(node);
    Node::accept(node->initializer, this);
    return false;
}

bool ScopeAstPath::visit(UiObjectBinding *node)
{
    _result.append(node);
    Node::accept(node->initializer, this);
    return false;
}

bool ScopeAstPath::visit(FunctionDeclaration *node)
{
    return visit(static_cast<FunctionExpression *>(node));
}

bool ScopeAstPath::visit(FunctionExpression *node)
{
    Node::accept(node->formals, this);
    _result.append(node);
    Node::accept(node->body, this);
    return false;
}

bool ScopeAstPath::containsOffset(SourceLocation start, SourceLocation end)
{
    return _offset >= start.begin() && _offset <= end.end();
}
