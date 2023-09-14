// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QDebug>
#include <QUrl>
#include <QVector>
#include <QSet>
#include <QStringList>
#include <QPointer>
#include <QImage>

#ifdef MULTILANGUAGE_TRANSLATIONPROVIDER
#include <multilanguagelink.h>
#endif

#include <QTranslator>
#include <memory>

#include <nodeinstanceserverinterface.h>
#include "servernodeinstance.h"
#include "debugoutputcommand.h"
#include "viewconfig.h"

#include <private/qabstractanimation_p.h>
#include <private/qobject_p.h>
#include <private/qquickbehavior_p.h>
#include <private/qquicktext_p.h>
#include <private/qquicktextinput_p.h>
#include <private/qquicktextedit_p.h>
#include <private/qquicktransition_p.h>
#include <private/qquickloader_p.h>

#include <private/qquickanimation_p.h>
#include <private/qqmlmetatype_p.h>
#include <private/qqmltimer_p.h>

namespace QtHelpers {
template <class T>
QList<T>toList(const QSet<T> &set)
{
    return QList<T>(set.begin(), set.end());
}
} // QtHelpers

#ifndef MULTILANGUAGE_TRANSLATIONPROVIDER
namespace MultiLanguage {
inline QByteArray databaseFilePath()
{
    return {};
}

class Translator : public QTranslator
{
public:
    void setLanguage(const QString &) {}
};

class Link
{
public:
    Link()
    {
        if (qEnvironmentVariableIsSet("QT_MULTILANGUAGE_DATABASE"))
            qWarning() << "QT_MULTILANGUAGE_DATABASE is set but QQmlDebugTranslationService is without MULTILANGUAGE_TRANSLATIONPROVIDER support compiled.";
    }
    std::unique_ptr<MultiLanguage::Translator> translator() {
        //should never be called
        Q_ASSERT(false);
        return std::make_unique<MultiLanguage::Translator>();
    }
    const bool isActivated = false;
};
} // namespace MultiLanguage
#endif

QT_BEGIN_NAMESPACE
class QFileSystemWatcher;
class QQmlView;
class QQuickView;
class QQuickWindow;
class QQmlEngine;
class QFileInfo;
class QQmlComponent;
QT_END_NAMESPACE

namespace QmlDesigner {

class NodeInstanceClientInterface;
class ValuesChangedCommand;
class ValuesModifiedCommand;
class PixmapChangedCommand;
class InformationChangedCommand;
class ChildrenChangedCommand;
class ReparentContainer;
class ComponentCompletedCommand;
class AddImportContainer;
class MockupTypeContainer;
class IdContainer;
class ChangeSelectionCommand;

namespace Internal {
    class ChildrenChangeEventFilter;
}

enum class TimerMode { DisableTimer, NormalTimer, SlowTimer };

class NodeInstanceServer : public NodeInstanceServerInterface
{
    Q_OBJECT
public:
    using ObjectPropertyPair = QPair<QPointer<QObject>, PropertyName>;
    using IdPropertyPair = QPair<qint32, QString>;
    using InstancePropertyPair= QPair<ServerNodeInstance, PropertyName>;
    using DummyPair = QPair<QString, QPointer<QObject> >;
    using InstancePropertyValueTriple = struct {
        ServerNodeInstance instance;
        PropertyName propertyName;
        QVariant propertyValue;
    };

    explicit NodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~NodeInstanceServer() override;

