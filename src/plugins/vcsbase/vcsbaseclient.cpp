// Copyright (C) 2016 Brian McGillion and Hugues Delorme
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "vcsbaseclient.h"

#include "vcsbaseclientsettings.h"
#include "vcsbaseeditor.h"
#include "vcsbaseeditorconfig.h"
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
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/shutdownguard.h>

#include <QDebug>
#include <QStringList>
#include <QVariant>

using namespace Core;
using namespace QtTaskTree;
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

FilePath VcsBaseClientImpl::vcsBinary(const Utils::FilePath &forDirectory) const
{
    if (!forDirectory.isLocal())
        return {};

    return m_baseSettings->binaryPath();
}

void VcsBaseClientImpl::setupCommand(Utils::Process &process,
                                     const FilePath &workingDirectory,
                                     const QStringList &args) const
{
    process.setEnvironment(workingDirectory.deviceEnvironment());
    process.setWorkingDirectory(workingDirectory);
    process.setCommand({vcsBinary(workingDirectory), args});
    process.setUseCtrlCStub(true);
}

Environment VcsBaseClientImpl::processEnvironment(const FilePath &appliedTo) const
{
    return appliedTo.deviceEnvironment();
}

QStringList VcsBaseClientImpl::splitLines(const QString &s)
{
    const QString output = stripLastNewline(s);
    if (output.isEmpty())
        return {};
    return output.split('\n');
}

QString VcsBaseClientImpl::stripLastNewline(const QString &in)
{
    if (in.endsWith('\n'))
        return in.chopped(1);
    return in;
}

CommandResult VcsBaseClientImpl::vcsSynchronousExec(const FilePath &workingDir,
                                                    const QStringList &args, RunFlags flags,
                                                    int timeoutS, const TextEncoding &encoding) const
{
    return vcsSynchronousExec(workingDir, {vcsBinary(workingDir), args}, flags, timeoutS, encoding);
}

CommandResult VcsBaseClientImpl::vcsSynchronousExec(const FilePath &workingDir,
                                                    const CommandLine &cmdLine,
                                                    RunFlags flags,
                                                    int timeoutS,
                                                    const TextEncoding &encoding) const
{
    return vcsRunBlocking({.runData = {cmdLine, workingDir, processEnvironment(workingDir)},
                           .flags = flags,
                           .encoding = encoding},
                          std::chrono::seconds(timeoutS > 0 ? timeoutS : vcsTimeoutS()));
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
    // "hash author subject" from the annotation context menu. Strip the rest.
    const int blankPos = changeCopy.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        changeCopy.truncate(blankPos);
    annotate(workingDirectory, file, line, changeCopy);
}

void VcsBaseClientImpl::executeInEditor(const FilePath &workingDirectory,
                                        const CommandLine &command,
                                        VcsBaseEditorWidget *editor) const
{
    const Storage<CommandResult> resultStorage;

    const auto task = vcsProcessTask(
        {.runData = {command, workingDirectory, processEnvironment(workingDirectory)},
         .encoding = editor->encoding()}, resultStorage);

    editor->executeTask(task, resultStorage);
}

void VcsBaseClientImpl::executeInEditor(const Utils::FilePath &workingDirectory,
                                        const QStringList &arguments,
                                        VcsBaseEditorWidget *editor) const
{
    executeInEditor(workingDirectory, {vcsBinary(workingDirectory), arguments}, editor);
}

void VcsBaseClientImpl::enqueueTask(const ExecutableItem &task)
{
    m_taskTreeRunner.enqueue({task});
}

ExecutableItem VcsBaseClientImpl::commandTask(const VcsCommandData &data) const
{
    const Storage<CommandResult> resultStorage;

    const VcsProcessData processData{
        .runData = {
            {vcsBinary(data.workingDirectory), data.arguments},
            data.workingDirectory,
            processEnvironment(data.workingDirectory)},
        .flags = data.flags,
        .progressParser = data.progressParser,
        .encoding = data.encoding
    };

    const auto task = data.commandHandler ? vcsProcessTask(processData, resultStorage)
                                          : vcsProcessTask(processData);

    const auto onDone = [resultStorage, commandHandler = data.commandHandler] {
        if (commandHandler)
            commandHandler(*resultStorage);
    };

    return Group {
        resultStorage,
        task,
        data.commandHandler ? onGroupDone(onDone) : nullItem
    };
}

