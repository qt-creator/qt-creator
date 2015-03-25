/**************************************************************************
**
** Copyright (C) 2015 Brian McGillion and Hugues Delorme
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "vcsbaseclient.h"
#include "vcscommand.h"
#include "vcsbaseclientsettings.h"
#include "vcsbaseeditorparameterwidget.h"

#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>

#include <utils/qtcassert.h>
#include <utils/synchronousprocess.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QStringList>
#include <QDir>
#include <QProcess>
#include <QSignalMapper>
#include <QTextCodec>
#include <QDebug>
#include <QFileInfo>
#include <QByteArray>
#include <QVariant>
#include <QProcessEnvironment>

/*!
    \class VcsBase::VcsBaseClient

    \brief The VcsBaseClient class is the base class for Mercurial and Bazaar
    'clients'.

    Provides base functionality for common commands (diff, log, etc).

    \sa VcsBase::VcsJobRunner
*/

Q_DECLARE_METATYPE(QVariant)

static Core::IEditor *locateEditor(const char *property, const QString &entry)
{
    foreach (Core::IDocument *document, Core::DocumentModel::openedDocuments())
        if (document->property(property).toString() == entry)
            return Core::DocumentModel::editorsForDocument(document).first();
    return 0;
}

namespace VcsBase {

class VcsBaseClientPrivate
{
public:
    VcsBaseClientPrivate(VcsBaseClient *client, VcsBaseClientSettings *settings);

    void bindCommandToEditor(VcsCommand *cmd, VcsBaseEditorWidget *editor);

    VcsBaseEditorParameterWidget *createDiffEditor();
    VcsBaseEditorParameterWidget *createLogEditor();

    VcsBaseClientSettings *m_clientSettings;
    QSignalMapper *m_cmdFinishedMapper;

    VcsBaseClient::ParameterWidgetCreator m_diffParamWidgetCreator;
    VcsBaseClient::ParameterWidgetCreator m_logParamWidgetCreator;
};

VcsBaseClientPrivate::VcsBaseClientPrivate(VcsBaseClient *client, VcsBaseClientSettings *settings) :
    m_clientSettings(settings),
    m_cmdFinishedMapper(new QSignalMapper(client))
{ }

void VcsBaseClientPrivate::bindCommandToEditor(VcsCommand *cmd, VcsBaseEditorWidget *editor)
{
    editor->setCommand(cmd);
    QObject::connect(cmd, &VcsCommand::finished,
                     m_cmdFinishedMapper, static_cast<void (QSignalMapper::*)()>(&QSignalMapper::map));
    m_cmdFinishedMapper->setMapping(cmd, editor);
}

VcsBaseEditorParameterWidget *VcsBaseClientPrivate::createDiffEditor()
{
    return m_diffParamWidgetCreator ? m_diffParamWidgetCreator() : 0;
}

VcsBaseEditorParameterWidget *VcsBaseClientPrivate::createLogEditor()
{
    return m_logParamWidgetCreator ? m_logParamWidgetCreator() : 0;
}

VcsBaseClient::StatusItem::StatusItem(const QString &s, const QString &f) :
    flags(s), file(f)
{ }

VcsBaseClient::VcsBaseClient(VcsBaseClientSettings *settings) :
    d(new VcsBaseClientPrivate(this, settings))
{
    qRegisterMetaType<QVariant>();
    connect(Core::ICore::instance(), &Core::ICore::saveSettingsRequested,
            this, &VcsBaseClient::saveSettings);
    connect(d->m_cmdFinishedMapper, static_cast<void (QSignalMapper::*)(QWidget*)>(&QSignalMapper::mapped),
            this, &VcsBaseClient::commandFinishedGotoLine);
}

VcsBaseClient::~VcsBaseClient()
{
    delete d;
}

bool VcsBaseClient::synchronousCreateRepository(const QString &workingDirectory,
                                                const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(CreateRepositoryCommand));
    args << extraOptions;
    QByteArray outputData;
    if (!vcsFullySynchronousExec(workingDirectory, args, &outputData))
        return false;
    VcsOutputWindow::append(
                Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(outputData)));

    resetCachedVcsInfo(workingDirectory);

    return true;
}

