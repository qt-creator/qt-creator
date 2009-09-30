#ifndef COMPLETIONVISITOR_H
#define COMPLETIONVISITOR_H

#include <QtCore/QMap>
#include <QtCore/QSet>
#include <QtCore/QStack>
#include <QtCore/QString>

#include "qmljsastfwd_p.h"
#include "qmljsastvisitor_p.h"
#include "qmljsengine_p.h"

namespace QmlEditor {
namespace Internal {

class QmlCompletionVisitor: protected QmlJS::AST::Visitor
{
public:
    QmlCompletionVisitor();

    QSet<QString> operator()(QmlJS::AST::UiProgram *ast, int pos);

protected:
    virtual bool preVisit(QmlJS::AST::Node *node);
    virtual void postVisit(QmlJS::AST::Node *) { m_parentStack.pop(); }

    virtual bool visit(QmlJS::AST::UiScriptBinding *ast);

private:
    QmlJS::AST::UiObjectDefinition *findParentObject(QmlJS::AST::Node *node) const;

private:
    QSet<QString> m_completions;
    quint32 m_pos;
    QStack<QmlJS::AST::Node *> m_parentStack;
    QMap<QmlJS::AST::Node *, QmlJS::AST::Node *> m_nodeParents;
    QMap<QmlJS::AST::Node *, QString> m_objectToId;
};

} // namespace Internal
} // namespace QmlEditor

#endif // COMPLETIONVISITOR_H
