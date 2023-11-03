// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppprojectupdater.h"

#include "cppeditortr.h"
#include "cppmodelmanager.h"
#include "cppprojectinfogenerator.h"
#include "generatedcodemodelsupport.h"

#include <coreplugin/progressmanager/taskprogress.h>

#include <projectexplorer/extracompiler.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

CppProjectUpdater::CppProjectUpdater() = default;
CppProjectUpdater::~CppProjectUpdater() = default;

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
    const auto infoGenerator = [=](QPromise<ProjectInfo::ConstPtr> &promise) {
        ProjectUpdateInfo fullProjectUpdateInfo = projectUpdateInfo;
        if (fullProjectUpdateInfo.rppGenerator)
            fullProjectUpdateInfo.rawProjectParts = fullProjectUpdateInfo.rppGenerator();
        Internal::ProjectInfoGenerator generator(fullProjectUpdateInfo);
        promise.addResult(generator.generate(promise));
    };

    using namespace Tasking;
    struct UpdateStorage {
        ProjectInfo::ConstPtr projectInfo = nullptr;
    };
    const TreeStorage<UpdateStorage> storage;
    const auto onInfoGeneratorSetup = [=](Async<ProjectInfo::ConstPtr> &async) {
        async.setConcurrentCallData(infoGenerator);
        async.setFutureSynchronizer(&m_futureSynchronizer);
    };
    const auto onInfoGeneratorDone = [=](const Async<ProjectInfo::ConstPtr> &async) {
        if (async.isResultAvailable())
            storage->projectInfo = async.result();
    };
    QList<GroupItem> tasks{parallel};
    tasks.append(AsyncTask<ProjectInfo::ConstPtr>(onInfoGeneratorSetup, onInfoGeneratorDone,
                                                  CallDoneIf::Success));
    for (QPointer<ExtraCompiler> compiler : compilers) {
        if (compiler && compiler->isDirty())
            tasks.append(compiler->compileFileItem());
    }

    const auto onDone = [this, storage, compilers](DoneWith result) {
        m_taskTree.release()->deleteLater();
        if (result != DoneWith::Success)
            return;
        QList<ExtraCompiler *> extraCompilers;
        QSet<FilePath> compilerFiles;
        for (const QPointer<ExtraCompiler> &compiler : compilers) {
            if (compiler) {
                extraCompilers += compiler.data();
                compilerFiles += Utils::toSet(compiler->targets());
            }
        }
        GeneratedCodeModelSupport::update(extraCompilers);
        auto updateFuture = CppModelManager::updateProjectInfo(storage->projectInfo, compilerFiles);
        m_futureSynchronizer.addFuture(updateFuture);
    };

    const Group root {
        Tasking::Storage(storage),
        Group(tasks),
        onGroupDone(onDone)
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
    setLanguage(Constants::CXX_LANGUAGE_ID);
    setCreator([] { return new CppProjectUpdater; });
}

} // namespace Internal

} // namespace CppEditor
