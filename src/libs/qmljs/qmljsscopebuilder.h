#ifndef QMLJSSCOPEBUILDER_H
#define QMLJSSCOPEBUILDER_H

#include <qmljs/qmljsdocument.h>

#include <QtCore/QList>

namespace QmlJS {

namespace AST {
    class Node;
}

namespace Interpreter {
    class Context;
    class Value;
    class ObjectValue;
}

class QMLJS_EXPORT ScopeBuilder
{
public:
    ScopeBuilder(Document::Ptr doc, Interpreter::Context *context);
    ~ScopeBuilder();

    void push(AST::Node *node);
    void push(const QList<AST::Node *> &nodes);
    void pop();

    static const Interpreter::ObjectValue *isPropertyChangesObject(Interpreter::Context *context, const Interpreter::ObjectValue *object);

private:
    void setQmlScopeObject(AST::Node *node);
    const Interpreter::Value *scopeObjectLookup(AST::UiQualifiedId *id);

    Document::Ptr _doc;
    Interpreter::Context *_context;
    QList<AST::Node *> _nodes;
};

} // namespace QmlJS

#endif // QMLJSSCOPEBUILDER_H
