// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "edit3dactions.h"
#include <itemlibraryentry.h>
#include <qmldesignercomponents_global.h>

#include <abstractview.h>
#include <modelcache.h>
#include <qmlobjectnode.h>

#include <QImage>
#include <QPointer>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <QVector>
#include <QVector3D>

#ifdef Q_OS_MACOS
extern "C" bool AXIsProcessTrusted();
#endif

QT_BEGIN_NAMESPACE
class QAction;
class QInputEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class BakeLights;
class CameraSpeedConfiguration;
class Edit3DWidget;
class SnapConfiguration;

class QMLDESIGNERCOMPONENTS_EXPORT Edit3DView : public AbstractView
{
    Q_OBJECT

public:
    struct ViewportToolState
    {
        int matOverride = 0;
        bool showWireframe = false;
        bool isPerspective = false;
    };

    static bool isQDSTrusted()
    {
#ifdef Q_OS_MACOS
        return AXIsProcessTrusted();
#else
        return true;
#endif
    }

    Edit3DView(ExternalDependenciesInterface &externalDependencies);

    bool hasWidget() const override { return true; }
    WidgetInfo widgetInfo() override;

    Edit3DWidget *edit3DWidget() const;

    void renderImage3DChanged(const QImage &img) override;
    void updateActiveScene3D(const QVariantMap &sceneState) override;
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void importsChanged(const Imports &addedImports, const Imports &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier,
                            const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void nodeAtPosReady(const ModelNode &modelNode, const QVector3D &pos3d) override;
    void nodeReparented(const ModelNode &node, const NodeAbstractProperty &newPropertyParent,
                        const NodeAbstractProperty &oldPropertyParent,
                        PropertyChangeFlags propertyChange) override;
    void nodeRemoved(const ModelNode &removedNode, const NodeAbstractProperty &parentProperty,
                     PropertyChangeFlags propertyChange) override;
    void propertiesRemoved(const QList<AbstractProperty> &propertyList) override;
    void bindingPropertiesChanged(const QList<BindingProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void variantPropertiesChanged(const QList<VariantProperty> &propertyList,
                                  PropertyChangeFlags propertyChange) override;
    void exportedTypeNamesChanged(const ExportedTypeNames &added,
                                  const ExportedTypeNames &removed) override;
    void sendInputEvent(QEvent *e) const;
    void edit3DViewResized(const QSize &size) const;

    QSize canvasSize() const;

    void createEdit3DActions();
    QVector<Edit3DAction *> leftActions() const;
    QVector<Edit3DAction *> rightActions() const;
    QVector<Edit3DAction *> visibilityToggleActions() const;
    QVector<Edit3DAction *> backgroundColorActions() const;
    QVector<Edit3DAction *> viewportPresetActions() const;
    Edit3DAction *edit3DAction(View3DActionType type) const;
    Edit3DBakeLightsAction *bakeLightsAction() const;

    void startContextMenu(const QPoint &pos);
    void showContextMenu();
    void dropMaterial(const ModelNode &matNode, const QPointF &pos);
    void dropBundleMaterial(const QPointF &pos);
    void dropBundleItem(const QPointF &pos);
    void dropTexture(const ModelNode &textureNode, const QPointF &pos);
    void dropComponent(const ItemLibraryEntry &entry, const QPointF &pos);
    void dropAssets(const QList<QUrl> &urls, const QPointF &pos);

    bool isBakingLightsSupported() const;

    void syncSnapAuxPropsToSettings();
    void setCameraSpeedAuxData(double speed, double multiplier);
    void getCameraSpeedAuxData(double &speed, double &multiplier);

    const QList<ViewportToolState> &viewportToolStates() const;
    void setViewportToolState(int viewportIndex, const ViewportToolState &state);

    int activeViewport() const;
    bool isMultiViewportView() const;
    void setFlyMode(bool enabled);
    void emitView3DAction(View3DActionType type, const QVariant &value);

private slots:
    void onEntriesChanged();

private:
    enum class NodeAtPosReqType {
        BundleEffectDrop,
        BundleMaterialDrop,
        ComponentDrop,
        MaterialDrop,
        TextureDrop,
        ContextMenu,
        AssetDrop,
        MainScenePick,
        None
    };

    void registerEdit3DAction(Edit3DAction *action);

    void createEdit3DWidget();
    void checkImports();
    void handleEntriesChanged();
    void showMaterialPropertiesView();
    void updateAlignActionStates();
    void setActive3DSceneId(qint32 sceneId);

    void createSelectBackgroundColorAction(QAction *syncEnvBackgroundAction);
    void createGridColorSelectionAction();
    void createResetColorAction(QAction *syncEnvBackgroundAction);
    void createSyncEnvBackgroundAction();
    void createViewportPresetActions();
    void createSeekerSliderAction();
    void syncCameraSpeedToNewView();
    QmlObjectNode currentSceneEnv();
    void storeCurrentSceneEnvironment();

    void setActiveViewport(int viewportIndex);

    QPoint resolveToolbarPopupPos(Edit3DAction *action) const;

    template<typename T, typename = typename std::enable_if<std::is_base_of<AbstractProperty , T>::value>::type>
    void maybeStoreCurrentSceneEnvironment(const QList<T> &propertyList);

    QPointer<Edit3DWidget> m_edit3DWidget;
    QVector<Edit3DAction *> m_leftActions;
    QVector<Edit3DAction *> m_rightActions;
    QVector<Edit3DAction *> m_visibilityToggleActions;
    QVector<Edit3DAction *> m_backgroundColorActions;
    QVector<Edit3DAction *> m_viewportPresetActions;

    QMap<View3DActionType, Edit3DAction *> m_edit3DActions;
    std::unique_ptr<Edit3DAction> m_selectionModeAction;
    std::unique_ptr<Edit3DAction> m_moveToolAction;
    std::unique_ptr<Edit3DAction> m_rotateToolAction;
    std::unique_ptr<Edit3DAction> m_scaleToolAction;
    std::unique_ptr<Edit3DAction> m_fitAction;
    std::unique_ptr<Edit3DAction> m_alignCamerasAction;
    std::unique_ptr<Edit3DAction> m_alignViewAction;
    std::unique_ptr<Edit3DAction> m_cameraModeAction;
    std::unique_ptr<Edit3DAction> m_orientationModeAction;
    std::unique_ptr<Edit3DAction> m_editLightAction;
    std::unique_ptr<Edit3DAction> m_showGridAction;
    std::unique_ptr<Edit3DAction> m_showLookAtAction;
    std::unique_ptr<Edit3DAction> m_showSelectionBoxAction;
    std::unique_ptr<Edit3DAction> m_showIconGizmoAction;
    std::unique_ptr<Edit3DAction> m_showCameraFrustumAction;
    std::unique_ptr<Edit3DCameraViewAction> m_cameraViewAction;
    std::unique_ptr<Edit3DAction> m_showParticleEmitterAction;
    std::unique_ptr<Edit3DAction> m_particleViewModeAction;
    std::unique_ptr<Edit3DAction> m_particlesPlayAction;
    std::unique_ptr<Edit3DAction> m_particlesRestartAction;
    std::unique_ptr<Edit3DParticleSeekerAction> m_seekerAction;
    std::unique_ptr<Edit3DAction> m_syncEnvBackgroundAction;
    std::unique_ptr<Edit3DAction> m_selectBackgroundColorAction;
    std::unique_ptr<Edit3DAction> m_selectGridColorAction;
    std::unique_ptr<Edit3DAction> m_resetColorAction;

    // Viewport presets actions
    std::unique_ptr<Edit3DAction> m_viewportPresetSingleAction;
    std::unique_ptr<Edit3DAction> m_viewportPresetQuadAction;
    std::unique_ptr<Edit3DAction> m_viewportPreset3Left1RightAction;
    std::unique_ptr<Edit3DAction> m_viewportPreset2HorizontalAction;
    std::unique_ptr<Edit3DAction> m_viewportPreset2VerticalAction;
    std::unique_ptr<Edit3DAction> m_viewportPresetsMenuAction;

    // View3DActionType::Empty actions
    std::unique_ptr<Edit3DAction> m_resetAction;
    std::unique_ptr<Edit3DAction> m_visibilityTogglesAction;
    std::unique_ptr<Edit3DAction> m_backgroundColorMenuAction;
    std::unique_ptr<Edit3DAction> m_snapToggleAction;
    std::unique_ptr<Edit3DAction> m_snapConfigAction;
    std::unique_ptr<Edit3DIndicatorButtonAction> m_cameraSpeedConfigAction;
    std::unique_ptr<Edit3DBakeLightsAction> m_bakeLightsAction;

    int particlemode;
    ModelCache<QImage> m_canvasCache;
    ModelNode m_droppedModelNode;
    ItemLibraryEntry m_droppedEntry;
    QStringList m_dropped3dImports;
    QString m_droppedTexture;
    NodeAtPosReqType m_nodeAtPosReqType;
    QPoint m_contextMenuPosMouse;
    QVector3D m_contextMenuPos3D;
    QTimer m_compressionTimer;
    QPointer<BakeLights> m_bakeLights;
    bool m_isBakingLightsSupported = false;
    QPointer<SnapConfiguration> m_snapConfiguration;
    QPointer<CameraSpeedConfiguration> m_cameraSpeedConfiguration;
    int m_activeViewport = 0;

    QList<ViewportToolState> m_viewportToolStates;
    ModelNode m_contextMenuPendingNode;
    ModelNode m_pickView3dNode;

    double m_previousCameraSpeed = -1.;
    double m_previousCameraMultiplier = -1.;
    QString m_currProjectPath;

    friend class Edit3DAction;
};

} // namespace QmlDesigner
