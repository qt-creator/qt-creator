#ifndef QMLMETATYPEBACKEND_H
#define QMLMETATYPEBACKEND_H

#include <qml/qml_global.h>
#include <qml/qmlpackageinfo.h>
#include <qml/qmlsymbol.h>

namespace Qml {
namespace MetaType {

class QmlTypeSystem;

class QML_EXPORT QmlMetaTypeBackend
{
public:
    QmlMetaTypeBackend(QmlTypeSystem *typeSystem);
    virtual ~QmlMetaTypeBackend() = 0;

    virtual QList<QmlSymbol *> availableTypes(const QString &package, int majorVersion, int minorVersion) = 0;
    virtual QmlSymbol *resolve(const QString &typeName, const QList<PackageInfo> &packages) = 0;

protected:
    QmlTypeSystem *typeSystem() const
    { return m_typeSystem; }

private:
    QmlTypeSystem *m_typeSystem;
};

} // namespace MetaType
} // namespace Qml

#endif // QMLMETATYPEBACKEND_H
