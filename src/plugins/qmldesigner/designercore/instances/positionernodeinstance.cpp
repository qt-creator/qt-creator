#include "positionernodeinstance.h"
#include <private/qdeclarativepositioners_p.h>
#include <invalidnodeinstanceexception.h>

namespace QmlDesigner {
namespace Internal {

PositionerNodeInstance::PositionerNodeInstance(QDeclarativeBasePositioner *item, bool hasContent)
    : QmlGraphicsItemNodeInstance(item, hasContent)
{
}

bool PositionerNodeInstance::isPositioner() const
{
    return true;
}

bool PositionerNodeInstance::isResizable() const
{
    return false;
}

void PositionerNodeInstance::setPropertyVariant(const QString &name, const QVariant &value)
{
    if (name == "move" || name == "add")
        return;

    QmlGraphicsItemNodeInstance::setPropertyVariant(name, value);
}

void PositionerNodeInstance::setPropertyBinding(const QString &name, const QString &expression)
{
    if (name == "move" || name == "add")
        return;

    QmlGraphicsItemNodeInstance::setPropertyBinding(name, expression);
}

PositionerNodeInstance::Pointer PositionerNodeInstance::create(const NodeMetaInfo &metaInfo, QDeclarativeContext *context, QObject *objectToBeWrapped)
{
    QPair<QGraphicsObject*, bool> objectPair;

    if (objectToBeWrapped)
        objectPair = qMakePair(qobject_cast<QGraphicsObject*>(objectToBeWrapped), false);
    else
        objectPair = GraphicsObjectNodeInstance::createGraphicsObject(metaInfo, context);

    QDeclarativeBasePositioner *positioner = dynamic_cast<QDeclarativeBasePositioner*>(objectPair.first);

    if (positioner == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new PositionerNodeInstance(positioner, objectPair.second));

    static_cast<QDeclarativeParserStatus*>(positioner)->classBegin();

    if (objectToBeWrapped)
        instance->setDeleteHeldInstance(false); // the object isn't owned

    instance->populateResetValueHash();

    return instance;
}
}
} // namespace QmlDesigner
