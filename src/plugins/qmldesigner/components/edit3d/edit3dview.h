/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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

#include <abstractview.h>
#include <modelcache.h>

#include <QImage>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <QVector>

QT_BEGIN_NAMESPACE
class QInputEvent;
QT_END_NAMESPACE

namespace QmlDesigner {

class Edit3DWidget;
class Edit3DAction;
class Edit3DCameraAction;
class SeekerSlider;

class QMLDESIGNERCORE_EXPORT Edit3DView : public AbstractView
{
    Q_OBJECT

public:
    Edit3DView(QObject *parent = nullptr);
    ~Edit3DView() override;

    WidgetInfo widgetInfo() override;

    Edit3DWidget *edit3DWidget() const;

    void selectedNodesChanged(const QList<ModelNode> &selectedNodeList, const QList<ModelNode> &lastSelectedNodeList) override;
    void renderImage3DChanged(const QImage &img) override;
    void updateActiveScene3D(const QVariantMap &sceneState) override;
    void modelAttached(Model *model) override;
    void modelAboutToBeDetached(Model *model) override;
    void importsChanged(const QList<Import> &addedImports, const QList<Import> &removedImports) override;
    void customNotification(const AbstractView *view, const QString &identifier, const QList<ModelNode> &nodeList, const QList<QVariant> &data) override;
    void nodeAtPosReady(const ModelNode &modelNode, const QVector3D &pos3d) override;

    void sendInputEvent(QInputEvent *e) const;
    void edit3DViewResized(const QSize &size) const;

    QSize canvasSize() const;

    void createEdit3DActions();
    QVector<Edit3DAction *> leftActions() const;
    QVector<Edit3DAction *> rightActions() const;
    QVector<Edit3DAction *> visibilityToggleActions() const;
    QVector<Edit3DAction *> backgroundColorActions() const;
    void setSeeker(SeekerSlider *slider);

    void addQuick3DImport();
    void startContextMenu(const QPoint &pos);
    void dropMaterial(const ModelNode &matNode, const QPointF &pos);
    void dropBundleMaterial(const QPointF &pos);

private slots:
    void onEntriesChanged();

private:
    enum class NodeAtPosReqType {
        BundleMaterialDrop,
        MaterialDrop,
        ContextMenu,
        None
    };

    void createEdit3DWidget();
    void checkImports();
    void handleEntriesChanged();

    Edit3DAction *createSelectBackgrounColorAction();
    Edit3DAction *createGridColorSelectionAction();
    Edit3DAction *createResetColorAction();
    Edit3DAction *createSyncBackgroundColorAction();

    QPointer<Edit3DWidget> m_edit3DWidget;
    QVector<Edit3DAction *> m_leftActions;
    QVector<Edit3DAction *> m_rightActions;
    QVector<Edit3DAction *> m_visibilityToggleActions;
    QVector<Edit3DAction *> m_backgroundColorActions;
    Edit3DAction *m_selectionModeAction = nullptr;
    Edit3DAction *m_moveToolAction = nullptr;
    Edit3DAction *m_rotateToolAction = nullptr;
    Edit3DAction *m_scaleToolAction = nullptr;
    Edit3DAction *m_fitAction = nullptr;
    Edit3DCameraAction *m_alignCamerasAction = nullptr;
    Edit3DCameraAction *m_alignViewAction = nullptr;
    Edit3DAction *m_cameraModeAction = nullptr;
    Edit3DAction *m_orientationModeAction = nullptr;
    Edit3DAction *m_editLightAction = nullptr;
    Edit3DAction *m_showGridAction = nullptr;
    Edit3DAction *m_showSelectionBoxAction = nullptr;
    Edit3DAction *m_showIconGizmoAction = nullptr;
    Edit3DAction *m_showCameraFrustumAction = nullptr;
    Edit3DAction *m_showParticleEmitterAction = nullptr;
    Edit3DAction *m_resetAction = nullptr;
    Edit3DAction *m_particleViewModeAction = nullptr;
    Edit3DAction *m_particlesPlayAction = nullptr;
    Edit3DAction *m_particlesRestartAction = nullptr;
    Edit3DAction *m_visibilityTogglesAction = nullptr;
    Edit3DAction *m_backgrondColorMenuAction = nullptr;
    SeekerSlider *m_seeker = nullptr;
    int particlemode;
    ModelCache<QImage> m_canvasCache;
    ModelNode m_droppedMaterial;
    NodeAtPosReqType m_nodeAtPosReqType;
    QPoint m_contextMenuPos;
    QTimer m_compressionTimer;
};

} // namespace QmlDesigner
