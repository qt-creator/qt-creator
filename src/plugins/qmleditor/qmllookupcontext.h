#ifndef QMLLOOKUPCONTEXT_H
#define QMLLOOKUPCONTEXT_H

#include <qml/metatype/qmltypesystem.h>
#include <qml/parser/qmljsastvisitor_p.h>
#include <qml/qmldocument.h>
#include <qml/qmlsymbol.h>

#include <QStack>

namespace QmlEditor {
namespace Internal {

class QmlLookupContext
{
public:
    QmlLookupContext(const QStack<Qml::QmlSymbol *> &scopes,
                     const QmlDocument::Ptr &doc,
                     const Snapshot &snapshot,
                     Qml::QmlTypeSystem *typeSystem);

    Qml::QmlSymbol *resolve(const QString &name);
    Qml::QmlSymbol *resolveType(const QString &name)
    { return resolveType(name, _doc->fileName()); }
    Qml::QmlSymbol *resolveType(QmlJS::AST::UiQualifiedId *name)
    { return resolveType(toString(name), _doc->fileName()); }

    QmlDocument::Ptr document() const
    { return _doc; }

    QList<Qml::QmlSymbol*> visibleSymbolsInScope();
    QList<Qml::QmlSymbol*> visibleTypes();

    QList<Qml::QmlSymbol*> expandType(Qml::QmlSymbol *symbol);

private:
    Qml::QmlSymbol *resolveType(const QString &name, const QString &fileName);
    Qml::QmlSymbol *resolveProperty(const QString &name, Qml::QmlSymbol *scope, const QString &fileName);
    Qml::QmlSymbol *resolveBuildinType(const QString &name);

    static QString toString(QmlJS::AST::UiQualifiedId *id);

private:
    QStack<Qml::QmlSymbol *> _scopes;
    QmlDocument::Ptr _doc;
    Snapshot _snapshot;
    Qml::QmlTypeSystem *m_typeSystem;
};

} // namespace Internal
} // namespace QmlEditor

#endif // QMLLOOKUPCONTEXT_H
