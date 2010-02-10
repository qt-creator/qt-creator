#include "behaviornodeinstance.h"

#include <private/qmlbehavior_p.h>

#include "invalidnodeinstanceexception.h"

namespace QmlDesigner {
namespace Internal {

BehaviorNodeInstance::BehaviorNodeInstance(QObject *object)
    : ObjectNodeInstance(object),
    m_isEnabled(true)
{
}

BehaviorNodeInstance::Pointer BehaviorNodeInstance::create(const NodeMetaInfo &nodeMetaInfo, QmlContext *context, QObject *objectToBeWrapped)
{
    QObject *object = 0;
    if (objectToBeWrapped)
        object = objectToBeWrapped;
    else
        object = createObject(nodeMetaInfo, context);

    QmlBehavior* behavior = qobject_cast<QmlBehavior*>(object);
    if (behavior == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new BehaviorNodeInstance(behavior));

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    behavior->setEnabled(false);

    return instance;
}

void BehaviorNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "enabled")
        return;

    ObjectNodeInstance::setPropertyVariant(name, value);
}

void BehaviorNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "enabled")
        return;

    ObjectNodeInstance::setPropertyBinding(name, expression);
}

QVariant BehaviorNodeInstance::property(const QString &name) const
{
    if (name == "enabled")
        return QVariant::fromValue(m_isEnabled);

    return ObjectNodeInstance::property(name);
}

void BehaviorNodeInstance::resetProperty(const QString &name)
{
    if (name == "enabled")
        m_isEnabled = true;

    ObjectNodeInstance::resetProperty(name);
}


} // namespace Internal
} // namespace QmlDesigner
