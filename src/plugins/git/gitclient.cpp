/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
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

#include "gitclient.h"
#include "gitutils.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "giteditor.h"
#include "gitplugin.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"
#include "mergetool.h"
#include "branchadddialog.h"
#include "gerrit/gerritplugin.h"

#include <vcsbase/submitfilemodel.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/id.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/coreconstants.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <diffeditor/diffeditorconstants.h>
#include <diffeditor/diffeditorcontroller.h>
#include <diffeditor/diffutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegExp>
#include <QSignalMapper>
#include <QTemporaryFile>

#include <QAction>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QToolButton>
#include <QTextCodec>

static const char GIT_DIRECTORY[] = ".git";
static const char graphLogFormatC[] = "%h %d %an %s %ci";
static const char HEAD[] = "HEAD";
static const char CHERRY_PICK_HEAD[] = "CHERRY_PICK_HEAD";
static const char noColorOption[] = "--no-color";
static const char decorateOption[] = "--decorate";
static const char showFormatC[] =
        "--pretty=format:commit %H%n"
        "Author: %an <%ae>, %ad (%ar)%n"
        "Committer: %cn <%ce>, %cd (%cr)%n"
        "%n"
        "%B";

using namespace Core;
using namespace DiffEditor;
using namespace Utils;
using namespace VcsBase;

namespace Git {
namespace Internal {

// Suppress git diff warnings about "LF will be replaced by CRLF..." on Windows.
static unsigned diffExecutionFlags()
{
    return HostOsInfo::isWindowsHost() ? unsigned(VcsCommand::SuppressStdErr) : 0u;
}

const unsigned silentFlags = unsigned(VcsCommand::SuppressCommandLogging
                                      | VcsCommand::SuppressStdErr);

/////////////////////////////////////

class BaseController : public DiffEditorController
{
    Q_OBJECT

public:
    BaseController(IDocument *document, const QString &dir);
    ~BaseController();
    void runCommand(const QList<QStringList> &args, QTextCodec *codec = 0);

private slots:
    virtual void processOutput(const QString &output);

protected:
    void processDiff(const QString &output, const QString &startupFile = QString());
    QStringList addConfigurationArguments(const QStringList &args) const;
    GitClient *gitClient() const;
    QStringList addHeadWhenCommandInProgress() const;

    const QString m_directory;

private:
    QPointer<VcsCommand> m_command;
};

BaseController::BaseController(IDocument *document, const QString &dir) :
    DiffEditorController(document),
    m_directory(dir),
    m_command(0)
{ }

BaseController::~BaseController()
{
    if (m_command)
        m_command->cancel();
}

void BaseController::runCommand(const QList<QStringList> &args, QTextCodec *codec)
{
    if (m_command) {
        m_command->disconnect();
        m_command->cancel();
    }

    m_command = new VcsCommand(m_directory, gitClient()->processEnvironment());
    m_command->setCodec(codec ? codec : EditorManager::defaultTextCodec());
    connect(m_command.data(), &VcsCommand::stdOutText, this, &BaseController::processOutput);
    connect(m_command.data(), &VcsCommand::finished, this, &BaseController::reloadFinished);
    m_command->addFlags(diffExecutionFlags());

    foreach (const QStringList &arg, args) {
        QTC_ASSERT(!arg.isEmpty(), continue);

        m_command->addJob(gitClient()->vcsBinary(), arg, gitClient()->vcsTimeoutS());
    }

    m_command->execute();
}

void BaseController::processDiff(const QString &output, const QString &startupFile)
{
    m_command.clear();

    bool ok;
    QList<FileData> fileDataList = DiffUtils::readPatch(output, &ok);
    setDiffFiles(fileDataList, m_directory, startupFile);
}

QStringList BaseController::addConfigurationArguments(const QStringList &args) const
{
    QTC_ASSERT(!args.isEmpty(), return args);

    QStringList realArgs;
    realArgs << args.at(0);
    realArgs << QLatin1String("-m"); // show diff against parents instead of merge commits
    realArgs << QLatin1String("-M") << QLatin1String("-C"); // Detect renames and copies
    realArgs << QLatin1String("--first-parent"); // show only first parent
    if (ignoreWhitespace())
        realArgs << QLatin1String("--ignore-space-change");
    realArgs << QLatin1String("--unified=") + QString::number(contextLineCount());
    realArgs << QLatin1String("--src-prefix=a/") << QLatin1String("--dst-prefix=b/");
    realArgs << args.mid(1);

    return realArgs;
}

void BaseController::processOutput(const QString &output)
{
    processDiff(output);
}

GitClient *BaseController::gitClient() const
{
    return GitPlugin::instance()->client();
}

QStringList BaseController::addHeadWhenCommandInProgress() const
{
    QStringList args;
    // This is workaround for lack of support for merge commits and resolving conflicts,
    // we compare the current state of working tree to the HEAD of current branch
    // instead of showing unsupported combined diff format.
    GitClient::CommandInProgress commandInProgress = gitClient()->checkCommandInProgress(m_directory);
    if (commandInProgress != GitClient::NoCommand)
        args << QLatin1String(HEAD);
    return args;
}

class RepositoryDiffController : public BaseController
{
    Q_OBJECT
public:
    RepositoryDiffController(IDocument *document, const QString &dir) :
        BaseController(document, dir)
    { }

    void reload() override;
};

void RepositoryDiffController::reload()
{
    QStringList args;
    args << QLatin1String("diff");
    args.append(addHeadWhenCommandInProgress());
    runCommand(QList<QStringList>() << addConfigurationArguments(args));
}

class FileDiffController : public BaseController
{
    Q_OBJECT
public:
    FileDiffController(IDocument *document, const QString &dir, const QString &fileName) :
        BaseController(document, dir),
        m_fileName(fileName)
    { }

    void reload() override;

private:
    const QString m_fileName;
};

void FileDiffController::reload()
{
    QStringList args;
    args << QLatin1String("diff");
    args.append(addHeadWhenCommandInProgress());
    args << QLatin1String("--") << m_fileName;
    runCommand(QList<QStringList>() << addConfigurationArguments(args));
}

class FileListDiffController : public BaseController
{
    Q_OBJECT
public:
    FileListDiffController(IDocument *document, const QString &dir,
                           const QStringList &stagedFiles, const QStringList &unstagedFiles) :
        BaseController(document, dir),
        m_stagedFiles(stagedFiles),
        m_unstagedFiles(unstagedFiles)
    { }

    void reload() override;

private:
    const QStringList m_stagedFiles;
    const QStringList m_unstagedFiles;
};

void FileListDiffController::reload()
{
    QList<QStringList> argLists;
    if (!m_stagedFiles.isEmpty()) {
        QStringList stagedArgs;
        stagedArgs << QLatin1String("diff") << QLatin1String("--cached") << QLatin1String("--")
                   << m_stagedFiles;
        argLists << addConfigurationArguments(stagedArgs);
    }

    if (!m_unstagedFiles.isEmpty()) {
        QStringList unstagedArgs;
        unstagedArgs << QLatin1String("diff") << addHeadWhenCommandInProgress()
                     << QLatin1String("--") << m_unstagedFiles;
        argLists << addConfigurationArguments(unstagedArgs);
    }

    if (!argLists.isEmpty())
        runCommand(argLists);
}

class ProjectDiffController : public BaseController
{
    Q_OBJECT
public:
    ProjectDiffController(IDocument *document, const QString &dir,
                          const QStringList &projectPaths) :
        BaseController(document, dir),
        m_projectPaths(projectPaths)
    { }

    void reload() override;

private:
    const QStringList m_projectPaths;
};

void ProjectDiffController::reload()
{
    QStringList args;
    args << QLatin1String("diff") << addHeadWhenCommandInProgress()
         << QLatin1String("--") << m_projectPaths;
    runCommand(QList<QStringList>() << addConfigurationArguments(args));
}

class BranchDiffController : public BaseController
{
    Q_OBJECT
public:
    BranchDiffController(IDocument *document, const QString &dir,
                         const QString &branch) :
        BaseController(document, dir),
        m_branch(branch)
    { }

    void reload() override;

private:
    const QString m_branch;
};

void BranchDiffController::reload()
{
    QStringList args;
    args << QLatin1String("diff") << addHeadWhenCommandInProgress() << m_branch;
    runCommand(QList<QStringList>() << addConfigurationArguments(args));
}

class ShowController : public BaseController
{
    Q_OBJECT
public:
    ShowController(IDocument *document, const QString &dir, const QString &id) :
        BaseController(document, dir),
        m_id(id),
        m_state(Idle)
    { }

    void reload() override;
    void processOutput(const QString &output) override;
    void reloadFinished(bool success) override;

private:
    const QString m_id;
    enum State { Idle, GettingDescription, GettingDiff };
    State m_state;
};

void ShowController::reload()
{
    QStringList args;
    args << QLatin1String("show") << QLatin1String("-s") << QLatin1String(noColorOption)
              << QLatin1String(decorateOption) << QLatin1String(showFormatC) << m_id;
    m_state = GettingDescription;
    runCommand(QList<QStringList>() << args, gitClient()->encoding(m_directory, "i18n.commitEncoding"));
}

void ShowController::processOutput(const QString &output)
{
    QTC_ASSERT(m_state != Idle, return);
    if (m_state == GettingDescription)
        setDescription(gitClient()->extendedShowDescription(m_directory, output));
    else if (m_state == GettingDiff)
        processDiff(output, VcsBasePlugin::source(document()));
}

void ShowController::reloadFinished(bool success)
{
    QTC_ASSERT(m_state != Idle, return);

    if (m_state == GettingDescription && success) {
        QStringList args;
        args << QLatin1String("show") << QLatin1String("--format=format:") // omit header, already generated
             << QLatin1String(noColorOption) << QLatin1String(decorateOption) << m_id;
        m_state = GettingDiff;
        runCommand(QList<QStringList>() << addConfigurationArguments(args));
        return;
    }

    m_state = Idle;
    BaseController::reloadFinished(success);
}

///////////////////////////////

class BaseGitDiffArgumentsWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    BaseGitDiffArgumentsWidget(VcsBaseClientSettings &settings, QWidget *parent = 0) :
        VcsBaseEditorParameterWidget(parent)
    {
        m_patienceButton = addToggleButton(
                    QLatin1String("--patience"),
                    tr("Patience"),
                    tr("Use the patience algorithm for calculating the differences."));
        mapSetting(m_patienceButton, settings.boolPointer(GitSettings::diffPatienceKey));
        m_ignoreWSButton = addToggleButton(
                    QLatin1String("--ignore-space-change"), tr("Ignore Whitespace"),
                    tr("Ignore whitespace only changes."));
        mapSetting(m_ignoreWSButton,
                   settings.boolPointer(GitSettings::ignoreSpaceChangesInDiffKey));
    }

protected:
    QToolButton *m_patienceButton;
    QToolButton *m_ignoreWSButton;
};

class GitBlameArgumentsWidget : public VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    GitBlameArgumentsWidget(VcsBaseClientSettings &settings, QWidget *parent = 0) :
        VcsBaseEditorParameterWidget(parent)
    {
        mapSetting(addToggleButton(QString(), tr("Omit Date"),
                                   tr("Hide the date of a change from the output.")),
                   settings.boolPointer(GitSettings::omitAnnotationDateKey));
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore Whitespace"),
                                   tr("Ignore whitespace only changes.")),
                   settings.boolPointer(GitSettings::ignoreSpaceChangesInBlameKey));
    }
};

class GitLogArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    GitLogArgumentsWidget(VcsBaseClientSettings &settings, QWidget *parent = 0) :
        BaseGitDiffArgumentsWidget(settings, parent)
    {
        QToolButton *diffButton = addToggleButton(QLatin1String("--patch"), tr("Show Diff"),
                                              tr("Show difference."));
        mapSetting(diffButton, settings.boolPointer(GitSettings::logDiffKey));
        connect(diffButton, &QToolButton::toggled, m_patienceButton, &QToolButton::setVisible);
        connect(diffButton, &QToolButton::toggled, m_ignoreWSButton, &QToolButton::setVisible);
        m_patienceButton->setVisible(diffButton->isChecked());
        m_ignoreWSButton->setVisible(diffButton->isChecked());
        QStringList graphArguments(QLatin1String("--graph"));
        graphArguments << QLatin1String("--oneline") << QLatin1String("--topo-order");
        graphArguments << (QLatin1String("--pretty=format:") + QLatin1String(graphLogFormatC));
        QToolButton *graphButton = addToggleButton(graphArguments, tr("Graph"),
                                              tr("Show textual graph log."));
        mapSetting(graphButton, settings.boolPointer(GitSettings::graphLogKey));
    }
};

class ConflictHandler : public QObject
{
    Q_OBJECT
public:
    static void attachToCommand(VcsCommand *command, const QString &abortCommand = QString()) {
        ConflictHandler *handler = new ConflictHandler(command->defaultWorkingDirectory(), abortCommand);
        handler->setParent(command); // delete when command goes out of scope

        command->addFlags(VcsCommand::ExpectRepoChanges);
        connect(command, &VcsCommand::stdOutText, handler, &ConflictHandler::readStdOut);
        connect(command, &VcsCommand::stdErrText, handler, &ConflictHandler::readStdErr);
    }

    static void handleResponse(const Utils::SynchronousProcessResponse &response,
                               const QString &workingDirectory,
                               const QString &abortCommand = QString())
    {
        ConflictHandler handler(workingDirectory, abortCommand);
        // No conflicts => do nothing
        if (response.result == SynchronousProcessResponse::Finished)
            return;
        handler.readStdOut(response.stdOut);
        handler.readStdErr(response.stdErr);
    }

private:
    ConflictHandler(const QString &workingDirectory, const QString &abortCommand) :
          m_workingDirectory(workingDirectory),
          m_abortCommand(abortCommand)
    { }

    ~ConflictHandler()
    {
        // If interactive rebase editor window is closed, plugin is terminated
        // but referenced here when the command ends
        if (GitPlugin *plugin = GitPlugin::instance()) {
            GitClient *client = plugin->client();
            if (m_commit.isEmpty() && m_files.isEmpty()) {
                if (client->checkCommandInProgress(m_workingDirectory) == GitClient::NoCommand)
                    client->endStashScope(m_workingDirectory);
            } else {
                client->handleMergeConflicts(m_workingDirectory, m_commit, m_files, m_abortCommand);
            }
        }
    }

