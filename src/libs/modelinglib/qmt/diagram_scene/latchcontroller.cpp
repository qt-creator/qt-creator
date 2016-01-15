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

#include "latchcontroller.h"

#include "diagramsceneconstants.h"
#include "diagramscenemodel.h"
#include "parts/alignlineitem.h"

#include "qmt/diagram/dobject.h"
#include "qmt/diagram_controller/diagramcontroller.h"
#include "qmt/infrastructure/qmtassert.h"

#include <QGraphicsScene>
#include <QGraphicsSceneMouseEvent>
#include <QKeyEvent>

namespace qmt {

LatchController::LatchController(QObject *parent)
    : QObject(parent),
      m_diagramSceneModel(0),
      m_horizontalAlignLine(new AlignLineItem(AlignLineItem::Horizontal, 0)),
      m_verticalAlignLine(new AlignLineItem(AlignLineItem::Vertical, 0)),
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
    if (m_verticalAlignLine->scene())
        m_verticalAlignLine->scene()->removeItem(m_verticalAlignLine);
    delete m_verticalAlignLine;
    if (m_horizontalAlignLine->scene())
        m_horizontalAlignLine->scene()->removeItem(m_horizontalAlignLine);
    delete m_horizontalAlignLine;
}

void LatchController::setDiagramSceneModel(DiagramSceneModel *diagramSceneModel)
{
    m_diagramSceneModel = diagramSceneModel;
}

void LatchController::addToGraphicsScene(QGraphicsScene *graphicsScene)
{
    QMT_CHECK(graphicsScene);
    graphicsScene->addItem(m_horizontalAlignLine);
    graphicsScene->addItem(m_verticalAlignLine);
}

void LatchController::removeFromGraphicsScene(QGraphicsScene *graphicsScene)
{
    Q_UNUSED(graphicsScene); // avoid warning in release mode

    if (m_verticalAlignLine->scene()) {
        QMT_CHECK(graphicsScene == m_verticalAlignLine->scene());
        m_verticalAlignLine->scene()->removeItem(m_verticalAlignLine);
    }
    if (m_horizontalAlignLine->scene()) {
        QMT_CHECK(graphicsScene == m_horizontalAlignLine->scene());
        m_horizontalAlignLine->scene()->removeItem(m_horizontalAlignLine);
    }
}

void LatchController::keyPressEventLatching(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
        handleLatches();
}

void LatchController::keyReleaseEventLatching(QKeyEvent *event)
{
    if (event->key() == Qt::Key_Shift)
        hideLatches();
}

void LatchController::mousePressEventLatching(QGraphicsSceneMouseEvent *event)
{
    mouseMoveEventLatching(event);
}

void LatchController::mouseMoveEventLatching(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier)
        handleLatches();
    else
        hideLatches();
}

void LatchController::mouseReleaseEventLatching(QGraphicsSceneMouseEvent *event)
{
    if (event->modifiers() & Qt::ShiftModifier) {
        handleLatches();
        applyLatches();
    } else {
        hideLatches();
    }
}

