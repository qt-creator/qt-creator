// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
    PxNodeController *pxNodeController = nullptr;
};

EditorDiagramView::EditorDiagramView(QWidget *parent)
    : qmt::DiagramView(parent),
      d(new EditorDiagramViewPrivate)
{
    auto droputils = new Utils::DropSupport(
                this,
                [](QDropEvent *event, Utils::DropSupport *)
            -> bool { return Utils::DropSupport::isFileDrop(event)
                                     || Utils::DropSupport::isValueDrop(event); });
    connect(droputils, &Utils::DropSupport::filesDropped,
            this, &EditorDiagramView::dropFiles);
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
        QPoint zoomOrigin = wheelEvent->position().toPoint();
        if (degree > 0)
            emit zoomIn(zoomOrigin);
        else if (degree < 0)
            emit zoomOut(zoomOrigin);
    }
}

void EditorDiagramView::dropProjectExplorerNodes(const QList<QVariant> &values, const QPoint &pos)
{
    for (const auto &value : values) {
        if (value.canConvert<ProjectExplorer::Node *>()) {
            auto node = value.value<ProjectExplorer::Node *>();
            QPointF scenePos = mapToScene(pos);
            auto folderNode = dynamic_cast<ProjectExplorer::FolderNode *>(node);
            if (folderNode) {
                d->pxNodeController->addFileSystemEntry(
                            folderNode->filePath().toString(), -1, -1,
                            diagramSceneModel()->findTopmostElement(scenePos),
                            scenePos, diagramSceneModel()->diagram());
            }
        }
    }
}

void EditorDiagramView::dropFiles(const QList<Utils::DropSupport::FileSpec> &files, const QPoint &pos)
{
    for (const auto &fileSpec : files) {
        QPointF scenePos = mapToScene(pos);
        d->pxNodeController->addFileSystemEntry(
                    fileSpec.filePath.toString(), fileSpec.line, fileSpec.column,
                    diagramSceneModel()->findTopmostElement(scenePos),
                    scenePos, diagramSceneModel()->diagram());
    }
}

} // namespace Internal
} // namespace ModelEditor
