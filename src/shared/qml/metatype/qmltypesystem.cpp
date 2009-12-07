#include "QmlMetaTypeBackend.h"
#include "qmltypesystem.h"

#ifdef BUILD_DECLARATIVE_BACKEND
#  include "QtDeclarativeMetaTypeBackend.h"
#endif // BUILD_DECLARATIVE_BACKEND

#include <QDebug>

using namespace Qml;
using namespace Qml::MetaType;

QmlTypeSystem::QmlTypeSystem()
{
#ifdef BUILD_DECLARATIVE_BACKEND
    backends.append(new Internal::QtDeclarativeMetaTypeBackend(this));
#endif // BUILD_DECLARATIVE_BACKEND
}

QmlTypeSystem::~QmlTypeSystem()
{
    qDeleteAll(backends);
}

QList<QmlSymbol *> QmlTypeSystem::availableTypes(const QString &package, int majorVersion, int minorVersion)
{
    QList<QmlSymbol *> results;

    foreach (QmlMetaTypeBackend *backend, backends)
        results.append(backend->availableTypes(package, majorVersion, minorVersion));

    return results;
}

QmlSymbol *QmlTypeSystem::resolve(const QString &typeName, const QList<PackageInfo> &packages)
{
    foreach (QmlMetaTypeBackend *backend, backends)
        if (QmlSymbol *symbol = backend->resolve(typeName, packages))
            return symbol;

    return 0;
}
