/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "vcsannotatetaskhandler.h"

#include "task.h"
#include "projectexplorerconstants.h"

#include <coreplugin/icore.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>

#include <QtCore/QFileInfo>

#include <QtGui/QAction>

using namespace ProjectExplorer::Internal;

VcsAnnotateTaskHandler::VcsAnnotateTaskHandler() :
    ITaskHandler(QLatin1String(Constants::VCS_ANNOTATE_TASK))
{ }

bool VcsAnnotateTaskHandler::canHandle(const ProjectExplorer::Task &task)
{
    QFileInfo fi(task.file);
    if (!fi.exists() || !fi.isFile() || !fi.isReadable())
        return false;
    Core::IVersionControl *vc = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(fi.absolutePath());
    if (!vc)
        return false;
    return vc->supportsOperation(Core::IVersionControl::AnnotateOperation);
}

void VcsAnnotateTaskHandler::handle(const ProjectExplorer::Task &task)
{
    QFileInfo fi(task.file);
    Core::IVersionControl *vc = Core::ICore::instance()->vcsManager()->findVersionControlForDirectory(fi.absolutePath());
    Q_ASSERT(vc);
    Q_ASSERT(vc->supportsOperation(Core::IVersionControl::AnnotateOperation));
    vc->vcsAnnotate(fi.absoluteFilePath(), task.line);
}

QAction *VcsAnnotateTaskHandler::createAction(QObject *parent)
{
    QAction *vcsannotateAction = new QAction(tr("&Annotate"), parent);
    vcsannotateAction->setToolTip("Annotate using version control system");
    return vcsannotateAction;
}