bool VcsBaseClient::synchronousClone(const QString &workingDir,
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

bool VcsBaseClient::synchronousAdd(const QString &workingDir, const QString &filename,
                                   const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(AddCommand) << extraOptions << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VcsBaseClient::synchronousRemove(const QString &workingDir, const QString &filename,
                                      const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(RemoveCommand) << extraOptions << filename;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VcsBaseClient::synchronousMove(const QString &workingDir,
                                    const QString &from, const QString &to,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(MoveCommand) << extraOptions << from << to;
    QByteArray stdOut;
    return vcsFullySynchronousExec(workingDir, args, &stdOut);
}

bool VcsBaseClient::synchronousPull(const QString &workingDir,
                                    const QString &srcLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions << srcLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VcsBasePlugin::SshPasswordPrompt
            | VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = vcsSynchronousExec(workingDir, args, flags);
    const bool ok = resp.result == Utils::SynchronousProcessResponse::Finished;
    if (ok)
        emit changed(QVariant(workingDir));
    return ok;
}

bool VcsBaseClient::synchronousPush(const QString &workingDir,
                                    const QString &dstLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PushCommand) << extraOptions << dstLocation;
    // Disable UNIX terminals to suppress SSH prompting
    const unsigned flags =
            VcsBasePlugin::SshPasswordPrompt
            | VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = vcsSynchronousExec(workingDir, args, flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

bool VcsBaseClient::vcsFullySynchronousExec(const QString &workingDir,
                                            const QStringList &args,
                                            QByteArray *output) const
{
    QProcess vcsProcess;
    if (!workingDir.isEmpty())
        vcsProcess.setWorkingDirectory(workingDir);
    vcsProcess.setProcessEnvironment(processEnvironment());

    VcsOutputWindow::appendCommand(workingDir, vcsBinary(), args);

    const Utils::FileName binary = vcsBinary();
    vcsProcess.start(binary.toString(), args);

    if (!vcsProcess.waitForStarted()) {
        VcsOutputWindow::appendError(tr("Unable to start process \"%1\": %2")
                                         .arg(binary.toUserOutput(), vcsProcess.errorString()));
        return false;
    }

    vcsProcess.closeWriteChannel();

    QByteArray stdErr;
    if (!Utils::SynchronousProcess::readDataFromProcess(vcsProcess, vcsTimeout() * 1000,
                                                        output, &stdErr, true)) {
        Utils::SynchronousProcess::stopProcess(vcsProcess);
        VcsOutputWindow::appendError(tr("Timed out after %1s waiting for the process %2 to finish.")
                                         .arg(vcsTimeout()).arg(binary.toUserOutput()));
        return false;
    }
    if (!stdErr.isEmpty())
        VcsOutputWindow::appendError(QString::fromLocal8Bit(stdErr));

    return vcsProcess.exitStatus() == QProcess::NormalExit && vcsProcess.exitCode() == 0;
}

Utils::SynchronousProcessResponse VcsBaseClient::vcsSynchronousExec(const QString &workingDirectory,
                                                                    const QStringList &args,
                                                                    unsigned flags,
                                                                    QTextCodec *outputCodec) const
{
    return VcsBasePlugin::runVcs(workingDirectory, vcsBinary(), args, vcsTimeout() * 1000,
                                 flags, outputCodec);
}

void VcsBaseClient::annotate(const QString &workingDir, const QString &file,
                             const QString &revision /* = QString() */,
                             int lineNumber /* = -1 */,
                             const QStringList &extraOptions)
{
    const QString vcsCmdString = vcsCommandString(AnnotateCommand);
    QStringList args;
    args << vcsCmdString << revisionSpec(revision) << extraOptions << file;
    const Core::Id kind = vcsEditorKind(AnnotateCommand);
    const QString id = VcsBaseEditor::getSource(workingDir, QStringList(file));
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true,
                                                  vcsCmdString.toLatin1().constData(), id);

    VcsCommand *cmd = createCommand(workingDir, editor);
    cmd->setCookie(lineNumber);
    enqueueJob(cmd, args);
}

void VcsBaseClient::diff(const QString &workingDir, const QStringList &files,
                         const QStringList &extraOptions)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const Core::Id kind = vcsEditorKind(DiffCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true,
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setWorkingDirectory(workingDir);

    VcsBaseEditorParameterWidget *paramWidget = editor->configurationWidget();
    if (!paramWidget && (paramWidget = d->createDiffEditor())) {
        // editor has been just created, createVcsEditor() didn't set a configuration widget yet
        connect(editor, &VcsBaseEditorWidget::diffChunkReverted,
                paramWidget, &VcsBaseEditorParameterWidget::executeCommand);
        connect(paramWidget, &VcsBaseEditorParameterWidget::commandExecutionRequested,
                [=] { diff(workingDir, files, extraOptions); } );
        editor->setConfigurationWidget(paramWidget);
    }

    QStringList args;
    const QStringList paramArgs = paramWidget != 0 ? paramWidget->arguments() : QStringList();
    args << vcsCmdString << extraOptions << paramArgs << files;
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(0) : VcsBaseEditor::getCodec(source);
    VcsCommand *command = createCommand(workingDir, editor);
    command->setCodec(codec);
    enqueueJob(command, args, exitCodeInterpreter(DiffCommand, command));
}

void VcsBaseClient::log(const QString &workingDir, const QStringList &files,
                        const QStringList &extraOptions,
                        bool enableAnnotationContextMenu)
{
    const QString vcsCmdString = vcsCommandString(LogCommand);
    const Core::Id kind = vcsEditorKind(LogCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const QString source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true,
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    VcsBaseEditorParameterWidget *paramWidget = editor->configurationWidget();
    if (!paramWidget && (paramWidget = d->createLogEditor())) {
        // editor has been just created, createVcsEditor() didn't set a configuration widget yet
        connect(paramWidget, &VcsBaseEditorParameterWidget::commandExecutionRequested,
                [=]() { this->log(workingDir, files, extraOptions, enableAnnotationContextMenu); } );
        editor->setConfigurationWidget(paramWidget);
    }

    QStringList args;
    const QStringList paramArgs = paramWidget != 0 ? paramWidget->arguments() : QStringList();
    args << vcsCmdString << extraOptions << paramArgs << files;
    enqueueJob(createCommand(workingDir, editor), args);
}

void VcsBaseClient::revertFile(const QString &workingDir,
                               const QString &file,
                               const QString &revision,
                               const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions << file;
    // Indicate repository change or file list
    VcsCommand *cmd = createCommand(workingDir);
    cmd->setCookie(QStringList(workingDir + QLatin1Char('/') + file));
    connect(cmd, &VcsCommand::success, this, &VcsBaseClient::changed, Qt::QueuedConnection);
    enqueueJob(cmd, args);
}

void VcsBaseClient::revertAll(const QString &workingDir, const QString &revision,
                              const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions;
    // Indicate repository change or file list
    VcsCommand *cmd = createCommand(workingDir);
    cmd->setCookie(QStringList(workingDir));
    connect(cmd, &VcsCommand::success, this, &VcsBaseClient::changed, Qt::QueuedConnection);
    enqueueJob(createCommand(workingDir), args);
}

void VcsBaseClient::status(const QString &workingDir, const QString &file,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions << file;
    VcsOutputWindow::setRepository(workingDir);
    VcsCommand *cmd = createCommand(workingDir, 0, VcsWindowOutputBind);
    connect(cmd, &VcsCommand::finished,
            VcsOutputWindow::instance(), &VcsOutputWindow::clearRepository,
            Qt::QueuedConnection);
    enqueueJob(cmd, args);
}

void VcsBaseClient::emitParsedStatus(const QString &repository, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions;
    VcsCommand *cmd = createCommand(repository);
    connect(cmd, &VcsCommand::output, this, &VcsBaseClient::statusParser);
    enqueueJob(cmd, args);
}

QString VcsBaseClient::vcsCommandString(VcsCommandTag cmd) const
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

Utils::ExitCodeInterpreter *VcsBaseClient::exitCodeInterpreter(VcsCommandTag cmd, QObject *parent) const
{
    Q_UNUSED(cmd)
    Q_UNUSED(parent)
    return 0;
}

void VcsBaseClient::setDiffParameterWidgetCreator(ParameterWidgetCreator creator)
{
    d->m_diffParamWidgetCreator = std::move(creator);
}

void VcsBaseClient::setLogParameterWidgetCreator(ParameterWidgetCreator creator)
{
    d->m_logParamWidgetCreator = std::move(creator);
}

void VcsBaseClient::import(const QString &repositoryRoot, const QStringList &files,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(ImportCommand));
    args << extraOptions << files;
    enqueueJob(createCommand(repositoryRoot), args);
}

void VcsBaseClient::view(const QString &source, const QString &id,
                         const QStringList &extraOptions)
{
    QStringList args;
    args << extraOptions << revisionSpec(id);
    const Core::Id kind = vcsEditorKind(DiffCommand);
    const QString title = vcsEditorTitle(vcsCommandString(LogCommand), id);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source, true, "view", id);

    const QFileInfo fi(source);
    const QString workingDirPath = fi.isFile() ? fi.absolutePath() : source;
    enqueueJob(createCommand(workingDirPath, editor), args);
}

void VcsBaseClient::update(const QString &repositoryRoot, const QString &revision,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(UpdateCommand));
    args << revisionSpec(revision) << extraOptions;
    VcsCommand *cmd = createCommand(repositoryRoot);
    cmd->setCookie(repositoryRoot);
    connect(cmd, &VcsCommand::success, this, &VcsBaseClient::changed, Qt::QueuedConnection);
    enqueueJob(cmd, args);
}

void VcsBaseClient::commit(const QString &repositoryRoot,
                           const QStringList &files,
                           const QString &commitMessageFile,
                           const QStringList &extraOptions)
{
    // Handling of commitMessageFile is a bit tricky :
    //   VcsBaseClient cannot do something with it because it doesn't know which
    //   option to use (-F ? but sub VCS clients might require a different option
    //   name like -l for hg ...)
    //
    //   So descendants of VcsBaseClient *must* redefine commit() and extend
    //   extraOptions with the usage for commitMessageFile (see BazaarClient::commit()
    //   for example)
    QStringList args(vcsCommandString(CommitCommand));
    args << extraOptions << files;
    VcsCommand *cmd = createCommand(repositoryRoot, 0, VcsWindowOutputBind);
    if (!commitMessageFile.isEmpty())
        connect(cmd, &VcsCommand::finished, [commitMessageFile]() { QFile(commitMessageFile).remove(); });
    enqueueJob(cmd, args);
}

VcsBaseClientSettings *VcsBaseClient::settings() const
{
    return d->m_clientSettings;
}

QString VcsBaseClient::vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const
{
    return vcsBinary().toFileInfo().baseName() +
            QLatin1Char(' ') + vcsCmd + QLatin1Char(' ') +
            Utils::FileName::fromString(sourceId).fileName();
}

VcsBaseEditorWidget *VcsBaseClient::createVcsEditor(Core::Id kind, QString title,
                                                    const QString &source, bool setSourceCodec,
                                                    const char *registerDynamicProperty,
                                                    const QString &dynamicPropertyValue) const
{
    VcsBaseEditorWidget *baseEditor = 0;
    Core::IEditor *outputEditor = locateEditor(registerDynamicProperty, dynamicPropertyValue);
    const QString progressMsg = tr("Working...");
    if (outputEditor) {
        // Exists already
        outputEditor->document()->setContents(progressMsg.toUtf8());
        baseEditor = VcsBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return 0);
        Core::EditorManager::activateEditor(outputEditor);
    } else {
        outputEditor = Core::EditorManager::openEditorWithContents(kind, &title, progressMsg.toUtf8());
        outputEditor->document()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        baseEditor = VcsBaseEditor::getVcsBaseEditor(outputEditor);
        connect(baseEditor, &VcsBaseEditorWidget::annotateRevisionRequested,
                this, &VcsBaseClient::annotateRevision);
        QTC_ASSERT(baseEditor, return 0);
        baseEditor->setSource(source);
        if (setSourceCodec)
            baseEditor->setCodec(VcsBaseEditor::getCodec(source));
    }

    baseEditor->setForceReadOnly(true);
    return baseEditor;
}

