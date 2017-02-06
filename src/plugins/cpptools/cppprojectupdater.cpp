/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "cppprojectupdater.h"

#include "cppmodelmanager.h"
#include "cppprojectinfogenerator.h"

#include <projectexplorer/toolchainmanager.h>

#include <utils/qtcassert.h>
#include <utils/runextensions.h>

namespace CppTools {

CppProjectUpdater::CppProjectUpdater(ProjectExplorer::Project *project)
    : m_project(project)
{
    connect(&m_generateFutureWatcher, &QFutureWatcher<void>::finished,
            this, &CppProjectUpdater::onProjectInfoGenerated);
}

CppProjectUpdater::~CppProjectUpdater()
{
    cancelAndWaitForFinished();
}

void CppProjectUpdater::update(const ProjectUpdateInfo &projectUpdateInfo)
{
    // Stop previous update.
    cancel();
    m_futureInterface.waitForFinished();
    m_futureInterface = QFutureInterface<void>();

    m_projectUpdateInfo = projectUpdateInfo;

    // Ensure that we do not operate on a deleted toolchain.
    using namespace ProjectExplorer;
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
            this, &CppProjectUpdater::onToolChainRemoved);

    // Run the project info generator in a worker thread and continue if that one is finished.
    const QFutureInterface<void> &futureInterface = m_futureInterface;
    const QFuture<ProjectInfo> future = Utils::runAsync([=]() {
        Internal::ProjectInfoGenerator generator(futureInterface, projectUpdateInfo);
        return generator.generate();
    });
    m_generateFutureWatcher.setFuture(future);
}

void CppProjectUpdater::cancel()
{
    disconnect(&m_generateFutureWatcher);
    m_futureInterface.cancel();
}

void CppProjectUpdater::cancelAndWaitForFinished()
{
    cancel();
    m_futureInterface.waitForFinished();
}

void CppProjectUpdater::onToolChainRemoved(ProjectExplorer::ToolChain *t)
{
    QTC_ASSERT(t, return);
    if (t == m_projectUpdateInfo.cToolChain || t == m_projectUpdateInfo.cxxToolChain)
        cancelAndWaitForFinished();
}

void CppProjectUpdater::onProjectInfoGenerated()
{
    // From now on we do not access the toolchain anymore, so disconnect.
    using namespace ProjectExplorer;
    disconnect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
               this, &CppProjectUpdater::onToolChainRemoved);

    if (m_futureInterface.isCanceled())
        return;

    QFuture<void> future = CppModelManager::instance()
            ->updateProjectInfo(m_futureInterface, m_generateFutureWatcher.result());
    QTC_CHECK(future != QFuture<void>());

    const ProjectInfo projectInfo = CppModelManager::instance()->projectInfo(m_project);
    QTC_CHECK(projectInfo.isValid());
    emit projectInfoUpdated(projectInfo);
}

} // namespace CppTools
