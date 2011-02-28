/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Brian McGillion & Hugues Delorme
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "vcsbaseclient.h"
#include "vcsjobrunner.h"
#include "vcsbaseclientsettings.h"

#include <QtDebug>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>

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

Q_DECLARE_METATYPE(QVariant)

inline Core::IEditor *locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->file()->property(property).toString() == entry)
            return ed;
    return 0;
}

namespace VCSBase {

VCSBaseClient::VCSBaseClient(const VCSBaseClientSettings &settings) :
    m_jobManager(0),
    m_core(Core::ICore::instance()),
    m_clientSettings(settings)
{
    qRegisterMetaType<QVariant>();
}

VCSBaseClient::~VCSBaseClient()
{
    if (m_jobManager) {
        delete m_jobManager;
        m_jobManager = 0;
    }
}

bool VCSBaseClient::synchronousCreateRepository(const QString &workingDirectory)
{
    const QStringList args(vcsCommandString(CreateRepositoryCommand));
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return false;
    QString output = QString::fromLocal8Bit(outputData);
    output.remove(QLatin1Char('\r'));
    VCSBase::VCSBaseOutputWindow::instance()->append(output);
    return true;
}

bool VCSBaseClient::synchronousClone(const QString &workingDir,
                                     const QString &srcLocation,
                                     const QString &dstLocation,
                                     const ExtraCommandOptions &extraOptions)
{
    QStringList args;
    args << vcsCommandString(CloneCommand)
         << cloneArguments(srcLocation, dstLocation, extraOptions);
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousAdd(const QString &workingDir, const QString &filename)
{
    QStringList args;
    args << vcsCommandString(AddCommand) << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousRemove(const QString &workingDir, const QString &filename)
{
    QStringList args;
    args << vcsCommandString(RemoveCommand) << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousMove(const QString &workingDir,
                                    const QString &from, const QString &to)
{
    QStringList args;
    args << vcsCommandString(MoveCommand) << from << to;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VCSBaseClient::synchronousPull(const QString &workingDir,
                                    const QString &srcLocation,
                                    const ExtraCommandOptions &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << pullArguments(srcLocation, extraOptions);
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
                                    const ExtraCommandOptions &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PushCommand) << pushArguments(dstLocation, extraOptions);
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

    const QString binary = m_clientSettings.binary();
    const QStringList arguments = m_clientSettings.standardArguments() + args;

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    outputWindow->appendCommand(workingDir, binary, args);

    vcsProcess.start(binary, arguments);

    if (!vcsProcess.waitForStarted()) {
        outputWindow->appendError(VCSJobRunner::msgStartFailed(binary, vcsProcess.errorString()));
        return false;
    }

    vcsProcess.closeWriteChannel();

    QByteArray stdErr;
    if (!Utils::SynchronousProcess::readDataFromProcess(vcsProcess, m_clientSettings.timeoutMilliSeconds(),
                                                        output, &stdErr, true)) {
        Utils::SynchronousProcess::stopProcess(vcsProcess);
        outputWindow->appendError(VCSJobRunner::msgTimeout(m_clientSettings.timeoutSeconds()));
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
    const QString binary = m_clientSettings.binary();
    const QStringList arguments = m_clientSettings.standardArguments() + args;
    return VCSBase::VCSBasePlugin::runVCS(workingDirectory, binary, arguments,
                                          m_clientSettings.timeoutMilliSeconds(),
                                          flags, outputCodec);
}

void VCSBaseClient::slotAnnotateRevisionRequested(const QString &source,
                                                  QString change,
                                                  int lineNumber)
{
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = change.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        change.truncate(blankPos);
    const QFileInfo fi(source);
    annotate(fi.absolutePath(), fi.fileName(), change, lineNumber);
}

void VCSBaseClient::annotate(const QString &workingDir, const QString &file,
                             const QString revision /* = QString() */,
                             int lineNumber /* = -1 */)
{
    Q_UNUSED(lineNumber)
    const QString vcsCmdString = vcsCommandString(AnnotateCommand);
    QStringList args;
    args << vcsCmdString << annotateArguments(file, revision, lineNumber);
    const QString kind = vcsEditorKind(AnnotateCommand);
    const QString id = VCSBase::VCSBaseEditorWidget::getSource(workingDir, QStringList(file));
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, file);

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);

    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::diff(const QString &workingDir, const QStringList &files)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    QStringList args;
    args << vcsCmdString << diffArguments(files);
    const QString kind = vcsEditorKind(DiffCommand);
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, files);
    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source, true,
                                                           vcsCmdString.toLatin1().constData(), id);
    editor->setDiffBaseDirectory(workingDir);

    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::log(const QString &workingDir, const QStringList &files,
                        bool enableAnnotationContextMenu)
{
    const QString vcsCmdString = vcsCommandString(LogCommand);
    QStringList args;
    args << vcsCmdString << logArguments(files);
    const QString kind = vcsEditorKind(LogCommand);
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, files);

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, workingDir, true,
                                                           vcsCmdString.toLatin1().constData(), id);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::revertFile(const QString &workingDir,
                               const QString &file,
                               const QString &revision)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revertArguments(file, revision);
    // Indicate repository change or file list
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args));
    job->setCookie(QStringList(workingDir + QLatin1Char('/') + file));
    connect(job.data(), SIGNAL(succeeded(QVariant)),
            this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::revertAll(const QString &workingDir, const QString &revision)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revertAllArguments(revision);
    // Indicate repository change or file list
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args));
    connect(job.data(), SIGNAL(succeeded(QVariant)),
            this, SIGNAL(changed(QVariant)), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::status(const QString &workingDir, const QString &file)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << statusArguments(file);
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    outwin->setRepository(workingDir);
    QSharedPointer<VCSJob> job(new VCSJob(workingDir, args));
    connect(job.data(), SIGNAL(succeeded(QVariant)),
            outwin, SLOT(clearRepository()), Qt::QueuedConnection);
    enqueueJob(job);
}

