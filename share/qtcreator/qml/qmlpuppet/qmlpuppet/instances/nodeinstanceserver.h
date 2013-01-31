/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef NODEINSTANCESERVER_H
#define NODEINSTANCESERVER_H

#include <QUrl>
#include <QVector>
#include <QSet>
#include <QStringList>

#include <nodeinstanceserverinterface.h>
#include "servernodeinstance.h"

#include "debugoutputcommand.h"

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
class QDeclarativeView;
class QSGView;
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
    typedef QPair<QString, QWeakPointer<QObject> > DummyPair;

    explicit NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~NodeInstanceServer();

    void createInstances(const CreateInstancesCommand &command);
    void changeFileUrl(const ChangeFileUrlCommand &command);
    void changePropertyValues(const ChangeValuesCommand &command);
    void changePropertyBindings(const ChangeBindingsCommand &command);
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command);
    void changeIds(const ChangeIdsCommand &command);
    void createScene(const CreateSceneCommand &command);
    void clearScene(const ClearSceneCommand &command);
    void removeInstances(const RemoveInstancesCommand &command);
    void removeProperties(const RemovePropertiesCommand &command);
    void reparentInstances(const ReparentInstancesCommand &command);
    void changeState(const ChangeStateCommand &command);
    void completeComponent(const CompleteComponentCommand &command);
    void changeNodeSource(const ChangeNodeSourceCommand &command);
    void token(const TokenCommand &command);
    void removeSharedMemory(const RemoveSharedMemoryCommand &command);

    ServerNodeInstance instanceForId(qint32 id) const;
    bool hasInstanceForId(qint32 id) const;

    ServerNodeInstance instanceForObject(QObject *object) const;
    bool hasInstanceForObject(QObject *object) const;

    virtual QDeclarativeEngine *engine() const = 0;
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

    virtual QDeclarativeView *declarativeView() const = 0;
    virtual QSGView *sgView() const = 0;

    void sendDebugOutput(DebugOutputCommand::Type type, const QString &message);

public slots:
    void refreshLocalFileProperty(const QString &path);
    void refreshDummyData(const QString &path);
    void emitParentChanged(QObject *child);

protected:
    QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container);
    void reparentInstances(const QVector<ReparentContainer> &containerVector);
    void addImportString(const QString &import);

    Internal::ChildrenChangeEventFilter *childrenChangeEventFilter();
    void resetInstanceProperty(const PropertyAbstractContainer &propertyContainer);
    void setInstancePropertyBinding(const PropertyBindingContainer &bindingContainer);
    void setInstancePropertyVariant(const PropertyValueContainer &valueContainer);
    void setInstanceAuxiliaryData(const PropertyValueContainer &auxiliaryContainer);
    void removeProperties(const QList<PropertyAbstractContainer> &propertyList);

    void insertInstanceRelationship(const ServerNodeInstance &instance);
    void removeInstanceRelationsip(qint32 instanceId);

    NodeInstanceClientInterface *nodeInstanceClient() const;

    void timerEvent(QTimerEvent *);

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

    virtual void initializeView(const QVector<AddImportContainer> &importVector) = 0;
    virtual QList<ServerNodeInstance> setupScene(const CreateSceneCommand &command) = 0;
    void loadDummyDataFiles(const QString& directory);
    void loadDummyDataContext(const QString& directory);
    void loadDummyDataFile(const QFileInfo& fileInfo);
    void loadDummyContextObjectFile(const QFileInfo& fileInfo);
    static QStringList dummyDataDirectories(const QString& directoryPath);

    void setTimerId(int timerId);
    int timerId() const;

    QDeclarativeContext *rootContext() const;


    const QVector<InstancePropertyPair> changedPropertyList() const;
    void clearChangedPropertyList();

    virtual void refreshBindings() = 0;

    void setupDummysForContext(QDeclarativeContext *context);

    void setupFileUrl(const QUrl &fileUrl);
    void setupImports(const QVector<AddImportContainer> &container);
    void setupDummyData(const QUrl &fileUrl);
    void setupDefaultDummyData();
    QList<ServerNodeInstance> setupInstances(const CreateSceneCommand &command);

    QList<QDeclarativeContext*> allSubContextsForObject(QObject *object);
    static QList<QObject*> allSubObjectsForObject(QObject *object);

    virtual void resizeCanvasSizeToRootItemSize() = 0;

private:
    ServerNodeInstance m_rootNodeInstance;
    ServerNodeInstance m_activeStateInstance;
    QHash<qint32, ServerNodeInstance> m_idInstanceHash;
    QHash<QObject*, ServerNodeInstance> m_objectInstanceHash;
    QMultiHash<QString, ObjectPropertyPair> m_fileSystemWatcherHash;
    QList<QPair<QString, QWeakPointer<QObject> > > m_dummyObjectList;
    QWeakPointer<QFileSystemWatcher> m_fileSystemWatcher;
    QWeakPointer<QFileSystemWatcher> m_dummdataFileSystemWatcher;
    QWeakPointer<Internal::ChildrenChangeEventFilter> m_childrenChangeEventFilter;
    QUrl m_fileUrl;
    NodeInstanceClientInterface *m_nodeInstanceClient;
    int m_timer;
    int m_renderTimerInterval;
    bool m_slowRenderTimer;
    int m_slowRenderTimerInterval;
    QVector<InstancePropertyPair> m_changedPropertyList;
    QStringList m_importList;
    QWeakPointer<QObject> m_dummyContextObject;
    QWeakPointer<QDeclarativeComponent> m_importComponent;
    QWeakPointer<QObject> m_importComponentObject;
};

}

#endif // NODEINSTANCESERVER_H
