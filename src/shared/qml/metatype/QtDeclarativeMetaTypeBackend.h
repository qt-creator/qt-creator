#ifndef QTDECLARATIVEMETATYPEBACKEND_H
#define QTDECLARATIVEMETATYPEBACKEND_H

#include <qml/metatype/QmlMetaTypeBackend.h>
#include <qml/metatype/nodemetainfo.h>
#include <qml/metatype/propertymetainfo.h>

#include <QtCore/QList>

namespace Qml {
namespace MetaType {
namespace Internal {

class QmlDeclarativeSymbol;
class QmlDeclarativeObjectSymbol;
class QmlDeclarativePropertySymbol;

class QtDeclarativeMetaTypeBackend: public QmlMetaTypeBackend
{
    friend class QmlDeclarativeSymbol;
    friend class QmlDeclarativeObjectSymbol;
    friend class QmlDeclarativePropertySymbol;

public:
    QtDeclarativeMetaTypeBackend(QmlTypeSystem *typeSystem);
    ~QtDeclarativeMetaTypeBackend();

    virtual QList<QmlSymbol *> availableTypes(const QString &package, int majorVersion, int minorVersion);
    virtual QmlSymbol *resolve(const QString &typeName, const QList<PackageInfo> &packages);

protected:
    QList<QmlSymbol *> members(const QKineticDesigner::NodeMetaInfo &metaInfo);
    QList<QmlSymbol *> inheritedMembers(const QKineticDesigner::NodeMetaInfo &metaInfo);
    QmlDeclarativeSymbol *typeOf(const QKineticDesigner::PropertyMetaInfo &metaInfo);

private:
    QmlDeclarativeSymbol *getSymbol(const QKineticDesigner::NodeMetaInfo &metaInfo);

private:
    QMap<QString, QmlDeclarativeSymbol*> m_symbols;
};

} // namespace Internal
} // namespace MetaType
} // namespace Qml

#endif // QTDECLARATIVEMETATYPEBACKEND_H
