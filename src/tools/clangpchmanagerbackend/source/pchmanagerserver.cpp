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
#include <projectpartqueue.h>
#include <removegeneratedfilesmessage.h>
#include <removeprojectpartsmessage.h>
#include <updategeneratedfilesmessage.h>
#include <updateprojectpartsmessage.h>

#include <utils/smallstring.h>

#include <QApplication>

namespace ClangBackEnd {

PchManagerServer::PchManagerServer(ClangPathWatcherInterface &fileSystemWatcher,
                                   ProjectPartQueueInterface &projectPartQueue,
                                   ProjectPartsInterface &projectParts,
                                   GeneratedFilesInterface &generatedFiles)
    : m_fileSystemWatcher(fileSystemWatcher),
      m_projectPartQueue(projectPartQueue),
      m_projectParts(projectParts),
      m_generatedFiles(generatedFiles)
{
    m_fileSystemWatcher.setNotifier(this);
}

void PchManagerServer::end()
{
    QCoreApplication::exit();

}

void PchManagerServer::updateProjectParts(UpdateProjectPartsMessage &&message)
{
    m_projectPartQueue.addProjectParts(m_projectParts.update(message.takeProjectsParts()));
}

void PchManagerServer::removeProjectParts(RemoveProjectPartsMessage &&message)
{
    m_fileSystemWatcher.removeIds(message.projectsPartIds);

    m_projectParts.remove(message.projectsPartIds);

    m_projectPartQueue.removeProjectParts(message.projectsPartIds);
}

void PchManagerServer::updateGeneratedFiles(UpdateGeneratedFilesMessage &&message)
{
    m_generatedFiles.update(message.takeGeneratedFiles());
}

void PchManagerServer::removeGeneratedFiles(RemoveGeneratedFilesMessage &&message)
{
    m_generatedFiles.remove(message.takeGeneratedFiles());
}

void PchManagerServer::pathsWithIdsChanged(const Utils::SmallStringVector &ids)
{
    m_projectPartQueue.addProjectParts(m_projectParts.projects(ids));
}

void PchManagerServer::pathsChanged(const FilePathIds &/*filePathIds*/)
{
}

} // namespace ClangBackEnd