    void readStdOut(const QString &data)
    {
        static QRegExp patchFailedRE(QLatin1String("Patch failed at ([^\\n]*)"));
        static QRegExp conflictedFilesRE(QLatin1String("Merge conflict in ([^\\n]*)"));
        if (patchFailedRE.indexIn(data) != -1)
            m_commit = patchFailedRE.cap(1);
        int fileIndex = -1;
        while ((fileIndex = conflictedFilesRE.indexIn(data, fileIndex + 1)) != -1)
            m_files.append(conflictedFilesRE.cap(1));
    }

    void readStdErr(const QString &data)
    {
        static QRegExp couldNotApplyRE(QLatin1String("[Cc]ould not (?:apply|revert) ([^\\n]*)"));
        if (couldNotApplyRE.indexIn(data) != -1)
            m_commit = couldNotApplyRE.cap(1);
    }
private:
    QString m_workingDirectory;
    QString m_abortCommand;
    QString m_commit;
    QStringList m_files;
};

class GitProgressParser : public ProgressParser
{
public:
    static void attachToCommand(VcsCommand *command)
    {
        command->setProgressParser(new GitProgressParser);
    }

private:
    GitProgressParser() : m_progressExp(QLatin1String("\\((\\d+)/(\\d+)\\)")) // e.g. Rebasing (7/42)
    { }

    void parseProgress(const QString &text) override
    {
        if (m_progressExp.lastIndexIn(text) != -1)
            setProgressAndMaximum(m_progressExp.cap(1).toInt(), m_progressExp.cap(2).toInt());
    }

    QRegExp m_progressExp;
};

static inline QString msgRepositoryNotFound(const QString &dir)
{
    return GitClient::tr("Cannot determine the repository for \"%1\".").arg(dir);
}

static inline QString msgParseFilesFailed()
{
    return  GitClient::tr("Cannot parse the file output.");
}

static inline QString msgCannotLaunch(const QString &binary)
{
    return GitClient::tr("Cannot launch \"%1\".").arg(QDir::toNativeSeparators(binary));
}

static inline void msgCannotRun(const QString &message, QString *errorMessage)
{
    if (errorMessage)
        *errorMessage = message;
    else
        VcsOutputWindow::appendError(message);
}

static inline void msgCannotRun(const QStringList &args, const QString &workingDirectory,
                                const QByteArray &error, QString *errorMessage)
{
    const QString message = GitClient::tr("Cannot run \"%1 %2\" in \"%2\": %3")
            .arg(QLatin1String("git ") + args.join(QLatin1Char(' ')),
                 QDir::toNativeSeparators(workingDirectory),
                 GitClient::commandOutputFromLocal8Bit(error));

    msgCannotRun(message, errorMessage);
}

// ---------------- GitClient

const char *GitClient::stashNamePrefix = "stash@{";

GitClient::GitClient() : VcsBase::VcsBaseClientImpl(new GitSettings),
    m_cachedGitVersion(0),
    m_disableEditor(false)
{
    m_gitQtcEditor = QString::fromLatin1("\"%1\" -client -block -pid %2")
            .arg(QCoreApplication::applicationFilePath())
            .arg(QCoreApplication::applicationPid());
}

QString GitClient::findRepositoryForDirectory(const QString &dir) const
{
    if (dir.isEmpty() || dir.endsWith(QLatin1String("/.git"))
            || dir.contains(QLatin1String("/.git/"))) {
        return QString();
    }
    QDir directory(dir);
    QString dotGit = QLatin1String(GIT_DIRECTORY);
    // QFileInfo is outside loop, because it is faster this way
    QFileInfo fileInfo;
    do {
        if (directory.exists(dotGit)) {
            fileInfo.setFile(directory, dotGit);
            if (fileInfo.isFile())
                return directory.absolutePath();
            else if (directory.exists(QLatin1String(".git/config")))
                return directory.absolutePath();
        }
    } while (!directory.isRoot() && directory.cdUp());
    return QString();
}

QString GitClient::findGitDirForRepository(const QString &repositoryDir) const
{
    static QHash<QString, QString> repoDirCache;
    QString &res = repoDirCache[repositoryDir];
    if (!res.isEmpty())
        return res;

    synchronousRevParseCmd(repositoryDir, QLatin1String("--git-dir"), &res);

    if (!QDir(res).isAbsolute())
        res.prepend(repositoryDir + QLatin1Char('/'));
    return res;
}

bool GitClient::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QByteArray output;
    QStringList arguments;
    arguments << QLatin1String("ls-files") << QLatin1String("--error-unmatch") << fileName;
    return vcsFullySynchronousExec(workingDirectory, arguments, &output, 0, silentFlags);
}

QTextCodec *GitClient::codecFor(GitClient::CodecType codecType, const QString &source) const
{
    if (codecType == CodecSource) {
        return QFileInfo(source).isFile() ? VcsBaseEditor::getCodec(source)
                                          : encoding(source, "gui.encoding");
    }
    if (codecType == CodecLogOutput)
        return encoding(source, "i18n.logOutputEncoding");
    return 0;
}

void GitClient::slotChunkActionsRequested(QMenu *menu, bool isValid)
{
    menu->addSeparator();
    QAction *stageChunkAction = menu->addAction(tr("Stage Chunk"));
    connect(stageChunkAction, &QAction::triggered, this, &GitClient::slotStageChunk);
    QAction *unstageChunkAction = menu->addAction(tr("Unstage Chunk"));
    connect(unstageChunkAction, &QAction::triggered, this, &GitClient::slotUnstageChunk);

    m_contextController = qobject_cast<DiffEditorController *>(sender());

    if (!isValid || !m_contextController) {
        stageChunkAction->setEnabled(false);
        unstageChunkAction->setEnabled(false);
    }
}

void GitClient::slotStageChunk()
{
    if (m_contextController.isNull())
        return;

    const QString patch = m_contextController->makePatch(false, true);
    if (patch.isEmpty())
        return;

    stage(patch, false);
}

void GitClient::slotUnstageChunk()
{
    if (m_contextController.isNull())
        return;

    const QString patch = m_contextController->makePatch(true, true);
    if (patch.isEmpty())
        return;

    stage(patch, true);
}

void GitClient::stage(const QString &patch, bool revert)
{
    QTemporaryFile patchFile;
    if (!patchFile.open())
        return;

    const QString baseDir = m_contextController->baseDirectory();
    QTextCodec *codec = EditorManager::defaultTextCodec();
    const QByteArray patchData = codec
            ? codec->fromUnicode(patch) : patch.toLocal8Bit();
    patchFile.write(patchData);
    patchFile.close();

    QStringList args = QStringList() << QLatin1String("--cached");
    if (revert)
        args << QLatin1String("--reverse");
    QString errorMessage;
    if (synchronousApplyPatch(baseDir, patchFile.fileName(),
                              &errorMessage, args)) {
        if (errorMessage.isEmpty()) {
            if (revert)
                VcsOutputWindow::append(tr("Chunk successfully unstaged"));
            else
                VcsOutputWindow::append(tr("Chunk successfully staged"));
        } else {
            VcsOutputWindow::append(errorMessage);
        }
        m_contextController->requestReload();
    } else {
        VcsOutputWindow::appendError(errorMessage);
    }
}

void GitClient::requestReload(const QString &documentId, const QString &source,
                              const QString &title,
                              std::function<DiffEditorController *(IDocument *)> factory) const
{
    // Creating document might change the referenced source. Store a copy and use it.
    const QString sourceCopy = source;

    IDocument *document = DiffEditorController::findOrCreateDocument(documentId, title);
    QTC_ASSERT(document, return);
    DiffEditorController *controller = factory(document);
    QTC_ASSERT(controller, return);

    connect(controller, &DiffEditorController::chunkActionsRequested,
            this, &GitClient::slotChunkActionsRequested, Qt::DirectConnection);
    connect(controller, &DiffEditorController::requestInformationForCommit,
            this, &GitClient::branchesForCommit);

    VcsBasePlugin::setSource(document, sourceCopy);
    EditorManager::activateEditorForDocument(document);
    controller->requestReload();
}

void GitClient::diffFiles(const QString &workingDirectory,
                          const QStringList &unstagedFileNames,
                          const QStringList &stagedFileNames) const
{
    requestReload(QLatin1String("Files:") + workingDirectory,
                  workingDirectory, tr("Git Diff Files"),
                  [this, workingDirectory, stagedFileNames, unstagedFileNames]
                  (IDocument *doc) -> DiffEditorController* {
                      return new FileListDiffController(doc, workingDirectory,
                                                        stagedFileNames, unstagedFileNames);
                  });
}

void GitClient::diffProject(const QString &workingDirectory, const QString &projectDirectory) const
{
    requestReload(QLatin1String("Project:") + workingDirectory,
                  workingDirectory, tr("Git Diff Project"),
                  [this, workingDirectory, projectDirectory]
                  (IDocument *doc) -> DiffEditorController* {
                      return new ProjectDiffController(doc, workingDirectory,
                                                       QStringList(projectDirectory));
                  });
}

void GitClient::diffRepository(const QString &workingDirectory)
{
    requestReload(QLatin1String("Repository:") + workingDirectory,
                  workingDirectory, tr("Git Diff Repository"),
                  [this, workingDirectory](IDocument *doc) -> DiffEditorController* {
                      return new RepositoryDiffController(doc, workingDirectory);
                  });
}

void GitClient::diffFile(const QString &workingDirectory, const QString &fileName) const
{
    const QString title = tr("Git Diff \"%1\"").arg(fileName);
    const QString sourceFile = VcsBaseEditor::getSource(workingDirectory, fileName);
    const QString documentId = QLatin1String("File:") + sourceFile;
    requestReload(documentId, sourceFile, title,
                  [this, workingDirectory, fileName]
                  (IDocument *doc) -> DiffEditorController* {
                      return new FileDiffController(doc, workingDirectory, fileName);
                  });
}

void GitClient::diffBranch(const QString &workingDirectory,
                           const QString &branchName) const
{
    const QString title = tr("Git Diff Branch \"%1\"").arg(branchName);
    const QString documentId = QLatin1String("Branch:") + branchName;
    requestReload(documentId, workingDirectory, title,
                               [this, workingDirectory, branchName]
                               (IDocument *doc) -> DiffEditorController* {
                                   return new BranchDiffController(doc, workingDirectory, branchName);
                               });
}

void GitClient::merge(const QString &workingDirectory,
                      const QStringList &unmergedFileNames)
{
    auto mergeTool = new MergeTool(this);
    if (!mergeTool->start(workingDirectory, unmergedFileNames))
        delete mergeTool;
}

void GitClient::status(const QString &workingDirectory)
{
    QStringList statusArgs;
    statusArgs << QLatin1String("status") << QLatin1String("-u");
    VcsOutputWindow::setRepository(workingDirectory);
    VcsCommand *command = vcsExec(workingDirectory, statusArgs, 0, true);
    connect(command, &VcsCommand::finished, VcsOutputWindow::instance(), &VcsOutputWindow::clearRepository,
            Qt::QueuedConnection);
}

void GitClient::log(const QString &workingDirectory, const QString &fileName,
                    bool enableAnnotationContextMenu, const QStringList &args)
{
    QString msgArg;
    if (!fileName.isEmpty())
        msgArg = fileName;
    else if (!args.isEmpty())
        msgArg = args.first();
    else
        msgArg = workingDirectory;
    // Creating document might change the referenced workingDirectory. Store a copy and use it.
    const QString workingDir = workingDirectory;
    const QString title = tr("Git Log \"%1\"").arg(msgArg);
    const Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    const QString sourceFile = VcsBaseEditor::getSource(workingDir, fileName);
    VcsBaseEditorWidget *editor = createVcsEditor(editorId, title, sourceFile,
                                                  codecFor(CodecLogOutput), "logTitle", msgArg);
    if (!editor->configurationWidget()) {
        auto *argWidget = new GitLogArgumentsWidget(settings());
        connect(argWidget, &VcsBaseEditorParameterWidget::commandExecutionRequested,
                [=]() { this->log(workingDir, fileName, enableAnnotationContextMenu, args); });
        editor->setConfigurationWidget(argWidget);
    }
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);
    editor->setWorkingDirectory(workingDir);

    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption)
              << QLatin1String(decorateOption);

    int logCount = settings().intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << QLatin1String("-n") << QString::number(logCount);

    auto *argWidget = editor->configurationWidget();
    argWidget->setBaseArguments(args);
    QStringList userArgs = argWidget->arguments();

    arguments.append(userArgs);

    if (!fileName.isEmpty())
        arguments << QLatin1String("--follow") << QLatin1String("--") << fileName;

    vcsExec(workingDir, arguments, editor);
}

void GitClient::reflog(const QString &workingDirectory)
{
    const QString title = tr("Git Reflog \"%1\"").arg(workingDirectory);
    const Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    VcsBaseEditorWidget *editor = createVcsEditor(editorId, title, workingDirectory, codecFor(CodecLogOutput),
                                                  "reflogRepository", workingDirectory);
    editor->setWorkingDirectory(workingDirectory);

    QStringList arguments;
    arguments << QLatin1String("reflog") << QLatin1String(noColorOption)
              << QLatin1String(decorateOption);

    int logCount = settings().intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << QLatin1String("-n") << QString::number(logCount);

    vcsExec(workingDirectory, arguments, editor);
}

// Do not show "0000" or "^32ae4"
static inline bool canShow(const QString &sha)
{
    if (sha.startsWith(QLatin1Char('^')))
        return false;
    if (sha.count(QLatin1Char('0')) == sha.size())
        return false;
    return true;
}

static inline QString msgCannotShow(const QString &sha)
{
    return GitClient::tr("Cannot describe \"%1\".").arg(sha);
}

void GitClient::show(const QString &source, const QString &id, const QString &name)
{
    if (!canShow(id)) {
        VcsOutputWindow::appendError(msgCannotShow(id));
        return;
    }

    const QString title = tr("Git Show \"%1\"").arg(name.isEmpty() ? id : name);
    const QFileInfo sourceFi(source);
    QString workingDirectory = sourceFi.isDir()
            ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (!repoDirectory.isEmpty())
        workingDirectory = repoDirectory;
    const QString documentId = QLatin1String("Show:") + id;
    requestReload(documentId, source, title,
                               [this, workingDirectory, id]
                               (IDocument *doc) -> DiffEditorController* {
                                   return new ShowController(doc, workingDirectory, id);
                               });
}

