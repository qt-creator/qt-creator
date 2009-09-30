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
    QmlLookupContext(const QStack<QmlJS::AST::Node *> &scopes,
                     const QmlDocument::Ptr &doc,
                     const Snapshot &snapshot);
    ~QmlLookupContext();

    QmlSymbol *resolve(const QString &name);
    QmlSymbol *resolveType(const QString &name)
    { return resolveType(name, _doc->fileName()); }
    QmlSymbol *resolveType(QmlJS::AST::UiQualifiedId *name)
    { return resolveType(toString(name), _doc->fileName()); }

    QmlDocument::Ptr document() const
    { return _doc; }

    QList<QmlSymbol*> visibleSymbols(QmlJS::AST::Node *scope);
    QList<QmlSymbol*> visibleTypes();

private:
    QmlSymbol *createSymbol(const QString &fileName, QmlJS::AST::UiObjectMember *node);

    QmlSymbol *resolveType(const QString &name, const QString &fileName);
    QmlSymbol *resolveProperty(const QString &name, QmlJS::AST::Node *scope, const QString &fileName);
    QmlSymbol *resolveProperty(const QString &name, QmlJS::AST::UiObjectInitializer *initializer, const QString &fileName);

    static QString toString(QmlJS::AST::UiQualifiedId *id);

private:
    QStack<QmlJS::AST::Node *> _scopes;
    QmlDocument::Ptr _doc;
    Snapshot _snapshot;
    QList<QmlSymbol*> _temporarySymbols;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLLOOKUPCONTEXT_H
