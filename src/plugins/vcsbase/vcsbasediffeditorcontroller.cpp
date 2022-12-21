// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbasediffeditorcontroller.h"
#include "vcsplugin.h"

#include <utils/asynctask.h>
#include <utils/environment.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

using namespace DiffEditor;
using namespace Utils;

namespace VcsBase {

static void readPatch(QFutureInterface<QList<FileData>> &futureInterface, const QString &patch)
{
    bool ok;
    const QList<FileData> &fileDataList = DiffUtils::readPatch(patch, &ok, &futureInterface);
    futureInterface.reportResult(fileDataList);
}

/////////////////////

class VcsBaseDiffEditorControllerPrivate
{
public:
    VcsBaseDiffEditorControllerPrivate(VcsBaseDiffEditorController *q) : q(q) {}

    VcsBaseDiffEditorController *q;
    Environment m_processEnvironment;
    FilePath m_vcsBinary;
    const Tasking::TreeStorage<QString> m_inputStorage;
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

Tasking::TreeStorage<QString> VcsBaseDiffEditorController::inputStorage() const
{
    return d->m_inputStorage;
}

Tasking::TaskItem VcsBaseDiffEditorController::postProcessTask()
{
    using namespace Tasking;

    const auto setupDiffProcessor = [this](AsyncTask<QList<FileData>> &async) {
        const QString *storage = inputStorage().activeStorage();
        QTC_ASSERT(storage, qWarning("Using postProcessTask() requires putting inputStorage() "
                                     "into task tree's root group."));
        const QString inputData = storage ? *storage : QString();
        async.setAsyncCallData(readPatch, inputData);
        async.setFutureSynchronizer(Internal::VcsPlugin::futureSynchronizer());
    };
    const auto onDiffProcessorDone = [this](const AsyncTask<QList<FileData>> &async) {
        setDiffFiles(async.result());
    };
    const auto onDiffProcessorError = [this](const AsyncTask<QList<FileData>> &) {
        setDiffFiles({});
    };
    return Async<QList<FileData>>(setupDiffProcessor, onDiffProcessorDone, onDiffProcessorError);
}

void VcsBaseDiffEditorController::setupCommand(QtcProcess &process, const QStringList &args) const
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
