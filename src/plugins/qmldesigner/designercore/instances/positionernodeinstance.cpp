#include "positionernodeinstance.h"
#include <private/qdeclarativepositioners_p.h>
#include <invalidnodeinstanceexception.h>

namespace QmlDesigner {
namespace Internal {

PositionerNodeInstance::PositionerNodeInstance(QDeclarativeBasePositioner *item)
    : QmlGraphicsItemNodeInstance(item)
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

PositionerNodeInstance::Pointer PositionerNodeInstance::create(QObject *object)
{ 
    QDeclarativeBasePositioner *positioner = qobject_cast<QDeclarativeBasePositioner*>(object);

    if (positioner == 0)
        throw InvalidNodeInstanceException(__LINE__, __FUNCTION__, __FILE__);

    Pointer instance(new PositionerNodeInstance(positioner));

    instance->setHasContent(!positioner->flags().testFlag(QGraphicsItem::ItemHasNoContents));
    positioner->setFlag(QGraphicsItem::ItemHasNoContents, false);

    static_cast<QDeclarativeParserStatus*>(positioner)->classBegin();

    instance->populateResetValueHash();

    return instance;
}
}
} // namespace QmlDesigner
