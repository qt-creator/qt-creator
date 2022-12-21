// Copyright (C) 2016 Jochen Becher
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "extdocumentcontroller.h"

#include "elementtasks.h"
#include "pxnodecontroller.h"

#include "qmt/project_controller/projectcontroller.h"
#include "qmt/tasks/diagramscenecontroller.h"

#include <QFileInfo>

namespace ModelEditor {
namespace Internal {

class ExtDocumentController::ExtDocumentControllerPrivate {
public:
    ElementTasks *elementTasks = nullptr;
    PxNodeController *pxNodeController = nullptr;
};

ExtDocumentController::ExtDocumentController(QObject *parent)
    : qmt::DocumentController(parent),
      d(new ExtDocumentControllerPrivate)
{
    d->elementTasks = new ElementTasks(this);
    d->pxNodeController = new PxNodeController(this);
    d->elementTasks->setDocumentController(this);
    d->elementTasks->setComponentViewController(d->pxNodeController->componentViewController());
    diagramSceneController()->setElementTasks(d->elementTasks);

    d->pxNodeController->setDiagramSceneController(diagramSceneController());

    connect(projectController(), &qmt::ProjectController::fileNameChanged,
            this, &ExtDocumentController::onProjectFileNameChanged);
}

ExtDocumentController::~ExtDocumentController()
{
    delete d;
}

ElementTasks *ExtDocumentController::elementTasks() const
{
    return d->elementTasks;
}

PxNodeController *ExtDocumentController::pxNodeController() const
{
    return d->pxNodeController;
}

void ExtDocumentController::onProjectFileNameChanged(const QString &fileName)
{
    QFileInfo fileInfo(fileName);
    d->pxNodeController->setAnchorFolder(fileInfo.path());
}

} // namespace Internal
} // namespace ModelEditor
