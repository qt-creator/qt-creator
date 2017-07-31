/****************************************************************************
**
** Copyright (C) 2016 Jochen Becher
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