void VCSBaseClient::statusWithSignal(const QString &repositoryRoot)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << statusArguments(QString());
    QSharedPointer<VCSJob> job(new VCSJob(repositoryRoot, args, VCSJob::RawDataEmitMode));
    connect(job.data(), SIGNAL(rawData(QByteArray)),
            this, SLOT(statusParser(QByteArray)));
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

void VCSBaseClient::statusParser(const QByteArray &data)
{
    QList<QPair<QString, QString> > statusList;

    QStringList rawStatusList = QTextCodec::codecForLocale()->toUnicode(data).split(QLatin1Char('\n'));

    foreach (const QString &string, rawStatusList) {
        QPair<QString, QString> status = parseStatusLine(string);
        if (!status.first.isEmpty() && !status.second.isEmpty())
            statusList.append(status);
    }

    emit parsedStatus(statusList);
}

void VCSBaseClient::import(const QString &repositoryRoot, const QStringList &files)
{
    QStringList args(vcsCommandString(ImportCommand));
    args << importArguments(files);
    QSharedPointer<VCSJob> job(new VCSJob(repositoryRoot, args));
    enqueueJob(job);
}

void VCSBaseClient::view(const QString &source, const QString &id)
{
    QStringList args(viewArguments(id));
    const QString kind = vcsEditorKind(DiffCommand);
    const QString title = vcsEditorTitle(vcsCommandString(LogCommand), id);

    VCSBase::VCSBaseEditorWidget *editor = createVCSEditor(kind, title, source,
                                                           true, "view", id);

    QSharedPointer<VCSJob> job(new VCSJob(source, args, editor));
    enqueueJob(job);
}

void VCSBaseClient::update(const QString &repositoryRoot, const QString &revision)
{
    QStringList args(vcsCommandString(UpdateCommand));
    args.append(updateArguments(revision));

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
                           const ExtraCommandOptions &extraOptions)
{
    QStringList args(vcsCommandString(CommitCommand));
    args.append(commitArguments(files, commitMessageFile, extraOptions));
    QSharedPointer<VCSJob> job(new VCSJob(repositoryRoot, args));
    enqueueJob(job);
}

void VCSBaseClient::settingsChanged()
{
    if (m_jobManager) {
        m_jobManager->setSettings(m_clientSettings.binary(),
                                  m_clientSettings.standardArguments(),
                                  m_clientSettings.timeoutMilliSeconds());
        m_jobManager->restart();
    }
}

QString VCSBaseClient::vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const
{
    return m_clientSettings.binary() + QLatin1Char(' ') + vcsCmd + QLatin1Char(' ') + sourceId;
}

VCSBase::VCSBaseEditorWidget *VCSBaseClient::createVCSEditor(const QString &kind, QString title,
                                                             const QString &source, bool setSourceCodec,
                                                             const char *registerDynamicProperty,
                                                             const QString &dynamicPropertyValue) const
{
    VCSBase::VCSBaseEditorWidget *baseEditor = 0;
    Core::IEditor *outputEditor = locateEditor(m_core, registerDynamicProperty, dynamicPropertyValue);
    const QString progressMsg = tr("Working...");
    if (outputEditor) {
        // Exists already
        outputEditor->createNew(progressMsg);
        baseEditor = VCSBase::VCSBaseEditorWidget::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return 0);
    } else {
        outputEditor = m_core->editorManager()->openEditorWithContents(kind, &title, progressMsg);
        outputEditor->file()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        baseEditor = VCSBase::VCSBaseEditorWidget::getVcsBaseEditor(outputEditor);
        connect(baseEditor, SIGNAL(annotateRevisionRequested(QString,QString,int)),
                this, SLOT(slotAnnotateRevisionRequested(QString,QString,int)));
        QTC_ASSERT(baseEditor, return 0);
        baseEditor->setSource(source);
        if (setSourceCodec)
            baseEditor->setCodec(VCSBase::VCSBaseEditorWidget::getCodec(source));
    }

    m_core->editorManager()->activateEditor(outputEditor, Core::EditorManager::ModeSwitch);
    baseEditor->setForceReadOnly(true);
    return baseEditor;
}

void VCSBaseClient::enqueueJob(const QSharedPointer<VCSJob> &job)
{
    if (!m_jobManager) {
        m_jobManager = new VCSJobRunner();
        m_jobManager->setSettings(m_clientSettings.binary(),
                                  m_clientSettings.standardArguments(),
                                  m_clientSettings.timeoutMilliSeconds());
        m_jobManager->start();
    }
    m_jobManager->enqueueJob(job);
}

} // namespace VCSBase