void GitClient::annotate(const QString &workingDir, const QString &file, const QString &revision,
                         int lineNumber, const QStringList &extraOptions)
{
    const Id editorId = Git::Constants::GIT_BLAME_EDITOR_ID;
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(file), revision);
    const QString title = tr("Git Blame \"%1\"").arg(id);
    const QString sourceFile = VcsBaseEditor::getSource(workingDir, file);

    VcsBaseEditorWidget *editor
            = createVcsEditor(editorId, title, sourceFile, codecFor(CodecSource, sourceFile),
                              "blameFileName", id);
    if (!editor->configurationWidget()) {
        auto *argWidget = new GitBlameArgumentsWidget(settings());
        argWidget->setBaseArguments(extraOptions);
        connect(argWidget, &VcsBaseEditorParameterWidget::commandExecutionRequested,
                [=] {
                    const int line = VcsBaseEditor::lineNumberOfCurrentEditor();
                    annotate(workingDir, file, revision, line, extraOptions);
                } );
        editor->setConfigurationWidget(argWidget);
    }

    editor->setWorkingDirectory(workingDir);
    QStringList arguments(QLatin1String("blame"));
    arguments << QLatin1String("--root");
    arguments.append(editor->configurationWidget()->arguments());
    arguments.append(extraOptions);
    arguments << QLatin1String("--") << file;
    if (!revision.isEmpty())
        arguments << revision;
    vcsExec(workingDir, arguments, editor, false, 0, lineNumber);
}

bool GitClient::synchronousCheckout(const QString &workingDirectory,
                                          const QString &ref,
                                          QString *errorMessage /* = 0 */)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments = setupCheckoutArguments(workingDirectory, ref);
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText, 0,
                                            VcsCommand::ExpectRepoChanges);
    VcsOutputWindow::append(commandOutputFromLocal8Bit(outputText));
    if (rc)
        updateSubmodulesIfNeeded(workingDirectory, true);
    else
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);
    return rc;
}

/* method used to setup arguments for checkout, in case user wants to create local branch */
QStringList GitClient::setupCheckoutArguments(const QString &workingDirectory,
                                              const QString &ref)
{
    QStringList arguments(QLatin1String("checkout"));
    arguments << ref;

    QStringList localBranches = synchronousRepositoryBranches(workingDirectory);

    if (localBranches.contains(ref))
        return arguments;

    if (QMessageBox::question(ICore::mainWindow(), tr("Create Local Branch"),
                              tr("Would you like to create a local branch?"),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return arguments;
    }

    if (synchronousCurrentLocalBranch(workingDirectory).isEmpty())
        localBranches.removeFirst();

    QString refSha;
    if (!synchronousRevParseCmd(workingDirectory, ref, &refSha))
        return arguments;

    QString output;
    QStringList forEachRefArgs(QLatin1String("refs/remotes/"));
    forEachRefArgs << QLatin1String("--format=%(objectname) %(refname:short)");
    if (!synchronousForEachRefCmd(workingDirectory, forEachRefArgs, &output))
        return arguments;

    QString remoteBranch;
    const QString head(QLatin1String("/HEAD"));

    foreach (const QString &singleRef, output.split(QLatin1Char('\n'))) {
        if (singleRef.startsWith(refSha)) {
            // branch name might be origin/foo/HEAD
            if (!singleRef.endsWith(head) || singleRef.count(QLatin1Char('/')) > 1) {
                remoteBranch = singleRef.mid(refSha.length() + 1);
                if (remoteBranch == ref)
                    break;
            }
        }
    }

    BranchAddDialog branchAddDialog(localBranches, true, ICore::mainWindow());
    branchAddDialog.setTrackedBranchName(remoteBranch, true);

    if (branchAddDialog.exec() != QDialog::Accepted)
        return arguments;

    arguments.removeLast();
    arguments << QLatin1String("-b") << branchAddDialog.branchName();
    if (branchAddDialog.track())
        arguments << QLatin1String("--track") << remoteBranch;
    else
        arguments << QLatin1String("--no-track") << ref;

    return arguments;
}

void GitClient::reset(const QString &workingDirectory, const QString &argument, const QString &commit)
{
    QStringList arguments;
    arguments << QLatin1String("reset") << argument;
    if (!commit.isEmpty())
        arguments << commit;

    unsigned flags = 0;
    if (argument == QLatin1String("--hard")) {
        if (gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules)) != StatusUnchanged) {
            if (QMessageBox::question(
                        Core::ICore::mainWindow(), tr("Reset"),
                        tr("All changes in working directory will be discarded. Are you sure?"),
                        QMessageBox::Yes | QMessageBox::No,
                        QMessageBox::No) == QMessageBox::No) {
                return;
            }
        }
        flags |= VcsCommand::ExpectRepoChanges;
    }
    vcsExec(workingDirectory, arguments, 0, true, flags);
}

void GitClient::addFile(const QString &workingDirectory, const QString &fileName)
{
    QStringList arguments;
    arguments << QLatin1String("add") << fileName;

    vcsExec(workingDirectory, arguments);
}

bool GitClient::synchronousLog(const QString &workingDirectory, const QStringList &arguments,
                               QString *output, QString *errorMessageIn, unsigned flags)
{
    QByteArray outputData;
    QByteArray errorData;
    QStringList allArguments;
    allArguments << QLatin1String("log") << QLatin1String(noColorOption);
    allArguments.append(arguments);
    const bool rc = vcsFullySynchronousExec(workingDirectory, allArguments, &outputData, &errorData, flags);
    if (rc) {
        if (QTextCodec *codec = encoding(workingDirectory, "i18n.logOutputEncoding"))
            *output = codec->toUnicode(outputData);
        else
            *output = commandOutputFromLocal8Bit(outputData);
    } else {
        msgCannotRun(tr("Cannot obtain log of \"%1\": %2")
                     .arg(QDir::toNativeSeparators(workingDirectory),
                          commandOutputFromLocal8Bit(errorData)), errorMessageIn);

    }
    return rc;
}

bool GitClient::synchronousAdd(const QString &workingDirectory, const QStringList &files)
{
    QByteArray outputText;
    QStringList arguments;
    arguments << QLatin1String("add") << files;
    return vcsFullySynchronousExec(workingDirectory, arguments, &outputText);
}

bool GitClient::synchronousDelete(const QString &workingDirectory,
                                  bool force,
                                  const QStringList &files)
{
    QByteArray outputText;
    QStringList arguments;
    arguments << QLatin1String("rm");
    if (force)
        arguments << QLatin1String("--force");
    arguments.append(files);
    return vcsFullySynchronousExec(workingDirectory, arguments, &outputText);
}

bool GitClient::synchronousMove(const QString &workingDirectory,
                                const QString &from,
                                const QString &to)
{
    QByteArray outputText;
    QStringList arguments;
    arguments << QLatin1String("mv");
    arguments << (from);
    arguments << (to);
    return vcsFullySynchronousExec(workingDirectory, arguments, &outputText);
}

bool GitClient::synchronousReset(const QString &workingDirectory,
                                 const QStringList &files,
                                 QString *errorMessage)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("reset");
    if (files.isEmpty())
        arguments << QLatin1String("--hard");
    else
        arguments << QLatin1String(HEAD) << QLatin1String("--") << files;
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText, &errorText);
    const QString output = commandOutputFromLocal8Bit(outputText);
    VcsOutputWindow::append(output);
    // Note that git exits with 1 even if the operation is successful
    // Assume real failure if the output does not contain "foo.cpp modified"
    // or "Unstaged changes after reset" (git 1.7.0).
    if (!rc && (!output.contains(QLatin1String("modified"))
                && !output.contains(QLatin1String("Unstaged changes after reset")))) {
        if (files.isEmpty()) {
            msgCannotRun(arguments, workingDirectory, errorText, errorMessage);
        } else {
            msgCannotRun(tr("Cannot reset %n file(s) in \"%1\": %2", 0, files.size())
                         .arg(QDir::toNativeSeparators(workingDirectory),
                              commandOutputFromLocal8Bit(errorText)),
                         errorMessage);
        }
        return false;
    }
    return true;
}

// Initialize repository
bool GitClient::synchronousInit(const QString &workingDirectory)
{
    QByteArray outputText;
    const QStringList arguments(QLatin1String("init"));
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText);
    // '[Re]Initialized...'
    VcsOutputWindow::append(commandOutputFromLocal8Bit(outputText));
    if (rc)
        resetCachedVcsInfo(workingDirectory);
    return rc;
}

/* Checkout, supports:
 * git checkout -- <files>
 * git checkout revision -- <files>
 * git checkout revision -- . */
bool GitClient::synchronousCheckoutFiles(const QString &workingDirectory,
                                         QStringList files /* = QStringList() */,
                                         QString revision /* = QString() */,
                                         QString *errorMessage /* = 0 */,
                                         bool revertStaging /* = true */)
{
    if (revertStaging && revision.isEmpty())
        revision = QLatin1String(HEAD);
    if (files.isEmpty())
        files = QStringList(QString(QLatin1Char('.')));
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("checkout");
    if (revertStaging)
        arguments << revision;
    arguments << QLatin1String("--") << files;
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText, &errorText,
                                            VcsCommand::ExpectRepoChanges);
    if (!rc) {
        const QString fileArg = files.join(QLatin1String(", "));
        //: Meaning of the arguments: %1: revision, %2: files, %3: repository,
        //: %4: Error message
        msgCannotRun(tr("Cannot checkout \"%1\" of %2 in \"%3\": %4")
                     .arg(revision, fileArg, workingDirectory,
                          commandOutputFromLocal8Bit(errorText)),
                     errorMessage);
        return false;
    }
    return true;
}

bool GitClient::stashAndCheckout(const QString &workingDirectory, const QString &ref)
{
    if (!beginStashScope(workingDirectory, QLatin1String("Checkout")))
        return false;
    if (!synchronousCheckout(workingDirectory, ref))
        return false;
    endStashScope(workingDirectory);
    return true;
}

static inline QString msgParentRevisionFailed(const QString &workingDirectory,
                                              const QString &revision,
                                              const QString &why)
{
    //: Failed to find parent revisions of a SHA1 for "annotate previous"
    return GitClient::tr("Cannot find parent revisions of \"%1\" in \"%2\": %3").arg(revision, workingDirectory, why);
}

static inline QString msgInvalidRevision()
{
    return GitClient::tr("Invalid revision");
}

// Split a line of "<commit> <parent1> ..." to obtain parents from "rev-list" or "log".
static inline bool splitCommitParents(const QString &line,
                                      QString *commit = 0,
                                      QStringList *parents = 0)
{
    if (commit)
        commit->clear();
    if (parents)
        parents->clear();
    QStringList tokens = line.trimmed().split(QLatin1Char(' '));
    if (tokens.size() < 2)
        return false;
    if (commit)
        *commit = tokens.front();
    tokens.pop_front();
    if (parents)
        *parents = tokens;
    return true;
}

bool GitClient::synchronousRevListCmd(const QString &workingDirectory, const QStringList &arguments,
                                      QString *output, QString *errorMessage) const
{
    QByteArray outputTextData;
    QByteArray errorText;

    QStringList args(QLatin1String("rev-list"));
    args << QLatin1String(noColorOption) << arguments;

    const bool rc = vcsFullySynchronousExec(workingDirectory, args, &outputTextData, &errorText,
                                            silentFlags);
    if (!rc) {
        msgCannotRun(args, workingDirectory, errorText, errorMessage);
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputTextData);
    return true;
}

// Find out the immediate parent revisions of a revision of the repository.
// Might be several in case of merges.
bool GitClient::synchronousParentRevisions(const QString &workingDirectory,
                                           const QString &revision,
                                           QStringList *parents,
                                           QString *errorMessage) const
{
    QString outputText;
    QString errorText;
    QStringList arguments;
    if (parents && !isValidRevision(revision)) { // Not Committed Yet
        *parents = QStringList(QLatin1String(HEAD));
        return true;
    }
    arguments << QLatin1String("--parents") << QLatin1String("--max-count=1") << revision;

    if (!synchronousRevListCmd(workingDirectory, arguments, &outputText, &errorText)) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, errorText);
        return false;
    }
    // Should result in one line of blank-delimited revisions, specifying current first
    // unless it is top.
    outputText.remove(QLatin1Char('\n'));
    if (!splitCommitParents(outputText, 0, parents)) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, msgInvalidRevision());
        return false;
    }
    return true;
}

// Short SHA1, author, subject
static const char defaultShortLogFormatC[] = "%h (%an \"%s";
static const int maxShortLogLength = 120;

QString GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision) const
{
    // Short SHA 1, author, subject
    QString output = synchronousShortDescription(workingDirectory, revision,
                                                 QLatin1String(defaultShortLogFormatC));
    if (output != revision) {
        if (output.length() > maxShortLogLength) {
            output.truncate(maxShortLogLength);
            output.append(QLatin1String("..."));
        }
        output.append(QLatin1String("\")"));
    }
    return output;
}

QString GitClient::synchronousCurrentLocalBranch(const QString &workingDirectory) const
{
    QByteArray outputTextData;
    QStringList arguments;
    QString branch;
    arguments << QLatin1String("symbolic-ref") << QLatin1String(HEAD);
    if (vcsFullySynchronousExec(workingDirectory, arguments, &outputTextData, 0, silentFlags)) {
        branch = commandOutputFromLocal8Bit(outputTextData.trimmed());
    } else {
        const QString gitDir = findGitDirForRepository(workingDirectory);
        const QString rebaseHead = gitDir + QLatin1String("/rebase-merge/head-name");
        QFile head(rebaseHead);
        if (head.open(QFile::ReadOnly))
            branch = QString::fromUtf8(head.readLine()).trimmed();
    }
    if (!branch.isEmpty()) {
        const QString refsHeadsPrefix = QLatin1String("refs/heads/");
        if (branch.startsWith(refsHeadsPrefix)) {
            branch.remove(0, refsHeadsPrefix.count());
            return branch;
        }
    }
    return QString();
}

