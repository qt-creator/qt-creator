/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "pchmanagerserver.h"

#include <pchmanagerclientinterface.h>
#include <precompiledheadersupdatedmessage.h>
#include <removepchprojectpartsmessage.h>
#include <updatepchprojectpartsmessage.h>

#include <utils/smallstring.h>

#include <QApplication>

namespace ClangBackEnd {

PchManagerServer::PchManagerServer(ClangPathWatcherInterface &fileSystemWatcher,
                                   PchCreatorInterface &pchCreator,
                                   ProjectPartsInterface &projectParts)
    : m_fileSystemWatcher(fileSystemWatcher),
      m_pchCreator(pchCreator),
      m_projectParts(projectParts)
{
    m_fileSystemWatcher.setNotifier(this);
}

void PchManagerServer::end()
{
    QCoreApplication::exit();

}

void PchManagerServer::updatePchProjectParts(UpdatePchProjectPartsMessage &&message)
{
    m_pchCreator.setGeneratedFiles(message.takeGeneratedFiles());

    m_pchCreator.generatePchs(m_projectParts.update(message.takeProjectsParts()));

    m_fileSystemWatcher.updateIdPaths(m_pchCreator.takeProjectsIncludes());
}

void PchManagerServer::removePchProjectParts(RemovePchProjectPartsMessage &&message)
{
    m_fileSystemWatcher.removeIds(message.projectsPartIds());

    m_projectParts.remove(message.projectsPartIds());
}

void PchManagerServer::pathsWithIdsChanged(const Utils::SmallStringVector &ids)
{
    m_pchCreator.generatePchs(m_projectParts.projects(ids));

    m_fileSystemWatcher.updateIdPaths(m_pchCreator.takeProjectsIncludes());
}

void PchManagerServer::taskFinished(TaskFinishStatus status, const ProjectPartPch &projectPartPch)
{
    if (status == TaskFinishStatus::Successfully)
        client()->precompiledHeadersUpdated(PrecompiledHeadersUpdatedMessage({projectPartPch.clone()}));
}

} // namespace ClangBackEnd
