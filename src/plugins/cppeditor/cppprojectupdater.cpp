// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppprojectupdater.h"

#include "cppeditortr.h"
#include "cppmodelmanager.h"
#include "cppprojectinfogenerator.h"
#include "generatedcodemodelsupport.h"

#include <coreplugin/progressmanager/taskprogress.h>

#include <projectexplorer/extracompiler.h>

#include <utils/algorithm.h>
#include <utils/asynctask.h>
#include <utils/qtcassert.h>

#include <QFutureInterface>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

CppProjectUpdater::CppProjectUpdater()
{
    m_futureSynchronizer.setCancelOnWait(true);
}

CppProjectUpdater::~CppProjectUpdater() = default;

void CppProjectUpdater::update(const ProjectUpdateInfo &projectUpdateInfo)
{
    update(projectUpdateInfo, {});
}

void CppProjectUpdater::update(const ProjectUpdateInfo &projectUpdateInfo,
                               const QList<ProjectExplorer::ExtraCompiler *> &extraCompilers)
{
    // Stop previous update.
    cancel();

    const QList<QPointer<ProjectExplorer::ExtraCompiler>> compilers =
        Utils::transform(extraCompilers, [](ExtraCompiler *compiler) {
            return QPointer<ExtraCompiler>(compiler);
        });

    using namespace ProjectExplorer;

    // Run the project info generator in a worker thread and continue if that one is finished.
    const auto infoGenerator = [=](QFutureInterface<ProjectInfo::ConstPtr> &futureInterface) {
        ProjectUpdateInfo fullProjectUpdateInfo = projectUpdateInfo;
        if (fullProjectUpdateInfo.rppGenerator)
            fullProjectUpdateInfo.rawProjectParts = fullProjectUpdateInfo.rppGenerator();
        Internal::ProjectInfoGenerator generator(futureInterface, fullProjectUpdateInfo);
        futureInterface.reportResult(generator.generate());
    };

    using namespace Tasking;
    struct UpdateStorage {
        ProjectInfo::ConstPtr projectInfo = nullptr;
    };
    const TreeStorage<UpdateStorage> storage;
    const auto setupInfoGenerator = [=](AsyncTask<ProjectInfo::ConstPtr> &async) {
        async.setAsyncCallData(infoGenerator);
        async.setFutureSynchronizer(&m_futureSynchronizer);
    };
    const auto onInfoGeneratorDone = [=](const AsyncTask<ProjectInfo::ConstPtr> &async) {
        if (async.isResultAvailable())
            storage->projectInfo = async.result();
    };
    QList<TaskItem> tasks{parallel};
    tasks.append(Async<ProjectInfo::ConstPtr>(setupInfoGenerator, onInfoGeneratorDone));
    for (QPointer<ExtraCompiler> compiler : compilers) {
        if (compiler && compiler->isDirty())
            tasks.append(compiler->compileFileItem());
    }

    const auto onDone = [this, storage, compilers] {
        QList<ExtraCompiler *> extraCompilers;
        QSet<FilePath> compilerFiles;
        for (const QPointer<ExtraCompiler> &compiler : compilers) {
            if (compiler) {
                extraCompilers += compiler.data();
                compilerFiles += Utils::toSet(compiler->targets());
            }
        }
        GeneratedCodeModelSupport::update(extraCompilers);
        auto updateFuture = CppModelManager::instance()->updateProjectInfo(storage->projectInfo,
                                                                           compilerFiles);
        m_futureSynchronizer.addFuture(updateFuture);
        m_taskTree.release()->deleteLater();
    };
    const auto onError = [this] {
        m_taskTree.release()->deleteLater();
    };

    const Group root {
        Storage(storage),
        Group(tasks),
        OnGroupDone(onDone),
        OnGroupError(onError)
    };
    m_taskTree.reset(new TaskTree(root));
    auto progress = new Core::TaskProgress(m_taskTree.get());
    progress->setDisplayName(Tr::tr("Preparing C++ Code Model"));
    m_taskTree->start();
}

void CppProjectUpdater::cancel()
{
    m_taskTree.reset();
    m_futureSynchronizer.cancelAllFutures();
}

namespace Internal {
CppProjectUpdaterFactory::CppProjectUpdaterFactory()
{
    setObjectName("CppProjectUpdaterFactory");
}

CppProjectUpdaterInterface *CppProjectUpdaterFactory::create()
{
    return new CppProjectUpdater;
}
} // namespace Internal

} // namespace CppEditor
