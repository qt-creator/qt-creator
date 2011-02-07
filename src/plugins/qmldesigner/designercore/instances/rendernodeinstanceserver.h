#ifndef RENDERNODEINSTANCESERVER_H
#define RENDERNODEINSTANCESERVER_H

#include "nodeinstanceserver.h"

namespace QmlDesigner {

class RenderNodeInstanceServer : public NodeInstanceServer
{
    Q_OBJECT
public:
    explicit RenderNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

protected:
    void findItemChangesAndSendChangeCommands();

private:
    QSet<ServerNodeInstance> m_dirtyInstanceSet;
};

} // namespace QmlDesigner

#endif // RENDERNODEINSTANCESERVER_H