bool GitClient::synchronousHeadRefs(const QString &workingDirectory, QStringList *output,
                                    QString *errorMessage) const
{
    QStringList args;
    args << QLatin1String("show-ref") << QLatin1String("--head")
         << QLatin1String("--abbrev=10") << QLatin1String("--dereference");
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, args, &outputText, &errorText,
                                            silentFlags);
    if (!rc) {
        msgCannotRun(args, workingDirectory, errorText, errorMessage);
        return false;
    }

    QByteArray headSha = outputText.left(10);
    QByteArray newLine("\n");

    int currentIndex = 15;

    while (true) {
        currentIndex = outputText.indexOf(headSha, currentIndex);
        if (currentIndex < 0)
            break;
        currentIndex += 11;
        output->append(QString::fromLocal8Bit(outputText.mid(currentIndex,
                                outputText.indexOf(newLine, currentIndex) - currentIndex)));
   }

    return true;
}

// Retrieve topic (branch, tag or HEAD hash)
QString GitClient::synchronousTopic(const QString &workingDirectory) const
{
    // First try to find branch
    QString branch = synchronousCurrentLocalBranch(workingDirectory);
    if (!branch.isEmpty())
        return branch;

    // Detached HEAD, try a tag or remote branch
    QStringList references;
    if (!synchronousHeadRefs(workingDirectory, &references))
        return QString();

    const QString tagStart(QLatin1String("refs/tags/"));
    const QString remoteStart(QLatin1String("refs/remotes/"));
    const QString dereference(QLatin1String("^{}"));
    QString remoteBranch;

    foreach (const QString &ref, references) {
        int derefInd = ref.indexOf(dereference);
        if (ref.startsWith(tagStart))
            return ref.mid(tagStart.size(), (derefInd == -1) ? -1 : derefInd - tagStart.size());
        if (ref.startsWith(remoteStart)) {
            remoteBranch = ref.mid(remoteStart.size(),
                                   (derefInd == -1) ? -1 : derefInd - remoteStart.size());
        }
    }
    if (!remoteBranch.isEmpty())
        return remoteBranch;

    // No tag or remote branch - try git describe
    QByteArray output;
    QStringList arguments;
    arguments << QLatin1String("describe");
    if (vcsFullySynchronousExec(workingDirectory, arguments, &output, 0, VcsCommand::NoOutput)) {
        const QString describeOutput = commandOutputFromLocal8Bit(output.trimmed());
        if (!describeOutput.isEmpty())
            return describeOutput;
    }
    return tr("Detached HEAD");
}

bool GitClient::synchronousRevParseCmd(const QString &workingDirectory, const QString &ref,
                                       QString *output, QString *errorMessage) const
{
    QStringList arguments(QLatin1String("rev-parse"));
    arguments << ref;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText, &errorText,
                                            silentFlags);
    *output = commandOutputFromLocal8Bit(outputText.trimmed());
    if (!rc)
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);

    return rc;
}

// Retrieve head revision
QString GitClient::synchronousTopRevision(const QString &workingDirectory, QString *errorMessageIn)
{
    QString revision;
    if (!synchronousRevParseCmd(workingDirectory, QLatin1String(HEAD), &revision, errorMessageIn))
        return QString();

    return revision;
}

void GitClient::synchronousTagsForCommit(const QString &workingDirectory, const QString &revision,
                                         QString &precedes, QString &follows) const
{
    QByteArray pr;
    QStringList arguments;
    arguments << QLatin1String("describe") << QLatin1String("--contains") << revision;
    vcsFullySynchronousExec(workingDirectory, arguments, &pr, 0, silentFlags);
    int tilde = pr.indexOf('~');
    if (tilde != -1)
        pr.truncate(tilde);
    else
        pr = pr.trimmed();
    precedes = QString::fromLocal8Bit(pr);

    QStringList parents;
    QString errorMessage;
    synchronousParentRevisions(workingDirectory, revision, &parents, &errorMessage);
    foreach (const QString &p, parents) {
        QByteArray pf;
        arguments.clear();
        arguments << QLatin1String("describe") << QLatin1String("--tags")
                  << QLatin1String("--abbrev=0") << p;
        vcsFullySynchronousExec(workingDirectory, arguments, &pf, 0, silentFlags);
        pf.truncate(pf.lastIndexOf('\n'));
        if (!pf.isEmpty()) {
            if (!follows.isEmpty())
                follows += QLatin1String(", ");
            follows += QString::fromLocal8Bit(pf);
        }
    }
}

void GitClient::branchesForCommit(const QString &revision)
{
    QStringList arguments;
    arguments << QLatin1String("branch") << QLatin1String(noColorOption)
              << QLatin1String("-a") << QLatin1String("--contains") << revision;

    auto controller = qobject_cast<DiffEditorController *>(sender());
    QString workingDirectory = controller->baseDirectory();
    VcsCommand *command = vcsExec(workingDirectory, arguments, 0, false, 0, workingDirectory);
    connect(command, &VcsCommand::stdOutText, controller,
            &DiffEditorController::informationForCommitReceived);
}

bool GitClient::isRemoteCommit(const QString &workingDirectory, const QString &commit)
{
    QStringList arguments;
    QByteArray outputText;
    arguments << QLatin1String("branch") << QLatin1String("-r")
              << QLatin1String("--contains") << commit;
    vcsFullySynchronousExec(workingDirectory, arguments, &outputText, 0, silentFlags);
    return !outputText.isEmpty();
}

bool GitClient::isFastForwardMerge(const QString &workingDirectory, const QString &branch)
{
    QStringList arguments;
    QByteArray outputText;
    arguments << QLatin1String("merge-base") << QLatin1String(HEAD) << branch;
    vcsFullySynchronousExec(workingDirectory, arguments, &outputText, 0, silentFlags);
    return commandOutputFromLocal8Bit(outputText).trimmed()
            == synchronousTopRevision(workingDirectory);
}

// Format an entry in a one-liner for selection list using git log.
QString GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                            const QString &format) const
{
    QString description;
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption)
              << (QLatin1String("--pretty=format:") + format)
              << QLatin1String("--max-count=1") << revision;
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputTextData, &errorText,
                                            silentFlags);
    if (!rc) {
        VcsOutputWindow::appendSilently(tr("Cannot describe revision \"%1\" in \"%2\": %3")
                                     .arg(revision, workingDirectory, commandOutputFromLocal8Bit(errorText)));
        return revision;
    }
    description = commandOutputFromLocal8Bit(outputTextData);
    if (description.endsWith(QLatin1Char('\n')))
        description.truncate(description.size() - 1);
    return description;
}

// Create a default message to be used for describing stashes
static inline QString creatorStashMessage(const QString &keyword = QString())
{
    QString rc = QCoreApplication::applicationName();
    rc += QLatin1Char(' ');
    if (!keyword.isEmpty()) {
        rc += keyword;
        rc += QLatin1Char(' ');
    }
    rc += QDateTime::currentDateTime().toString(Qt::ISODate);
    return rc;
}

/* Do a stash and return the message as identifier. Note that stash names (stash{n})
 * shift as they are pushed, so, enforce the use of messages to identify them. Flags:
 * StashPromptDescription: Prompt the user for a description message.
 * StashImmediateRestore: Immediately re-apply this stash (used for snapshots), user keeps on working
 * StashIgnoreUnchanged: Be quiet about unchanged repositories (used for IVersionControl's snapshots). */

QString GitClient::synchronousStash(const QString &workingDirectory, const QString &messageKeyword,
                                    unsigned flags, bool *unchanged) const
{
    if (unchanged)
        *unchanged = false;
    QString message;
    bool success = false;
    // Check for changes and stash
    QString errorMessage;
    switch (gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules), 0, &errorMessage)) {
    case  StatusChanged: {
        message = creatorStashMessage(messageKeyword);
        do {
            if ((flags & StashPromptDescription)) {
                if (!inputText(ICore::mainWindow(),
                               tr("Stash Description"), tr("Description:"), &message))
                    break;
            }
            if (!executeSynchronousStash(workingDirectory, message))
                break;
            if ((flags & StashImmediateRestore)
                && !synchronousStashRestore(workingDirectory, QLatin1String("stash@{0}")))
                break;
            success = true;
        } while (false);
        break;
    }
    case StatusUnchanged:
        if (unchanged)
            *unchanged = true;
        if (!(flags & StashIgnoreUnchanged))
            VcsOutputWindow::appendWarning(msgNoChangedFiles());
        break;
    case StatusFailed:
        VcsOutputWindow::appendError(errorMessage);
        break;
    }
    if (!success)
        message.clear();
    return message;
}

bool GitClient::executeSynchronousStash(const QString &workingDirectory,
                                        const QString &message,
                                        bool unstagedOnly,
                                        QString *errorMessage) const
{
    QStringList arguments;
    arguments << QLatin1String("stash") << QLatin1String("save");
    if (unstagedOnly)
        arguments << QLatin1String("--keep-index");
    if (!message.isEmpty())
        arguments << message;
    const unsigned flags = VcsCommand::ShowStdOut
            | VcsCommand::ExpectRepoChanges
            | VcsCommand::ShowSuccessMessage;
    const SynchronousProcessResponse response = vcsSynchronousExec(workingDirectory, arguments, flags);
    const bool rc = response.result == SynchronousProcessResponse::Finished;
    if (!rc)
        msgCannotRun(arguments, workingDirectory, response.stdErr.toLocal8Bit(), errorMessage);

    return rc;
}

// Resolve a stash name from message
bool GitClient::stashNameFromMessage(const QString &workingDirectory,
                                     const QString &message, QString *name,
                                     QString *errorMessage) const
{
    // All happy
    if (message.startsWith(QLatin1String(stashNamePrefix))) {
        *name = message;
        return true;
    }
    // Retrieve list and find via message
    QList<Stash> stashes;
    if (!synchronousStashList(workingDirectory, &stashes, errorMessage))
        return false;
    foreach (const Stash &s, stashes) {
        if (s.message == message) {
            *name = s.name;
            return true;
        }
    }
    //: Look-up of a stash via its descriptive message failed.
    msgCannotRun(tr("Cannot resolve stash message \"%1\" in \"%2\".")
                 .arg(message, workingDirectory), errorMessage);
    return  false;
}

