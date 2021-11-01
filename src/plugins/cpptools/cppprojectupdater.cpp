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
#include "generatedcodemodelsupport.h"

#include <coreplugin/progressmanager/progressmanager.h>

#include <projectexplorer/toolchainmanager.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QFutureInterface>

using namespace ProjectExplorer;

namespace CppTools {

CppProjectUpdater::CppProjectUpdater()
{
    connect(&m_generateFutureWatcher,
            &QFutureWatcher<ProjectInfo>::finished,
            this,
            &CppProjectUpdater::onProjectInfoGenerated);
    m_futureSynchronizer.setCancelOnWait(true);
}

CppProjectUpdater::~CppProjectUpdater()
{
    cancel();
}

void CppProjectUpdater::update(const ProjectUpdateInfo &projectUpdateInfo)
{
    update(projectUpdateInfo, {});
}

void CppProjectUpdater::update(const ProjectUpdateInfo &projectUpdateInfo,
                               const QList<ProjectExplorer::ExtraCompiler *> &extraCompilers)
{
    // Stop previous update.
    cancel();

    m_extraCompilers = Utils::transform(extraCompilers, [](ExtraCompiler *compiler) {
        return QPointer<ExtraCompiler>(compiler);
    });
    m_projectUpdateInfo = projectUpdateInfo;

    // Ensure that we do not operate on a deleted toolchain.
    using namespace ProjectExplorer;
    connect(ToolChainManager::instance(), &ToolChainManager::toolChainRemoved,
            this, &CppProjectUpdater::onToolChainRemoved);

    // Run the project info generator in a worker thread and continue if that one is finished.
    auto generateFuture = Utils::runAsync([=](QFutureInterface<ProjectInfo> &futureInterface) {
        ProjectUpdateInfo fullProjectUpdateInfo = projectUpdateInfo;
        if (fullProjectUpdateInfo.rppGenerator)
            fullProjectUpdateInfo.rawProjectParts = fullProjectUpdateInfo.rppGenerator();
        Internal::ProjectInfoGenerator generator(futureInterface, fullProjectUpdateInfo);
        futureInterface.reportResult(generator.generate());
    });
    m_generateFutureWatcher.setFuture(generateFuture);
    m_futureSynchronizer.addFuture(generateFuture);

    // extra compilers
    for (QPointer<ExtraCompiler> compiler : qAsConst(m_extraCompilers)) {
        if (compiler->isDirty()) {
            QPointer<QFutureWatcher<void>> watcher = new QFutureWatcher<void>;
            // queued connection to delay after the extra compiler updated its result contents,
            // which is also done in the main thread when compiler->run() finished
            connect(watcher, &QFutureWatcherBase::finished,
                    this, [this, watcher] {
                        // In very unlikely case the CppProjectUpdater::cancel() could have been
                        // invoked after posting the finished() signal and before this handler
                        // gets called. In this case the watcher is already deleted.
                        if (!watcher)
                            return;
                        m_projectUpdateFutureInterface->setProgressValue(
                            m_projectUpdateFutureInterface->progressValue() + 1);
                        m_extraCompilersFutureWatchers.remove(watcher);
                        watcher->deleteLater();
                        if (!watcher->isCanceled())
                            checkForExtraCompilersFinished();
                    },
                    Qt::QueuedConnection);
            m_extraCompilersFutureWatchers += watcher;
            watcher->setFuture(QFuture<void>(compiler->run()));
            m_futureSynchronizer.addFuture(watcher->future());
        }
    }

    m_projectUpdateFutureInterface.reset(new QFutureInterface<void>);
    m_projectUpdateFutureInterface->setProgressRange(0, m_extraCompilersFutureWatchers.size()
                                                        + 1 /*generateFuture*/);
    m_projectUpdateFutureInterface->setProgressValue(0);
    m_projectUpdateFutureInterface->reportStarted();
    Core::ProgressManager::addTask(m_projectUpdateFutureInterface->future(),
                                   tr("Preparing C++ Code Model"),
                                   "CppProjectUpdater");
}

void CppProjectUpdater::cancel()
{
    if (m_projectUpdateFutureInterface && m_projectUpdateFutureInterface->isRunning())
        m_projectUpdateFutureInterface->reportFinished();
    m_generateFutureWatcher.setFuture({});
    m_isProjectInfoGenerated = false;
    qDeleteAll(m_extraCompilersFutureWatchers);
    m_extraCompilersFutureWatchers.clear();
    m_extraCompilers.clear();
    m_futureSynchronizer.cancelAllFutures();
}

void CppProjectUpdater::onToolChainRemoved(ToolChain *t)
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

    m_projectUpdateFutureInterface->setProgressValue(m_projectUpdateFutureInterface->progressValue()
                                                     + 1);
    m_isProjectInfoGenerated = true;
    checkForExtraCompilersFinished();
}

void CppProjectUpdater::checkForExtraCompilersFinished()
{
    if (!m_extraCompilersFutureWatchers.isEmpty() || !m_isProjectInfoGenerated)
        return; // still need to wait

    m_projectUpdateFutureInterface->reportFinished();
    m_projectUpdateFutureInterface.reset();

    QList<ExtraCompiler *> extraCompilers;
    QSet<QString> compilerFiles;
    for (const QPointer<ExtraCompiler> &compiler : qAsConst(m_extraCompilers)) {
        if (compiler) {
            extraCompilers += compiler.data();
            compilerFiles += Utils::transform<QSet>(compiler->targets(), &Utils::FilePath::toString);
        }
    }
    GeneratedCodeModelSupport::update(extraCompilers);
    m_extraCompilers.clear();

    auto updateFuture = CppModelManager::instance()
                            ->updateProjectInfo(m_generateFutureWatcher.result(), compilerFiles);
    m_futureSynchronizer.addFuture(updateFuture);
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
