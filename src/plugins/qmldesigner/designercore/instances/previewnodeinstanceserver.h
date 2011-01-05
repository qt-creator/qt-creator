#ifndef PREVIEWNODEINSTANCESERVER_H
#define PREVIEWNODEINSTANCESERVER_H

#include "nodeinstanceserver.h"

namespace QmlDesigner {

class PreviewNodeInstanceServer : public NodeInstanceServer
{
    Q_OBJECT
public:
    explicit PreviewNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

signals:

public slots:

};

} // namespace QmlDesigner

#endif // PREVIEWNODEINSTANCESERVER_H
