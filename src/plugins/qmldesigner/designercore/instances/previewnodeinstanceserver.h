#ifndef PREVIEWNODEINSTANCESERVER_H
#define PREVIEWNODEINSTANCESERVER_H

#include "nodeinstanceserver.h"

namespace QmlDesigner {

class PreviewNodeInstanceServer : public NodeInstanceServer
{
    Q_OBJECT
public:
    explicit PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    void createScene(const CreateSceneCommand &command);
    void changeState(const ChangeStateCommand &command);

protected:
    void findItemChangesAndSendChangeCommands();
    void startRenderTimer();

private:
    ServerNodeInstance m_actualState;

};

} // namespace QmlDesigner

#endif // PREVIEWNODEINSTANCESERVER_H
