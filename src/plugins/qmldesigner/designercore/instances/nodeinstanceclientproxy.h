#ifndef NODEINSTANCECLIENTPROXY_H
#define NODEINSTANCECLIENTPROXY_H

#include "nodeinstanceclientinterface.h"

#include <QObject>
#include <QHash>
#include <QWeakPointer>

QT_BEGIN_NAMESPACE
class QLocalSocket;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceServerInterface;
class CreateSceneCommand;
class CreateInstancesCommand;
class ClearSceneCommand;
class ReparentInstancesCommand;
class ChangeFileUrlCommand;
class ChangeValuesCommand;
class ChangeBindingsCommand;
class ChangeIdsCommand;
class RemoveInstancesCommand;
class RemovePropertiesCommand;
class AddImportCommand;
class CompleteComponentCommand;
class ChangeStateCommand;

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
    void writeCommand(const QVariant &command);
    void dispatchCommand(const QVariant &command);
    NodeInstanceServerInterface *nodeInstanceServer() const;

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void changeState(const ChangeStateCommand &command);
    void addImport(const AddImportCommand &command);
    void completeComponent(const CompleteComponentCommand &command);

private slots:
    void readDataStream();

private:
    QLocalSocket *m_socket;
    NodeInstanceServerInterface *m_nodeInstanceServer;
    quint32 m_blockSize;
};

} // namespace QmlDesigner

#endif // NODEINSTANCECLIENTPROXY_H
