// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "qt5nodeinstanceserver.h"
#include "tokencommand.h"
#include "valueschangedcommand.h"
#include "changeselectioncommand.h"
#include "requestmodelnodepreviewimagecommand.h"
#include "propertybindingcontainer.h"
#include "propertyabstractcontainer.h"
#include "animationdriver.h"

#ifdef QUICK3D_PARTICLES_MODULE
#include <QtQuick3DParticles/private/qquick3dparticlesystem_p.h>
#endif

#include <QTimer>
#include <QElapsedTimer>
#include <QVariant>
#include <QPointer>
#include <QImage>
#include <QUrl>

QT_BEGIN_NAMESPACE
class QDragMoveEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class Qt5InformationNodeInstanceServer : public Qt5NodeInstanceServer
{
    Q_OBJECT

public:
    explicit Qt5InformationNodeInstanceServer(NodeInstanceClientInterface *nodeInstanceClient);
    ~Qt5InformationNodeInstanceServer() override;

    void reparentInstances(const ReparentInstancesCommand &command) override;
    void clearScene(const ClearSceneCommand &command) override;
    void update3DViewState(const Update3dViewStateCommand &command) override;
    void createScene(const CreateSceneCommand &command) override;
    void completeComponent(const CompleteComponentCommand &command) override;
    void token(const TokenCommand &command) override;
    void removeSharedMemory(const RemoveSharedMemoryCommand &command) override;
    void changeSelection(const ChangeSelectionCommand &command) override;
    void changePropertyValues(const ChangeValuesCommand &command) override;
    void removeInstances(const RemoveInstancesCommand &command) override;
    void inputEvent(const InputEventCommand &command) override;
    void view3DAction(const View3DActionCommand &command) override;
    void requestModelNodePreviewImage(const RequestModelNodePreviewImageCommand &command) override;
    void changeAuxiliaryValues(const ChangeAuxiliaryCommand &command) override;
    void changePropertyBindings(const ChangeBindingsCommand &command) override;
    void changeIds(const ChangeIdsCommand &command) override;
    void changeState(const ChangeStateCommand &command) override;
    void removeProperties(const RemovePropertiesCommand &command) override;

    void handleInstanceLocked(const ServerNodeInstance &instance, bool enable, bool checkAncestors) override;
    void handleInstanceHidden(const ServerNodeInstance &instance, bool enable, bool checkAncestors) override;
    void handlePickTarget(const ServerNodeInstance &instance) override;

    bool isInformationServer() const override;
    void handleDynamicAddObject();

private slots:
    void handleSelectionChanged(const QVariant &objs);
    void handleObjectPropertyCommit(const QVariant &objects, const QVariant &propNames);
    void handleObjectPropertyChange(const QVariant &objects, const QVariant &propNames);
    void handleActiveSceneChange();
    void handleToolStateChanged(const QString &sceneId, const QString &tool,
                                const QVariant &toolState);
    void handleView3DSizeChange();
    void handleView3DDestroyed(QObject *obj);
    void handleNode3DDestroyed(QObject *obj);

protected:
    void collectItemChangesAndSendChangeCommands() override;
    void sendChildrenChangedCommand(const QList<ServerNodeInstance> &childList);
    void sendTokenBack();
    bool isDirtyRecursiveForNonInstanceItems(QQuickItem *item) const;
    bool isDirtyRecursiveForParentInstances(QQuickItem *item) const;
    void selectInstances(const QList<ServerNodeInstance> &instanceList);
    void modifyProperties(const QVector<InstancePropertyValueTriple> &properties);
    QList<ServerNodeInstance> createInstances(const QVector<InstanceContainer> &container) override;
    void initializeAuxiliaryViews() override;

private:
    void handleObjectPropertyChangeTimeout();
    void handleSelectionChangeTimeout();
    void handleDynamicAddObjectTimeout();
    void createEditView3D();
    void create3DPreviewView();
    void setup3DEditView(const QList<ServerNodeInstance> &instanceList,
                         const CreateSceneCommand &command);
    void createCameraAndLightGizmos(const QList<ServerNodeInstance> &instanceList) const;
    void add3DViewPorts(const QList<ServerNodeInstance> &instanceList);
    void add3DScenes(const QList<ServerNodeInstance> &instanceList);
    QObject *findView3DForInstance(const ServerNodeInstance &instance) const;
    QObject *findView3DForSceneRoot(QObject *sceneRoot) const;
    QObject *find3DSceneRoot(const ServerNodeInstance &instance) const;
    QObject *find3DSceneRoot(QObject *obj) const;
    QVector<InstancePropertyValueTriple> propertyToPropertyValueTriples(
            const ServerNodeInstance &instance,
            const PropertyName &propertyName,
            const QVariant &variant);
    void modifyVariantValue(const QObjectList &objects,
                            const QList<PropertyName> &propNames,
                            ValuesModifiedCommand::TransactionOption option);
    void updateView3DRect(QObject *view3D);
    void updateActiveSceneToEditView3D(bool timerCall = false);
    void removeNode3D(QObject *node);
    void resolveSceneRoots();
    ServerNodeInstance active3DSceneInstance() const;
    void updateNodesRecursive(QQuickItem *item);
    QQuickItem *getContentItemForRendering(QQuickItem *rootItem);
    void render3DEditView(int count = 1);
    void doRender3DEditView();
    void renderModelNodeImageView();
    void doRenderModelNodeImageView();
    void doRenderModelNode3DImageView(const RequestModelNodePreviewImageCommand &cmd);
    void doRenderModelNode2DImageView(const RequestModelNodePreviewImageCommand &cmd);
    void updateLockedAndHiddenStates(const QSet<ServerNodeInstance> &instances);
    void handleInputEvents();
    void resolveImportSupport();
    void updateMaterialPreviewData(const QVector<PropertyValueContainer> &valueChanges);
    void updateRotationBlocks(const QVector<PropertyValueContainer> &valueChanges);
    void updateSnapSettings(const QVector<PropertyValueContainer> &valueChanges);
    void updateColorSettings(const QVector<PropertyValueContainer> &valueChanges);
    void removeRotationBlocks(const QVector<qint32> &instanceIds);
    void getNodeAtPos(const QPointF &pos);

