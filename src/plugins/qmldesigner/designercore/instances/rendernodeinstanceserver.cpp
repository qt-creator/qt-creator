#include "rendernodeinstanceserver.h"

#include "nodeinstanceclientinterface.h"

#include "pixmapchangedcommand.h"

#include <QDeclarativeView>
#include <QGraphicsItem>

#include <private/qgraphicsitem_p.h>

namespace QmlDesigner {

RenderNodeInstanceServer::RenderNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    NodeInstanceServer(nodeInstanceClient)
{
}

void RenderNodeInstanceServer::findItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;
    if (!inFunction) {
        inFunction = true;

        bool adjustSceneRect = false;

        if (delcarativeView()) {
            foreach (QGraphicsItem *item, delcarativeView()->items()) {
                QGraphicsObject *graphicsObject = item->toGraphicsObject();
                if (graphicsObject && hasInstanceForObject(graphicsObject)) {
                    ServerNodeInstance instance = instanceForObject(graphicsObject);
                    QGraphicsItemPrivate *d = QGraphicsItemPrivate::get(item);

                    if((d->dirty && d->notifyBoundingRectChanged)|| (d->dirty && !d->dirtySceneTransform) || nonInstanceChildIsDirty(graphicsObject))
                        m_dirtyInstanceSet.insert(instance);

                    if (d->geometryChanged) {
                        if (instance.isRootNodeInstance())
                            delcarativeView()->scene()->setSceneRect(item->boundingRect());
                    }

                }
            }

            foreach (const InstancePropertyPair& property, changedPropertyList()) {
                const ServerNodeInstance instance = property.first;
                const QString propertyName = property.second;

                if (instance.isRootNodeInstance() && (propertyName == "width" || propertyName == "height"))
                    adjustSceneRect = true;

                if (propertyName == "width" || propertyName == "height")
                    m_dirtyInstanceSet.insert(instance);
            }

            clearChangedPropertyList();
            resetAllItems();

            if (!m_dirtyInstanceSet.isEmpty() && nodeInstanceClient()->bytesToWrite() < 10000) {
                nodeInstanceClient()->pixmapChanged(createPixmapChangedCommand(m_dirtyInstanceSet.toList()));
                m_dirtyInstanceSet.clear();
            }

            if (adjustSceneRect) {
                QRectF boundingRect = rootNodeInstance().boundingRect();
                if (boundingRect.isValid()) {
                    delcarativeView()->setSceneRect(boundingRect);
                }
            }

            slowDownRenderTimer();
            nodeInstanceClient()->flush();
            nodeInstanceClient()->synchronizeWithClientProcess();
        }

        inFunction = false;
    }
}

} // namespace QmlDesigner
