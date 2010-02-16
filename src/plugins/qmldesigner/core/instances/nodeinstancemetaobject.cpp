#include "nodeinstancemetaobject.h"

#include "objectnodeinstance.h"
#include <QSharedPointer>
#include <qnumeric.h>

namespace QmlDesigner {
namespace Internal {

NodeInstanceMetaObject::NodeInstanceMetaObject(const ObjectNodeInstance::Pointer &nodeInstance)
    : QmlOpenMetaObject(nodeInstance->object()),
    m_nodeInstance(nodeInstance),
    m_context(nodeInstance->modelNode().isRootNode() ? nodeInstance->context() : 0)
{
}

NodeInstanceMetaObject::NodeInstanceMetaObject(const ObjectNodeInstancePointer &nodeInstance, QObject *object, const QString &prefix)
    : QmlOpenMetaObject(object),
    m_nodeInstance(nodeInstance),
    m_prefix(prefix)
{
}

void NodeInstanceMetaObject::createNewProperty(const QString &name)
{
    createProperty(name.toLatin1(), 0);
}

int NodeInstanceMetaObject::metaCall(QMetaObject::Call call, int id, void **a)
{
    int metaCallReturnValue = -1;

    if (call == QMetaObject::WriteProperty
        && reinterpret_cast<QVariant *>(a[0])->type() == QVariant::Double
        && qIsNaN(reinterpret_cast<QVariant *>(a[0])->toDouble())) {
        return -1;
    }

    QVariant oldValue;

    if (call == QMetaObject::WriteProperty && !property(id).hasNotifySignal())
    {
        oldValue = property(id).read(m_nodeInstance->object());
    }

    if (( call == QMetaObject::ReadProperty || call == QMetaObject::WriteProperty)
        && id >= type()->propertyOffset()) {
        int propId = id - type()->propertyOffset();
        if (call == QMetaObject::ReadProperty) {
            propertyRead(propId);
            *reinterpret_cast<QVariant *>(a[0]) = value(propId);
        } else if (call == QMetaObject::WriteProperty) {
            if (value(propId) != *reinterpret_cast<QVariant *>(a[0]))  {
                propertyWrite(propId);
                setValue(propId, *reinterpret_cast<QVariant *>(a[0]));
                dynamicPropertyWritten(propId);
                notifyPropertyChange(id);
                activate(object(), type()->signalOffset() + propId, 0);
            }
        }
    } else {
        if (parent())
            metaCallReturnValue = parent()->metaCall(call, id, a);
        else
            metaCallReturnValue = object()->qt_metacall(call, id, a);

        if (call == QMetaObject::WriteProperty
            && !property(id).hasNotifySignal()
            && oldValue != property(id).read(m_nodeInstance->object()))
            notifyPropertyChange(id);
    }

    return metaCallReturnValue;

}

void NodeInstanceMetaObject::dynamicPropertyWritten(int propertyId)
{

    if (m_context)
        m_context->setContextProperty(name(propertyId), value(propertyId));
}

void NodeInstanceMetaObject::notifyPropertyChange(int id)
{
    ObjectNodeInstance::Pointer objectNodeInstance = m_nodeInstance.toStrongRef();

    if (objectNodeInstance && objectNodeInstance->nodeInstanceView()) {
        if (id < type()->propertyOffset()) {
            objectNodeInstance->nodeInstanceView()->notifyPropertyChange(objectNodeInstance->modelNode(), m_prefix + property(id).name());
        } else {
            objectNodeInstance->nodeInstanceView()->notifyPropertyChange(objectNodeInstance->modelNode(), m_prefix + name(id - type()->propertyOffset()));
        }
    }
}

} // namespace Internal
} // namespace QmlDesigner
