// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
      m_horizontalAlignLine(new AlignLineItem(AlignLineItem::Horizontal, nullptr)),
      m_verticalAlignLine(new AlignLineItem(AlignLineItem::Vertical, nullptr))
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
    QMT_ASSERT(graphicsScene, return);
    graphicsScene->addItem(m_horizontalAlignLine);
    graphicsScene->addItem(m_verticalAlignLine);
}

void LatchController::removeFromGraphicsScene(QGraphicsScene *graphicsScene)
{
    Q_UNUSED(graphicsScene) // avoid warning in release mode

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

    const QList<ILatchable::Latch> palpedHorizontals = palpedLatchable->horizontalLatches(horizAction, true);
    const QList<ILatchable::Latch> palpedVerticals = palpedLatchable->verticalLatches(vertAction, true);

    qreal horizMinDist = 10.0;
    ILatchable::Latch bestHorizLatch;
    bool foundBestHoriz = false;
    qreal vertMinDist = 10.0;
    ILatchable::Latch bestVertLatch;
    bool foundBestVert = false;

    for (QGraphicsItem *item : m_diagramSceneModel->graphicsItems()) {
        if (item != m_diagramSceneModel->focusItem() && !m_diagramSceneModel->isSelectedItem(item)) {
            if (auto latchable = dynamic_cast<ILatchable *>(item)) {
                const QList<ILatchable::Latch> horizontals = latchable->horizontalLatches(horizAction, false);
                for (const ILatchable::Latch &palpedLatch : palpedHorizontals) {
                    for (const ILatchable::Latch &latch : horizontals) {
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
                const QList<ILatchable::Latch> verticals = latchable->verticalLatches(vertAction, false);
                for (const ILatchable::Latch &palpedLatch : palpedVerticals) {
                    for (const ILatchable::Latch &latch : verticals) {
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
            for (QGraphicsItem *item : m_diagramSceneModel->selectedItems()) {
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
            for (QGraphicsItem *item : m_diagramSceneModel->selectedItems()) {
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
