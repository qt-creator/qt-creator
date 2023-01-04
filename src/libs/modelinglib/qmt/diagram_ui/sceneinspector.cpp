// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "sceneinspector.h"

#include "qmt/diagram_scene/diagramsceneconstants.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/diagram_scene/capabilities/moveable.h"
#include "qmt/diagram_scene/capabilities/resizable.h"
#include "qmt/diagram_ui/diagramsmanager.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QSizeF>
#include <QGraphicsItem>

namespace qmt {

SceneInspector::SceneInspector(QObject *parent)
    : QObject(parent)
{
}

SceneInspector::~SceneInspector()
{
}

void SceneInspector::setDiagramsManager(DiagramsManager *diagramsManager)
{
    m_diagramsManager = diagramsManager;
}

QSizeF SceneInspector::rasterSize() const
{
    return QSizeF(RASTER_WIDTH, RASTER_HEIGHT);
}

QSizeF SceneInspector::minimalSize(const DElement *element, const MDiagram *diagram) const
{
    DiagramSceneModel *diagramSceneModel = m_diagramsManager->diagramSceneModel(diagram);
    QMT_CHECK(diagramSceneModel);
    if (diagramSceneModel) {
        const QGraphicsItem *item = diagramSceneModel->graphicsItem(const_cast<DElement *>(element));
        QMT_CHECK(item);
        if (item) {
            if (auto resizable = dynamic_cast<const IResizable *>(item))
                return resizable->minimumSize();
        }
    }
    QMT_CHECK(false);
    return QSizeF();
}

IMoveable *SceneInspector::moveable(const DElement *element, const MDiagram *diagram) const
{
    DiagramSceneModel *diagramSceneModel = m_diagramsManager->diagramSceneModel(diagram);
    QMT_CHECK(diagramSceneModel);
    if (diagramSceneModel) {
        QGraphicsItem *item = diagramSceneModel->graphicsItem(const_cast<DElement *>(element));
        QMT_CHECK(item);
        if (item) {
            if (auto moveable = dynamic_cast<IMoveable *>(item))
                return moveable;
        }
    }
    QMT_CHECK(false);
    return nullptr;
}

IResizable *SceneInspector::resizable(const DElement *element, const MDiagram *diagram) const
{
    DiagramSceneModel *diagramSceneModel = m_diagramsManager->diagramSceneModel(diagram);
    QMT_CHECK(diagramSceneModel);
    if (diagramSceneModel) {
        QGraphicsItem *item = diagramSceneModel->graphicsItem(const_cast<DElement *>(element));
        QMT_CHECK(item);
        if (item) {
            if (auto resizeable = dynamic_cast<IResizable *>(item))
                return resizeable;
        }
    }
    QMT_CHECK(false);
    return nullptr;
}

} // namespace qmt
