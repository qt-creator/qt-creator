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

/////////////////////

// We need a way to disconnect from signals posted from different thread
// so that signals that are already posted from the other thread and not delivered
// yet will be ignored. Unfortunately, simple QObject::disconnect() doesn't
// work like that, since signals that are already posted and are awaiting
// to be delivered WILL BE delivered later, even after a call to QObject::disconnect().
// The delivery will happen when the control returns to the main event loop.

// This proxy class solves the above problem. Instead of a call to
// QObject::disconnect(), which would still deliver posted signals,
// we delete the proxy object immediately. In this way signals which are
// already posted and are awaiting to be delivered won't be delivered to the
// destroyed object.

// So the only reason for this proxy object is to be able to disconnect
// effectively from the signals posted from different threads.

class VcsCommandResultProxy : public QObject {
    Q_OBJECT
public:
    VcsCommandResultProxy(VcsCommand *command, VcsBaseDiffEditorControllerPrivate *target);
private:
    void storeOutput(const QString &output);
    void commandFinished(bool success);

    VcsBaseDiffEditorControllerPrivate *m_target;
};

/////////////////////

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
    void storeOutput(const QString &output);
    void commandFinished(bool success);

    VcsBaseDiffEditorController *q;
    VcsBaseClientImpl *m_client;
    const QString m_directory;
    QString m_startupFile;
    QString m_output;
    QPointer<VcsCommand> m_command;
    QPointer<VcsCommandResultProxy> m_commandResultProxy;
    QFutureWatcher<QList<FileData>> *m_processWatcher = nullptr;
};

/////////////////////

VcsCommandResultProxy::VcsCommandResultProxy(VcsCommand *command,
                          VcsBaseDiffEditorControllerPrivate *target)
    : QObject(target->q)
    , m_target(target)
{
    connect(command, &VcsCommand::stdOutText,
            this, &VcsCommandResultProxy::storeOutput);
    connect(command, &VcsCommand::finished,
            this, &VcsCommandResultProxy::commandFinished);
    connect(command, &VcsCommand::destroyed,
            this, &QObject::deleteLater);
}

void VcsCommandResultProxy::storeOutput(const QString &output)
{
    m_target->storeOutput(output);
}

void VcsCommandResultProxy::commandFinished(bool success)
{
    m_target->commandFinished(success);
}

VcsBaseDiffEditorControllerPrivate::VcsBaseDiffEditorControllerPrivate(
        VcsBaseDiffEditorController *controller,
        VcsBaseClientImpl *client,
        const QString &workingDirectory)
    : q(controller)
    , m_client(client)
    , m_directory(workingDirectory)
{
}

VcsBaseDiffEditorControllerPrivate::~VcsBaseDiffEditorControllerPrivate()
{
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

    QObject::connect(m_processWatcher, &QFutureWatcher<QList<FileData>>::finished,
                     [this] () { processingFinished(); } );

    m_processWatcher->setFuture(Utils::runAsync(&readPatch, patch));

    ProgressManager::addTask(m_processWatcher->future(),
                             VcsBaseDiffEditorController::tr("Processing diff"), "DiffEditor");
}

void VcsBaseDiffEditorControllerPrivate::cancelReload()
{
    if (m_command) {
        m_command->cancel();
        m_command.clear();
    }

    // Disconnect effectively, don't deliver already posted signals
    if (m_commandResultProxy)
        delete m_commandResultProxy.data();

    if (m_processWatcher) {
        // Cancel the running process without the further processingFinished()
        // notification for this process.
        m_processWatcher->future().cancel();
        delete m_processWatcher;
        m_processWatcher = nullptr;
    }

    m_output = QString();
}

void VcsBaseDiffEditorControllerPrivate::storeOutput(const QString &output)
{
    m_output = output;
}

void VcsBaseDiffEditorControllerPrivate::commandFinished(bool success)
{
    if (m_command)
        m_command.clear();

    // Prevent direct deletion of m_commandResultProxy inside the possible
    // subsequent synchronous calls to cancelReload() [called e.g. by
    // processCommandOutput() overload], since
    // commandFinished() is called directly by the m_commandResultProxy.
    // m_commandResultProxy is removed via deleteLater right after
    // a call to this commandFinished() is finished
    if (m_commandResultProxy)
        m_commandResultProxy.clear();

    if (!success) {
        cancelReload();
        q->reloadFinished(success);
        return;
    }

    q->processCommandOutput(QString(m_output)); // pass a copy of m_output
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
    // Cancel the possible ongoing reload without the commandFinished() nor
    // processingFinished() notifications, as right after that
    // we re-reload it from scratch. So no intermediate "Retrieving data failed."
    // and "Waiting for data..." will be shown.
    d->cancelReload();

    d->m_command = new VcsCommand(workingDirectory(), d->m_client->processEnvironment());
    d->m_command->setCodec(codec ? codec : EditorManager::defaultTextCodec());
    d->m_commandResultProxy = new VcsCommandResultProxy(d->m_command.data(), d);
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

#include "vcsbasediffeditorcontroller.moc"