QProcessEnvironment VcsBaseClient::processEnvironment() const
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    VcsBasePlugin::setProcessEnvironment(&environment, false);
    return environment;
}

VcsCommand *VcsBaseClient::createCommand(const QString &workingDirectory,
                                         VcsBaseEditorWidget *editor,
                                         JobOutputBindMode mode) const
{
    auto cmd = new VcsCommand(vcsBinary(), workingDirectory,
                              processEnvironment());
    cmd->setDefaultTimeout(vcsTimeout());
    if (editor)
        d->bindCommandToEditor(cmd, editor);
    if (mode == VcsWindowOutputBind) {
        cmd->addFlags(VcsBasePlugin::ShowStdOutInLogWindow);
        if (editor) // assume that the commands output is the important thing
            cmd->addFlags(VcsBasePlugin::SilentOutput);
    } else if (editor) {
        connect(cmd, &VcsCommand::output, editor, &VcsBaseEditorWidget::setPlainText);
    }

    return cmd;
}

void VcsBaseClient::enqueueJob(VcsCommand *cmd, const QStringList &args, Utils::ExitCodeInterpreter *interpreter)
{
    cmd->addJob(args, vcsTimeout(), interpreter);
    cmd->execute();
}

void VcsBaseClient::resetCachedVcsInfo(const QString &workingDir)
{
    Core::VcsManager::resetVersionControlForDirectory(workingDir);
}

