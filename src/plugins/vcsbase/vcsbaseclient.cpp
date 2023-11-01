// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbaseclient.h"

#include "vcsbaseclientsettings.h"
#include "vcsbaseeditor.h"
#include "vcsbaseeditorconfig.h"
#include "vcsbaseplugin.h"
#include "vcsbasetr.h"
#include "vcscommand.h"
#include "vcsoutputwindow.h"

#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>

#include <texteditor/textdocument.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QDebug>
#include <QStringList>
#include <QTextCodec>
#include <QVariant>

using namespace Core;
using namespace Utils;

/*!
    \class VcsBase::VcsBaseClient

    \brief The VcsBaseClient class is the base class for Mercurial and Bazaar
    'clients'.

    Provides base functionality for common commands (diff, log, etc).

    \sa VcsBase::VcsJobRunner
*/

static IEditor *locateEditor(const char *property, const QString &entry)
{
    const QList<IDocument *> documents = DocumentModel::openedDocuments();
    for (IDocument *document : documents)
        if (document->property(property).toString() == entry)
            return DocumentModel::editorsForDocument(document).constFirst();
    return nullptr;
}

namespace VcsBase {

VcsBaseClientImpl::VcsBaseClientImpl(VcsBaseSettings *baseSettings)
    : m_baseSettings(baseSettings)
{
    connect(ICore::instance(), &ICore::saveSettingsRequested,
            this, &VcsBaseClientImpl::saveSettings);
}

FilePath VcsBaseClientImpl::vcsBinary() const
{
    return m_baseSettings->binaryPath();
}

VcsCommand *VcsBaseClientImpl::createCommand(const FilePath &workingDirectory,
                                             VcsBaseEditorWidget *editor) const
{
    auto cmd = createVcsCommand(workingDirectory, processEnvironment());
    if (editor) {
        editor->setCommand(cmd);
        connect(cmd, &VcsCommand::done, editor, [editor, cmd] {
            if (cmd->result() != ProcessResult::FinishedWithSuccess) {
                editor->textDocument()->setPlainText(Tr::tr("Failed to retrieve data."));
                return;
            }
            editor->setPlainText(cmd->cleanedStdOut());
            editor->gotoDefaultLine();
        });
    }
    return cmd;
}

void VcsBaseClientImpl::setupCommand(Utils::Process &process,
                                     const FilePath &workingDirectory,
                                     const QStringList &args) const
{
    process.setEnvironment(processEnvironment());
    process.setWorkingDirectory(workingDirectory);
    process.setCommand({vcsBinary(), args});
    process.setUseCtrlCStub(true);
}

void VcsBaseClientImpl::enqueueJob(VcsCommand *cmd, const QStringList &args,
                                   const ExitCodeInterpreter &interpreter) const
{
    cmd->addJob({vcsBinary(), args}, vcsTimeoutS(), {}, interpreter);
    cmd->start();
}

Environment VcsBaseClientImpl::processEnvironment() const
{
    Environment environment = Environment::systemEnvironment();
    VcsBase::setProcessEnvironment(&environment);
    return environment;
}

QStringList VcsBaseClientImpl::splitLines(const QString &s)
{
    const QChar newLine = QLatin1Char('\n');
    QString output = s;
    if (output.endsWith(newLine))
        output.truncate(output.size() - 1);
    if (output.isEmpty())
        return {};
    return output.split(newLine);
}

QString VcsBaseClientImpl::stripLastNewline(const QString &in)
{
    if (in.endsWith('\n'))
        return in.left(in.count() - 1);
    return in;
}

CommandResult VcsBaseClientImpl::vcsSynchronousExec(const FilePath &workingDir,
              const QStringList &args, RunFlags flags, int timeoutS, QTextCodec *codec) const
{
    return vcsSynchronousExec(workingDir, {vcsBinary(), args}, flags, timeoutS, codec);
}

CommandResult VcsBaseClientImpl::vcsSynchronousExec(const FilePath &workingDir,
              const CommandLine &cmdLine, RunFlags flags, int timeoutS, QTextCodec *codec) const
{
    return VcsCommand::runBlocking(workingDir, processEnvironment(), cmdLine, flags,
                                   timeoutS > 0 ? timeoutS : vcsTimeoutS(), codec);
}

void VcsBaseClientImpl::resetCachedVcsInfo(const FilePath &workingDir)
{
    VcsManager::resetVersionControlForDirectory(workingDir);
}

void VcsBaseClientImpl::annotateRevisionRequested(const FilePath &workingDirectory,
                                                  const QString &file, const QString &change,
                                                  int line)
{
    QString changeCopy = change;
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = changeCopy.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        changeCopy.truncate(blankPos);
    annotate(workingDirectory, file, line, changeCopy);
}

void VcsBaseClientImpl::vcsExecWithHandler(const FilePath &workingDirectory,
                                           const QStringList &arguments,
                                           const QObject *context,
                                           const CommandHandler &handler,
                                           RunFlags additionalFlags, QTextCodec *codec) const
{
    VcsCommand *command = createCommand(workingDirectory);
    command->addFlags(additionalFlags);
    command->setCodec(codec);
    command->addJob({vcsBinary(), arguments}, vcsTimeoutS());
    if (handler) {
        const QObject *actualContext = context ? context : this;
        connect(command, &VcsCommand::done, actualContext, [command, handler] {
            handler(CommandResult(*command));
        });
    }
    command->start();
}

void VcsBaseClientImpl::vcsExec(const FilePath &workingDirectory,
                                const QStringList &arguments,
                                RunFlags additionalFlags) const
{
    VcsCommand *command = createCommand(workingDirectory);
    command->addFlags(additionalFlags);
    command->addJob({vcsBinary(), arguments}, vcsTimeoutS());
    command->start();
}

void VcsBaseClientImpl::vcsExecWithEditor(const Utils::FilePath &workingDirectory,
                                          const QStringList &arguments,
                                          VcsBaseEditorWidget *editor) const
{
    VcsCommand *command = createCommand(workingDirectory, editor);
    command->setCodec(editor->codec());
    command->addJob({vcsBinary(), arguments}, vcsTimeoutS());
    command->start();
}

int VcsBaseClientImpl::vcsTimeoutS() const
{
    return m_baseSettings->timeout();
}

VcsCommand *VcsBaseClientImpl::createVcsCommand(const FilePath &defaultWorkingDir,
                                                const Environment &environment)
{
    return new VcsCommand(defaultWorkingDir, environment);
}

VcsBaseEditorWidget *VcsBaseClientImpl::createVcsEditor(Id kind, QString title,
                                                        const FilePath &source, QTextCodec *codec,
                                                        const char *registerDynamicProperty,
                                                        const QString &dynamicPropertyValue) const
{
    VcsBaseEditorWidget *baseEditor = nullptr;
    IEditor *outputEditor = locateEditor(registerDynamicProperty, dynamicPropertyValue);
    const QString progressMsg = Tr::tr("Working...");
    if (outputEditor) {
        // Exists already
        outputEditor->document()->setContents(progressMsg.toUtf8());
        baseEditor = VcsBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return nullptr);
        EditorManager::activateEditor(outputEditor);
    } else {
        outputEditor = EditorManager::openEditorWithContents(kind, &title, progressMsg.toUtf8());
        outputEditor->document()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        baseEditor = VcsBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(baseEditor, return nullptr);
        connect(baseEditor, &VcsBaseEditorWidget::annotateRevisionRequested,
                this, &VcsBaseClientImpl::annotateRevisionRequested);
        baseEditor->setSource(source);
        baseEditor->setDefaultLineNumber(1);
        if (codec)
            baseEditor->setCodec(codec);
    }

