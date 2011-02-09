#ifndef NODEINSTANCESERVER_H
#define NODEINSTANCESERVER_H

#include <QUrl>
#include <QVector>
#include <QSet>
#include <QStringList>

#include <nodeinstanceserverinterface.h>
#include "servernodeinstance.h"

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
class QDeclarativeView;
class QDeclarativeEngine;
class QGraphicsObject;
class QFileInfo;
class QDeclarativeComponent;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceClientInterface;
class ValuesChangedCommand;
class PixmapChangedCommand;
class InformationChangedCommand;
class ChildrenChangedCommand;
class ReparentContainer;
class ComponentCompletedCommand;
class AddImportContainer;

namespace Internal {
    class ChildrenChangeEventFilter;
}

class NodeInstanceServer : public NodeInstanceServerInterface
{
    Q_OBJECT
public:
    typedef QPair<QWeakPointer<QObject>, QString>  ObjectPropertyPair;
    typedef QPair<qint32, QString>  IdPropertyPair;
    typedef QPair<ServerNodeInstance, QString>  InstancePropertyPair;
    explicit NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~NodeInstanceServer();

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeState(const ChangeStateCommand &command);
    void addImport(const AddImportCommand &command);
    void completeComponent(const CompleteComponentCommand &command);

    ServerNodeInstance instanceForId(qint32 id) const;
    bool hasInstanceForId(qint32 id) const;

    ServerNodeInstance instanceForObject(QObject *object) const;
    bool hasInstanceForObject(QObject *object) const;

    QDeclarativeEngine *engine() const;
    QDeclarativeContext *context() const;

    void removeAllInstanceRelationships();

    QFileSystemWatcher *fileSystemWatcher();
    QFileSystemWatcher *dummydataFileSystemWatcher();
    Internal::ChildrenChangeEventFilter *childrenChangeEventFilter() const;
    void addFilePropertyToFileSystemWatcher(QObject *object, const QString &propertyName, const QString &path);
    void removeFilePropertyFromFileSystemWatcher(QObject *object, const QString &propertyName, const QString &path);

    QUrl fileUrl() const;

    ServerNodeInstance activeStateInstance() const;
    void setStateInstance(const ServerNodeInstance &stateInstance);
    void clearStateInstance();

    ServerNodeInstance rootNodeInstance() const;

    void notifyPropertyChange(qint32 instanceid, const QString &propertyName);

    QStringList imports() const;
    QObject *dummyContextObject() const;

public slots:
    void refreshLocalFileProperty(const QString &path);
    void refreshDummyData(const QString &path);
    void emitParentChanged(QObject *child);

protected:
    QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container);
    void reparentInstances(const QVector<ReparentContainer> &containerVector);
    void addImports(const QVector<AddImportContainer> &container);

    Internal::ChildrenChangeEventFilter *childrenChangeEventFilter();
    void resetInstanceProperty(const PropertyAbstractContainer &propertyContainer);
    void setInstancePropertyBinding(const PropertyBindingContainer &bindingContainer);
    void setInstancePropertyVariant(const PropertyValueContainer &valueContainer);
    void removeProperties(const QList<PropertyAbstractContainer> &propertyList);

    void insertInstanceRelationship(const ServerNodeInstance &instance);
    void removeInstanceRelationsip(qint32 instanceId);

    NodeInstanceClientInterface *nodeInstanceClient() const;

    void timerEvent(QTimerEvent *);

    bool nonInstanceChildIsDirty(QGraphicsObject *graphicsObject) const;
    virtual void findItemChangesAndSendChangeCommands();
    void resetAllItems();

    ValuesChangedCommand createValuesChangedCommand(const QList<ServerNodeInstance> &instanceList) const;
    ValuesChangedCommand createValuesChangedCommand(const QVector<InstancePropertyPair> &propertyList) const;
    PixmapChangedCommand createPixmapChangedCommand(const QList<ServerNodeInstance> &instanceList) const;
    InformationChangedCommand createAllInformationChangedCommand(const QList<ServerNodeInstance> &instanceList, bool initial = false) const;
    ChildrenChangedCommand createChildrenChangedCommand(const ServerNodeInstance &parentInstance, const QList<ServerNodeInstance> &instanceList) const;
    ComponentCompletedCommand createComponentCompletedCommand(const QList<ServerNodeInstance> &instanceList);
    void sendChildrenChangedCommand(const QList<ServerNodeInstance> childList);

    void addChangedProperty(const InstancePropertyPair &property);

    virtual void startRenderTimer();
    void slowDownRenderTimer();
    void stopRenderTimer();
    void setRenderTimerInterval(int timerInterval);
    int renderTimerInterval() const;
    void setSlowRenderTimerInterval(int timerInterval);

    void initializeDeclarativeView();
    QList<ServerNodeInstance> setupScene(const CreateSceneCommand &command);
    void loadDummyDataFiles(const QString& directory);
    void loadDummyDataFile(const QFileInfo& fileInfo);
    void loadDummyContextObjectFile(const QFileInfo& fileInfo);
    QImage renderPreviewImage();

    void setTimerId(int timerId);
    int timerId() const;

    QDeclarativeView *delcarativeView() const;
    const QVector<InstancePropertyPair> changedPropertyList() const;
    void clearChangedPropertyList();

    void refreshBindings();

private:
    ServerNodeInstance m_rootNodeInstance;
    ServerNodeInstance m_activeStateInstance;
    QHash<qint32, ServerNodeInstance> m_idInstanceHash;
    QHash<QObject*, ServerNodeInstance> m_objectInstanceHash;
    QMultiHash<QString, ObjectPropertyPair> m_fileSystemWatcherHash;
    QWeakPointer<QFileSystemWatcher> m_fileSystemWatcher;
    QWeakPointer<QFileSystemWatcher> m_dummdataFileSystemWatcher;
    QWeakPointer<QDeclarativeView> m_declarativeView;
    QWeakPointer<Internal::ChildrenChangeEventFilter> m_childrenChangeEventFilter;
    QUrl m_fileUrl;
    NodeInstanceClientInterface *m_nodeInstanceClient;
    int m_timer;
    int m_renderTimerInterval;
    bool m_slowRenderTimer;
    int m_slowRenderTimerInterval;
    QVector<InstancePropertyPair> m_changedPropertyList;
    QStringList m_importList;
    QList<ServerNodeInstance> m_completedComponentList;
    QWeakPointer<QObject> m_dummyContextObject;
    QWeakPointer<QDeclarativeComponent> m_importComponent;
    QWeakPointer<QObject> m_importComponentObject;
};

}

#endif // NODEINSTANCESERVER_H
