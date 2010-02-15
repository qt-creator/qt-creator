#include "nodeinstancesignalspy.h"


#include <QMetaProperty>
#include <QMetaObject>
#include <QtDebug>
#include <private/qmlmetaproperty_p.h>

#include "objectnodeinstance.h"
#include <QSharedPointer>

namespace QmlDesigner {
namespace Internal {

NodeInstanceSignalSpy::NodeInstanceSignalSpy() :
    QObject()
{
}

void NodeInstanceSignalSpy::setObjectNodeInstance(const ObjectNodeInstance::Pointer &nodeInstance)
{
    methodeOffset = QObject::staticMetaObject.methodCount() + 1;
    registerObject(nodeInstance->object());
    m_objectNodeInstance = nodeInstance;

}

void NodeInstanceSignalSpy::registerObject(QObject *spiedObject, const QString &prefix)
{
    for (int index = QObject::staticMetaObject.propertyOffset();
         index < spiedObject->metaObject()->propertyCount();
         index++) {
             QMetaProperty metaProperty = spiedObject->metaObject()->property(index);
             if (metaProperty.isReadable()
                 && !metaProperty.isWritable()
                 && QmlMetaType::isQObject(metaProperty.userType())) {
                  QObject *propertyObject = QmlMetaType::toQObject(metaProperty.read(spiedObject));
                  registerObject(propertyObject, prefix + metaProperty.name() + ".");
             } else if (metaProperty.hasNotifySignal()) {
                 QMetaMethod metaMethod = metaProperty.notifySignal();
                 bool isConnecting = QMetaObject::connect(spiedObject, metaMethod.methodIndex(), this, methodeOffset, Qt::DirectConnection);
                 Q_ASSERT(isConnecting);
                 m_indexPropertyHash.insert(methodeOffset, prefix + metaProperty.name());
                 methodeOffset++;
             }
         }
}

int NodeInstanceSignalSpy::qt_metacall(QMetaObject::Call call, int methodId, void **a)
{
    if (call == QMetaObject::InvokeMetaMethod && methodId > QObject::staticMetaObject.methodCount()) {
        ObjectNodeInstance::Pointer nodeInstance = m_objectNodeInstance.toStrongRef();

        if (nodeInstance && nodeInstance->nodeInstanceView()) {
            nodeInstance->nodeInstanceView()->notifyPropertyChange(nodeInstance->modelNode(), m_indexPropertyHash.value(methodId));
        }

    }

    return QObject::qt_metacall(call, methodId, a);
}

} // namespace Internal
} // namespace QmlDesigner
