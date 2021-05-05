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

#include <QFutureInterface>

namespace CppTools {

CppProjectUpdater::CppProjectUpdater()
{
    connect(&m_generateFutureWatcher,
            &QFutureWatcher<ProjectInfo>::finished,
            this,
            &CppProjectUpdater::onProjectInfoGenerated);
}

CppProjectUpdater::~CppProjectUpdater()
{
    cancelAndWaitForFinished();
}

void CppProjectUpdater::update(const ProjectExplorer::ProjectUpdateInfo &projectUpdateInfo)
{
    // Stop previous update.
    cancel();

    m_projectUpdateInfo = projectUpdateInfo;

    // Ensure that we do not operate on a deleted toolchain.
    using namespace ProjectExplorer;
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
            this, &CppProjectUpdater::onToolChainRemoved);

    // Run the project info generator in a worker thread and continue if that one is finished.
    m_generateFuture = Utils::runAsync([=](QFutureInterface<ProjectInfo> &futureInterface) {
        ProjectUpdateInfo fullProjectUpdateInfo = projectUpdateInfo;
        if (fullProjectUpdateInfo.rppGenerator)
            fullProjectUpdateInfo.rawProjectParts = fullProjectUpdateInfo.rppGenerator();
        Internal::ProjectInfoGenerator generator(futureInterface, fullProjectUpdateInfo);
        futureInterface.reportResult(generator.generate());
    });
    m_generateFutureWatcher.setFuture(m_generateFuture);
}

void CppProjectUpdater::cancel()
{
    m_generateFutureWatcher.setFuture({});
    m_generateFuture.cancel();
    m_updateFuture.cancel();
}

void CppProjectUpdater::cancelAndWaitForFinished()
{
    cancel();
    if (m_generateFuture.isRunning())
        m_generateFuture.waitForFinished();
    if (m_updateFuture.isRunning())
        m_updateFuture.waitForFinished();
}

void CppProjectUpdater::onToolChainRemoved(ProjectExplorer::ToolChain *t)
{
    QTC_ASSERT(t, return);
    if (t == m_projectUpdateInfo.cToolChain || t == m_projectUpdateInfo.cxxToolChain)
        cancel();
}

void CppProjectUpdater::onProjectInfoGenerated()
{
    // From now on we do not access the toolchain anymore, so disconnect.
    using namespace ProjectExplorer;
    disconnect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
               this, &CppProjectUpdater::onToolChainRemoved);

    if (m_generateFutureWatcher.isCanceled() || m_generateFutureWatcher.future().resultCount() < 1)
        return;

    m_updateFuture = CppModelManager::instance()->updateProjectInfo(
        m_generateFutureWatcher.result());
}

CppProjectUpdaterFactory::CppProjectUpdaterFactory()
{
    setObjectName("CppProjectUpdaterFactory");
}

CppProjectUpdaterInterface *CppProjectUpdaterFactory::create()
{
    return new CppProjectUpdater;
}

} // namespace CppTools
