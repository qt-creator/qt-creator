/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "vcsbaseclient.h"
#include "vcsjobrunner.h"
#include "vcsbaseclientsettings.h"
#include "vcsbaseeditorparameterwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/vcsmanager.h>

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QtCore/QStringList>
#include <QtCore/QSharedPointer>
#include <QtCore/QDir>
#include <QtCore/QProcess>
#include <QtCore/QTextCodec>
#include <QtCore/QtDebug>
#include <QtCore/QFileInfo>
#include <QtCore/QByteArray>
#include <QtCore/QMetaType>

/*!
    \class VCSBase::VCSBaseClient

    \brief Base class for Mercurial and Bazaar 'clients'.

    Provides base functionality for common commands (diff, log, etc).

    \sa VCSBase::VCSJobRunner
*/

Q_DECLARE_METATYPE(QVariant)

inline Core::IEditor *locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->file()->property(property).toString() == entry)
            return ed;
    return 0;
}

namespace VCSBase {

class VCSBaseClientPrivate
{
public:
    VCSBaseClientPrivate(VCSBaseClient *client, VCSBaseClientSettings *settings);

    void statusParser(QByteArray data);
    void annotateRevision(QString source, QString change, int lineNumber);
    void saveSettings();

    void updateJobRunnerSettings();

