/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "vcsannotatetaskhandler.h"

#include "task.h"

#include <coreplugin/iversioncontrol.h>
#include <coreplugin/vcsmanager.h>
#include <utils/qtcassert.h>

#include <QAction>
#include <QFileInfo>

using namespace Core;

namespace ProjectExplorer {
namespace Internal {

bool VcsAnnotateTaskHandler::canHandle(const Task &task) const
{
    QFileInfo fi(task.file.toFileInfo());
    if (!fi.exists() || !fi.isFile() || !fi.isReadable())
        return false;
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(fi.absolutePath());
    if (!vc)
        return false;
    return vc->supportsOperation(IVersionControl::AnnotateOperation);
}

void VcsAnnotateTaskHandler::handle(const Task &task)
{
    QFileInfo fi(task.file.toFileInfo());
    IVersionControl *vc = VcsManager::findVersionControlForDirectory(fi.absolutePath());
    QTC_ASSERT(vc, return);
    QTC_ASSERT(vc->supportsOperation(IVersionControl::AnnotateOperation), return);
    vc->vcsAnnotate(fi.absoluteFilePath(), task.movedLine);
}

QAction *VcsAnnotateTaskHandler::createAction(QObject *parent) const
{
    QAction *vcsannotateAction = new QAction(tr("&Annotate"), parent);
    vcsannotateAction->setToolTip(tr("Annotate using version control system."));
    return vcsannotateAction;
}

} // namespace Internal
} // namespace ProjectExplorer