bool GitClient::synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                                     QString *output, QString *errorMessage) const
{
    branchArgs.push_front(QLatin1String("branch"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, branchArgs, &outputText, &errorText);
    *output = commandOutputFromLocal8Bit(outputText);
    if (!rc)
        msgCannotRun(branchArgs, workingDirectory, errorText, errorMessage);

    return rc;
}

bool GitClient::synchronousTagCmd(const QString &workingDirectory, QStringList tagArgs,
                                  QString *output, QString *errorMessage) const
{
    tagArgs.push_front(QLatin1String("tag"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, tagArgs, &outputText, &errorText);
    *output = commandOutputFromLocal8Bit(outputText);
    if (!rc)
        msgCannotRun(tagArgs, workingDirectory, errorText, errorMessage);

    return rc;
}

bool GitClient::synchronousForEachRefCmd(const QString &workingDirectory, QStringList args,
                                      QString *output, QString *errorMessage) const
{
    args.push_front(QLatin1String("for-each-ref"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, args, &outputText, &errorText,
                                            silentFlags);
    *output = SynchronousProcess::normalizeNewlines(QString::fromUtf8(outputText));
    if (!rc)
        msgCannotRun(args, workingDirectory, errorText, errorMessage);

    return rc;
}

bool GitClient::synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                                     QString *output, QString *errorMessage, bool silent) const
{
    remoteArgs.push_front(QLatin1String("remote"));
    QByteArray outputText;
    QByteArray errorText;
    if (!vcsFullySynchronousExec(workingDirectory, remoteArgs, &outputText, &errorText,
                                 silent ? silentFlags : 0)) {
        msgCannotRun(remoteArgs, workingDirectory, errorText, errorMessage);
        return false;
    }
    if (output)
        *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

QMap<QString,QString> GitClient::synchronousRemotesList(const QString &workingDirectory,
                                                        QString *errorMessage) const
{
    QMap<QString,QString> result;
    QString output;
    QString error;
    QStringList args(QLatin1String("-v"));
    if (!synchronousRemoteCmd(workingDirectory, args, &output, &error, true)) {
        msgCannotRun(error, errorMessage);
        return result;
    }
    QStringList remotes = output.split(QLatin1String("\n"));

    foreach (const QString &remote, remotes) {
        if (!remote.endsWith(QLatin1String(" (push)")))
            continue;

        int tabIndex = remote.indexOf(QLatin1Char('\t'));
        if (tabIndex == -1)
            continue;
        QString url = remote.mid(tabIndex + 1, remote.length() - tabIndex - 8);
        result.insert(remote.left(tabIndex), url);
    }
    return result;
}

QStringList GitClient::synchronousSubmoduleStatus(const QString &workingDirectory,
                                                  QString *errorMessage) const
{
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;

    // get submodule status
    arguments << QLatin1String("submodule") << QLatin1String("status");
    if (!vcsFullySynchronousExec(workingDirectory, arguments, &outputTextData, &errorText,
                                 silentFlags)) {
        msgCannotRun(tr("Cannot retrieve submodule status of \"%1\": %2")
                     .arg(QDir::toNativeSeparators(workingDirectory),
                          commandOutputFromLocal8Bit(errorText)), errorMessage);
        return QStringList();
    }
    return commandOutputLinesFromLocal8Bit(outputTextData);
}

SubmoduleDataMap GitClient::submoduleList(const QString &workingDirectory) const
{
    SubmoduleDataMap result;
    QString gitmodulesFileName = workingDirectory + QLatin1String("/.gitmodules");
    if (!QFile::exists(gitmodulesFileName))
        return result;

    static QMap<QString, SubmoduleDataMap> cachedSubmoduleData;

    if (cachedSubmoduleData.contains(workingDirectory))
        return cachedSubmoduleData.value(workingDirectory);

    QStringList allConfigs = readConfigValue(workingDirectory, QLatin1String("-l"))
            .split(QLatin1Char('\n'));
    const QString submoduleLineStart = QLatin1String("submodule.");
    foreach (const QString &configLine, allConfigs) {
        if (!configLine.startsWith(submoduleLineStart))
            continue;

        int nameStart = submoduleLineStart.size();
        int nameEnd   = configLine.indexOf(QLatin1Char('.'), nameStart);

        QString submoduleName = configLine.mid(nameStart, nameEnd - nameStart);

        SubmoduleData submoduleData;
        if (result.contains(submoduleName))
            submoduleData = result[submoduleName];

        if (configLine.mid(nameEnd, 5) == QLatin1String(".url="))
            submoduleData.url = configLine.mid(nameEnd + 5);
        else if (configLine.mid(nameEnd, 8) == QLatin1String(".ignore="))
            submoduleData.ignore = configLine.mid(nameEnd + 8);
        else
            continue;

        result.insert(submoduleName, submoduleData);
    }

    // if config found submodules
    if (!result.isEmpty()) {
        QSettings gitmodulesFile(gitmodulesFileName, QSettings::IniFormat);

        foreach (const QString &submoduleName, result.keys()) {
            gitmodulesFile.beginGroup(QLatin1String("submodule \"") +
                                      submoduleName + QLatin1Char('"'));
            result[submoduleName].dir = gitmodulesFile.value(QLatin1String("path")).toString();
            QString ignore = gitmodulesFile.value(QLatin1String("ignore")).toString();
            if (!ignore.isEmpty() && result[submoduleName].ignore.isEmpty())
                result[submoduleName].ignore = ignore;
            gitmodulesFile.endGroup();
        }
    }
    cachedSubmoduleData.insert(workingDirectory, result);

    return result;
}

bool GitClient::synchronousShow(const QString &workingDirectory, const QString &id,
                                 QString *output, QString *errorMessage) const
{
    if (!canShow(id)) {
        *errorMessage = msgCannotShow(id);
        return false;
    }
    QStringList args(QLatin1String("show"));
    args << QLatin1String(decorateOption) << QLatin1String(noColorOption) << id;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, args, &outputText, &errorText);
    if (rc)
        *output = commandOutputFromLocal8Bit(outputText);
    else
        msgCannotRun(QStringList(QLatin1String("show")), workingDirectory, errorText, errorMessage);
    return rc;
}

// Retrieve list of files to be cleaned
bool GitClient::cleanList(const QString &workingDirectory, const QString &flag, QStringList *files, QString *errorMessage)
{
    QStringList args;
    args << QLatin1String("clean") << QLatin1String("--dry-run") << flag;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, args, &outputText, &errorText);
    if (!rc) {
        msgCannotRun(QStringList(QLatin1String("clean")), workingDirectory,
                     errorText, errorMessage);
        return false;
    }
    // Filter files that git would remove
    const QString prefix = QLatin1String("Would remove ");
    foreach (const QString &line, commandOutputLinesFromLocal8Bit(outputText))
        if (line.startsWith(prefix))
            files->push_back(line.mid(prefix.size()));
    return true;
}

bool GitClient::synchronousCleanList(const QString &workingDirectory, QStringList *files,
                                     QStringList *ignoredFiles, QString *errorMessage)
{
    bool res = cleanList(workingDirectory, QLatin1String("-df"), files, errorMessage);
    res &= cleanList(workingDirectory, QLatin1String("-dXf"), ignoredFiles, errorMessage);

    SubmoduleDataMap submodules = submoduleList(workingDirectory);
    foreach (const SubmoduleData &submodule, submodules) {
        if (submodule.ignore != QLatin1String("all")
                && submodule.ignore != QLatin1String("dirty")) {
            res &= synchronousCleanList(workingDirectory + QLatin1Char('/') + submodule.dir,
                                        files, ignoredFiles, errorMessage);
        }
    }
    return res;
}

bool GitClient::synchronousApplyPatch(const QString &workingDirectory,
                                      const QString &file, QString *errorMessage,
                                      const QStringList &arguments)
{
    QStringList args;
    args << QLatin1String("apply") << QLatin1String("--whitespace=fix") << arguments << file;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, args, &outputText, &errorText);
    if (rc) {
        if (!errorText.isEmpty())
            *errorMessage = tr("There were warnings while applying \"%1\" to \"%2\":\n%3")
                .arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
    } else {
        *errorMessage = tr("Cannot apply patch \"%1\" to \"%2\": %3")
                .arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    return true;
}

QProcessEnvironment GitClient::processEnvironment() const
{
    QProcessEnvironment environment = VcsBaseClientImpl::processEnvironment();
    QString gitPath = settings().stringValue(GitSettings::pathKey);
    if (!gitPath.isEmpty()) {
        gitPath += HostOsInfo::pathListSeparator();
        gitPath += environment.value(QLatin1String("PATH"));
        environment.insert(QLatin1String("PATH"), gitPath);
    }
    if (HostOsInfo::isWindowsHost()
            && settings().boolValue(GitSettings::winSetHomeEnvironmentKey)) {
        environment.insert(QLatin1String("HOME"), QDir::toNativeSeparators(QDir::homePath()));
    }
    environment.insert(QLatin1String("GIT_EDITOR"), m_disableEditor ? QLatin1String("true") : m_gitQtcEditor);
    return environment;
}

bool GitClient::beginStashScope(const QString &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    QTC_ASSERT(!repoDirectory.isEmpty(), return false);
    StashInfo &stashInfo = m_stashInfo[repoDirectory];
    return stashInfo.init(repoDirectory, command, flag, pushAction);
}

GitClient::StashInfo &GitClient::stashInfo(const QString &workingDirectory)
{
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    QTC_CHECK(m_stashInfo.contains(repoDirectory));
    return m_stashInfo[repoDirectory];
}

void GitClient::endStashScope(const QString &workingDirectory)
{
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (!m_stashInfo.contains(repoDirectory))
        return;
    m_stashInfo[repoDirectory].end();
}

bool GitClient::isValidRevision(const QString &revision) const
{
    if (revision.length() < 1)
        return false;
    for (int i = 0; i < revision.length(); ++i)
        if (revision.at(i) != QLatin1Char('0'))
            return true;
    return false;
}

void GitClient::updateSubmodulesIfNeeded(const QString &workingDirectory, bool prompt)
{
    if (!m_updatedSubmodules.isEmpty() || submoduleList(workingDirectory).isEmpty())
        return;

    QStringList submoduleStatus = synchronousSubmoduleStatus(workingDirectory);
    if (submoduleStatus.isEmpty())
        return;

    bool updateNeeded = false;
    foreach (const QString &status, submoduleStatus) {
        if (status.startsWith(QLatin1Char('+'))) {
            updateNeeded = true;
            break;
        }
    }
    if (!updateNeeded)
        return;

    if (prompt && QMessageBox::question(ICore::mainWindow(), tr("Submodules Found"),
            tr("Would you like to update submodules?"),
            QMessageBox::Yes | QMessageBox::No) == QMessageBox::No) {
        return;
    }

    foreach (const QString &statusLine, submoduleStatus) {
        // stash only for lines starting with +
        // because only they would be updated
        if (!statusLine.startsWith(QLatin1Char('+')))
            continue;

        // get submodule name
        const int nameStart  = statusLine.indexOf(QLatin1Char(' '), 2) + 1;
        const int nameLength = statusLine.indexOf(QLatin1Char(' '), nameStart) - nameStart;
        const QString submoduleDir = workingDirectory + QLatin1Char('/')
                + statusLine.mid(nameStart, nameLength);

        if (beginStashScope(submoduleDir, QLatin1String("SubmoduleUpdate"))) {
            m_updatedSubmodules.append(submoduleDir);
        } else {
            finishSubmoduleUpdate();
            return;
        }
    }

    QStringList arguments;
    arguments << QLatin1String("submodule") << QLatin1String("update");

    VcsCommand *cmd = vcsExec(workingDirectory, arguments, 0, true,
                              VcsCommand::ExpectRepoChanges);
    connect(cmd, &VcsCommand::finished, this, &GitClient::finishSubmoduleUpdate);
}

void GitClient::finishSubmoduleUpdate()
{
    foreach (const QString &submoduleDir, m_updatedSubmodules)
        endStashScope(submoduleDir);
    m_updatedSubmodules.clear();
}

void GitClient::fetchFinished(const QVariant &cookie)
{
    GitPlugin::instance()->updateBranches(cookie.toString());
}

GitClient::StatusResult GitClient::gitStatus(const QString &workingDirectory, StatusMode mode,
                                             QString *output, QString *errorMessage) const
{
    // Run 'status'. Note that git returns exitcode 1 if there are no added files.
    QByteArray outputText;
    QByteArray errorText;

    QStringList statusArgs;
    statusArgs << QLatin1String("status");
    if (mode & NoUntracked)
        statusArgs << QLatin1String("--untracked-files=no");
    else
        statusArgs << QLatin1String("--untracked-files=all");
    if (mode & NoSubmodules)
        statusArgs << QLatin1String("--ignore-submodules=all");
    statusArgs << QLatin1String("--porcelain") << QLatin1String("-b");

    const bool statusRc = vcsFullySynchronousExec(workingDirectory, statusArgs,
                                                  &outputText, &errorText, silentFlags);
    if (output)
        *output = commandOutputFromLocal8Bit(outputText);

    static const char * NO_BRANCH = "## HEAD (no branch)\n";

    const bool branchKnown = !outputText.startsWith(NO_BRANCH);
    // Is it something really fatal?
    if (!statusRc && !branchKnown) {
        if (errorMessage) {
            const QString error = commandOutputFromLocal8Bit(errorText);
            *errorMessage = tr("Cannot obtain status: %1").arg(error);
        }
        return StatusFailed;
    }
    // Unchanged (output text depending on whether -u was passed)
    QList<QByteArray> lines = outputText.split('\n');
    foreach (const QByteArray &line, lines)
        if (!line.isEmpty() && !line.startsWith('#'))
            return StatusChanged;
    return StatusUnchanged;
}

QString GitClient::commandInProgressDescription(const QString &workingDirectory) const
{
    switch (checkCommandInProgress(workingDirectory)) {
    case NoCommand:
        break;
    case Rebase:
    case RebaseMerge:
        return tr("REBASING");
    case Revert:
        return tr("REVERTING");
    case CherryPick:
        return tr("CHERRY-PICKING");
    case Merge:
        return tr("MERGING");
    }
    return QString();
}

GitClient::CommandInProgress GitClient::checkCommandInProgress(const QString &workingDirectory) const
{
    const QString gitDir = findGitDirForRepository(workingDirectory);
    if (QFile::exists(gitDir + QLatin1String("/MERGE_HEAD")))
        return Merge;
    else if (QFile::exists(gitDir + QLatin1String("/rebase-apply/rebasing")))
        return Rebase;
    else if (QFile::exists(gitDir + QLatin1String("/rebase-merge")))
        return RebaseMerge;
    else if (QFile::exists(gitDir + QLatin1String("/REVERT_HEAD")))
        return Revert;
    else if (QFile::exists(gitDir + QLatin1String("/CHERRY_PICK_HEAD")))
        return CherryPick;
    else
        return NoCommand;
}

void GitClient::continueCommandIfNeeded(const QString &workingDirectory, bool allowContinue)
{
    CommandInProgress command = checkCommandInProgress(workingDirectory);
    ContinueCommandMode continueMode;
    if (allowContinue)
        continueMode = command == RebaseMerge ? ContinueOnly : SkipIfNoChanges;
    else
        continueMode = SkipOnly;
    switch (command) {
    case Rebase:
    case RebaseMerge:
        continuePreviousGitCommand(workingDirectory, tr("Continue Rebase"),
                                   tr("Rebase is in progress. What do you want to do?"),
                                   tr("Continue"), QLatin1String("rebase"), continueMode);
        break;
    case Merge:
        continuePreviousGitCommand(workingDirectory, tr("Continue Merge"),
                tr("You need to commit changes to finish merge.\nCommit now?"),
                tr("Commit"), QLatin1String("merge"), continueMode);
        break;
    case Revert:
        continuePreviousGitCommand(workingDirectory, tr("Continue Revert"),
                tr("You need to commit changes to finish revert.\nCommit now?"),
                tr("Commit"), QLatin1String("revert"), continueMode);
        break;
    case CherryPick:
        continuePreviousGitCommand(workingDirectory, tr("Continue Cherry-Picking"),
                tr("You need to commit changes to finish cherry-picking.\nCommit now?"),
                tr("Commit"), QLatin1String("cherry-pick"), continueMode);
        break;
    default:
        break;
    }
}

void GitClient::continuePreviousGitCommand(const QString &workingDirectory,
                                           const QString &msgBoxTitle, QString msgBoxText,
                                           const QString &buttonName, const QString &gitCommand,
                                           ContinueCommandMode continueMode)
{
    bool isRebase = gitCommand == QLatin1String("rebase");
    bool hasChanges = false;
    switch (continueMode) {
    case ContinueOnly:
        hasChanges = true;
        break;
    case SkipIfNoChanges:
        hasChanges = gitStatus(workingDirectory, StatusMode(NoUntracked | NoSubmodules))
            == GitClient::StatusChanged;
        if (!hasChanges)
            msgBoxText.prepend(tr("No changes found.") + QLatin1Char(' '));
        break;
    case SkipOnly:
        hasChanges = false;
        break;
    }

    QMessageBox msgBox(QMessageBox::Question, msgBoxTitle, msgBoxText,
                       QMessageBox::NoButton, ICore::mainWindow());
    if (hasChanges || isRebase)
        msgBox.addButton(hasChanges ? buttonName : tr("Skip"), QMessageBox::AcceptRole);
    msgBox.addButton(QMessageBox::Abort);
    msgBox.addButton(QMessageBox::Ignore);
    switch (msgBox.exec()) {
    case QMessageBox::Ignore:
        break;
    case QMessageBox::Abort:
        synchronousAbortCommand(workingDirectory, gitCommand);
        break;
    default: // Continue/Skip
        if (isRebase)
            rebase(workingDirectory, QLatin1String(hasChanges ? "--continue" : "--skip"));
        else
            GitPlugin::instance()->startCommit();
    }
}

QString GitClient::extendedShowDescription(const QString &workingDirectory, const QString &text) const
{
    if (!text.startsWith(QLatin1String("commit ")))
        return text;
    QString modText = text;
    QString precedes, follows;
    int lastHeaderLine = modText.indexOf(QLatin1String("\n\n")) + 1;
    const QString commit = modText.mid(7, 8);
    synchronousTagsForCommit(workingDirectory, commit, precedes, follows);
    if (!precedes.isEmpty())
        modText.insert(lastHeaderLine, QLatin1String("Precedes: ") + precedes + QLatin1Char('\n'));
    if (!follows.isEmpty())
        modText.insert(lastHeaderLine, QLatin1String("Follows: ") + follows + QLatin1Char('\n'));

    // Empty line before headers and commit message
    const int emptyLine = modText.indexOf(QLatin1String("\n\n"));
    if (emptyLine != -1)
        modText.insert(emptyLine, QLatin1Char('\n') + QLatin1String(DiffEditor::Constants::EXPAND_BRANCHES));

    return modText;
}

// Quietly retrieve branch list of remote repository URL
//
// The branch HEAD is pointing to is always returned first.
QStringList GitClient::synchronousRepositoryBranches(const QString &repositoryURL,
                                                     const QString &workingDirectory) const
{
    QStringList arguments(QLatin1String("ls-remote"));
    arguments << repositoryURL << QLatin1String(HEAD) << QLatin1String("refs/heads/*");
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::SuppressStdErr
            | VcsCommand::SuppressFailMessage;
    const SynchronousProcessResponse resp = vcsSynchronousExec(workingDirectory, arguments, flags);
    QStringList branches;
    branches << tr("<Detached HEAD>");
    QString headSha;
    // split "82bfad2f51d34e98b18982211c82220b8db049b<tab>refs/heads/master"
    bool headFound = false;
    foreach (const QString &line, resp.stdOut.split(QLatin1Char('\n'))) {
        if (line.endsWith(QLatin1String("\tHEAD"))) {
            QTC_CHECK(headSha.isNull());
            headSha = line.left(line.indexOf(QLatin1Char('\t')));
            continue;
        }

        const QString pattern = QLatin1String("\trefs/heads/");
        const int pos = line.lastIndexOf(pattern);
        if (pos != -1) {
            const QString branchName = line.mid(pos + pattern.count());
            if (!headFound && line.startsWith(headSha)) {
                branches[0] = branchName;
                headFound = true;
            } else {
                branches.push_back(branchName);
            }
        }
    }
    return branches;
}

void GitClient::launchGitK(const QString &workingDirectory, const QString &fileName)
{
    const QFileInfo binaryInfo = vcsBinary().toFileInfo();
    QDir foundBinDir(binaryInfo.dir());
    const bool foundBinDirIsCmdDir = foundBinDir.dirName() == QLatin1String("cmd");
    QProcessEnvironment env = processEnvironment();
    if (tryLauchingGitK(env, workingDirectory, fileName, foundBinDir.path()))
        return;

    QString gitkPath = foundBinDir.path() + QLatin1String("/gitk");
    VcsOutputWindow::appendSilently(msgCannotLaunch(gitkPath));

    if (foundBinDirIsCmdDir) {
        foundBinDir.cdUp();
        if (tryLauchingGitK(env, workingDirectory, fileName,
                            foundBinDir.path() + QLatin1String("/bin"))) {
            return;
        }
        gitkPath = foundBinDir.path() + QLatin1String("/gitk");
        VcsOutputWindow::appendSilently(msgCannotLaunch(gitkPath));
    }

    Environment sysEnv = Environment::systemEnvironment();
    const FileName exec = sysEnv.searchInPath(QLatin1String("gitk"));

    if (!exec.isEmpty() && tryLauchingGitK(env, workingDirectory, fileName,
                                           exec.parentDir().toString())) {
        return;
    }

    VcsOutputWindow::appendError(msgCannotLaunch(QLatin1String("gitk")));
}

void GitClient::launchRepositoryBrowser(const QString &workingDirectory)
{
    const QString repBrowserBinary = settings().stringValue(GitSettings::repositoryBrowserCmd);
    if (!repBrowserBinary.isEmpty())
        QProcess::startDetached(repBrowserBinary, QStringList(workingDirectory), workingDirectory);
}

bool GitClient::tryLauchingGitK(const QProcessEnvironment &env,
                                const QString &workingDirectory,
                                const QString &fileName,
                                const QString &gitBinDirectory)
{
    QString binary = gitBinDirectory + QLatin1String("/gitk");
    QStringList arguments;
    if (HostOsInfo::isWindowsHost()) {
        // If git/bin is in path, use 'wish' shell to run. Otherwise (git/cmd), directly run gitk
        QString wish = gitBinDirectory + QLatin1String("/wish");
        if (QFileInfo(wish + QLatin1String(".exe")).exists()) {
            arguments << binary;
            binary = wish;
        }
    }
    const QString gitkOpts = settings().stringValue(GitSettings::gitkOptionsKey);
    if (!gitkOpts.isEmpty())
        arguments.append(QtcProcess::splitArgs(gitkOpts, HostOsInfo::hostOs()));
    if (!fileName.isEmpty())
        arguments << QLatin1String("--") << fileName;
    VcsOutputWindow::appendCommand(workingDirectory, FileName::fromString(binary), arguments);
    // This should always use QProcess::startDetached (as not to kill
    // the child), but that does not have an environment parameter.
    bool success = false;
    if (!settings().stringValue(GitSettings::pathKey).isEmpty()) {
        auto process = new QProcess(this);
        process->setWorkingDirectory(workingDirectory);
        process->setProcessEnvironment(env);
        process->start(binary, arguments);
        success = process->waitForStarted();
        if (success)
            connect(process, static_cast<void (QProcess::*)(int)>(&QProcess::finished),
                    process, &QProcess::deleteLater);
        else
            delete process;
    } else {
        success = QProcess::startDetached(binary, arguments, workingDirectory);
    }

    return success;
}

bool GitClient::launchGitGui(const QString &workingDirectory) {
    bool success = true;
    FileName gitBinary = vcsBinary();
    if (gitBinary.isEmpty()) {
        success = false;
    } else {
        success = QProcess::startDetached(gitBinary.toString(), QStringList(QLatin1String("gui")),
                                          workingDirectory);
    }

    if (!success)
        VcsOutputWindow::appendError(msgCannotLaunch(QLatin1String("git gui")));

    return success;
}

FileName GitClient::gitBinDirectory()
{
    const QString git = vcsBinary().toString();
    if (git.isEmpty())
        return FileName();

    // Is 'git\cmd' in the path (folder containing .bats)?
    QString path = QFileInfo(git).absolutePath();
    // Git for Windows (msysGit) has git and gitk redirect executables in {setup dir}/cmd
    // and the real binaries are in {setup dir}/bin. If cmd is configured in PATH
    // or in Git settings, return bin instead.
    if (HostOsInfo::isWindowsHost()
            && path.endsWith(QLatin1String("/cmd"), HostOsInfo::fileNameCaseSensitivity())) {
        path.replace(path.size() - 3, 3, QLatin1String("bin"));
    }
    return FileName::fromString(path);
}

FileName GitClient::vcsBinary() const
{
    bool ok;
    Utils::FileName binary = static_cast<GitSettings &>(settings()).gitExecutable(&ok);
    if (!ok)
        return Utils::FileName();
    return binary;
}

QTextCodec *GitClient::encoding(const QString &workingDirectory, const QByteArray &configVar) const
{
    QString codecName = readConfigValue(workingDirectory, QLatin1String(configVar)).trimmed();
    // Set default commit encoding to 'UTF-8', when it's not set,
    // to solve displaying error of commit log with non-latin characters.
    if (codecName.isEmpty())
        return QTextCodec::codecForName("UTF-8");
    return QTextCodec::codecForName(codecName.toUtf8());
}

// returns first line from log and removes it
static QByteArray shiftLogLine(QByteArray &logText)
{
    const int index = logText.indexOf('\n');
    const QByteArray res = logText.left(index);
    logText.remove(0, index + 1);
    return res;
}

bool GitClient::readDataFromCommit(const QString &repoDirectory, const QString &commit,
                                   CommitData &commitData, QString *errorMessage,
                                   QString *commitTemplate)
{
    // Get commit data as "SHA1<lf>author<lf>email<lf>message".
    QStringList args(QLatin1String("log"));
    args << QLatin1String("--max-count=1") << QLatin1String("--pretty=format:%h\n%an\n%ae\n%B");
    args << commit;
    QByteArray outputText;
    if (!vcsFullySynchronousExec(repoDirectory, args, &outputText, 0, silentFlags)) {
        if (errorMessage)
            *errorMessage = tr("Cannot retrieve last commit data of repository \"%1\".").arg(repoDirectory);
        return false;
    }
    QTextCodec *authorCodec = HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : commitData.commitEncoding;
    commitData.amendSHA1 = QString::fromLatin1(shiftLogLine(outputText));
    commitData.panelData.author = authorCodec->toUnicode(shiftLogLine(outputText));
    commitData.panelData.email = authorCodec->toUnicode(shiftLogLine(outputText));
    if (commitTemplate)
        *commitTemplate = commitData.commitEncoding->toUnicode(outputText);
    return true;
}

bool GitClient::getCommitData(const QString &workingDirectory,
                              QString *commitTemplate,
                              CommitData &commitData,
                              QString *errorMessage)
{
    commitData.clear();

    // Find repo
    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return false;
    }

    commitData.panelInfo.repository = repoDirectory;

    QString gitDir = findGitDirForRepository(repoDirectory);
    if (gitDir.isEmpty()) {
        *errorMessage = tr("The repository \"%1\" is not initialized.").arg(repoDirectory);
        return false;
    }

    // Run status. Note that it has exitcode 1 if there are no added files.
    QString output;
    if (commitData.commitType == FixupCommit) {
        QStringList arguments;
        arguments << QLatin1String(HEAD) << QLatin1String("--not")
                  << QLatin1String("--remotes") << QLatin1String("-n1");
        synchronousLog(repoDirectory, arguments, &output, errorMessage);
        if (output.isEmpty()) {
            *errorMessage = msgNoCommits(false);
            return false;
        }
    }
    const StatusResult status = gitStatus(repoDirectory, ShowAll, &output, errorMessage);
    switch (status) {
    case  StatusChanged:
        break;
    case StatusUnchanged:
        if (commitData.commitType == AmendCommit) // amend might be run just for the commit message
            break;
        *errorMessage = msgNoChangedFiles();
        return false;
    case StatusFailed:
        return false;
    }

    //    Output looks like:
    //    ## branch_name
    //    MM filename
    //     A new_unstaged_file
    //    R  old -> new
    //     D deleted_file
    //    ?? untracked_file
    if (status != StatusUnchanged) {
        if (!commitData.parseFilesFromStatus(output)) {
            *errorMessage = msgParseFilesFailed();
            return false;
        }

        // Filter out untracked files that are not part of the project
        QStringList untrackedFiles = commitData.filterFiles(UntrackedFile);

        VcsBaseSubmitEditor::filterUntrackedFilesOfProject(repoDirectory, &untrackedFiles);
        QList<CommitData::StateFilePair> filteredFiles;
        QList<CommitData::StateFilePair>::const_iterator it = commitData.files.constBegin();
        for ( ; it != commitData.files.constEnd(); ++it) {
            if (it->first == UntrackedFile && !untrackedFiles.contains(it->second))
                continue;
            filteredFiles.append(*it);
        }
        commitData.files = filteredFiles;

        if (commitData.files.isEmpty() && commitData.commitType != AmendCommit) {
            *errorMessage = msgNoChangedFiles();
            return false;
        }
    }

    commitData.commitEncoding = encoding(workingDirectory, "i18n.commitEncoding");

    // Get the commit template or the last commit message
    switch (commitData.commitType) {
    case AmendCommit: {
        if (!readDataFromCommit(repoDirectory, QLatin1String(HEAD), commitData,
                                errorMessage, commitTemplate)) {
            return false;
        }
        break;
    }
    case SimpleCommit: {
        bool authorFromCherryPick = false;
        QDir gitDirectory(gitDir);
        // For cherry-picked commit, read author data from the commit (but template from MERGE_MSG)
        if (gitDirectory.exists(QLatin1String(CHERRY_PICK_HEAD))) {
            authorFromCherryPick = readDataFromCommit(repoDirectory,
                                                      QLatin1String(CHERRY_PICK_HEAD),
                                                      commitData);
            commitData.amendSHA1.clear();
        }
        if (!authorFromCherryPick) {
            // the format is:
            // Joe Developer <joedev@example.com> unixtimestamp +HHMM
            QString author_info = readGitVar(workingDirectory, QLatin1String("GIT_AUTHOR_IDENT"));
            int lt = author_info.lastIndexOf(QLatin1Char('<'));
            int gt = author_info.lastIndexOf(QLatin1Char('>'));
            if (gt == -1 || uint(lt) > uint(gt)) {
                // shouldn't happen!
                commitData.panelData.author.clear();
                commitData.panelData.email.clear();
            } else {
                commitData.panelData.author = author_info.left(lt - 1);
                commitData.panelData.email = author_info.mid(lt + 1, gt - lt - 1);
            }
        }
        // Commit: Get the commit template
        QString templateFilename = gitDirectory.absoluteFilePath(QLatin1String("MERGE_MSG"));
        if (!QFile::exists(templateFilename))
            templateFilename = gitDirectory.absoluteFilePath(QLatin1String("SQUASH_MSG"));
        if (!QFile::exists(templateFilename)) {
            FileName templateName = FileName::fromUserInput(
                        readConfigValue(workingDirectory, QLatin1String("commit.template")));
            templateFilename = templateName.toString();
        }
        if (!templateFilename.isEmpty()) {
            // Make relative to repository
            const QFileInfo templateFileInfo(templateFilename);
            if (templateFileInfo.isRelative())
                templateFilename = repoDirectory + QLatin1Char('/') + templateFilename;
            FileReader reader;
            if (!reader.fetch(templateFilename, QIODevice::Text, errorMessage))
                return false;
            *commitTemplate = QString::fromLocal8Bit(reader.data());
        }
        break;
    }
    case FixupCommit:
        break;
    }

    commitData.enablePush = !synchronousRemotesList(repoDirectory).isEmpty();
    if (commitData.enablePush) {
        CommandInProgress commandInProgress = checkCommandInProgress(repoDirectory);
        if (commandInProgress == Rebase || commandInProgress == RebaseMerge)
            commitData.enablePush = false;
    }

    return true;
}

