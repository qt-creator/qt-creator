// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "edit3dactions.h"
#include "itemlibraryinfo.h"
#include <qmldesignercomponents_global.h>

#include <abstractview.h>
#include <modelcache.h>

#include <QImage>
#include <QPointer>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <QVector>

QT_BEGIN_NAMESPACE
class QAction;
class QInputEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class BakeLights;
class Edit3DWidget;
class SnapConfiguration;

class QMLDESIGNERCOMPONENTS_EXPORT Edit3DView : public AbstractView
{
    Q_OBJECT

public:
    Edit3DView(ExternalDependenciesInterface &externalDependencies);

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

    void sendInputEvent(QInputEvent *e) const;
    void edit3DViewResized(const QSize &size) const;

    QSize canvasSize() const;

    void createEdit3DActions();
    QVector<Edit3DAction *> leftActions() const;
    QVector<Edit3DAction *> rightActions() const;
    QVector<Edit3DAction *> visibilityToggleActions() const;
    QVector<Edit3DAction *> backgroundColorActions() const;
    Edit3DAction *edit3DAction(View3DActionType type) const;
    Edit3DBakeLightsAction *bakeLightsAction() const;

    void addQuick3DImport();
    void startContextMenu(const QPoint &pos);
    void dropMaterial(const ModelNode &matNode, const QPointF &pos);
    void dropBundleMaterial(const QPointF &pos);
    void dropBundleEffect(const QPointF &pos);
    void dropTexture(const ModelNode &textureNode, const QPointF &pos);
    void dropComponent(const ItemLibraryEntry &entry, const QPointF &pos);
    void dropAsset(const QString &file, const QPointF &pos);

    bool isBakingLightsSupported() const;

    void syncSnapAuxPropsToSettings();

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
        None
    };

    void registerEdit3DAction(Edit3DAction *action);

    void createEdit3DWidget();
    void checkImports();
    void handleEntriesChanged();
    void showMaterialPropertiesView();
    void updateAlignActionStates();

    void createSelectBackgroundColorAction(QAction *syncEnvBackgroundAction);
    void createGridColorSelectionAction();
    void createResetColorAction(QAction *syncEnvBackgroundAction);
    void createSyncEnvBackgroundAction();
    void createSeekerSliderAction();

    QPoint resolveToolbarPopupPos(Edit3DAction *action) const;

    QPointer<Edit3DWidget> m_edit3DWidget;
    QVector<Edit3DAction *> m_leftActions;
    QVector<Edit3DAction *> m_rightActions;
    QVector<Edit3DAction *> m_visibilityToggleActions;
    QVector<Edit3DAction *> m_backgroundColorActions;

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
    std::unique_ptr<Edit3DAction> m_showSelectionBoxAction;
    std::unique_ptr<Edit3DAction> m_showIconGizmoAction;
    std::unique_ptr<Edit3DAction> m_showCameraFrustumAction;
    std::unique_ptr<Edit3DAction> m_showParticleEmitterAction;
    std::unique_ptr<Edit3DAction> m_particleViewModeAction;
    std::unique_ptr<Edit3DAction> m_particlesPlayAction;
    std::unique_ptr<Edit3DAction> m_particlesRestartAction;
    std::unique_ptr<Edit3DParticleSeekerAction> m_seekerAction;
    std::unique_ptr<Edit3DAction> m_syncEnvBackgroundAction;
    std::unique_ptr<Edit3DAction> m_selectBackgroundColorAction;
    std::unique_ptr<Edit3DAction> m_selectGridColorAction;
    std::unique_ptr<Edit3DAction> m_resetColorAction;

    // View3DActionType::Empty actions
    std::unique_ptr<Edit3DAction> m_resetAction;
    std::unique_ptr<Edit3DAction> m_visibilityTogglesAction;
    std::unique_ptr<Edit3DAction> m_backgroundColorMenuAction;
    std::unique_ptr<Edit3DAction> m_snapToggleAction;
    std::unique_ptr<Edit3DAction> m_snapConfigAction;
    std::unique_ptr<Edit3DBakeLightsAction> m_bakeLightsAction;

    int particlemode;
    ModelCache<QImage> m_canvasCache;
    ModelNode m_droppedModelNode;
    ItemLibraryEntry m_droppedEntry;
    QString m_droppedFile;
    NodeAtPosReqType m_nodeAtPosReqType;
    QPoint m_contextMenuPos;
    QTimer m_compressionTimer;
    QPointer<BakeLights> m_bakeLights;
    bool m_isBakingLightsSupported = false;
    QPointer<SnapConfiguration> m_snapConfiguration;

    friend class Edit3DAction;
};

} // namespace QmlDesigner
