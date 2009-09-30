#include <QtCore/QDebug>

#include "qmlcompletionvisitor.h"
#include "qmljsast_p.h"

using namespace QmlJS;
using namespace QmlJS::AST;

namespace QmlEditor {
namespace Internal {

QmlCompletionVisitor::QmlCompletionVisitor()
{
}

QSet<QString> QmlCompletionVisitor::operator()(QmlJS::AST::UiProgram *ast, int pos)
{
    m_completions.clear();
    m_pos = (quint32) pos;

    Node::acceptChild(ast, this);

    return m_completions;
}

bool QmlCompletionVisitor::preVisit(QmlJS::AST::Node *node)
{
    if (!m_parentStack.isEmpty())
        m_nodeParents[node] = m_parentStack.top();
    m_parentStack.push(node);
    return true;
}

static QString toString(Statement *stmt)
{
    if (ExpressionStatement *exprStmt = AST::cast<ExpressionStatement*>(stmt)) {
        if (IdentifierExpression *idExpr = AST::cast<IdentifierExpression *>(exprStmt->expression)) {
            return idExpr->name->asString();
        }
    }

    return QString();
}

bool QmlCompletionVisitor::visit(UiScriptBinding *ast)
{
    if (!ast)
        return false;

    UiObjectDefinition *parentObject = findParentObject(ast);

    if (ast->qualifiedId && ast->qualifiedId->name->asString() == QLatin1String("id")) {
        const QString nodeId = toString(ast->statement);
        if (!nodeId.isEmpty())
            m_objectToId[parentObject] = nodeId;
    } else if (m_objectToId.contains(parentObject)) {
        if (ast->qualifiedId && ast->qualifiedId->name) {
            const QString parentId = m_objectToId[parentObject];
            m_completions.insert(parentId + "." + ast->qualifiedId->name->asString());
        }
    }

    if (ast->firstSourceLocation().begin() >= m_pos && m_pos <= ast->lastSourceLocation().end()) {
        UiObjectDefinition *parentsParent = findParentObject(parentObject);

        if (parentsParent) {
            m_completions.insert(QLatin1String("parent"));
        }
    }

    return true;
}

UiObjectDefinition *QmlCompletionVisitor::findParentObject(Node *node) const
{
    if (!node)
        return 0;

    Node *candidate = m_nodeParents[node];
    if (candidate == 0)
        return 0;

    if (UiObjectDefinition *parentObject = AST::cast<UiObjectDefinition *>(candidate))
        return parentObject;
    else
        return findParentObject(candidate);
}

} // namespace Internal
} // namespace QmlEditor