void VcsBaseClientImpl::enqueueCommand(const VcsCommandData &data)
{
    enqueueTask(commandTask(data));
}

int VcsBaseClientImpl::vcsTimeoutS() const
{
    return m_baseSettings->timeout();
}

VcsBaseEditorWidget *VcsBaseClientImpl::createVcsEditor(Id kind, QString title,
                                                        const FilePath &source,
                                                        const TextEncoding &encoding,
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
        if (encoding.isValid())
            baseEditor->setEncoding(encoding);
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
    VcsOutputWindow::appendSilently(workingDirectory, result.cleanedStdOut());

    resetCachedVcsInfo(workingDirectory);

    return true;
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
                                    const FilePath &from,
                                    const FilePath &to,
                                    const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(MoveCommand) << extraOptions << from.path() << to.path();
    return vcsSynchronousExec(workingDir, args).result() == ProcessResult::FinishedWithSuccess;
}

void VcsBaseClient::pull(const FilePath &workingDir, const QString &srcLocation,
                         const QStringList &extraOptions, const CommandHandler &commandHandler)
{
    const auto handler = [this, workingDir, commandHandler](const CommandResult &result) {
        if (result.result() == ProcessResult::FinishedWithSuccess)
            emit repositoryChanged(workingDir);
        if (commandHandler)
            commandHandler(result);
    };
    QStringList args;
    args << vcsCommandString(PullCommand) << extraOptions;
    if (!srcLocation.isEmpty())
        args << srcLocation;
    const RunFlags flags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;
    enqueueCommand({workingDir, args, flags, {}, {}, handler});
}

void VcsBaseClient::push(const FilePath &workingDir, const QString &dstLocation,
                         const QStringList &extraOptions)
{
    QStringList args;
    args << vcsCommandString(PushCommand) << extraOptions;
    if (!dstLocation.isEmpty())
        args << dstLocation;
    const RunFlags flags = RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage;
    enqueueCommand({workingDir, args, flags});
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
    const QString id = VcsBaseEditor::getSource(workingDir, QStringList(file)).toUrlishString();
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getEncoding(source),
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setDefaultLineNumber(lineNumber);
    executeInEditor(workingDir, args, editor);
}

void VcsBaseClient::diff(const FilePath &workingDir, const QStringList &files)
{
    const QString vcsCmdString = vcsCommandString(DiffCommand);
    const Id kind = vcsEditorKind(DiffCommand);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString title = vcsEditorTitle(vcsCmdString, id);
    const FilePath source = VcsBaseEditor::getSource(workingDir, files);
    VcsBaseEditorWidget *editor = createVcsEditor(kind, title, source,
                                                  VcsBaseEditor::getEncoding(source),
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setWorkingDirectory(workingDir);

    VcsBaseEditorConfig *editorConfig = editor->editorConfig();
    if (!editorConfig) {
        if (m_diffConfigCreator)
            editorConfig = m_diffConfigCreator(editor->toolBar());
        if (editorConfig) {
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(editor, &VcsBaseEditorWidget::diffChunkReverted,
                    editorConfig, &VcsBaseEditorConfig::executeCommand);
            connect(editorConfig, &VcsBaseEditorConfig::commandExecutionRequested,
                    this, [this, workingDir, files] { diff(workingDir, files); });
            editor->setEditorConfig(editorConfig);
        }
    }

    QStringList args = {vcsCmdString};
    if (editorConfig)
        args << editorConfig->arguments();
    args << files;

    const Storage<CommandResult> resultStorage;

    const auto task = vcsProcessTask(
        {.runData = {{vcsBinary(workingDir), args}, workingDir, processEnvironment(workingDir)},
         .interpreter = exitCodeInterpreter(DiffCommand),
         .encoding = source.isEmpty() ? TextEncoding() : VcsBaseEditor::getEncoding(source)},
        resultStorage);

    editor->executeTask(task, resultStorage);
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
                                                  VcsBaseEditor::getEncoding(source),
                                                  vcsCmdString.toLatin1().constData(), id);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);

    VcsBaseEditorConfig *editorConfig = editor->editorConfig();
    if (!editorConfig) {
        if (m_logConfigCreator)
            editorConfig = m_logConfigCreator(editor->toolBar());
        if (editorConfig) {
            editorConfig->setBaseArguments(extraOptions);
            // editor has been just created, createVcsEditor() didn't set a configuration widget yet
            connect(editorConfig, &VcsBaseEditorConfig::commandExecutionRequested, this,
                    [this, workingDir, files, extraOptions, enableAnnotationContextMenu, addAuthOptions] {
                log(workingDir, files, extraOptions, enableAnnotationContextMenu, addAuthOptions);
            });
            editor->setEditorConfig(editorConfig);
        }
    }

    CommandLine cmd{vcsBinary(workingDir), {vcsCmdString}};
    if (addAuthOptions)
        addAuthOptions(cmd);
    if (editorConfig)
        cmd << editorConfig->arguments();
    else
        cmd << extraOptions;
    cmd << files;
    executeInEditor(workingDir, cmd, editor);
}

