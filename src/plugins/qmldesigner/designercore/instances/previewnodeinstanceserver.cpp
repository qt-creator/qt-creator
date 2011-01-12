#include "previewnodeinstanceserver.h"
#include "nodeinstanceclientinterface.h"
#include "statepreviewimagechangedcommand.h"

namespace QmlDesigner {

PreviewNodeInstanceServer::PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient) :
    NodeInstanceServer(nodeInstanceClient)
{
    setRenderTimerInterval(50);
    setSlowRenderTimerInterval(10000);
}

void PreviewNodeInstanceServer::createScene(const CreateSceneCommand &command)
{
    initializeDeclarativeView();
    setupScene(command);

    startRenderTimer();
}

void PreviewNodeInstanceServer::findItemChangesAndSendChangeCommands()
{
    static bool inFunction = false;

    if (!inFunction && nodeInstanceClient()->bytesToWrite() < 10000) {
        inFunction = true;
        QVector<ImageContainer> imageContainerVector;
        imageContainerVector.append(ImageContainer(0, renderPreviewImage()));

        foreach (ServerNodeInstance instance,  rootNodeInstance().stateInstances()) {
            instance.activateState();
            imageContainerVector.append(ImageContainer(instance.instanceId(), renderPreviewImage()));
            instance.deactivateState();
        }

        nodeInstanceClient()->statePreviewImagesChanged(StatePreviewImageChangedCommand(imageContainerVector));

        slowDownRenderTimer();
        inFunction = false;
    }
}

void PreviewNodeInstanceServer::changeState(const ChangeStateCommand &/*command*/)
{

}
} // namespace QmlDesigner
