#ifndef QMLLOOKUPCONTEXT_H
#define QMLLOOKUPCONTEXT_H

#include <QStack>

#include "duidocument.h"
#include "qmljsastvisitor_p.h"
#include "qmlsymbol.h"

namespace DuiEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                     const DuiDocument::Ptr &doc,
                     const Snapshot &snapshot);

    QmlSymbol *resolve(const QString &name) const;

    DuiDocument::Ptr document() const
    { return _doc; }

private:
    QStack<QmlJS::AST::Node *> _scopes;
    DuiDocument::Ptr _doc;
    Snapshot _snapshot;
};

} // namespace Internal
} // namespace DuiEditor

#endif // QMLLOOKUPCONTEXT_H
