// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "vcsbasediffeditorcontroller.h"
#include "vcsbaseclient.h"
#include "vcscommand.h"
#include "vcsplugin.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/asynctask.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/futuresynchronizer.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/runextensions.h>

#include <QPointer>

using namespace DiffEditor;
using namespace Core;
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
    ~VcsBaseDiffEditorControllerPrivate();

    void processingFinished();
    void processDiff(const QString &patch);
    void cancelReload();
    void commandFinished(bool success);

    VcsBaseDiffEditorController *q;
    FilePath m_directory;
    Environment m_processEnvironment;
    FilePath m_vcsBinary;
    int m_vscTimeoutS;
    QString m_startupFile;
    QPointer<VcsCommand> m_command;
    QFutureWatcher<QList<FileData>> *m_processWatcher = nullptr;
};

VcsBaseDiffEditorControllerPrivate::~VcsBaseDiffEditorControllerPrivate()
{
    delete m_command;
    cancelReload();
}

void VcsBaseDiffEditorControllerPrivate::processingFinished()
{
    QTC_ASSERT(m_processWatcher, return);

    // success is false when the user clicked the cancel micro button
    // inside the progress indicator
    const bool success = !m_processWatcher->future().isCanceled();
    const QList<FileData> fileDataList = success
            ? m_processWatcher->future().result() : QList<FileData>();

    // Prevent direct deletion of m_processWatcher since
    // processingFinished() is called directly by the m_processWatcher.
    m_processWatcher->deleteLater();
    m_processWatcher = nullptr;

    q->setDiffFiles(fileDataList, q->workingDirectory(), q->startupFile());
    q->reloadFinished(success);
}

void VcsBaseDiffEditorControllerPrivate::processDiff(const QString &patch)
{
    cancelReload();

    m_processWatcher = new QFutureWatcher<QList<FileData>>();

    QObject::connect(m_processWatcher, &QFutureWatcherBase::finished,
                     q, [this] { processingFinished(); });

    m_processWatcher->setFuture(Utils::runAsync(&readPatch, patch));

    ProgressManager::addTask(m_processWatcher->future(),
                             VcsBaseDiffEditorController::tr("Processing diff"), "DiffEditor");
}

void VcsBaseDiffEditorControllerPrivate::cancelReload()
{
    m_command.clear();

    if (m_processWatcher) {
        // Cancel the running process without the further processingFinished()
        // notification for this process.
        m_processWatcher->future().cancel();
        delete m_processWatcher;
        m_processWatcher = nullptr;
    }
}

void VcsBaseDiffEditorControllerPrivate::commandFinished(bool success)
{
    const QString output = m_command->cleanedStdOut();
    // Don't delete here, as it is called from command finished signal.
    // Clear it only, as we may call runCommand() again from inside processCommandOutput overload.
    m_command.clear();

    if (!success) {
        cancelReload();
        q->reloadFinished(success);
        return;
    }

    q->processCommandOutput(output);
}

/////////////////////

VcsBaseDiffEditorController::VcsBaseDiffEditorController(Core::IDocument *document)
    : DiffEditorController(document)
    , d(new VcsBaseDiffEditorControllerPrivate(this))
{}

VcsBaseDiffEditorController::~VcsBaseDiffEditorController()
{
    delete d;
}

void VcsBaseDiffEditorController::setupCommand(QtcProcess &process, const QStringList &args) const
{
    process.setEnvironment(d->m_processEnvironment);
    process.setWorkingDirectory(workingDirectory());
    process.setCommand({d->m_vcsBinary, args});
}

void VcsBaseDiffEditorController::setupDiffProcessor(AsyncTask<QList<FileData>> &processor,
                                                     const QString &patch) const
{
    processor.setAsyncCallData(readPatch, patch);
    processor.setFutureSynchronizer(Internal::VcsPlugin::futureSynchronizer());
}

void VcsBaseDiffEditorController::runCommand(const QList<QStringList> &args, RunFlags flags, QTextCodec *codec)
{
    // Cancel the possible ongoing reload without the commandFinished() nor
    // processingFinished() notifications, as right after that
    // we re-reload it from scratch. So no intermediate "Retrieving data failed."
    // and "Waiting for data..." will be shown.
    delete d->m_command;
    d->cancelReload();

    d->m_command = VcsBaseClient::createVcsCommand(workingDirectory(), d->m_processEnvironment);
    d->m_command->setDisplayName(displayName());
    d->m_command->setCodec(codec ? codec : EditorManager::defaultTextCodec());
    connect(d->m_command.data(), &VcsCommand::done, this, [this] {
        d->commandFinished(d->m_command->result() == ProcessResult::FinishedWithSuccess);
    });
    d->m_command->addFlags(flags);

    for (const QStringList &arg : args) {
        QTC_ASSERT(!arg.isEmpty(), continue);

        d->m_command->addJob({d->m_vcsBinary, arg}, d->m_vscTimeoutS);
    }

    d->m_command->start();
}

void VcsBaseDiffEditorController::processCommandOutput(const QString &output)
{
    d->processDiff(output);
}

FilePath VcsBaseDiffEditorController::workingDirectory() const
{
    return d->m_directory;
}

void VcsBaseDiffEditorController::setStartupFile(const QString &startupFile)
{
    d->m_startupFile = startupFile;
}

QString VcsBaseDiffEditorController::startupFile() const
{
    return d->m_startupFile;
}

void VcsBase::VcsBaseDiffEditorController::setWorkingDirectory(const FilePath &workingDir)
{
    d->m_directory = workingDir;
    setBaseDirectory(workingDir);
}

void VcsBaseDiffEditorController::setVcsTimeoutS(int value)
{
    d->m_vscTimeoutS = value;
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