    void createInstances(const CreateInstancesCommand &command) override;
    void changeFileUrl(const ChangeFileUrlCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void update3DViewState(const Update3dViewStateCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;
    void reparentInstances(const ReparentInstancesCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void changeNodeSource(const ChangeNodeSourceCommand &command) override;
    void token(const TokenCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void changeSelection(const ChangeSelectionCommand &command) override;
    void inputEvent(const InputEventCommand &command) override;
    void view3DAction(const View3DActionCommand &command) override;
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command) override;
    void changeLanguage(const ChangeLanguageCommand &command) override;
    void changePreviewImageSize(const ChangePreviewImageSizeCommand &command) override;

    ServerNodeInstance instanceForId(qint32 id) const;
    bool hasInstanceForId(qint32 id) const;

    ServerNodeInstance instanceForObject(QObject *object) const;
    bool hasInstanceForObject(QObject *object) const;

    const QVector<ServerNodeInstance> &nodeInstances() const { return m_idInstances; }

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

    QList<ServerNodeInstance> allGroupStateInstances() const;
    QList<ServerNodeInstance> allView3DInstances() const;
    QList<ServerNodeInstance> allCameraInstances() const;

    void notifyPropertyChange(qint32 instanceid, const PropertyName &propertyName);

    QByteArray importCode() const;
    QObject *dummyContextObject() const;

    virtual QQmlView *declarativeView() const = 0;
    virtual QQuickView *quickView() const = 0;
    virtual QQuickWindow *quickWindow() const = 0;
    virtual QQuickItem *rootItem() const = 0;
    virtual void setRootItem(QQuickItem *item) = 0;

    void sendDebugOutput(DebugOutputCommand::Type type, const QString &message, qint32 instanceId = 0);
    void sendDebugOutput(DebugOutputCommand::Type type, const QString &message, const QVector<qint32> &instanceIds);

    void removeInstanceRelationsipForDeletedObject(QObject *object, qint32 instanceId);

    void incrementNeedsExtraRender();
    void decrementNeedsExtraRender();
    void handleExtraRender();

    void disableTimer();

    virtual void collectItemChangesAndSendChangeCommands() = 0;

    virtual void handleInstanceLocked(const ServerNodeInstance &instance, bool enable, bool checkAncestors);
    virtual void handleInstanceHidden(const ServerNodeInstance &instance, bool enable, bool checkAncestors);
    virtual void handlePickTarget(const ServerNodeInstance &instance);

    virtual QImage grabWindow() = 0;
    virtual QImage grabItem(QQuickItem *item) = 0;
    virtual bool renderWindow() = 0;

    virtual bool isInformationServer() const;
    virtual bool isPreviewServer() const;
    void addAnimation(QQuickAbstractAnimation *animation);
    QVector<QQuickAbstractAnimation *> animations() const;
    QVariant animationDefaultValue(int index) const;

public slots:
    void refreshLocalFileProperty(const QString &path);
    void refreshDummyData(const QString &path);
    void emitParentChanged(QObject *child);

protected:
    virtual QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container);
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

    ValuesChangedCommand createValuesChangedCommand(const QList<ServerNodeInstance> &instanceList) const;
    ValuesChangedCommand createValuesChangedCommand(const QVector<InstancePropertyPair> &propertyList) const;
    ValuesModifiedCommand createValuesModifiedCommand(const QVector<InstancePropertyValueTriple> &propertyList) const;
    PixmapChangedCommand createPixmapChangedCommand(const QList<ServerNodeInstance> &instanceList) const;
    InformationChangedCommand createAllInformationChangedCommand(const QList<ServerNodeInstance> &instanceList, bool initial = false) const;
    ChildrenChangedCommand createChildrenChangedCommand(const ServerNodeInstance &parentInstance, const QList<ServerNodeInstance> &instanceList) const;
    ComponentCompletedCommand createComponentCompletedCommand(const QList<ServerNodeInstance> &instanceList);
    ChangeSelectionCommand createChangeSelectionCommand(const QList<ServerNodeInstance> &instanceList);

    void sheduleRootItemRender();

    void addChangedProperty(const InstancePropertyPair &property);

    virtual void startRenderTimer();
    void slowDownRenderTimer();
    void stopRenderTimer();
    void setRenderTimerInterval(int timerInterval);
    int renderTimerInterval() const;
    void setSlowRenderTimerInterval(int timerInterval);

    virtual void initializeView() = 0;
    virtual void initializeAuxiliaryViews();
    virtual void setupScene(const CreateSceneCommand &command) = 0;
    void setTranslationLanguage(const QString &language);
    void loadDummyDataFiles(const QString &directory);
    void loadDummyDataContext(const QString &directory);
    void loadDummyDataFile(const QFileInfo &fileInfo);
    void loadDummyContextObjectFile(const QFileInfo &fileInfo);
    static QStringList dummyDataDirectories(const QString &directoryPath);

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

    virtual void resizeCanvasToRootItem() = 0;
    void setupState(qint32 stateInstanceId);
    void registerFonts(const QUrl &resourceUrl) const;

private:
    void setupOnlyWorkingImports(const QStringList &workingImportStatementList);
    ServerNodeInstance m_rootNodeInstance;
    ServerNodeInstance m_activeStateInstance;
    QVector<ServerNodeInstance> m_idInstances;
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
    TimerMode m_timerMode = TimerMode::NormalTimer;
    int m_timerModeInterval = 200;
    QVector<InstancePropertyPair> m_changedPropertyList;
    QByteArray m_importCode;
    QPointer<QObject> m_dummyContextObject;
    QPointer<QQmlComponent> m_importComponent;
    QPointer<QObject> m_importComponentObject;
    std::unique_ptr<MultiLanguage::Link> multilanguageLink;
    int m_needsExtraRenderCount = 0;
    int m_extraRenderCurrentPass = 0;
    QVector<QQuickAbstractAnimation *> m_animations;
    QVector<QVariant> m_defaultValues;
};

}
