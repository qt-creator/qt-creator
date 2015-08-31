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

#include "editordiagramview.h"

#include "pxnodecontroller.h"

#include "qmt/diagram_scene/diagramscenemodel.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <projectexplorer/projectnodes.h>
#include <utils/dropsupport.h>

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
    // TODO use slot instead of lambda, lambda is to long and complex
    connect(droputils, &Utils::DropSupport::valuesDropped,
            this,
            [this](const QList<QVariant> &values, const QPoint &pos) {
        foreach (const QVariant &value, values) {
            if (value.canConvert<ProjectExplorer::Node *>()) {
                auto node = value.value<ProjectExplorer::Node *>();
                QPointF scenePos = mapToScene(pos);
                d->pxNodeController->addExplorerNode(
                            node, getDiagramSceneModel()->findTopmostElement(scenePos),
                            scenePos, getDiagramSceneModel()->getDiagram());
            }
        }
    });
}

EditorDiagramView::~EditorDiagramView()
{
    delete d;
}

void EditorDiagramView::setPxNodeController(PxNodeController *pxNodeController)
{
    d->pxNodeController = pxNodeController;
}

} // namespace Internal
} // namespace ModelEditor