    baseEditor->setForceReadOnly(true);
    return baseEditor;
}

void VcsBaseClientImpl::saveSettings()
{
    m_baseSettings->writeSettings();
}

VcsBaseClient::VcsBaseClient(VcsBaseSettings *baseSettings)
    : VcsBaseClientImpl(baseSettings)
{
    qRegisterMetaType<QVariant>();
}

bool VcsBaseClient::synchronousCreateRepository(const FilePath &workingDirectory,
                                                const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(CreateRepositoryCommand));
    args << extraOptions;
    const CommandResult result = vcsSynchronousExec(workingDirectory, args);
    if (result.result() != ProcessResult::FinishedWithSuccess)
        return false;
    VcsOutputWindow::append(result.cleanedStdOut());

    resetCachedVcsInfo(workingDirectory);

    return true;
}

bool VcsBaseClient::synchronousClone(const FilePath &workingDir,
                                     const QString &srcLocation,
                                     const QString &dstLocation,
                                     const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(CloneCommand)
         << extraOptions << srcLocation << dstLocation;

    const CommandResult result = vcsSynchronousExec(workingDir, args);
    resetCachedVcsInfo(workingDir);
    return result.result() == ProcessResult::FinishedWithSuccess;
}

bool VcsBaseClient::synchronousAdd(const FilePath &workingDir,
                                   const QString &relFileName,
                                   const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(AddCommand) << extraOptions << relFileName;
    return vcsSynchronousExec(workingDir, args).result() == ProcessResult::FinishedWithSuccess;
}

