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
