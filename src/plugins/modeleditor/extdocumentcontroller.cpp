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
    ElementTasks *elementTasks = 0;
    PxNodeController *pxNodeController = 0;
};

ExtDocumentController::ExtDocumentController(QObject *parent)
    : qmt::DocumentController(parent),
      d(new ExtDocumentControllerPrivate)
{
    d->elementTasks = new ElementTasks;
    d->pxNodeController = new PxNodeController(this);
    // TODO use more specific dependency
    d->elementTasks->setDocumentController(this);
    getDiagramSceneController()->setElementTasks(d->elementTasks);

    d->pxNodeController->setDiagramSceneController(getDiagramSceneController());

    connect(getProjectController(), &qmt::ProjectController::fileNameChanged,
            this, &ExtDocumentController::onProjectFileNameChanged);
}

ExtDocumentController::~ExtDocumentController()
{
    delete d->elementTasks;
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