bool VcsBaseClient::synchronousRemove(const FilePath &workingDir,
                                      const QString &filename,
                                      const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(RemoveCommand) << extraOptions << filename;
    return vcsSynchronousExec(workingDir, args).result() == ProcessResult::FinishedWithSuccess;
}

bool VcsBaseClient::synchronousMove(const FilePath &workingDir,
                                    const QString &from,
                                    const QString &to,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(MoveCommand) << extraOptions << from << to;
    return vcsSynchronousExec(workingDir, args).result() == ProcessResult::FinishedWithSuccess;
}

bool VcsBaseClient::synchronousPull(const FilePath &workingDir,
                                    const QString &srcLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions << srcLocation;
    const RunFlags flags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;
    const bool ok = vcsSynchronousExec(workingDir, args, flags).result()
            == ProcessResult::FinishedWithSuccess;
    if (ok)
        emit changed(workingDir.toVariant());
    return ok;
}

bool VcsBaseClient::synchronousPush(const FilePath &workingDir,
                                    const QString &dstLocation,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PushCommand) << extraOptions << dstLocation;
    const RunFlags flags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;
    return vcsSynchronousExec(workingDir, args, flags).result()
            == ProcessResult::FinishedWithSuccess;
}

void VcsBaseClient::annotate(const Utils::FilePath &workingDir, const QString &file,
              int lineNumber /* = -1 */, const QString &revision /* = {} */,
              const QStringList &extraOptions /* = {} */, int firstLine /* = -1 */)
{
    Q_UNUSED(firstLine)
    const QString vcsCmdString = vcsCommandString(AnnotateCommand);
    QStringList args;
    args << vcsCmdString << revisionSpec(revision) << extraOptions << file;
    const Id kind = vcsEditorKind(AnnotateCommand);
    const QString id = VcsBaseEditor::getSource(workingDir, QStringList(file)).toString();
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source),
                                                  vcsCmdString.toLatin1().constData(), id);

    VcsCommand *cmd = createCommand(workingDir, editor);
    editor->setDefaultLineNumber(lineNumber);
    enqueueJob(cmd, args);
}

void VcsBaseClient::diff(const FilePath &workingDir, const QStringList &files,
                         const QStringList &extraOptions)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const Id kind = vcsEditorKind(DiffCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source),
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setWorkingDirectory(workingDir);

    VcsBaseEditorConfig *paramWidget = editor->editorConfig();
    if (!paramWidget) {
        if (m_diffConfigCreator)
            paramWidget = m_diffConfigCreator(editor->toolBar());
        if (paramWidget) {
            paramWidget->setBaseArguments(extraOptions);
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(editor, &VcsBaseEditorWidget::diffChunkReverted,
                    paramWidget, &VcsBaseEditorConfig::executeCommand);
            connect(paramWidget, &VcsBaseEditorConfig::commandExecutionRequested,
                    this, [=] { diff(workingDir, files, extraOptions); });
            editor->setEditorConfig(paramWidget);
        }
    }

    QStringList args = {vcsCmdString};
    if (paramWidget)
        args << paramWidget->arguments();
    else
        args << extraOptions;
    args << files;
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(nullptr)
                                         : VcsBaseEditor::getCodec(source);
    VcsCommand *command = createCommand(workingDir, editor);
    command->setCodec(codec);
    enqueueJob(command, args, exitCodeInterpreter(DiffCommand));
}

void VcsBaseClient::log(const FilePath &workingDir,
                        const QStringList &files,
                        const QStringList &extraOptions,
                        bool enableAnnotationContextMenu,
                        const std::function<void(Utils::CommandLine &)> &addAuthOptions)
{
    const QString vcsCmdString = vcsCommandString(LogCommand);
    const Id kind = vcsEditorKind(LogCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source),
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    VcsBaseEditorConfig *paramWidget = editor->editorConfig();
    if (!paramWidget) {
        if (m_logConfigCreator)
            paramWidget = m_logConfigCreator(editor->toolBar());
        if (paramWidget) {
            paramWidget->setBaseArguments(extraOptions);
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(paramWidget, &VcsBaseEditorConfig::commandExecutionRequested, this, [=] {
                this->log(workingDir,
                          files,
                          extraOptions,
                          enableAnnotationContextMenu,
                          addAuthOptions);
            });
            editor->setEditorConfig(paramWidget);
        }
    }

    CommandLine args{vcsBinary(), {vcsCmdString}};
    if (addAuthOptions)
        addAuthOptions(args);
    if (paramWidget)
        args << paramWidget->arguments();
    else
        args << extraOptions;
    args << files;
    VcsCommand *cmd = createCommand(workingDir, editor);
    cmd->addJob(args, vcsTimeoutS());
    cmd->start();
}

