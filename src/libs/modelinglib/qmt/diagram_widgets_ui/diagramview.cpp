// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
        // Signal is connected after diagram is updated. Enforce setting of scene rect.
        onSceneRectChanged(m_diagramSceneModel->sceneRect());
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
                if (m_diagramSceneModel->diagramSceneController()
                        ->isAddingAllowed(Uid(QUuid(key)), m_diagramSceneModel->diagram()))
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
                if (diagramSceneController->isAddingAllowed(Uid(QUuid(key)),
                                                            m_diagramSceneModel->diagram())) {
                    diagramSceneController->addExistingModelElement(Uid(QUuid(key)),
                                                                    mapToScene(event->pos()),
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
    // add some adjustment to all 4 sides
    static const qreal ADJUSTMENT = 80;
    QRectF rect = sceneRect.adjusted(-ADJUSTMENT, -ADJUSTMENT, ADJUSTMENT, ADJUSTMENT);
    setSceneRect(rect);
}

} // namespace qmt
