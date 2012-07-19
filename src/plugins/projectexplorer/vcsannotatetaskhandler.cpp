/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: http://www.qt-project.org/
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**************************************************************************/

#include "vcsannotatetaskhandler.h"

#include "task.h"
#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <QFileInfo>

#include <QAction>

using namespace ProjectExplorer::Internal;

bool VcsAnnotateTaskHandler::canHandle(const ProjectExplorer::Task &task) const
{
    QFileInfo fi(task.file.toFileInfo());
    if (!fi.exists() || !fi.isFile() || !fi.isReadable())
        return false;
    Core::IVersionControl *vc = Core::ICore::vcsManager()->findVersionControlForDirectory(fi.absolutePath());
    if (!vc)
        return false;
    return vc->supportsOperation(Core::IVersionControl::AnnotateOperation);
}

void VcsAnnotateTaskHandler::handle(const ProjectExplorer::Task &task)
{
    QFileInfo fi(task.file.toFileInfo());
    Core::IVersionControl *vc = Core::ICore::vcsManager()->findVersionControlForDirectory(fi.absolutePath());
    Q_ASSERT(vc);
    Q_ASSERT(vc->supportsOperation(Core::IVersionControl::AnnotateOperation));
    vc->vcsAnnotate(fi.absoluteFilePath(), task.movedLine);
}

QAction *VcsAnnotateTaskHandler::createAction(QObject *parent) const
{
    QAction *vcsannotateAction = new QAction(tr("&Annotate"), parent);
    vcsannotateAction->setToolTip(tr("Annotate using version control system"));
    return vcsannotateAction;
}
