#ifndef QMLEXPRESSIONUNDERCURSOR_H
#define QMLEXPRESSIONUNDERCURSOR_H

#include <QStack>
#include <QTextCursor>

#include "qmljsastvisitor_p.h"

namespace DuiEditor {
namespace Internal {

class QmlExpressionUnderCursor: protected QmlJS::AST::Visitor
{
public:
    QmlExpressionUnderCursor();

    void operator()(const QTextCursor &cursor, QmlJS::AST::UiProgram *program);

    QStack<QmlJS::AST::Node *> expressionScopes() const
    { return _expressionScopes; }

    QmlJS::AST::Node *expressionNode() const
    { return _expressionNode; }

    int expressionOffset() const
    { return _expressionOffset; }

    int expressionLength() const
    { return _expressionLength; }

protected:
    virtual bool visit(QmlJS::AST::Block *ast);
    virtual bool visit(QmlJS::AST::FieldMemberExpression *ast);
    virtual bool visit(QmlJS::AST::IdentifierExpression *ast);
    virtual bool visit(QmlJS::AST::UiImport *ast);
    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);
    virtual bool visit(QmlJS::AST::UiQualifiedId *ast);

    virtual void endVisit(QmlJS::AST::Block *);
    virtual void endVisit(QmlJS::AST::UiObjectBinding *);
    virtual void endVisit(QmlJS::AST::UiObjectDefinition *);

private:
    QStack<QmlJS::AST::Node *> _scopes;
    QStack<QmlJS::AST::Node *> _expressionScopes;
    QmlJS::AST::Node *_expressionNode;
    int _expressionOffset;
    int _expressionLength;
    quint32 _pos;
};

} // namespace Internal
} // namespace DuiEditor

#endif // QMLEXPRESSIONUNDERCURSOR_H
