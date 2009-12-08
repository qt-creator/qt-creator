#ifndef QMLTYPESYSTEM_H
#define QMLTYPESYSTEM_H

#include <qml/qml_global.h>
#include <qml/qmlpackageinfo.h>
#include <qml/qmlsymbol.h>

#include <QtCore/QList>
#include <QtCore/QObject>

namespace Qml {

class QmlMetaTypeBackend;

class QML_EXPORT QmlTypeSystem: public QObject
{
    Q_OBJECT

public:
    QmlTypeSystem();
    virtual ~QmlTypeSystem();

    QList<QmlSymbol *> availableTypes(const QString &package, int majorVersion, int minorVersion);
    QmlSymbol *resolve(const QString &typeName, const QList<PackageInfo> &packages);

private:
    QList<QmlMetaTypeBackend *> backends;
};

} // namespace Qml

#endif // QMLTYPESYSTEM_H
