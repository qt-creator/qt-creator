#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "qmlexpressionundercursor.h"
#include "qmllookupcontext.h"
#include "resolveqmlexpression.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlLookupContext::QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                                   QmlJS::AST::Node *expressionNode,
                                   const DuiDocument::Ptr &doc,
                                   const Snapshot &snapshot):
        _scopes(scopes),
        _expressionNode(expressionNode),
        _doc(doc),
        _snapshot(snapshot)
{
}

QmlLookupContext::Symbol *QmlLookupContext::resolve(const QString &name) const
{
    // ### TODO: look at property definitions

    // look at the ids.
    foreach (DuiDocument::Ptr doc, _snapshot) {
        const DuiDocument::IdTable ids = doc->ids();
        const QPair<SourceLocation, Node *> use = ids.value(name);

        if (Node *node = use.second)
            return node;
    }

    return 0;
}
