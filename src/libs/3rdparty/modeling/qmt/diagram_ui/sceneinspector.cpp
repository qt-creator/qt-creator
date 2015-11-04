/***************************************************************************
**
** Copyright (C) 2015 Jochen Becher
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    : QObject(parent),
      m_diagramsManager(0)
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
            if (const IResizable *resizable = dynamic_cast<const IResizable *>(item)) {
                return resizable->minimumSize();
            }
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
            if (IMoveable *moveable = dynamic_cast<IMoveable *>(item)) {
                return moveable;
            }
        }
    }
    QMT_CHECK(false);
    return 0;
}

IResizable *SceneInspector::resizable(const DElement *element, const MDiagram *diagram) const
{
    DiagramSceneModel *diagramSceneModel = m_diagramsManager->diagramSceneModel(diagram);
    QMT_CHECK(diagramSceneModel);
    if (diagramSceneModel) {
        QGraphicsItem *item = diagramSceneModel->graphicsItem(const_cast<DElement *>(element));
        QMT_CHECK(item);
        if (item) {
            if (IResizable *resizeable = dynamic_cast<IResizable *>(item)) {
                return resizeable;
            }
        }
    }
    QMT_CHECK(false);
    return 0;
}

}
