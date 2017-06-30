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

#include "vcsbasediffeditorcontroller.h"
#include "vcsbaseclient.h"
#include "vcscommand.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <diffeditor/diffutils.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>

#include <QPointer>

using namespace DiffEditor;
using namespace Core;

namespace VcsBase {

static void readPatch(QFutureInterface<QList<FileData>> &futureInterface,
                      const QString &patch)
{
    bool ok;
    const QList<FileData> &fileDataList = DiffUtils::readPatch(patch, &ok, &futureInterface);
    futureInterface.reportResult(fileDataList);
}

class VcsBaseDiffEditorControllerPrivate
{
public:
    VcsBaseDiffEditorControllerPrivate(VcsBaseDiffEditorController *controller,
                                       VcsBaseClientImpl *client,
                                       const QString &workingDirectory);
    ~VcsBaseDiffEditorControllerPrivate();

    void processingFinished();
    void processDiff(const QString &patch);
    void cancelReload();
    void commandFinished(bool success);

    VcsBaseDiffEditorController *q;
    VcsBaseClientImpl *m_client;
    const QString m_directory;
    QString m_startupFile;
    QString m_output;
    QPointer<VcsCommand> m_command;
    QFutureWatcher<QList<FileData>> m_processWatcher;
};

VcsBaseDiffEditorControllerPrivate::VcsBaseDiffEditorControllerPrivate(VcsBaseDiffEditorController *controller,
        VcsBaseClientImpl *client,
        const QString &workingDirectory)
    : q(controller)
    , m_client(client)
    , m_directory(workingDirectory)
{
    QObject::connect(&m_processWatcher, &QFutureWatcher<QList<FileData>>::finished, q,
                     [this] () { processingFinished(); });
}

VcsBaseDiffEditorControllerPrivate::~VcsBaseDiffEditorControllerPrivate()
{
    cancelReload();
}

void VcsBaseDiffEditorControllerPrivate::processingFinished()
{
    bool success = !m_processWatcher.future().isCanceled();
    const QList<FileData> fileDataList = success
            ? m_processWatcher.future().result() : QList<FileData>();

    q->setDiffFiles(fileDataList, q->workingDirectory(), q->startupFile());
    q->reloadFinished(success);
}

void VcsBaseDiffEditorControllerPrivate::processDiff(const QString &patch)
{
    m_processWatcher.setFuture(Utils::runAsync(&readPatch, patch));

    ProgressManager::addTask(m_processWatcher.future(),
                                   q->tr("Processing diff"), "DiffEditor");
}

void VcsBaseDiffEditorControllerPrivate::cancelReload()
{
    if (m_command) {
        m_command->disconnect();
        m_command->cancel();
        m_command.clear();
    }

    if (m_processWatcher.future().isRunning()) {
        m_processWatcher.future().cancel();
        m_processWatcher.setFuture(QFuture<QList<FileData>>());
    }
    m_output = QString();
}

void VcsBaseDiffEditorControllerPrivate::commandFinished(bool success)
{
    if (m_command)
        m_command.clear();

    if (!success) {
        cancelReload();
        q->reloadFinished(success);
        return;
    }

    q->processCommandOutput(m_output);
}

/////////////////////

VcsBaseDiffEditorController::VcsBaseDiffEditorController(IDocument *document,
                                                         VcsBaseClientImpl *client,
                                                         const QString &workingDirectory)
    : DiffEditorController(document)
    , d(new VcsBaseDiffEditorControllerPrivate(this, client, workingDirectory))
{
}

VcsBaseDiffEditorController::~VcsBaseDiffEditorController()
{
    delete d;
}

void VcsBaseDiffEditorController::runCommand(const QList<QStringList> &args, unsigned flags, QTextCodec *codec)
{
    d->cancelReload();

    d->m_command = new VcsCommand(workingDirectory(), d->m_client->processEnvironment());
    d->m_command->setCodec(codec ? codec : EditorManager::defaultTextCodec());
    connect(d->m_command.data(), &VcsCommand::stdOutText, this,
            [this] (const QString &output) { d->m_output = output; });
    connect(d->m_command.data(), &VcsCommand::finished, this,
            [this] (bool success) { d->commandFinished(success); });
    d->m_command->addFlags(flags);

    for (const QStringList &arg : args) {
        QTC_ASSERT(!arg.isEmpty(), continue);

        d->m_command->addJob(d->m_client->vcsBinary(), arg, d->m_client->vcsTimeoutS());
    }

    d->m_command->execute();
}

void VcsBaseDiffEditorController::processCommandOutput(const QString &output)
{
    d->processDiff(output);
}

VcsBaseClientImpl *VcsBaseDiffEditorController::client() const
{
    return d->m_client;
}

QString VcsBaseDiffEditorController::workingDirectory() const
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

} // namespace VcsBase
