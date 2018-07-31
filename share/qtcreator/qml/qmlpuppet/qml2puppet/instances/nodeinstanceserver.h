/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <QUrl>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QPointer>

#include <nodeinstanceserverinterface.h>
#include "servernodeinstance.h"
#include "debugoutputcommand.h"

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
class QQmlView;
class QQuickView;
class QQmlEngine;
class QFileInfo;
class QQmlComponent;
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
class MockupTypeContainer;
class IdContainer;

namespace Internal {
    class ChildrenChangeEventFilter;
}

class NodeInstanceServer : public NodeInstanceServerInterface
{
    Q_OBJECT
public:
    using ObjectPropertyPair = QPair<QPointer<QObject>, PropertyName>;
    using IdPropertyPair = QPair<qint32, QString>;
    using InstancePropertyPair= QPair<ServerNodeInstance, PropertyName>;
    using DummyPair = QPair<QString, QPointer<QObject> >;


    explicit NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);

    void createInstances(const CreateInstancesCommand &command) override;
    void changeFileUrl(const ChangeFileUrlCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void changeNodeSource(const ChangeNodeSourceCommand &command) override;
    void token(const TokenCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;

    ServerNodeInstance instanceForId(qint32 id) const;
    bool hasInstanceForId(qint32 id) const;

    ServerNodeInstance instanceForObject(QObject *object) const;
    bool hasInstanceForObject(QObject *object) const;

    virtual QQmlEngine *engine() const = 0;
    QQmlContext *context() const;

    void removeAllInstanceRelationships();

    QFileSystemWatcher *fileSystemWatcher();
    QFileSystemWatcher *dummydataFileSystemWatcher();
    Internal::ChildrenChangeEventFilter *childrenChangeEventFilter() const;
    void addFilePropertyToFileSystemWatcher(QObject *object, const PropertyName &propertyName, const QString &path);
    void removeFilePropertyFromFileSystemWatcher(QObject *object, const PropertyName &propertyName, const QString &path);

    QUrl fileUrl() const;

    ServerNodeInstance activeStateInstance() const;
    void setStateInstance(const ServerNodeInstance &stateInstance);
    void clearStateInstance();

    ServerNodeInstance rootNodeInstance() const;

    void notifyPropertyChange(qint32 instanceid, const PropertyName &propertyName);

    QByteArray importCode() const;
    QObject *dummyContextObject() const;

    virtual QQmlView *declarativeView() const = 0;
    virtual QQuickView *quickView() const = 0;

    void sendDebugOutput(DebugOutputCommand::Type type, const QString &message, qint32 instanceId = 0);
    void sendDebugOutput(DebugOutputCommand::Type type, const QString &message, const QVector<qint32> &instanceIds);

    void removeInstanceRelationsipForDeletedObject(QObject *object);
public slots:
    void refreshLocalFileProperty(const QString &path);
    void refreshDummyData(const QString &path);
    void emitParentChanged(QObject *child);

protected:
    QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container);
    void reparentInstances(const QVector<ReparentContainer> &containerVector);

    Internal::ChildrenChangeEventFilter *childrenChangeEventFilter();
    void resetInstanceProperty(const PropertyAbstractContainer &propertyContainer);
    void setInstancePropertyBinding(const PropertyBindingContainer &bindingContainer);
    void setInstancePropertyVariant(const PropertyValueContainer &valueContainer);
    void setInstanceAuxiliaryData(const PropertyValueContainer &auxiliaryContainer);
    void removeProperties(const QList<PropertyAbstractContainer> &propertyList);

    void insertInstanceRelationship(const ServerNodeInstance &instance);
    void removeInstanceRelationsip(qint32 instanceId);

    NodeInstanceClientInterface *nodeInstanceClient() const;

    void timerEvent(QTimerEvent *) override;

    virtual void collectItemChangesAndSendChangeCommands() = 0;

    ValuesChangedCommand createValuesChangedCommand(const QList<ServerNodeInstance> &instanceList) const;
    ValuesChangedCommand createValuesChangedCommand(const QVector<InstancePropertyPair> &propertyList) const;
    PixmapChangedCommand createPixmapChangedCommand(const QList<ServerNodeInstance> &instanceList) const;
    InformationChangedCommand createAllInformationChangedCommand(const QList<ServerNodeInstance> &instanceList, bool initial = false) const;
    ChildrenChangedCommand createChildrenChangedCommand(const ServerNodeInstance &parentInstance, const QList<ServerNodeInstance> &instanceList) const;
    ComponentCompletedCommand createComponentCompletedCommand(const QList<ServerNodeInstance> &instanceList);

    void addChangedProperty(const InstancePropertyPair &property);

    virtual void startRenderTimer();
    void slowDownRenderTimer();
    void stopRenderTimer();
    void setRenderTimerInterval(int timerInterval);
    int renderTimerInterval() const;
    void setSlowRenderTimerInterval(int timerInterval);

    virtual void initializeView() = 0;
    virtual void setupScene(const CreateSceneCommand &command) = 0;
    void loadDummyDataFiles(const QString& directory);
    void loadDummyDataContext(const QString& directory);
    void loadDummyDataFile(const QFileInfo& fileInfo);
    void loadDummyContextObjectFile(const QFileInfo& fileInfo);
    static QStringList dummyDataDirectories(const QString& directoryPath);

    void setTimerId(int timerId);
    int timerId() const;

    QQmlContext *rootContext() const;


    const QVector<InstancePropertyPair> changedPropertyList() const;
    void clearChangedPropertyList();

    virtual void refreshBindings() = 0;

    void setupDummysForContext(QQmlContext *context);

    void setupMockupTypes(const QVector<MockupTypeContainer> &container);
    void setupFileUrl(const QUrl &fileUrl);
    void setupImports(const QVector<AddImportContainer> &container);
    void setupDummyData(const QUrl &fileUrl);
    void setupDefaultDummyData();
    QList<ServerNodeInstance> setupInstances(const CreateSceneCommand &command);

    QList<QQmlContext*> allSubContextsForObject(QObject *object);
    static QList<QObject*> allSubObjectsForObject(QObject *object);

    virtual void resizeCanvasSizeToRootItemSize() = 0;

private:
    void setupOnlyWorkingImports(const QStringList &workingImportStatementList);
    ServerNodeInstance m_rootNodeInstance;
    ServerNodeInstance m_activeStateInstance;
    QHash<qint32, ServerNodeInstance> m_idInstanceHash;
    QHash<QObject*, ServerNodeInstance> m_objectInstanceHash;
    QMultiHash<QString, ObjectPropertyPair> m_fileSystemWatcherHash;
    QList<QPair<QString, QPointer<QObject> > > m_dummyObjectList;
    QPointer<QFileSystemWatcher> m_fileSystemWatcher;
    QPointer<QFileSystemWatcher> m_dummdataFileSystemWatcher;
    QPointer<Internal::ChildrenChangeEventFilter> m_childrenChangeEventFilter;
    QUrl m_fileUrl;
    NodeInstanceClientInterface *m_nodeInstanceClient;
    int m_timer = 0;
    int m_renderTimerInterval = 16;
    bool m_slowRenderTimer = false;
    int m_slowRenderTimerInterval = 200;
    QVector<InstancePropertyPair> m_changedPropertyList;
    QByteArray m_importCode;
    QPointer<QObject> m_dummyContextObject;
    QPointer<QQmlComponent> m_importComponent;
    QPointer<QObject> m_importComponentObject;
};

}