void VcsBaseClient::revertFile(const FilePath &workingDir,
                               const QString &file,
                               const QString &revision,
                               const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions << file;
    const FilePaths files = {workingDir.pathAppended(file)};
    enqueueCommand({.workingDirectory = workingDir, .arguments = args,
                    .commandHandler = [this, files](const CommandResult &result) {
                        if (result.result() == ProcessResult::FinishedWithSuccess)
                            emit filesChanged(files);
                    }});
}

void VcsBaseClient::revertAll(const FilePath &workingDir,
                              const QString &revision,
                              const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(RevertCommand));
    args << revisionSpec(revision) << extraOptions;
    const FilePaths files = {workingDir};
    enqueueCommand({.workingDirectory = workingDir, .arguments = args,
                    .commandHandler = [this, files](const CommandResult &result) {
                        if (result.result() == ProcessResult::FinishedWithSuccess)
                            emit filesChanged(files);
                    }});
}

void VcsBaseClient::status(const FilePath &workingDir,
                           const QString &file,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions << file;
    enqueueCommand({workingDir, args, RunFlags::ShowStdOut});
}

void VcsBaseClient::emitParsedStatus(const FilePath &repository, const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(StatusCommand));
    args << extraOptions;
    enqueueCommand({.workingDirectory = repository, .arguments = args,
                    .commandHandler = [this](const CommandResult &result) {
                        statusParser(result.cleanedStdOut());
                    }});
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
    enqueueCommand({repositoryRoot, args});
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
                                                  VcsBaseEditor::getEncoding(source), "view", id);

    const FilePath workingDirPath = source.isFile() ? source.absolutePath() : source;
    executeInEditor(workingDirPath, args, editor);
}

void VcsBaseClient::update(const FilePath &repositoryRoot, const QString &revision,
                           const QStringList &extraOptions)
{
    QStringList args(vcsCommandString(UpdateCommand));
    args << revisionSpec(revision) << extraOptions;
    enqueueCommand({.workingDirectory = repositoryRoot, .arguments = args,
                    .commandHandler = [this, repositoryRoot](const CommandResult &result) {
                        if (result.result() == ProcessResult::FinishedWithSuccess)
                            emit repositoryChanged(repositoryRoot);
                    }});
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
    enqueueCommand({.workingDirectory = repositoryRoot, .arguments = args,
                    .flags = RunFlags::ShowStdOut,
                    .commandHandler = [commitMessageFile](const CommandResult &) {
                        if (!commitMessageFile.isEmpty())
                            QFile(commitMessageFile).remove();
                    }});
}

QString VcsBaseClient::vcsEditorTitle(const QString &vcsCmd, const QString &sourceId) const
{
    return vcsBinary({}).baseName() + QLatin1Char(' ') + vcsCmd + QLatin1Char(' ')
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