// Log message for commits/amended commits to go to output window
static inline QString msgCommitted(const QString &amendSHA1, int fileCount)
{
    if (amendSHA1.isEmpty())
        return GitClient::tr("Committed %n file(s).", 0, fileCount) + QLatin1Char('\n');
    if (fileCount)
        return GitClient::tr("Amended \"%1\" (%n file(s)).", 0, fileCount).arg(amendSHA1) + QLatin1Char('\n');
    return GitClient::tr("Amended \"%1\".").arg(amendSHA1);
}

bool GitClient::addAndCommit(const QString &repositoryDirectory,
                             const GitSubmitEditorPanelData &data,
                             CommitType commitType,
                             const QString &amendSHA1,
                             const QString &messageFile,
                             SubmitFileModel *model)
{
    const QString renameSeparator = QLatin1String(" -> ");

    QStringList filesToAdd;
    QStringList filesToRemove;
    QStringList filesToReset;

    int commitCount = 0;

    for (int i = 0; i < model->rowCount(); ++i) {
        const FileStates state = static_cast<FileStates>(model->extraData(i).toInt());
        QString file = model->file(i);
        const bool checked = model->checked(i);

        if (checked)
            ++commitCount;

        if (state == UntrackedFile && checked)
            filesToAdd.append(file);

        if ((state & StagedFile) && !checked) {
            if (state & (ModifiedFile | AddedFile | DeletedFile)) {
                filesToReset.append(file);
            } else if (state & (RenamedFile | CopiedFile)) {
                const QString newFile = file.mid(file.indexOf(renameSeparator) + renameSeparator.count());
                filesToReset.append(newFile);
            }
        } else if (state & UnmergedFile && checked) {
            QTC_ASSERT(false, continue); // There should not be unmerged files when committing!
        }

        if (state == ModifiedFile && checked) {
            filesToReset.removeAll(file);
            filesToAdd.append(file);
        } else if (state == AddedFile && checked) {
            QTC_ASSERT(false, continue); // these should be untracked!
        } else if (state == DeletedFile && checked) {
            filesToReset.removeAll(file);
            filesToRemove.append(file);
        } else if (state == RenamedFile && checked) {
            QTC_ASSERT(false, continue); // git mv directly stages.
        } else if (state == CopiedFile && checked) {
            QTC_ASSERT(false, continue); // only is noticed after adding a new file to the index
        } else if (state == UnmergedFile && checked) {
            QTC_ASSERT(false, continue); // There should not be unmerged files when committing!
        }
    }

    if (!filesToReset.isEmpty() && !synchronousReset(repositoryDirectory, filesToReset))
        return false;

    if (!filesToRemove.isEmpty() && !synchronousDelete(repositoryDirectory, true, filesToRemove))
        return false;

    if (!filesToAdd.isEmpty() && !synchronousAdd(repositoryDirectory, filesToAdd))
        return false;

    // Do the final commit
    QStringList args;
    args << QLatin1String("commit");
    if (commitType == FixupCommit) {
        args << QLatin1String("--fixup") << amendSHA1;
    } else {
        args << QLatin1String("-F") << QDir::toNativeSeparators(messageFile);
        if (commitType == AmendCommit)
            args << QLatin1String("--amend");
        const QString &authorString =  data.authorString();
        if (!authorString.isEmpty())
             args << QLatin1String("--author") << authorString;
        if (data.bypassHooks)
            args << QLatin1String("--no-verify");
    }

    QByteArray outputText;
    QByteArray errorText;

    const bool rc = vcsFullySynchronousExec(repositoryDirectory, args, &outputText, &errorText);
    const QString stdErr = commandOutputFromLocal8Bit(errorText);
    if (rc) {
        VcsOutputWindow::appendMessage(msgCommitted(amendSHA1, commitCount));
        VcsOutputWindow::appendError(stdErr);
    } else {
        VcsOutputWindow::appendError(tr("Cannot commit %n file(s): %1\n", 0, commitCount).arg(stdErr));
    }

    return rc;
}

