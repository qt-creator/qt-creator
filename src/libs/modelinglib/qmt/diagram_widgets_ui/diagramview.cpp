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

#include "diagramview.h"

#include "qmt/diagram_ui/diagram_mime_types.h"

#include "qmt/infrastructure/uid.h"
#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDragLeaveEvent>
#include <QDropEvent>
#include <QMimeData>

namespace qmt {

DiagramView::DiagramView(QWidget *parent)
    : QGraphicsView(parent)
{
    setAcceptDrops(true);
    setAlignment(Qt::AlignLeft | Qt::AlignTop);
    setDragMode(QGraphicsView::RubberBandDrag);
    setFrameShape(QFrame::NoFrame);
    setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
}

DiagramView::~DiagramView()
{
}

DiagramSceneModel *DiagramView::diagramSceneModel() const
{
    return m_diagramSceneModel;
}

void DiagramView::setDiagramSceneModel(DiagramSceneModel *diagramSceneModel)
{
    setScene(nullptr);
    m_diagramSceneModel = diagramSceneModel;
    if (diagramSceneModel) {
        setScene(m_diagramSceneModel->graphicsScene());
        connect(m_diagramSceneModel, &DiagramSceneModel::sceneRectChanged,
                this, &DiagramView::onSceneRectChanged, Qt::QueuedConnection);
    }
}

void DiagramView::dragEnterEvent(QDragEnterEvent *event)
{
    if (event->mimeData()->hasFormat(QLatin1String(MIME_TYPE_MODEL_ELEMENTS))) {
        bool accept = false;
        QDataStream dataStream(event->mimeData()->data(QLatin1String(MIME_TYPE_MODEL_ELEMENTS)));
        while (dataStream.status() == QDataStream::Ok) {
            QString key;
            dataStream >> key;
            if (!key.isEmpty()) {
                if (m_diagramSceneModel->diagramSceneController()->isAddingAllowed(Uid(key), m_diagramSceneModel->diagram()))
                    accept = true;
            }
        }
        event->setAccepted(accept);
    } else if (event->mimeData()->hasFormat(QLatin1String(MIME_TYPE_NEW_MODEL_ELEMENTS))) {
        bool accept = false;
        QDataStream dataStream(event->mimeData()->data(QLatin1String(MIME_TYPE_NEW_MODEL_ELEMENTS)));
        while (dataStream.status() == QDataStream::Ok) {
            QString newElementId;
            dataStream >> newElementId;
            if (!newElementId.isEmpty()) {
                // TODO ask a factory if newElementId is a known type
                accept = true;
            }
        }
        event->setAccepted(accept);
    } else {
        event->ignore();
    }
}

void DiagramView::dragLeaveEvent(QDragLeaveEvent *event)
{
    event->accept();
}

void DiagramView::dragMoveEvent(QDragMoveEvent *event)
{
    event->accept();
}

void DiagramView::dropEvent(QDropEvent *event)
{
    event->setDropAction(Qt::MoveAction);
    DiagramSceneController *diagramSceneController = m_diagramSceneModel->diagramSceneController();
    if (event->mimeData()->hasFormat(QLatin1String(MIME_TYPE_MODEL_ELEMENTS))) {
        QDataStream dataStream(event->mimeData()->data(QLatin1String(MIME_TYPE_MODEL_ELEMENTS)));
        while (dataStream.status() == QDataStream::Ok) {
            QString key;
            dataStream >> key;
            if (!key.isEmpty()) {
                if (diagramSceneController->isAddingAllowed(Uid(key), m_diagramSceneModel->diagram())) {
                    diagramSceneController->addExistingModelElement(Uid(key), mapToScene(event->pos()),
                                                                    m_diagramSceneModel->diagram());
                }
            }
        }
        event->accept();
    } else if (event->mimeData()->hasFormat(QLatin1String(MIME_TYPE_NEW_MODEL_ELEMENTS))) {
        QDataStream dataStream(event->mimeData()->data(QLatin1String(MIME_TYPE_NEW_MODEL_ELEMENTS)));
        while (dataStream.status() == QDataStream::Ok) {
            QString newElementId;
            QString name;
            QString stereotype;
            dataStream >> newElementId >> name >> stereotype;
            if (!newElementId.isEmpty()) {
                QPointF pos = mapToScene(event->pos());
                diagramSceneController->dropNewElement(
                            newElementId, name, stereotype, m_diagramSceneModel->findTopmostElement(pos),
                            pos, m_diagramSceneModel->diagram(), event->pos(), size());
            }
        }
        event->accept();
    } else  {
        event->ignore();
    }
}

void DiagramView::onSceneRectChanged(const QRectF &sceneRect)
{
    // TODO add some adjustment to all 4 sides?
    setSceneRect(sceneRect);
}

} // namespace qmt