    void createAuxiliaryQuickView(const QUrl &url, RenderViewData &viewData);
#ifdef QUICK3D_PARTICLES_MODULE
    void handleParticleSystemSelected(QQuick3DParticleSystem* targetParticleSystem);
    void resetParticleSystem();
    void handleParticleSystemDeselected();
#endif
    void setSceneEnvironmentData(qint32 instanceId);
    QVariantList alignCameraList() const;
    void updateSceneEnvToHelper();
    bool isSceneEnvironmentBgProperty(const PropertyName &name) const;

    RenderViewData m_editView3DData;
    RenderViewData m_modelNode3DImageViewData;
    RenderViewData m_modelNode2DImageViewData;

    bool m_editView3DSetupDone = false;
    QSet<RequestModelNodePreviewImageCommand> m_modelNodePreviewImageCommands;
    QHash<QString, QImage> m_modelNodePreviewImageCache;
    QSet<QObject *> m_view3Ds;
    QMultiHash<QObject *, QObject *> m_3DSceneMap; // key: scene root, value: node
    QObject *m_active3DView = nullptr;
    QList<QObject *> m_priorityView3DsToRender;
    QObject *m_active3DScene = nullptr;
    QSet<ServerNodeInstance> m_parentChangedSet;
    QList<ServerNodeInstance> m_completedComponentList;
    QList<TokenCommand> m_tokenList;
    QTimer m_propertyChangeTimer;
    QTimer m_selectionChangeTimer;
    QTimer m_render3DEditViewTimer;
    QTimer m_renderModelNodeImageViewTimer;
    QTimer m_inputEventTimer;
    QTimer m_dynamicAddObjectTimer;
    QTimer m_activeSceneIdUpdateTimer;
#ifdef QUICK3D_PARTICLES_MODULE
    bool m_particleAnimationPlaying = true;
    AnimationDriver *m_particleAnimationDriver = nullptr;
    QMetaObject::Connection m_particleAnimationConnection;
    QQuick3DParticleSystem* m_targetParticleSystem = nullptr;
#endif
    QObjectList m_changedNodes;
    QList<PropertyName> m_changedProperties;
    ChangeSelectionCommand m_lastSelectionChangeCommand;
    QList<InputEventCommand> m_pendingInputEventCommands;
    QObject *m_3dHelper = nullptr;
    int m_need3DEditViewRender = 0;
    QSet<QObject *> m_dynamicObjectConstructors;

    // Current or previous camera selections for each scene
    QHash<QObject *, QObjectList> m_selectedCameras; // key: scene root, value: camera node

    struct PreviewData {
        QString env;
        QString envValue;
        QString model;
    };
    PreviewData m_materialPreviewData;
};

} // namespace QmlDesigner