/* Revert: This function can be called with a file list (to revert single
 * files)  or a single directory (revert all). Qt Creator currently has only
 * 'revert single' in its VCS menus, but the code is prepared to deal with
 * reverting a directory pending a sophisticated selection dialog in the
 * VcsBase plugin. */
GitClient::RevertResult GitClient::revertI(QStringList files,
                                           bool *ptrToIsDirectory,
                                           QString *errorMessage,
                                           bool revertStaging)
{
    if (files.empty())
        return RevertCanceled;

    // Figure out the working directory
    const QFileInfo firstFile(files.front());
    const bool isDirectory = firstFile.isDir();
    if (ptrToIsDirectory)
        *ptrToIsDirectory = isDirectory;
    const QString workingDirectory = isDirectory ? firstFile.absoluteFilePath() : firstFile.absolutePath();

    const QString repoDirectory = VcsManager::findTopLevelForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return RevertFailed;
    }

    // Check for changes
    QString output;
    switch (gitStatus(repoDirectory, StatusMode(NoUntracked | NoSubmodules), &output, errorMessage)) {
    case StatusChanged:
        break;
    case StatusUnchanged:
        return RevertUnchanged;
    case StatusFailed:
        return RevertFailed;
    }
    CommitData data;
    if (!data.parseFilesFromStatus(output)) {
        *errorMessage = msgParseFilesFailed();
        return RevertFailed;
    }

    // If we are looking at files, make them relative to the repository
    // directory to match them in the status output list.
    if (!isDirectory) {
        const QDir repoDir(repoDirectory);
        const QStringList::iterator cend = files.end();
        for (QStringList::iterator it = files.begin(); it != cend; ++it)
            *it = repoDir.relativeFilePath(*it);
    }

    // From the status output, determine all modified [un]staged files.
    const QStringList allStagedFiles = data.filterFiles(StagedFile | ModifiedFile);
    const QStringList allUnstagedFiles = data.filterFiles(ModifiedFile);
    // Unless a directory was passed, filter all modified files for the
    // argument file list.
    QStringList stagedFiles = allStagedFiles;
    QStringList unstagedFiles = allUnstagedFiles;
    if (!isDirectory) {
        const QSet<QString> filesSet = files.toSet();
        stagedFiles = allStagedFiles.toSet().intersect(filesSet).toList();
        unstagedFiles = allUnstagedFiles.toSet().intersect(filesSet).toList();
    }
    if ((!revertStaging || stagedFiles.empty()) && unstagedFiles.empty())
        return RevertUnchanged;

    // Ask to revert (to do: Handle lists with a selection dialog)
    const QMessageBox::StandardButton answer
        = QMessageBox::question(ICore::mainWindow(),
                                tr("Revert"),
                                tr("The file has been changed. Do you want to revert it?"),
                                QMessageBox::Yes | QMessageBox::No,
                                QMessageBox::No);
    if (answer == QMessageBox::No)
        return RevertCanceled;

    // Unstage the staged files
    if (revertStaging && !stagedFiles.empty() && !synchronousReset(repoDirectory, stagedFiles, errorMessage))
        return RevertFailed;
    QStringList filesToRevert = unstagedFiles;
    if (revertStaging)
        filesToRevert += stagedFiles;
    // Finally revert!
    if (!synchronousCheckoutFiles(repoDirectory, filesToRevert, QString(), errorMessage, revertStaging))
        return RevertFailed;
    return RevertOk;
}

void GitClient::revert(const QStringList &files, bool revertStaging)
{
    bool isDirectory;
    QString errorMessage;
    switch (revertI(files, &isDirectory, &errorMessage, revertStaging)) {
    case RevertOk:
        GitPlugin::instance()->gitVersionControl()->emitFilesChanged(files);
        break;
    case RevertCanceled:
        break;
    case RevertUnchanged: {
        const QString msg = (isDirectory || files.size() > 1) ? msgNoChangedFiles() : tr("The file is not modified.");
        VcsOutputWindow::appendWarning(msg);
    }
        break;
    case RevertFailed:
        VcsOutputWindow::appendError(errorMessage);
        break;
    }
}

void GitClient::fetch(const QString &workingDirectory, const QString &remote)
{
    QStringList arguments(QLatin1String("fetch"));
    arguments << (remote.isEmpty() ? QLatin1String("--all") : remote);
    VcsCommand *command = vcsExec(workingDirectory, arguments, 0, true, 0, workingDirectory);
    connect(command, &VcsCommand::success, this, &GitClient::fetchFinished);
}

bool GitClient::executeAndHandleConflicts(const QString &workingDirectory,
                                          const QStringList &arguments,
                                          const QString &abortCommand) const
{
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::ShowStdOut
            | VcsCommand::ExpectRepoChanges
            | VcsCommand::ShowSuccessMessage;
    const SynchronousProcessResponse resp = vcsSynchronousExec(workingDirectory, arguments, flags);
    // Notify about changed files or abort the rebase.
    ConflictHandler::handleResponse(resp, workingDirectory, abortCommand);
    return resp.result == SynchronousProcessResponse::Finished;
}

bool GitClient::synchronousPull(const QString &workingDirectory, bool rebase)
{
    QString abortCommand;
    QStringList arguments(QLatin1String("pull"));
    if (rebase) {
        arguments << QLatin1String("--rebase");
        abortCommand = QLatin1String("rebase");
    } else {
        abortCommand = QLatin1String("merge");
    }

    bool ok = executeAndHandleConflicts(workingDirectory, arguments, abortCommand);

    if (ok)
        updateSubmodulesIfNeeded(workingDirectory, true);

    return ok;
}

void GitClient::synchronousAbortCommand(const QString &workingDir, const QString &abortCommand)
{
    // Abort to clean if something goes wrong
    if (abortCommand.isEmpty()) {
        // no abort command - checkout index to clean working copy.
        synchronousCheckoutFiles(VcsManager::findTopLevelForDirectory(workingDir),
                                 QStringList(), QString(), 0, false);
        return;
    }
    QStringList arguments;
    arguments << abortCommand << QLatin1String("--abort");
    QByteArray stdOut;
    vcsFullySynchronousExec(workingDir, arguments, &stdOut, 0, VcsCommand::ExpectRepoChanges);
    VcsOutputWindow::append(commandOutputFromLocal8Bit(stdOut));
}

QString GitClient::synchronousTrackingBranch(const QString &workingDirectory, const QString &branch)
{
    QString remote;
    QString localBranch = branch.isEmpty() ? synchronousCurrentLocalBranch(workingDirectory) : branch;
    if (localBranch.isEmpty())
        return QString();
    localBranch.prepend(QLatin1String("branch."));
    remote = readConfigValue(workingDirectory, localBranch + QLatin1String(".remote"));
    if (remote.isEmpty())
        return QString();
    const QString rBranch = readConfigValue(workingDirectory, localBranch + QLatin1String(".merge"))
            .replace(QLatin1String("refs/heads/"), QString());
    if (rBranch.isEmpty())
        return QString();
    return remote + QLatin1Char('/') + rBranch;
}

bool GitClient::synchronousSetTrackingBranch(const QString &workingDirectory,
                                             const QString &branch, const QString &tracking)
{
    QByteArray outputText;
    QStringList arguments;
    arguments << QLatin1String("branch");
    if (gitVersion() >= 0x010800)
        arguments << (QLatin1String("--set-upstream-to=") + tracking) << branch;
    else
        arguments << QLatin1String("--set-upstream") << branch << tracking;
    return vcsFullySynchronousExec(workingDirectory, arguments, &outputText);
}

void GitClient::handleMergeConflicts(const QString &workingDir, const QString &commit,
                                     const QStringList &files, const QString &abortCommand)
{
    QString message;
    if (!commit.isEmpty()) {
        message = tr("Conflicts detected with commit %1.").arg(commit);
    } else if (!files.isEmpty()) {
        QString fileList;
        QStringList partialFiles = files;
        while (partialFiles.count() > 20)
            partialFiles.removeLast();
        fileList = partialFiles.join(QLatin1Char('\n'));
        if (partialFiles.count() != files.count())
            fileList += QLatin1String("\n...");
        message = tr("Conflicts detected with files:\n%1").arg(fileList);
    } else {
        message = tr("Conflicts detected.");
    }
    QMessageBox mergeOrAbort(QMessageBox::Question, tr("Conflicts Detected"), message,
                             QMessageBox::NoButton, ICore::mainWindow());
    QPushButton *mergeToolButton = mergeOrAbort.addButton(tr("Run &Merge Tool"),
                                                          QMessageBox::AcceptRole);
    mergeOrAbort.addButton(QMessageBox::Ignore);
    if (abortCommand == QLatin1String("rebase"))
        mergeOrAbort.addButton(tr("&Skip"), QMessageBox::RejectRole);
    if (!abortCommand.isEmpty())
        mergeOrAbort.addButton(QMessageBox::Abort);
    switch (mergeOrAbort.exec()) {
    case QMessageBox::Abort:
        synchronousAbortCommand(workingDir, abortCommand);
        break;
    case QMessageBox::Ignore:
        break;
    default: // Merge or Skip
        if (mergeOrAbort.clickedButton() == mergeToolButton) {
            merge(workingDir);
        } else if (!abortCommand.isEmpty()) {
            QStringList arguments = QStringList() << abortCommand << QLatin1String("--skip");
            executeAndHandleConflicts(workingDir, arguments, abortCommand);
        }
    }
}

void GitClient::addFuture(const QFuture<void> &future)
{
    m_synchronizer.addFuture(future);
}

// Subversion: git svn
void GitClient::synchronousSubversionFetch(const QString &workingDirectory)
{
    QStringList args;
    args << QLatin1String("svn") << QLatin1String("fetch");
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::ShowStdOut
            | VcsCommand::ShowSuccessMessage;
    vcsSynchronousExec(workingDirectory, args, flags);
}

void GitClient::subversionLog(const QString &workingDirectory)
{
    QStringList arguments;
    arguments << QLatin1String("svn") << QLatin1String("log");
    int logCount = settings().intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << (QLatin1String("--limit=") + QString::number(logCount));

    // Create a command editor, no highlighting or interaction.
    const QString title = tr("Git SVN Log");
    const Id editorId = Git::Constants::GIT_COMMAND_LOG_EDITOR_ID;
    const QString sourceFile = VcsBaseEditor::getSource(workingDirectory, QStringList());
    VcsBaseEditorWidget *editor = createVcsEditor(editorId, title, sourceFile, codecFor(CodecNone),
                                                  "svnLog", sourceFile);
    editor->setWorkingDirectory(workingDirectory);
    vcsExec(workingDirectory, arguments, editor);
}

void GitClient::push(const QString &workingDirectory, const QStringList &pushArgs)
{
    QStringList arguments(QLatin1String("push"));
    if (!pushArgs.isEmpty())
        arguments += pushArgs;
    vcsExec(workingDirectory, arguments, 0, true);
}

