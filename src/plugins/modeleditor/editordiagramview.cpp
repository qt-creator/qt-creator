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

#include "editordiagramview.h"

#include "pxnodecontroller.h"

#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <projectexplorer/projectnodes.h>
#include <utils/dropsupport.h>

#include <QWheelEvent>

namespace ModelEditor {
namespace Internal {

class EditorDiagramView::EditorDiagramViewPrivate {
public:
    PxNodeController *pxNodeController = 0;
};

EditorDiagramView::EditorDiagramView(QWidget *parent)
    : qmt::DiagramView(parent),
      d(new EditorDiagramViewPrivate)
{
    auto droputils = new Utils::DropSupport(
                this,
                [this](QDropEvent *event, Utils::DropSupport *dropSupport)
            -> bool { return dropSupport->isValueDrop(event); });
    connect(droputils, &Utils::DropSupport::valuesDropped,
            this, &EditorDiagramView::dropProjectExplorerNodes);
}

EditorDiagramView::~EditorDiagramView()
{
    delete d;
}

void EditorDiagramView::setPxNodeController(PxNodeController *pxNodeController)
{
    d->pxNodeController = pxNodeController;
}

void EditorDiagramView::wheelEvent(QWheelEvent *wheelEvent)
{
    if (wheelEvent->modifiers() == Qt::ControlModifier) {
        int degree = wheelEvent->angleDelta().y() / 8;
        if (degree > 0)
            emit zoomIn();
        else if (degree < 0)
            emit zoomOut();
    }
}

void EditorDiagramView::dropProjectExplorerNodes(const QList<QVariant> &values, const QPoint &pos)
{
    foreach (const QVariant &value, values) {
        if (value.canConvert<ProjectExplorer::Node *>()) {
            auto node = value.value<ProjectExplorer::Node *>();
            QPointF scenePos = mapToScene(pos);
            d->pxNodeController->addExplorerNode(
                        node, diagramSceneModel()->findTopmostElement(scenePos),
                        scenePos, diagramSceneModel()->diagram());
        }
    }
}

} // namespace Internal
} // namespace ModelEditor
