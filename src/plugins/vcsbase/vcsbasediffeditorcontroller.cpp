// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbasediffeditorcontroller.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/async.h>
#include <utils/environment.h>
#include <utils/futuresynchronizer.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

using namespace DiffEditor;
using namespace Tasking;
using namespace Utils;

namespace VcsBase {

class VcsBaseDiffEditorControllerPrivate
{
public:
    VcsBaseDiffEditorControllerPrivate(VcsBaseDiffEditorController *q) : q(q) {}

    VcsBaseDiffEditorController *q;
    Environment m_processEnvironment;
    FilePath m_vcsBinary;
    const TreeStorage<QString> m_inputStorage;
};

/////////////////////

VcsBaseDiffEditorController::VcsBaseDiffEditorController(Core::IDocument *document)
    : DiffEditorController(document)
    , d(new VcsBaseDiffEditorControllerPrivate(this))
{}

VcsBaseDiffEditorController::~VcsBaseDiffEditorController()
{
    delete d;
}

TreeStorage<QString> VcsBaseDiffEditorController::inputStorage() const
{
    return d->m_inputStorage;
}

GroupItem VcsBaseDiffEditorController::postProcessTask()
{
    const auto setupDiffProcessor = [this](Async<QList<FileData>> &async) {
        const QString *storage = inputStorage().activeStorage();
        QTC_ASSERT(storage, qWarning("Using postProcessTask() requires putting inputStorage() "
                                     "into task tree's root group."));
        const QString inputData = storage ? *storage : QString();
        async.setFutureSynchronizer(ExtensionSystem::PluginManager::futureSynchronizer());
        async.setConcurrentCallData(&DiffUtils::readPatchWithPromise, inputData);
    };
    const auto onDiffProcessorDone = [this](const Async<QList<FileData>> &async) {
        setDiffFiles(async.isResultAvailable() ? async.result() : QList<FileData>());
        // TODO: We should set the right starting line here
    };
    const auto onDiffProcessorError = [this](const Async<QList<FileData>> &) {
        setDiffFiles({});
    };
    return AsyncTask<QList<FileData>>(setupDiffProcessor, onDiffProcessorDone, onDiffProcessorError);
}

void VcsBaseDiffEditorController::setupCommand(Process &process, const QStringList &args) const
{
    process.setEnvironment(d->m_processEnvironment);
    process.setWorkingDirectory(workingDirectory());
    process.setCommand({d->m_vcsBinary, args});
    process.setUseCtrlCStub(true);
}

void VcsBaseDiffEditorController::setVcsBinary(const FilePath &path)
{
    d->m_vcsBinary = path;
}

void VcsBaseDiffEditorController::setProcessEnvironment(const Environment &value)
{
    d->m_processEnvironment = value;
}

} // namespace VcsBase
