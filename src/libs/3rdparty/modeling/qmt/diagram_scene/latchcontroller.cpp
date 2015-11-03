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

#include "latchcontroller.h"

#include "diagramsceneconstants.h"
#include "diagramscenemodel.h"
#include "parts/alignlineitem.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>

namespace qmt {

LatchController::LatchController(QObject *parent)
    : QObject(parent),
      m_diagramSceneModel(0),
      m_horizontalAlignLine(new AlignLineItem(AlignLineItem::HORIZONTAL, 0)),
      m_verticalAlignLine(new AlignLineItem(AlignLineItem::VERTICAL, 0)),
      m_foundHorizontalLatch(false),
      m_horizontalDist(0.0),
      m_foundVerticalLatch(false),
      m_verticalDist(0.0)
{
    m_horizontalAlignLine->setZValue(LATCH_LINES_ZVALUE);
    m_horizontalAlignLine->setVisible(false);

    m_verticalAlignLine->setZValue(LATCH_LINES_ZVALUE);
    m_verticalAlignLine->setVisible(false);
}

LatchController::~LatchController()
{
    if (m_verticalAlignLine->scene()) {
        m_verticalAlignLine->scene()->removeItem(m_verticalAlignLine);
    }
    delete m_verticalAlignLine;
    if (m_horizontalAlignLine->scene()) {
        m_horizontalAlignLine->scene()->removeItem(m_horizontalAlignLine);
    }
    delete m_horizontalAlignLine;
}

void LatchController::setDiagramSceneModel(DiagramSceneModel *diagram_scene_model)
{
    m_diagramSceneModel = diagram_scene_model;
}

void LatchController::addToGraphicsScene(QGraphicsScene *graphics_scene)
{
    QMT_CHECK(graphics_scene);
    graphics_scene->addItem(m_horizontalAlignLine);
    graphics_scene->addItem(m_verticalAlignLine);
}

void LatchController::removeFromGraphicsScene(QGraphicsScene *graphics_scene)
{
    Q_UNUSED(graphics_scene); // avoid warning in release mode

    if (m_verticalAlignLine->scene()) {
        QMT_CHECK(graphics_scene == m_verticalAlignLine->scene());
        m_verticalAlignLine->scene()->removeItem(m_verticalAlignLine);
    }
    if (m_horizontalAlignLine->scene()) {
        QMT_CHECK(graphics_scene == m_horizontalAlignLine->scene());
        m_horizontalAlignLine->scene()->removeItem(m_horizontalAlignLine);
    }
}

void LatchController::mousePressEventLatching(QGraphicsSceneMouseEvent *event)
{
    // TODO add keyboard event handler to handle latching also on modified change without move
    mouseMoveEventLatching(event);
}

void LatchController::mouseMoveEventLatching(QGraphicsSceneMouseEvent *event)
{
    m_foundHorizontalLatch = false;
    m_foundVerticalLatch = false;

    if (!(event->modifiers() & Qt::ShiftModifier)) {
        m_horizontalAlignLine->setVisible(false);
        m_verticalAlignLine->setVisible(false);
        return;
    }

    if (!m_diagramSceneModel->getFocusItem()) {
        return;
    }

    ILatchable *palped_latchable = dynamic_cast<ILatchable *>(m_diagramSceneModel->getFocusItem());
    if (!palped_latchable) {
        return;
    }

    ILatchable::Action horiz_action = ILatchable::MOVE;
    ILatchable::Action vert_action = ILatchable::MOVE;

    QList<ILatchable::Latch> palped_horizontals = palped_latchable->getHorizontalLatches(horiz_action, true);
    QList<ILatchable::Latch> palped_verticals = palped_latchable->getVerticalLatches(vert_action, true);

    qreal horiz_min_dist = 10.0;
    ILatchable::Latch best_horiz_latch;
    bool found_best_horiz = false;
    qreal vert_min_dist = 10.0;
    ILatchable::Latch best_vert_latch;
    bool found_best_vert = false;

    foreach (QGraphicsItem *item, m_diagramSceneModel->getGraphicsItems()) {
        if (item != m_diagramSceneModel->getFocusItem() && !m_diagramSceneModel->isSelectedItem(item)) {
            if (ILatchable *latchable = dynamic_cast<ILatchable *>(item)) {
                QList<ILatchable::Latch> horizontals = latchable->getHorizontalLatches(horiz_action, false);
                foreach (const ILatchable::Latch &palped_latch, palped_horizontals) {
                    foreach (const ILatchable::Latch &latch, horizontals) {
                        if (palped_latch.m_latchType == latch.m_latchType) {
                            // calculate distance and minimal distance with sign because this is needed later to move the objects
                            qreal dist = latch.m_pos - palped_latch.m_pos;
                            if (qAbs(dist) < qAbs(horiz_min_dist)) {
                                horiz_min_dist = dist;
                                best_horiz_latch = latch;
                                found_best_horiz = true;
                            }
                        }
                    }
                }
                QList<ILatchable::Latch> verticals = latchable->getVerticalLatches(vert_action, false);
                foreach (const ILatchable::Latch &palped_latch, palped_verticals) {
                    foreach (const ILatchable::Latch &latch, verticals) {
                        if (palped_latch.m_latchType == latch.m_latchType) {
                            // calculate distance and minimal distance with sign because this is needed later to move the objects
                            qreal dist = latch.m_pos - palped_latch.m_pos;
                            if (qAbs(dist) < qAbs(vert_min_dist)) {
                                vert_min_dist = dist;
                                best_vert_latch = latch;
                                found_best_vert = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (found_best_horiz) {
        switch (best_horiz_latch.m_latchType) {
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            m_verticalAlignLine->setLine(best_horiz_latch.m_pos, best_horiz_latch.m_otherPos1, best_horiz_latch.m_otherPos2);
            m_verticalAlignLine->setVisible(true);
            m_foundHorizontalLatch = true;
            m_horizontalLatch = best_horiz_latch;
            m_horizontalDist = horiz_min_dist;
            break;
        case ILatchable::NONE:
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            QMT_CHECK(false);
            break;
        }
    } else {
        m_verticalAlignLine->setVisible(false);
    }

    if (found_best_vert) {
        switch (best_vert_latch.m_latchType) {
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            m_horizontalAlignLine->setLine(best_vert_latch.m_pos, best_vert_latch.m_otherPos1, best_vert_latch.m_otherPos2);
            m_horizontalAlignLine->setVisible(true);
            m_foundVerticalLatch = true;
            m_verticalLatch = best_vert_latch;
            m_verticalDist = vert_min_dist;
            break;
        case ILatchable::NONE:
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            QMT_CHECK(false);
            break;
        }
    } else {
        m_horizontalAlignLine->setVisible(false);
    }
}

void LatchController::mouseReleaseEventLatching(QGraphicsSceneMouseEvent *event)
{
    Q_UNUSED(event);

    if (m_foundHorizontalLatch) {
        switch (m_horizontalLatch.m_latchType) {
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            foreach (QGraphicsItem *item, m_diagramSceneModel->getSelectedItems()) {
                DElement *element = m_diagramSceneModel->getElement(item);
                if (DObject *selected_object = dynamic_cast<DObject *>(element)) {
                    m_diagramSceneModel->getDiagramController()->startUpdateElement(selected_object, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
                    QPointF new_pos = selected_object->getPos();
                    new_pos.setX(new_pos.x() + m_horizontalDist);
                    selected_object->setPos(new_pos);
                    m_diagramSceneModel->getDiagramController()->finishUpdateElement(selected_object, m_diagramSceneModel->getDiagram(), false);
                }
            }
            break;
        case ILatchable::NONE:
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            QMT_CHECK(false);
            break;
        }
    }

    if (m_foundVerticalLatch) {
        switch (m_verticalLatch.m_latchType) {
        case ILatchable::TOP:
        case ILatchable::BOTTOM:
        case ILatchable::VCENTER:
            foreach (QGraphicsItem *item, m_diagramSceneModel->getSelectedItems()) {
                DElement *element = m_diagramSceneModel->getElement(item);
                if (DObject *selected_object = dynamic_cast<DObject *>(element)) {
                    m_diagramSceneModel->getDiagramController()->startUpdateElement(selected_object, m_diagramSceneModel->getDiagram(), DiagramController::UPDATE_GEOMETRY);
                    QPointF new_pos = selected_object->getPos();
                    new_pos.setY(new_pos.y() + m_verticalDist);
                    selected_object->setPos(new_pos);
                    m_diagramSceneModel->getDiagramController()->finishUpdateElement(selected_object, m_diagramSceneModel->getDiagram(), false);
                }
            }
            break;
        case ILatchable::NONE:
        case ILatchable::LEFT:
        case ILatchable::RIGHT:
        case ILatchable::HCENTER:
            QMT_CHECK(false);
            break;
        }

    }

    m_horizontalAlignLine->setVisible(false);
    m_verticalAlignLine->setVisible(false);
}

}