    VCSJobRunner *m_jobManager;
    Core::ICore *m_core;
    VCSBaseClientSettings *m_clientSettings;

private:
    VCSBaseClient *m_client;
};

VCSBaseClientPrivate::VCSBaseClientPrivate(VCSBaseClient *client, VCSBaseClientSettings *settings) :
    m_jobManager(0), m_core(Core::ICore::instance()), m_clientSettings(settings), m_client(client)
{
}

void VCSBaseClientPrivate::statusParser(QByteArray data)
{
    QList<VCSBaseClient::StatusItem> lineInfoList;

    QStringList rawStatusList = QTextCodec::codecForLocale()->toUnicode(data).split(QLatin1Char('\n'));

    foreach (const QString &string, rawStatusList) {
        const VCSBaseClient::StatusItem lineInfo = m_client->parseStatusLine(string);
        if (!lineInfo.flags.isEmpty() && !lineInfo.file.isEmpty())
            lineInfoList.append(lineInfo);
    }

    emit m_client->parsedStatus(lineInfoList);
}

void VCSBaseClientPrivate::annotateRevision(QString source, QString change, int lineNumber)
{
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = change.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        change.truncate(blankPos);
    const QFileInfo fi(source);
    m_client->annotate(fi.absolutePath(), fi.fileName(), change, lineNumber);
}

void VCSBaseClientPrivate::saveSettings()
{
    m_clientSettings->writeSettings(m_core->settings());
}

void VCSBaseClientPrivate::updateJobRunnerSettings()
{
    if (m_jobManager && m_clientSettings) {
        m_jobManager->setBinary(m_clientSettings->stringValue(VCSBaseClientSettings::binaryPathKey));
        m_jobManager->setTimeoutMs(m_clientSettings->intValue(VCSBaseClientSettings::timeoutKey) * 1000);
    }
}

VCSBaseClient::StatusItem::StatusItem()
{
}

VCSBaseClient::StatusItem::StatusItem(const QString &s, const QString &f) :
    flags(s), file(f)
{
}

VCSBaseClient::VCSBaseClient(VCSBaseClientSettings *settings) :
    d(new VCSBaseClientPrivate(this, settings))
{
    qRegisterMetaType<QVariant>();
    connect(d->m_core, SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
}

VCSBaseClient::~VCSBaseClient()
{
    delete d->m_jobManager;
    d->m_jobManager = 0;
    delete d;
}

bool VCSBaseClient::synchronousCreateRepository(const QString &workingDirectory,
                                                const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(CreateRepositoryCommand));
    args << extraOptions;
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return false;
    QString output = QString::fromLocal8Bit(outputData);
    output.remove(QLatin1Char('\r'));
    VCSBase::VCSBaseOutputWindow::instance()->append(output);

    resetCachedVcsInfo(workingDirectory);

    return true;
}

bool VCSBaseClient::synchronousClone(const QString &workingDir,
                                     const QString &srcLocation,
                                     const QString &dstLocation,
                                     const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(CloneCommand)
         << extraOptions << srcLocation << dstLocation;
    QByteArray stdOut;
    const bool cloneOk = vcsFullySynchronousExec(workingDir, args, &stdOut);
    resetCachedVcsInfo(workingDir);
    return cloneOk;
}

bool VCSBaseClient::synchronousAdd(const QString &workingDir, const QString &filename,
                                   const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(AddCommand) << extraOptions << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousRemove(const QString &workingDir, const QString &filename,
                                      const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(RemoveCommand) << extraOptions << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousMove(const QString &workingDir,
                                    const QString &from, const QString &to,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(MoveCommand) << extraOptions << from << to;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousPull(const QString &workingDir,
                                    const QString &srcLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions << srcLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VCSBase::VCSBasePlugin::SshPasswordPrompt
            | VCSBase::VCSBasePlugin::ShowStdOutInLogWindow
            | VCSBase::VCSBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = vcsSynchronousExec(workingDir, args, flags);
    const bool ok = resp.result == Utils::SynchronousProcessResponse::Finished;
    if (ok)
        emit changed(QVariant(workingDir));
    return ok;
}

bool VCSBaseClient::synchronousPush(const QString &workingDir,
                                    const QString &dstLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PushCommand) << extraOptions << dstLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VCSBase::VCSBasePlugin::SshPasswordPrompt
            | VCSBase::VCSBasePlugin::ShowStdOutInLogWindow
            | VCSBase::VCSBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = vcsSynchronousExec(workingDir, args, flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

bool VCSBaseClient::vcsFullySynchronousExec(const QString &workingDir,
                                            const QStringList &args,
                                            QByteArray *output)
{
    QProcess vcsProcess;
    if (!workingDir.isEmpty())
        vcsProcess.setWorkingDirectory(workingDir);
    VCSJobRunner::setProcessEnvironment(&vcsProcess);

    const QString binary = settings()->stringValue(VCSBaseClientSettings::binaryPathKey);

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    outputWindow->appendCommand(workingDir, binary, args);

    vcsProcess.start(binary, args);

    if (!vcsProcess.waitForStarted()) {
        outputWindow->appendError(VCSJobRunner::msgStartFailed(binary, vcsProcess.errorString()));
        return false;
    }

    vcsProcess.closeWriteChannel();

    QByteArray stdErr;
    const int timeoutSec = settings()->intValue(VCSBaseClientSettings::timeoutKey);
    if (!Utils::SynchronousProcess::readDataFromProcess(vcsProcess, timeoutSec * 1000,
                                                        output, &stdErr, true)) {
        Utils::SynchronousProcess::stopProcess(vcsProcess);
        outputWindow->appendError(VCSJobRunner::msgTimeout(binary, timeoutSec));
        return false;
    }
    if (!stdErr.isEmpty())
        outputWindow->append(QString::fromLocal8Bit(stdErr));

    return vcsProcess.exitStatus() == QProcess::NormalExit && vcsProcess.exitCode() == 0;
}

Utils::SynchronousProcessResponse VCSBaseClient::vcsSynchronousExec(
    const QString &workingDirectory,
    const QStringList &args,
    unsigned flags,
    QTextCodec *outputCodec)
{
    const QString binary = settings()->stringValue(VCSBaseClientSettings::binaryPathKey);
    const int timeoutSec = settings()->intValue(VCSBaseClientSettings::timeoutKey);
    return VCSBase::VCSBasePlugin::runVCS(workingDirectory, binary, args,
                                          timeoutSec * 1000, flags, outputCodec);
}

void VCSBaseClient::annotate(const QString &workingDir, const QString &file,
                             const QString revision /* = QString() */,
                             int lineNumber /* = -1 */,
                             const QStringList &extraOptions)
{
    Q_UNUSED(lineNumber)
    const QString vcsCmdString = vcsCommandString(AnnotateCommand);
    QStringList args;
    args << vcsCmdString << revisionSpec(revision) << extraOptions << file;
    const QString kind = vcsEditorKind(AnnotateCommand);
    const QString id = VCSBase::VCSBaseEditorWidget::getSource(workingDir, QStringList(file));
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, file);

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);

    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::diff(const QString &workingDir, const QStringList &files,
                         const QStringList &extraOptions)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const QString kind = vcsEditorKind(DiffCommand);
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, files);
    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);
    editor->setRevertDiffChunkEnabled(true);
    editor->setDiffBaseDirectory(workingDir);

    VCSBaseEditorParameterWidget *paramWidget = createDiffEditor(workingDir, files, extraOptions);
    if (paramWidget != 0) {
        connect(editor, SIGNAL(diffChunkReverted(VCSBase::DiffChunk)),
                paramWidget, SLOT(executeCommand()));
        editor->setConfigurationWidget(paramWidget);
    }

    QStringList args;
    const QStringList paramArgs = paramWidget != 0 ? paramWidget->arguments() : QStringList();
    args << vcsCmdString << extraOptions << paramArgs << files;
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::log(const QString &workingDir, const QStringList &files,
                        const QStringList &extraOptions,
                        bool enableAnnotationContextMenu)
{
    const QString vcsCmdString = vcsCommandString(LogCommand);
    const QString kind = vcsEditorKind(LogCommand);
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, files);

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    VCSBaseEditorParameterWidget *paramWidget = createLogEditor(workingDir, files, extraOptions);
    if (paramWidget != 0)
        editor->setConfigurationWidget(paramWidget);

    QStringList args;
    const QStringList paramArgs = paramWidget != 0 ? paramWidget->arguments() : QStringList();
    args << vcsCmdString << extraOptions << paramArgs << files;
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::revertFile(const QString &workingDir,
                               const QString &file,
                               const QString &revision,
                               const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions << file;
    // Indicate repository change or file list
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args));
    job->setCookie(QStringList(workingDir + QLatin1Char('/') + file));
    connect(job.data(), SIGNAL(succeeded(QVariant)),
            this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::revertAll(const QString &workingDir, const QString &revision,
                              const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions;
    // Indicate repository change or file list
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args));
    connect(job.data(), SIGNAL(succeeded(QVariant)),
            this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::status(const QString &workingDir, const QString &file,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions << file;
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    outwin->setRepository(workingDir);
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args));
    connect(job.data(), SIGNAL(succeeded(QVariant)),
            outwin, SLOT(clearRepository()), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::emitParsedStatus(const QString &repository, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions;
    QSharedPointer<VCSJob> job(new VCSJob(repository, args, VCSJob::RawDataEmitMode));
    connect(job.data(), SIGNAL(rawData(QByteArray)), this, SLOT(statusParser(QByteArray)));
    enqueueJob(job);
}

QString VCSBaseClient::vcsCommandString(VCSCommand cmd) const
{
    switch (cmd) {
    case CreateRepositoryCommand: return QLatin1String("init");
    case CloneCommand: return QLatin1String("clone");
    case AddCommand: return QLatin1String("add");
    case RemoveCommand: return QLatin1String("remove");
    case MoveCommand: return QLatin1String("rename");
    case PullCommand: return QLatin1String("pull");
    case PushCommand: return QLatin1String("push");
    case CommitCommand: return QLatin1String("commit");
    case ImportCommand: return QLatin1String("import");
    case UpdateCommand: return QLatin1String("update");
    case RevertCommand: return QLatin1String("revert");
    case AnnotateCommand: return QLatin1String("annotate");
    case DiffCommand: return QLatin1String("diff");
    case LogCommand: return QLatin1String("log");
    case StatusCommand: return QLatin1String("status");
    }
    return QString();
}

void VCSBaseClient::import(const QString &repositoryRoot, const QStringList &files,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(ImportCommand));
    args << extraOptions << files;
    QSharedPointer<VCSJob> job(new VCSJob(repositoryRoot, args));
    enqueueJob(job);
}

void VCSBaseClient::view(const QString &source, const QString &id,
                         const QStringList &extraOptions)
{
    QStringList args;
    args << extraOptions << revisionSpec(id);
    const QString kind = vcsEditorKind(DiffCommand);
    const QString title = vcsEditorTitle(vcsCommandString(LogCommand), id);

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source,
                                                           true, "view", id);