void LatchController::handleLatches()
{
    m_foundHorizontalLatch = false;
    m_foundVerticalLatch = false;

    if (!m_diagramSceneModel->focusItem())
        return;

    auto palpedLatchable = dynamic_cast<ILatchable *>(m_diagramSceneModel->focusItem());
    if (!palpedLatchable)
        return;

    ILatchable::Action horizAction = palpedLatchable->horizontalLatchAction();
    ILatchable::Action vertAction = palpedLatchable->verticalLatchAction();

    // TODO fix resize of items with latches
    if (horizAction != ILatchable::Move || vertAction != ILatchable::Move)
        return;

    QList<ILatchable::Latch> palpedHorizontals = palpedLatchable->horizontalLatches(horizAction, true);
    QList<ILatchable::Latch> palpedVerticals = palpedLatchable->verticalLatches(vertAction, true);

    qreal horizMinDist = 10.0;
    ILatchable::Latch bestHorizLatch;
    bool foundBestHoriz = false;
    qreal vertMinDist = 10.0;
    ILatchable::Latch bestVertLatch;
    bool foundBestVert = false;

    foreach (QGraphicsItem *item, m_diagramSceneModel->graphicsItems()) {
        if (item != m_diagramSceneModel->focusItem() && !m_diagramSceneModel->isSelectedItem(item)) {
            if (auto latchable = dynamic_cast<ILatchable *>(item)) {
                QList<ILatchable::Latch> horizontals = latchable->horizontalLatches(horizAction, false);
                foreach (const ILatchable::Latch &palpedLatch, palpedHorizontals) {
                    foreach (const ILatchable::Latch &latch, horizontals) {
                        if (palpedLatch.m_latchType == latch.m_latchType) {
                            // calculate distance and minimal distance with sign
                            // because this is needed later to move the objects
                            qreal dist = latch.m_pos - palpedLatch.m_pos;
                            if (qAbs(dist) < qAbs(horizMinDist)) {
                                horizMinDist = dist;
                                bestHorizLatch = latch;
                                foundBestHoriz = true;
                            }
                        }
                    }
                }
                QList<ILatchable::Latch> verticals = latchable->verticalLatches(vertAction, false);
                foreach (const ILatchable::Latch &palpedLatch, palpedVerticals) {
                    foreach (const ILatchable::Latch &latch, verticals) {
                        if (palpedLatch.m_latchType == latch.m_latchType) {
                            // calculate distance and minimal distance with sign
                            // because this is needed later to move the objects
                            qreal dist = latch.m_pos - palpedLatch.m_pos;
                            if (qAbs(dist) < qAbs(vertMinDist)) {
                                vertMinDist = dist;
                                bestVertLatch = latch;
                                foundBestVert = true;
                            }
                        }
                    }
                }
            }
        }
    }

    if (foundBestHoriz) {
        switch (bestHorizLatch.m_latchType) {
        case ILatchable::Left:
        case ILatchable::Right:
        case ILatchable::Hcenter:
            m_verticalAlignLine->setLine(bestHorizLatch.m_pos, bestHorizLatch.m_otherPos1, bestHorizLatch.m_otherPos2);
            m_verticalAlignLine->setVisible(true);
            m_foundHorizontalLatch = true;
            m_horizontalLatch = bestHorizLatch;
            m_horizontalDist = horizMinDist;
            break;
        case ILatchable::None:
        case ILatchable::Top:
        case ILatchable::Bottom:
        case ILatchable::Vcenter:
            QMT_CHECK(false);
            break;
        }
    } else {
        m_verticalAlignLine->setVisible(false);
    }

    if (foundBestVert) {
        switch (bestVertLatch.m_latchType) {
        case ILatchable::Top:
        case ILatchable::Bottom:
        case ILatchable::Vcenter:
            m_horizontalAlignLine->setLine(bestVertLatch.m_pos, bestVertLatch.m_otherPos1, bestVertLatch.m_otherPos2);
            m_horizontalAlignLine->setVisible(true);
            m_foundVerticalLatch = true;
            m_verticalLatch = bestVertLatch;
            m_verticalDist = vertMinDist;
            break;
        case ILatchable::None:
        case ILatchable::Left:
        case ILatchable::Right:
        case ILatchable::Hcenter:
            QMT_CHECK(false);
            break;
        }
    } else {
        m_horizontalAlignLine->setVisible(false);
    }
}

void LatchController::hideLatches()
{
    m_foundHorizontalLatch = false;
    m_foundVerticalLatch = false;
    m_horizontalAlignLine->setVisible(false);
    m_verticalAlignLine->setVisible(false);
}

void LatchController::applyLatches()
{
    // TODO fix calculation of distance. Final position is usually not correct.
    if (m_foundHorizontalLatch) {
        switch (m_horizontalLatch.m_latchType) {
        case ILatchable::Left:
        case ILatchable::Right:
        case ILatchable::Hcenter:
            foreach (QGraphicsItem *item, m_diagramSceneModel->selectedItems()) {
                DElement *element = m_diagramSceneModel->element(item);
                if (auto selectedObject = dynamic_cast<DObject *>(element)) {
                    m_diagramSceneModel->diagramController()->startUpdateElement(selectedObject, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
                    QPointF newPos = selectedObject->pos();
                    newPos.setX(newPos.x() + m_horizontalDist);
                    selectedObject->setPos(newPos);
                    m_diagramSceneModel->diagramController()->finishUpdateElement(selectedObject, m_diagramSceneModel->diagram(), false);
                }
            }
            break;
        case ILatchable::None:
        case ILatchable::Top:
        case ILatchable::Bottom:
        case ILatchable::Vcenter:
            QMT_CHECK(false);
            break;
        }
    }

    if (m_foundVerticalLatch) {
        switch (m_verticalLatch.m_latchType) {
        case ILatchable::Top:
        case ILatchable::Bottom:
        case ILatchable::Vcenter:
            foreach (QGraphicsItem *item, m_diagramSceneModel->selectedItems()) {
                DElement *element = m_diagramSceneModel->element(item);
                if (auto selectedObject = dynamic_cast<DObject *>(element)) {
                    m_diagramSceneModel->diagramController()->startUpdateElement(selectedObject, m_diagramSceneModel->diagram(), DiagramController::UpdateGeometry);
                    QPointF newPos = selectedObject->pos();
                    newPos.setY(newPos.y() + m_verticalDist);
                    selectedObject->setPos(newPos);
                    m_diagramSceneModel->diagramController()->finishUpdateElement(selectedObject, m_diagramSceneModel->diagram(), false);
                }
            }
            break;
        case ILatchable::None:
        case ILatchable::Left:
        case ILatchable::Right:
        case ILatchable::Hcenter:
            QMT_CHECK(false);
            break;
        }
    }

    hideLatches();
}

} // namespace qmt