void VcsBaseClient::revertFile(const FilePath &workingDir,
                               const QString &file,
                               const QString &revision,
                               const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions << file;
    // Indicate repository change or file list
    VcsCommand *cmd = createCommand(workingDir);
    const QStringList files = QStringList(workingDir.pathAppended(file).toString());
    connect(cmd, &VcsCommand::done, this, [this, files, cmd] {
        if (cmd->result() == ProcessResult::FinishedWithSuccess)
            emit changed(files);
    });
    enqueueJob(cmd, args);
}

void VcsBaseClient::revertAll(const FilePath &workingDir,
                              const QString &revision,
                              const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions;
    // Indicate repository change or file list
    VcsCommand *cmd = createCommand(workingDir);
    const QStringList files = QStringList(workingDir.toString());
    connect(cmd, &VcsCommand::done, this, [this, files, cmd] {
        if (cmd->result() == ProcessResult::FinishedWithSuccess)
            emit changed(files);
    });
    enqueueJob(cmd, args);
}

void VcsBaseClient::status(const FilePath &workingDir,
                           const QString &file,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions << file;
    VcsOutputWindow::setRepository(workingDir);
    VcsCommand *cmd = createCommand(workingDir);
    cmd->addFlags(RunFlags::ShowStdOut);
    connect(cmd, &VcsCommand::done, VcsOutputWindow::instance(), &VcsOutputWindow::clearRepository);
    enqueueJob(cmd, args);
}

void VcsBaseClient::emitParsedStatus(const FilePath &repository, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions;
    VcsCommand *cmd = createCommand(repository);
    connect(cmd, &VcsCommand::done, this, [this, cmd] { statusParser(cmd->cleanedStdOut()); });
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
    return {};
}

ExitCodeInterpreter VcsBaseClient::exitCodeInterpreter(VcsCommandTag cmd) const
{
    Q_UNUSED(cmd)
    return {};
}

void VcsBaseClient::setDiffConfigCreator(ConfigCreator creator)
{
    m_diffConfigCreator = std::move(creator);
}

void VcsBaseClient::setLogConfigCreator(ConfigCreator creator)
{
    m_logConfigCreator = std::move(creator);
}

void VcsBaseClient::import(const FilePath &repositoryRoot,
                           const QStringList &files,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(ImportCommand));
    args << extraOptions << files;
    enqueueJob(createCommand(repositoryRoot), args);
}

void VcsBaseClient::view(const FilePath &source,
                         const QString &id,
                         const QStringList &extraOptions)
{
    QStringList args;
    args << extraOptions << revisionSpec(id);
    const Id kind = vcsEditorKind(DiffCommand);
    const QString title = vcsEditorTitle(vcsCommandString(LogCommand), id);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getCodec(source), "view", id);

    const FilePath workingDirPath = source.isFile() ? source.absolutePath() : source;
    enqueueJob(createCommand(workingDirPath, editor), args);
}

void VcsBaseClient::update(const FilePath &repositoryRoot, const QString &revision,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(UpdateCommand));
    args << revisionSpec(revision) << extraOptions;
    VcsCommand *cmd = createCommand(repositoryRoot);
    connect(cmd, &VcsCommand::done, this, [this, repositoryRoot, cmd] {
        if (cmd->result() == ProcessResult::FinishedWithSuccess)
            emit changed(repositoryRoot.toString());
    });
    enqueueJob(cmd, args);
}

void VcsBaseClient::commit(const FilePath &repositoryRoot,
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
    VcsCommand *cmd = createCommand(repositoryRoot);
    cmd->addFlags(RunFlags::ShowStdOut);
    if (!commitMessageFile.isEmpty())
        connect(cmd, &VcsCommand::done, [commitMessageFile] { QFile(commitMessageFile).remove(); });
    enqueueJob(cmd, args);
}

QString VcsBaseClient::vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const
{
    return vcsBinary().baseName() + QLatin1Char(' ') + vcsCmd + QLatin1Char(' ')
           + FilePath::fromString(sourceId).fileName();
}

void VcsBaseClient::statusParser(const QString &text)
{
    QList<VcsBaseClient::StatusItem> lineInfoList;

    const QStringList rawStatusList = text.split(QLatin1Char('\n'));

    for (const QString &string : rawStatusList) {
        const VcsBaseClient::StatusItem lineInfo = parseStatusLine(string);
        if (!lineInfo.flags.isEmpty() && !lineInfo.file.isEmpty())
            lineInfoList.append(lineInfo);
    }

    emit parsedStatus(lineInfoList);
}

} // namespace VcsBase

#include "moc_vcsbaseclient.cpp"