    const QFileInfo fi(source);
    const QString workingDirPath = fi.isFile() ? fi.absolutePath() : source;
    QSharedPointer<VCSJob> job(new VCSJob(workingDirPath, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::update(const QString &repositoryRoot, const QString &revision,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(UpdateCommand));
    args << revisionSpec(revision) << extraOptions;
    QSharedPointer<VCSJob> job(new VCSJob(repositoryRoot, args));
    job->setCookie(repositoryRoot);
    // Suppress SSH prompting
    job->setUnixTerminalDisabled(VCSBase::VCSBasePlugin::isSshPromptConfigured());
    connect(job.data(), SIGNAL(succeeded(QVariant)), this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::commit(const QString &repositoryRoot,
                           const QStringList &files,
                           const QString &commitMessageFile,
                           const QStringList &extraOptions)
{
    // Handling of commitMessageFile is a bit tricky :
    //   VCSBaseClient cannot do something with it because it doesn't know which
    //   option to use (-F ? but sub VCS clients might require a different option
    //   name like -l for hg ...)
    //
    //   So descendants of VCSBaseClient *must* redefine commit() and extend
    //   extraOptions with the usage for commitMessageFile (see BazaarClient::commit()
    //   for example)
    Q_UNUSED(commitMessageFile);
    QStringList args(vcsCommandString(CommitCommand));
    args << extraOptions << files;
    QSharedPointer<VCSJob> job(new VCSJob(repositoryRoot, args));
    enqueueJob(job);
}

VCSBaseClientSettings *VCSBaseClient::settings() const
{
    return d->m_clientSettings;
}

void VCSBaseClient::handleSettingsChanged()
{
    if (d->m_jobManager) {
        d->updateJobRunnerSettings();
        d->m_jobManager->restart();
    }
}

VCSBaseEditorParameterWidget *VCSBaseClient::createDiffEditor(const QString &workingDir,
                                                              const QStringList &files,
                                                              const QStringList &extraOptions)
{
    Q_UNUSED(workingDir);
    Q_UNUSED(files);
    Q_UNUSED(extraOptions);
    return 0;
}

VCSBaseEditorParameterWidget *VCSBaseClient::createLogEditor(const QString &workingDir,
                                                             const QStringList &files,
                                                             const QStringList &extraOptions)
{
    Q_UNUSED(workingDir);
    Q_UNUSED(files);
    Q_UNUSED(extraOptions);
    return 0;
}

QString VCSBaseClient::vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const
{
    const QString binary = settings()->stringValue(VCSBaseClientSettings::binaryPathKey);
    return QFileInfo(binary).baseName() +
            QLatin1Char(' ') + vcsCmd + QLatin1Char(' ') +
            QFileInfo(sourceId).fileName();
}

VCSBase::VCSBaseEditorWidget *VCSBaseClient::createVCSEditor(const QString &kind, QString title,
                                                             const QString &source, bool setSourceCodec,
                                                             const char *registerDynamicProperty,
                                                             const QString &dynamicPropertyValue) const
{
    VCSBase::VCSBaseEditorWidget *baseEditor = 0;
    Core::IEditor *outputEditor = locateEditor(d->m_core, registerDynamicProperty, dynamicPropertyValue);
    const QString progressMsg = tr("Working...");
    if (outputEditor) {
        // Exists already
        outputEditor->createNew(progressMsg);
        baseEditor = VCSBase::VCSBaseEditorWidget::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return 0);
    } else {
        outputEditor = d->m_core->editorManager()->openEditorWithContents(kind, &title, progressMsg);
        outputEditor->file()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        baseEditor = VCSBase::VCSBaseEditorWidget::getVcsBaseEditor(outputEditor);
        connect(baseEditor, SIGNAL(annotateRevisionRequested(QString,QString,int)),
                this, SLOT(annotateRevision(QString,QString,int)));
        QTC_ASSERT(baseEditor, return 0);
        baseEditor->setSource(source);
        if (setSourceCodec)
            baseEditor->setCodec(VCSBase::VCSBaseEditorWidget::getCodec(source));
    }

    baseEditor->setForceReadOnly(true);
    d->m_core->editorManager()->activateEditor(outputEditor, Core::EditorManager::ModeSwitch);
    return baseEditor;
}

void VCSBaseClient::resetCachedVcsInfo(const QString &workingDir)
{
    Core::VcsManager *vcsManager = d->m_core->vcsManager();
    vcsManager->resetVersionControlForDirectory(workingDir);
}

void VCSBaseClient::enqueueJob(const QSharedPointer<VCSJob> &job)
{
    if (!d->m_jobManager) {
        d->m_jobManager = new VCSJobRunner();
        d->updateJobRunnerSettings();
        d->m_jobManager->start();
    }
    d->m_jobManager->enqueueJob(job);
}

} // namespace VCSBase

#include "moc_vcsbaseclient.cpp"
