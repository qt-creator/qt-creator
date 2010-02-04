#include "nodeinstancemetaobject.h"

namespace QmlDesigner {
namespace Internal {

NodeInstanceMetaObject::NodeInstanceMetaObject(QObject *object, QmlContext *context)
    : QmlOpenMetaObject(object),
    m_context(context)
{
}

void NodeInstanceMetaObject::createNewProperty(const QString &name)
{
    createProperty(name.toLatin1(), 0);
}

int NodeInstanceMetaObject::metaCall(QMetaObject::Call c, int id, void **a)
{
    if (( c == QMetaObject::ReadProperty || c == QMetaObject::WriteProperty)
        && id >= type()->propertyOffset()) {
        int propId = id - type()->propertyOffset();
        if (c == QMetaObject::ReadProperty) {
            propertyRead(propId);
            *reinterpret_cast<QVariant *>(a[0]) = value(propId);
        } else if (c == QMetaObject::WriteProperty) {
            if (value(propId) != *reinterpret_cast<QVariant *>(a[0]))  {
                propertyWrite(propId);
                setValue(propId, *reinterpret_cast<QVariant *>(a[0]));
                propertyWritten(propId);
                activate(object(), type()->signalOffset() + propId, 0);
            }
        }
        return -1;
    } else {
        if (parent())
            return parent()->metaCall(c, id, a);
        else
            return object()->qt_metacall(c, id, a);
    }

}

void NodeInstanceMetaObject::propertyWritten(int propertyId)
{
    if (m_context)
        m_context->setContextProperty(name(propertyId), value(propertyId));
}

} // namespace Internal
} // namespace QmlDesigner