bool GitClient::synchronousMerge(const QString &workingDirectory, const QString &branch,
                                 bool allowFastForward)
{
    QString command = QLatin1String("merge");
    QStringList arguments(command);

    if (!allowFastForward)
        arguments << QLatin1String("--no-ff");
    arguments << branch;
    return executeAndHandleConflicts(workingDirectory, arguments, command);
}

bool GitClient::canRebase(const QString &workingDirectory) const
{
    const QString gitDir = findGitDirForRepository(workingDirectory);
    if (QFileInfo::exists(gitDir + QLatin1String("/rebase-apply"))
            || QFileInfo::exists(gitDir + QLatin1String("/rebase-merge"))) {
        VcsOutputWindow::appendError(
                    tr("Rebase, merge or am is in progress. Finish "
                       "or abort it and then try again."));
        return false;
    }
    return true;
}

void GitClient::rebase(const QString &workingDirectory, const QString &argument)
{
    VcsCommand *command = vcsExecAbortable(workingDirectory,
                                           QStringList() << QLatin1String("rebase") << argument);
    GitProgressParser::attachToCommand(command);
}

void GitClient::cherryPick(const QString &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, QStringList() << QLatin1String("cherry-pick") << argument);
}

void GitClient::revert(const QString &workingDirectory, const QString &argument)
{
    vcsExecAbortable(workingDirectory, QStringList() << QLatin1String("revert") << argument);
}

// Executes a command asynchronously. Work tree is expected to be clean.
// Stashing is handled prior to this call.
VcsCommand *GitClient::vcsExecAbortable(const QString &workingDirectory,
                                        const QStringList &arguments)
{
    QTC_ASSERT(!arguments.isEmpty(), return 0);

    QString abortCommand = arguments.at(0);
    // Git might request an editor, so this must be done asynchronously and without timeout
    VcsCommand *command = createCommand(workingDirectory, 0, VcsWindowOutputBind);
    command->setCookie(workingDirectory);
    command->addFlags(VcsCommand::ShowSuccessMessage);
    command->addJob(vcsBinary(), arguments, 0);
    command->execute();
    ConflictHandler::attachToCommand(command, abortCommand);

    return command;
}

bool GitClient::synchronousRevert(const QString &workingDirectory, const QString &commit)
{
    QStringList arguments;
    const QString command = QLatin1String("revert");
    // Do not stash if --continue or --abort is given as the commit
    if (!commit.startsWith(QLatin1Char('-')) && !beginStashScope(workingDirectory, command))
        return false;

    arguments << command << QLatin1String("--no-edit") << commit;

    return executeAndHandleConflicts(workingDirectory, arguments, command);
}

bool GitClient::synchronousCherryPick(const QString &workingDirectory, const QString &commit)
{
    const QString command = QLatin1String("cherry-pick");
    // "commit" might be --continue or --abort
    const bool isRealCommit = !commit.startsWith(QLatin1Char('-'));
    if (isRealCommit && !beginStashScope(workingDirectory, command))
        return false;

    QStringList arguments(command);
    if (isRealCommit && isRemoteCommit(workingDirectory, commit))
        arguments << QLatin1String("-x");
    arguments << commit;

    return executeAndHandleConflicts(workingDirectory, arguments, command);
}

void GitClient::interactiveRebase(const QString &workingDirectory, const QString &commit, bool fixup)
{
    QStringList arguments;
    arguments << QLatin1String("rebase") << QLatin1String("-i");
    if (fixup)
        arguments << QLatin1String("--autosquash");
    arguments << commit + QLatin1Char('^');
    if (fixup)
        m_disableEditor = true;
    VcsCommand *command = vcsExecAbortable(workingDirectory, arguments);
    GitProgressParser::attachToCommand(command);
    if (fixup)
        m_disableEditor = false;
}

QString GitClient::msgNoChangedFiles()
{
    return tr("There are no modified files.");
}

QString GitClient::msgNoCommits(bool includeRemote)
{
    return includeRemote ? tr("No commits were found") : tr("No local commits were found");
}

void GitClient::stashPop(const QString &workingDirectory, const QString &stash)
{
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("pop");
    if (!stash.isEmpty())
        arguments << stash;
    VcsCommand *cmd = vcsExec(workingDirectory, arguments, 0, true, VcsCommand::ExpectRepoChanges);
    ConflictHandler::attachToCommand(cmd);
}

bool GitClient::synchronousStashRestore(const QString &workingDirectory,
                                        const QString &stash,
                                        bool pop,
                                        const QString &branch /* = QString()*/) const
{
    QStringList arguments(QLatin1String("stash"));
    if (branch.isEmpty())
        arguments << QLatin1String(pop ? "pop" : "apply") << stash;
    else
        arguments << QLatin1String("branch") << branch << stash;
    return executeAndHandleConflicts(workingDirectory, arguments);
}

bool GitClient::synchronousStashRemove(const QString &workingDirectory,
                            const QString &stash /* = QString() */,
                            QString *errorMessage /* = 0 */) const
{
    QStringList arguments(QLatin1String("stash"));
    if (stash.isEmpty())
        arguments << QLatin1String("clear");
    else
        arguments << QLatin1String("drop") << stash;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText, &errorText);
    if (rc) {
        const QString output = commandOutputFromLocal8Bit(outputText);
        if (!output.isEmpty())
            VcsOutputWindow::append(output);
    } else {
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);
    }
    return rc;
}

bool GitClient::synchronousStashList(const QString &workingDirectory,
                                     QList<Stash> *stashes,
                                     QString *errorMessage /* = 0 */) const
{
    stashes->clear();
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("list") << QLatin1String(noColorOption);
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);
        return false;
    }
    Stash stash;
    foreach (const QString &line, commandOutputLinesFromLocal8Bit(outputText))
        if (stash.parseStashLine(line))
            stashes->push_back(stash);
    return true;
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar) const
{
    QStringList arguments;
    arguments << QLatin1String("config") << configVar;
    return readOneLine(workingDirectory, arguments);
}

QString GitClient::readGitVar(const QString &workingDirectory, const QString &configVar) const
{
    QStringList arguments;
    arguments << QLatin1String("var") << configVar;
    return readOneLine(workingDirectory, arguments);
}

QString GitClient::readOneLine(const QString &workingDirectory, const QStringList &arguments) const
{
    // msysGit always uses UTF-8 for configuration:
    // https://github.com/msysgit/msysgit/wiki/Git-for-Windows-Unicode-Support#convert-config-files
    static QTextCodec *codec = HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : QTextCodec::codecForLocale();

    QByteArray outputText;
    if (!vcsFullySynchronousExec(workingDirectory, arguments, &outputText, 0, silentFlags))
        return QString();
    if (HostOsInfo::isWindowsHost())
        outputText.replace("\r\n", "\n");

    return SynchronousProcess::normalizeNewlines(codec->toUnicode(outputText.trimmed()));
}

bool GitClient::cloneRepository(const QString &directory,const QByteArray &url)
{
    QDir workingDirectory(directory);
    const unsigned flags = VcsCommand::SshPasswordPrompt
            | VcsCommand::ShowStdOut | VcsCommand::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        if (!synchronousInit(workingDirectory.path()))
            return false;

        QStringList arguments(QLatin1String("remote"));
        arguments << QLatin1String("add") << QLatin1String("origin") << QLatin1String(url);
        if (!vcsFullySynchronousExec(workingDirectory.path(), arguments, 0))
            return false;

        arguments.clear();
        arguments << QLatin1String("fetch");
        const SynchronousProcessResponse resp
                = vcsSynchronousExec(workingDirectory.path(), arguments, flags);
        if (resp.result != SynchronousProcessResponse::Finished)
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.remote")
                  <<  QLatin1String("origin");
        if (!vcsFullySynchronousExec(workingDirectory.path(), arguments, 0))
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.merge")
                  << QLatin1String("refs/heads/master");
        if (!vcsFullySynchronousExec(workingDirectory.path(), arguments, 0))
            return false;

        return true;
    } else {
        QStringList arguments(QLatin1String("clone"));
        arguments << QLatin1String(url) << workingDirectory.dirName();
        workingDirectory.cdUp();
        const SynchronousProcessResponse resp
                = vcsSynchronousExec(workingDirectory.path(), arguments, flags);
        resetCachedVcsInfo(workingDirectory.absolutePath());
        return (resp.result == SynchronousProcessResponse::Finished);
    }
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::gitVersion(QString *errorMessage) const
{
    const FileName newGitBinary = vcsBinary();
    if (m_gitVersionForBinary != newGitBinary && !newGitBinary.isEmpty()) {
        // Do not execute repeatedly if that fails (due to git
        // not being installed) until settings are changed.
        m_cachedGitVersion = synchronousGitVersion(errorMessage);
        m_gitVersionForBinary = newGitBinary;
    }
    return m_cachedGitVersion;
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::synchronousGitVersion(QString *errorMessage) const
{
    if (vcsBinary().isEmpty())
        return 0;

    // run git --version
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = vcsFullySynchronousExec(QString(), QStringList(QLatin1String("--version")),
                                            &outputText, &errorText, silentFlags);
    if (!rc) {
        msgCannotRun(tr("Cannot determine Git version: %1")
                     .arg(commandOutputFromLocal8Bit(errorText)),
                     errorMessage);
        return 0;
    }
    // cut 'git version 1.6.5.1.sha'
    // another form: 'git version 1.9.rc1'
    const QString output = commandOutputFromLocal8Bit(outputText);
    QRegExp versionPattern(QLatin1String("^[^\\d]+(\\d+)\\.(\\d+)\\.(\\d+|rc\\d).*$"));
    QTC_ASSERT(versionPattern.isValid(), return 0);
    QTC_ASSERT(versionPattern.exactMatch(output), return 0);
    const unsigned majorV = versionPattern.cap(1).toUInt(0, 16);
    const unsigned minorV = versionPattern.cap(2).toUInt(0, 16);
    const unsigned patchV = versionPattern.cap(3).toUInt(0, 16);
    return version(majorV, minorV, patchV);
}

GitClient::StashInfo::StashInfo() :
    m_client(GitPlugin::instance()->client()),
    m_pushAction(NoPush)
{
}

bool GitClient::StashInfo::init(const QString &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    m_workingDir = workingDirectory;
    m_flags = flag;
    m_pushAction = pushAction;
    QString errorMessage;
    QString statusOutput;
    switch (m_client->gitStatus(m_workingDir, StatusMode(NoUntracked | NoSubmodules),
                              &statusOutput, &errorMessage)) {
    case GitClient::StatusChanged:
        if (m_flags & NoPrompt)
            executeStash(command, &errorMessage);
        else
            stashPrompt(command, statusOutput, &errorMessage);
        break;
    case GitClient::StatusUnchanged:
        m_stashResult = StashUnchanged;
        break;
    case GitClient::StatusFailed:
        m_stashResult = StashFailed;
        break;
    }

    if (m_stashResult == StashFailed)
        VcsOutputWindow::appendError(errorMessage);
    return !stashingFailed();
}

void GitClient::StashInfo::stashPrompt(const QString &command, const QString &statusOutput,
                                       QString *errorMessage)
{
    QMessageBox msgBox(QMessageBox::Question, tr("Uncommitted Changes Found"),
                       tr("What would you like to do with local changes in:")
                       + QLatin1String("\n\n\"")
                       + QDir::toNativeSeparators(m_workingDir) + QLatin1Char('\"'),
                       QMessageBox::NoButton, ICore::mainWindow());

    msgBox.setDetailedText(statusOutput);

    QPushButton *stashAndPopButton = msgBox.addButton(tr("Stash && Pop"), QMessageBox::AcceptRole);
    stashAndPopButton->setToolTip(tr("Stash local changes and pop when %1 finishes.").arg(command));

    QPushButton *stashButton = msgBox.addButton(tr("Stash"), QMessageBox::AcceptRole);
    stashButton->setToolTip(tr("Stash local changes and execute %1.").arg(command));

    QPushButton *discardButton = msgBox.addButton(tr("Discard"), QMessageBox::AcceptRole);
    discardButton->setToolTip(tr("Discard (reset) local changes and execute %1.").arg(command));

    QPushButton *ignoreButton = 0;
    if (m_flags & AllowUnstashed) {
        ignoreButton = msgBox.addButton(QMessageBox::Ignore);
        ignoreButton->setToolTip(tr("Execute %1 with local changes in working directory.")
                                 .arg(command));
    }

    QPushButton *cancelButton = msgBox.addButton(QMessageBox::Cancel);
    cancelButton->setToolTip(tr("Cancel %1.").arg(command));

    msgBox.exec();

    if (msgBox.clickedButton() == discardButton) {
        m_stashResult = m_client->synchronousReset(m_workingDir, QStringList(), errorMessage) ?
                        StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == ignoreButton) { // At your own risk, so.
        m_stashResult = NotStashed;
    } else if (msgBox.clickedButton() == cancelButton) {
        m_stashResult = StashCanceled;
    } else if (msgBox.clickedButton() == stashButton) {
        const bool result = m_client->executeSynchronousStash(
                    m_workingDir, creatorStashMessage(command), false, errorMessage);
        m_stashResult = result ? StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == stashAndPopButton) {
        executeStash(command, errorMessage);
    }
}

void GitClient::StashInfo::executeStash(const QString &command, QString *errorMessage)
{
    m_message = creatorStashMessage(command);
    if (!m_client->executeSynchronousStash(m_workingDir, m_message, false, errorMessage))
        m_stashResult = StashFailed;
    else
        m_stashResult = Stashed;
 }

bool GitClient::StashInfo::stashingFailed() const
{
    switch (m_stashResult) {
    case StashCanceled:
    case StashFailed:
        return true;
    case NotStashed:
        return !(m_flags & AllowUnstashed);
    default:
        return false;
    }
}

void GitClient::StashInfo::end()
{
    if (m_stashResult == Stashed) {
        QString stashName;
        if (m_client->stashNameFromMessage(m_workingDir, m_message, &stashName))
            m_client->stashPop(m_workingDir, stashName);
    }

    if (m_pushAction == NormalPush)
        m_client->push(m_workingDir);
    else if (m_pushAction == PushToGerrit)
        GitPlugin::instance()->gerritPlugin()->push(m_workingDir);

    m_pushAction = NoPush;
    m_stashResult = NotStashed;
}

} // namespace Internal
} // namespace Git

#include "gitclient.moc"
