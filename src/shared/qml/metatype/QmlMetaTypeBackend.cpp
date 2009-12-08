#include "QmlMetaTypeBackend.h"
#include "qmltypesystem.h"

using namespace Qml;

QmlMetaTypeBackend::QmlMetaTypeBackend(QmlTypeSystem *typeSystem):
        m_typeSystem(typeSystem)
{
    Q_ASSERT(typeSystem);
}

QmlMetaTypeBackend::~QmlMetaTypeBackend()
{
}