Utils::FileName VcsBaseClient::vcsBinary() const
{
    return d->m_clientSettings->binaryPath();
}

int VcsBaseClient::vcsTimeout() const
{
    return d->m_clientSettings->intValue(VcsBaseClientSettings::timeoutKey);
}

void VcsBaseClient::statusParser(const QString &text)
{
    QList<VcsBaseClient::StatusItem> lineInfoList;

    QStringList rawStatusList = text.split(QLatin1Char('\n'));

    foreach (const QString &string, rawStatusList) {
        const VcsBaseClient::StatusItem lineInfo = parseStatusLine(string);
        if (!lineInfo.flags.isEmpty() && !lineInfo.file.isEmpty())
            lineInfoList.append(lineInfo);
    }

    emit parsedStatus(lineInfoList);
}

void VcsBaseClient::annotateRevision(const QString &workingDirectory,  const QString &file,
                                     const QString& change, int lineNumber)
{
    QString changeCopy = change;
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = changeCopy.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        changeCopy.truncate(blankPos);
    annotate(workingDirectory, file, changeCopy, lineNumber);
}

void VcsBaseClient::saveSettings()
{
    settings()->writeSettings(Core::ICore::settings());
}

void VcsBaseClient::commandFinishedGotoLine(QWidget *editorObject)
{
    VcsBaseEditorWidget *editor = qobject_cast<VcsBaseEditorWidget *>(editorObject);
    VcsCommand *cmd = qobject_cast<VcsCommand *>(d->m_cmdFinishedMapper->mapping(editor));
    if (editor && cmd) {
        if (!cmd->lastExecutionSuccess()) {
            editor->reportCommandFinished(false, cmd->lastExecutionExitCode(), cmd->cookie());
        } else if (cmd->cookie().type() == QVariant::Int) {
            const int line = cmd->cookie().toInt();
            if (line >= 0)
                editor->gotoLine(line);
        }
        d->m_cmdFinishedMapper->removeMappings(cmd);
    }
}

} // namespace VcsBase

#include "moc_vcsbaseclient.cpp"
