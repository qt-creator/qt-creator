// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsannotatetaskhandler.h"

#include "projectexplorertr.h"
#include "task.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QFileInfo>

using namespace Core;
using namespace Utils;

namespace ProjectExplorer {
namespace Internal {

bool VcsAnnotateTaskHandler::canHandle(const Task &task) const
{
    QFileInfo fi(task.file.toFileInfo());
    if (!fi.exists() || !fi.isFile() || !fi.isReadable())
        return false;
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(task.file.absolutePath());
    if (!vc)
        return false;
    return vc->supportsOperation(IVersionControl::AnnotateOperation);
}

void VcsAnnotateTaskHandler::handle(const Task &task)
{
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(task.file.absolutePath());
    QTC_ASSERT(vc, return);
    QTC_ASSERT(vc->supportsOperation(IVersionControl::AnnotateOperation), return);
    vc->vcsAnnotate(task.file.absoluteFilePath(), task.movedLine);
}

QAction *VcsAnnotateTaskHandler::createAction(QObject *parent) const
{
    QAction *vcsannotateAction = new QAction(Tr::tr("&Annotate"), parent);
    vcsannotateAction->setToolTip(Tr::tr("Annotate using version control system."));
    return vcsannotateAction;
}

} // namespace Internal
} // namespace ProjectExplorer
