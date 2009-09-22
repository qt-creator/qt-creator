#ifndef QMLLOOKUPCONTEXT_H
#define QMLLOOKUPCONTEXT_H

#include <QStack>

#include "duidocument.h"
#include "qmljsastvisitor_p.h"

namespace DuiEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                     QmlJS::AST::Node *expressionNode,
                     const DuiDocument::Ptr &doc,
                     const Snapshot &snapshot);

private:
    QStack<QmlJS::AST::Node *> _scopes;
    QmlJS::AST::Node *_expressionNode;
    DuiDocument::Ptr _doc;
    Snapshot _snapshot;
};

} // namespace Internal
} // namespace DuiEditor

#endif // QMLLOOKUPCONTEXT_H
