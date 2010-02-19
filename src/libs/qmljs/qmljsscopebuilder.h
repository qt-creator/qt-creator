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
}

class ScopeBuilder
{
public:
    ScopeBuilder(Document::Ptr doc, Interpreter::Context *context);
    ~ScopeBuilder();

    void push(AST::Node *node);
    void pop();

private:
    void setQmlScopeObject(AST::Node *node);

    Document::Ptr _doc;
    Interpreter::Context *_context;
    QList<AST::Node *> _nodes;
};

} // namespace QmlJS

#endif // QMLJSSCOPEBUILDER_H
