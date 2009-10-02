#ifndef QMLLOOKUPCONTEXT_H
#define QMLLOOKUPCONTEXT_H

#include <QStack>

#include "qmldocument.h"
#include "qmljsastvisitor_p.h"
#include "qmlsymbol.h"

namespace QmlEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<QmlSymbol *> &scopes,
                     const QmlDocument::Ptr &doc,
                     const Snapshot &snapshot);

    QmlSymbol *resolve(const QString &name);
    QmlSymbol *resolveType(const QString &name)
    { return resolveType(name, _doc->fileName()); }
    QmlSymbol *resolveType(QmlJS::AST::UiQualifiedId *name)
    { return resolveType(toString(name), _doc->fileName()); }

    QmlDocument::Ptr document() const
    { return _doc; }

    QList<QmlSymbol*> visibleSymbolsInScope();
    QList<QmlSymbol*> visibleTypes();

private:
    QmlSymbol *resolveType(const QString &name, const QString &fileName);
    QmlSymbol *resolveProperty(const QString &name, QmlSymbol *scope, const QString &fileName);

    static QString toString(QmlJS::AST::UiQualifiedId *id);

private:
    QStack<QmlSymbol *> _scopes;
    QmlDocument::Ptr _doc;
    Snapshot _snapshot;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLLOOKUPCONTEXT_H
