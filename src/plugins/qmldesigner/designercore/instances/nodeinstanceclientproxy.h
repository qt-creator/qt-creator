#ifndef NODEINSTANCECLIENTPROXY_H
#define NODEINSTANCECLIENTPROXY_H

#include "nodeinstanceclientinterface.h"

#include <QObject>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServerInterface;

class NodeInstanceClientProxy : public QObject, public NodeInstanceClientInterface
{
    Q_OBJECT

public:
    NodeInstanceClientProxy(QObject *parent = 0);

    void informationChanged(const InformationChangedCommand &command);
    void valuesChanged(const ValuesChangedCommand &command);
    void pixmapChanged(const PixmapChangedCommand &command);
    void childrenChanged(const ChildrenChangedCommand &command);
    void statePreviewImagesChanged(const StatePreviewImageChangedCommand &command);
    void componentCompleted(const ComponentCompletedCommand &command);

    void flush();
    qint64 bytesToWrite() const;

protected:
    void writeSlowCommand(const QVariant &command);
    void writeFastCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command);
    NodeInstanceServerInterface *nodeInstanceServer() const;

private slots:
    void readDataStream();

private:
    QLocalSocket *m_slowSocket;
    QLocalSocket *m_fastSocket;
    NodeInstanceServerInterface *m_nodeinstanceServer;
    quint32 m_blockSize;
};

} // namespace QmlDesigner

#endif // NODEINSTANCECLIENTPROXY_H
