#include "qmljsast_p.h"
#include "qmljsengine_p.h"

#include "qmlexpressionundercursor.h"
#include "qmllookupcontext.h"
#include "qmlresolveexpression.h"

using namespace DuiEditor;
using namespace DuiEditor::Internal;
using namespace QmlJS;
using namespace QmlJS::AST;

QmlLookupContext::QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                                   const DuiDocument::Ptr &doc,
                                   const Snapshot &snapshot):
        _scopes(scopes),
        _doc(doc),
        _snapshot(snapshot)
{
}

QmlSymbol *QmlLookupContext::resolve(const QString &name) const
{
    // ### TODO: look at property definitions

    // look at the ids.
    const DuiDocument::IdTable ids = _doc->ids();

    if (ids.contains(name))
        return ids[name];
    else
        return 0;
}
