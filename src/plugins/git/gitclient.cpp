/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "gitclient.h"
#include "gitutils.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"
#include "mergetool.h"
#include "branchadddialog.h"
#include "gerrit/gerritplugin.h"

#include <vcsbase/submitfilemodel.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/vcsmanager.h>
#include <coreplugin/id.h>
#include <coreplugin/iversioncontrol.h>
#include <coreplugin/coreconstants.h>

#include <utils/hostosinfo.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/fileutils.h>
#include <vcsbase/command.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <diffeditor/diffeditorconstants.h>
#include <diffeditor/diffeditorcontroller.h>
#include <diffeditor/diffeditordocument.h>
#include <diffeditor/diffeditormanager.h>
#include <diffeditor/diffeditorreloader.h>
#include <diffeditor/diffutils.h>

#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QRegExp>
#include <QSignalMapper>
#include <QTemporaryFile>

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

namespace Git {
namespace Internal {

// Suppress git diff warnings about "LF will be replaced by CRLF..." on Windows.
static inline unsigned diffExecutionFlags()
{
    return Utils::HostOsInfo::isWindowsHost() ? unsigned(VcsBase::VcsBasePlugin::SuppressStdErrInLogWindow) : 0u;
}

using VcsBase::VcsBasePlugin;

class GitDiffHandler : public QObject
{
    Q_OBJECT

public:
    GitDiffHandler(DiffEditor::DiffEditorController *editorController,
                   const QString &workingDirectory);

    // index -> working tree
    void diffFile(const QString &fileName);
    // stagedFileNames:   HEAD -> index
    // unstagedFileNames: index -> working tree
    void diffFiles(const QStringList &stagedFileNames,
                   const QStringList &unstagedFileNames);
    // index -> working tree
    void diffProjects(const QStringList &projectPaths);
    // index -> working tree
    void diffRepository();
    // branch HEAD -> working tree
    void diffBranch(const QString &branchName);
    // id^ -> id
    void show(const QString &id);

private slots:
    void slotShowDescriptionReceived(const QString &data);
    void slotDiffOutputReceived(const QString &contents);

private:
    void postCollectShowDescription(const QString &id);
    void postCollectDiffOutput(const QStringList &arguments);
    void postCollectDiffOutput(const QList<QStringList> &argumentsList);
    void addJob(VcsBase::Command *command, const QStringList &arguments);
    int timeout() const;
    QProcessEnvironment processEnvironment() const;
    QString gitPath() const;

    QPointer<DiffEditor::DiffEditorController> m_editorController;
    const QString m_workingDirectory;
    GitClient *m_gitClient;
    const QString m_waitMessage;

    QString m_id;
};

GitDiffHandler::GitDiffHandler(DiffEditor::DiffEditorController *editorController,
               const QString &workingDirectory)
    : m_editorController(editorController),
      m_workingDirectory(workingDirectory),
      m_gitClient(GitPlugin::instance()->gitClient()),
      m_waitMessage(tr("Waiting for data..."))
{
}

void GitDiffHandler::diffFile(const QString &fileName)
{
    postCollectDiffOutput(QStringList() << QLatin1String("--") << fileName);
}

void GitDiffHandler::diffFiles(const QStringList &stagedFileNames,
                               const QStringList &unstagedFileNames)
{
    QList<QStringList> arguments;

    QStringList stagedArguments;
    stagedArguments << QLatin1String("--cached");
    stagedArguments << QLatin1String("--");
    stagedArguments << stagedFileNames;
    arguments << stagedArguments;

    if (!unstagedFileNames.isEmpty()) {
        QStringList unstagedArguments;
        unstagedArguments << QLatin1String("--");
        unstagedArguments << unstagedFileNames;
        arguments << unstagedArguments;
    }

    postCollectDiffOutput(arguments);
}

void GitDiffHandler::diffProjects(const QStringList &projectPaths)
{
    postCollectDiffOutput(QStringList() << QLatin1String("--") << projectPaths);
}

void GitDiffHandler::diffRepository()
{
    postCollectDiffOutput(QStringList());
}

void GitDiffHandler::diffBranch(const QString &branchName)
{
    postCollectDiffOutput(QStringList() << branchName);
}

void GitDiffHandler::show(const QString &id)
{
    m_id = id;
    postCollectShowDescription(id);
}

void GitDiffHandler::postCollectShowDescription(const QString &id)
{
    if (m_editorController.isNull()) {
        deleteLater();
        return;
    }

    m_editorController->clear(m_waitMessage);
    VcsBase::Command *command = new VcsBase::Command(gitPath(),
                                                     m_workingDirectory,
                                                     processEnvironment());
    command->setCodec(m_gitClient->encoding(m_workingDirectory,
                                            "i18n.commitEncoding"));
    connect(command, SIGNAL(output(QString)),
            this, SLOT(slotShowDescriptionReceived(QString)));
    QStringList arguments;
    arguments << QLatin1String("show")
              << QLatin1String("-s")
              << QLatin1String(noColorOption)
              << QLatin1String(decorateOption)
              << id;
    command->addJob(arguments, timeout());
    command->execute();
}

void GitDiffHandler::slotShowDescriptionReceived(const QString &description)
{
    if (m_editorController.isNull()) {
        deleteLater();
        return;
    }

    postCollectDiffOutput(QStringList() << m_id + QLatin1Char('^') << m_id);

    // need to be called after postCollectDiffOutput(), since it clears the description
    m_editorController->setDescription(
                m_gitClient->extendedShowDescription(m_workingDirectory,
                                                     description));
}

void GitDiffHandler::addJob(VcsBase::Command *command, const QStringList &arguments)
{
    QStringList args;
    args << QLatin1String("diff");
    if (m_editorController->isIgnoreWhitespace())
        args << QLatin1String("--ignore-space-change");
    args << QLatin1String("--unified=") + QString::number(
                m_editorController->contextLinesNumber());
    args << arguments;
    command->addJob(args, timeout());
}

void GitDiffHandler::postCollectDiffOutput(const QStringList &arguments)
{
    postCollectDiffOutput(QList<QStringList>() << arguments);
}

void GitDiffHandler::postCollectDiffOutput(const QList<QStringList> &argumentsList)
{
    if (m_editorController.isNull()) {
        deleteLater();
        return;
    }

    m_editorController->clear(m_waitMessage);
    VcsBase::Command *command = new VcsBase::Command(gitPath(),
                                                     m_workingDirectory,
                                                     processEnvironment());
    command->setCodec(Core::EditorManager::defaultTextCodec());
    connect(command, SIGNAL(output(QString)),
            this, SLOT(slotDiffOutputReceived(QString)));
    command->addFlags(diffExecutionFlags());

    for (int i = 0; i < argumentsList.count(); i++)
        addJob(command, argumentsList.at(i));

    command->execute();
}

void GitDiffHandler::slotDiffOutputReceived(const QString &contents)
{
    if (m_editorController.isNull()) {
        deleteLater();
        return;
    }

    bool ok;
    QList<DiffEditor::FileData> fileDataList
            = DiffEditor::DiffUtils::readPatch(
                contents, m_editorController->isIgnoreWhitespace(), &ok);
    m_editorController->setDiffFiles(fileDataList, m_workingDirectory);
    deleteLater();
}

int GitDiffHandler::timeout() const
{
    return m_gitClient->settings()->intValue(GitSettings::timeoutKey);
}

QProcessEnvironment GitDiffHandler::processEnvironment() const
{
    return m_gitClient->processEnvironment();
}

QString GitDiffHandler::gitPath() const
{
    return m_gitClient->gitBinaryPath();
}

/////////////////////////////////////

class GitDiffEditorReloader : public DiffEditor::DiffEditorReloader
{
    Q_OBJECT
public:
    enum DiffType {
        DiffRepository,
        DiffFile,
        DiffFileList,
        DiffProjectList,
        DiffBranch,
        DiffShow
    };

    GitDiffEditorReloader(QObject *parent);
    void setWorkingDirectory(const QString &workingDir) {
        m_workingDirectory = workingDir;
    }
    void setDiffType(DiffType type) { m_diffType = type; }
    void setFileName(const QString &fileName) { m_fileName = fileName; }
    void setFileList(const QStringList &stagedFiles,
                     const QStringList &unstagedFiles) {
        m_stagedFiles = stagedFiles;
        m_unstagedFiles = unstagedFiles;
    }
    void setProjectList(const QStringList &projectFiles) {
        m_projectFiles = projectFiles;
    }
    void setBranchName(const QString &branchName) {
        m_branchName = branchName;
    }
    void setId(const QString &id) { m_id = id; }
    void setDisplayName(const QString &displayName) {
        m_displayName = displayName;
    }


protected:
    void reload();

private:
    GitClient *m_gitClient;

    QString m_workingDirectory;
    DiffType m_diffType;
    QString m_fileName;
    QStringList m_stagedFiles;
    QStringList m_unstagedFiles;
    QStringList m_projectFiles;
    QString m_branchName;
    QString m_id;
    QString m_displayName;
};

GitDiffEditorReloader::GitDiffEditorReloader(QObject *parent)
    : DiffEditorReloader(parent),
      m_gitClient(GitPlugin::instance()->gitClient())
{
}

void GitDiffEditorReloader::reload()
{
    const QString workingDirectory = m_diffType == DiffShow
            ? m_gitClient->findRepositoryForDirectory(m_workingDirectory)
            : m_workingDirectory;
    GitDiffHandler *handler = new GitDiffHandler(diffEditorController(),
                                                 workingDirectory);
    connect(handler, SIGNAL(destroyed()), this, SLOT(reloadFinished()));

    switch (m_diffType) {
    case DiffRepository:
        handler->diffRepository();
        break;
    case DiffFile:
        handler->diffFile(m_fileName);
        break;
    case DiffFileList:
        handler->diffFiles(m_stagedFiles, m_unstagedFiles);
        break;
    case DiffProjectList:
        handler->diffProjects(m_projectFiles);
        break;
    case DiffBranch:
        handler->diffBranch(m_branchName);
        break;
    case DiffShow:
        handler->show(m_id);
        break;
    default:
        break;
    }
}

///////////////////////////////

class BaseGitDiffArgumentsWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    BaseGitDiffArgumentsWidget(GitClient *client, const QString &directory,
                               const QStringList &args) :
        m_workingDirectory(directory),
        m_client(client)
    {
        QTC_ASSERT(!directory.isEmpty(), return);
        QTC_ASSERT(m_client, return);

        m_patienceButton = addToggleButton(
                    QLatin1String("--patience"),
                    tr("Patience"),
                    tr("Use the patience algorithm for calculating the differences."));
        mapSetting(m_patienceButton, client->settings()->boolPointer(
                       GitSettings::diffPatienceKey));
        m_ignoreWSButton = addToggleButton(
                    QLatin1String("--ignore-space-change"), tr("Ignore Whitespace"),
                    tr("Ignore whitespace only changes."));
        mapSetting(m_ignoreWSButton,
                   m_client->settings()->boolPointer(GitSettings::ignoreSpaceChangesInDiffKey));

        setBaseArguments(args);
    }

protected:
    QString m_workingDirectory;
    GitClient *m_client;
    QToolButton *m_patienceButton;
    QToolButton *m_ignoreWSButton;
};

class GitBlameArgumentsWidget : public VcsBase::VcsBaseEditorParameterWidget
{
    Q_OBJECT

public:
    GitBlameArgumentsWidget(Git::Internal::GitClient *client,
                            const QString &directory,
                            const QStringList &args,
                            const QString &revision, const QString &fileName) :
        m_editor(0),
        m_client(client),
        m_workingDirectory(directory),
        m_revision(revision),
        m_fileName(fileName)
    {
        mapSetting(addToggleButton(QString(), tr("Omit Date"),
                                   tr("Hide the date of a change from the output.")),
                   m_client->settings()->boolPointer(GitSettings::omitAnnotationDateKey));
        mapSetting(addToggleButton(QLatin1String("-w"), tr("Ignore Whitespace"),
                                   tr("Ignore whitespace only changes.")),
                   m_client->settings()->boolPointer(GitSettings::ignoreSpaceChangesInBlameKey));

        setBaseArguments(args);
    }

    void setEditor(VcsBase::VcsBaseEditorWidget *editor)
    {
        QTC_ASSERT(editor, return);
        m_editor = editor;
    }

    void executeCommand()
    {
        int line = -1;
        if (m_editor)
            line = m_editor->lineNumberOfCurrentEditor();
        m_client->blame(m_workingDirectory, baseArguments(), m_fileName, m_revision, line);
    }

private:
    VcsBase::VcsBaseEditorWidget *m_editor;
    GitClient *m_client;
    QString m_workingDirectory;
    QString m_revision;
    QString m_fileName;
};

class GitLogArgumentsWidget : public BaseGitDiffArgumentsWidget
{
    Q_OBJECT

public:
    GitLogArgumentsWidget(Git::Internal::GitClient *client,
                          const QString &directory,
                          bool enableAnnotationContextMenu,
                          const QStringList &args,
                          const QString &fileName) :
        BaseGitDiffArgumentsWidget(client, directory, args),
        m_client(client),
        m_workingDirectory(directory),
        m_enableAnnotationContextMenu(enableAnnotationContextMenu)
    {
        QTC_ASSERT(!directory.isEmpty(), return);
        QToolButton *diffButton = addToggleButton(QLatin1String("--patch"), tr("Show Diff"),
                                              tr("Show difference."));
        mapSetting(diffButton, m_client->settings()->boolPointer(GitSettings::logDiffKey));
        connect(diffButton, SIGNAL(toggled(bool)), m_patienceButton, SLOT(setVisible(bool)));
        connect(diffButton, SIGNAL(toggled(bool)), m_ignoreWSButton, SLOT(setVisible(bool)));
        m_patienceButton->setVisible(diffButton->isChecked());
        m_ignoreWSButton->setVisible(diffButton->isChecked());
        QStringList graphArguments(QLatin1String("--graph"));
        graphArguments << QLatin1String("--oneline") << QLatin1String("--topo-order");
        graphArguments << (QLatin1String("--pretty=format:") + QLatin1String(graphLogFormatC));
        QToolButton *graphButton = addToggleButton(graphArguments, tr("Graph"),
                                              tr("Show textual graph log."));
        mapSetting(graphButton, m_client->settings()->boolPointer(GitSettings::graphLogKey));
        setFileName(fileName);
    }

    void setFileName(const QString &fileNames)
    {
        m_fileName = fileNames;
    }

    void executeCommand()
    {
        m_client->log(m_workingDirectory, m_fileName, m_enableAnnotationContextMenu, baseArguments());
    }

private:
    GitClient *m_client;
    QString m_workingDirectory;
    bool m_enableAnnotationContextMenu;
    QString m_fileName;
};

class ConflictHandler : public QObject
{
    Q_OBJECT
public:
    ConflictHandler(VcsBase::Command *parentCommand,
                    const QString &workingDirectory,
                    const QString &command = QString())
        : QObject(parentCommand),
          m_workingDirectory(workingDirectory),
          m_command(command)
    {
        if (parentCommand) {
            parentCommand->addFlags(VcsBasePlugin::ExpectRepoChanges);
            connect(parentCommand, SIGNAL(output(QString)), this, SLOT(readStdOut(QString)));
            connect(parentCommand, SIGNAL(errorText(QString)), this, SLOT(readStdErr(QString)));
        }
    }

    ~ConflictHandler()
    {
        // If interactive rebase editor window is closed, plugin is terminated
        // but referenced here when the command ends
        if (GitPlugin *plugin = GitPlugin::instance()) {
            GitClient *client = plugin->gitClient();
            if (m_commit.isEmpty() && m_files.isEmpty()) {
                if (client->checkCommandInProgress(m_workingDirectory) == GitClient::NoCommand)
                    client->endStashScope(m_workingDirectory);
            } else {
                client->handleMergeConflicts(m_workingDirectory, m_commit, m_files, m_command);
            }
        }
    }

public slots:
    void readStdOut(const QString &data)
    {
        static QRegExp patchFailedRE(QLatin1String("Patch failed at ([^\\n]*)"));
        static QRegExp conflictedFilesRE(QLatin1String("Merge conflict in ([^\\n]*)"));
        if (patchFailedRE.indexIn(data) != -1)
            m_commit = patchFailedRE.cap(1);
        int fileIndex = -1;
        while ((fileIndex = conflictedFilesRE.indexIn(data, fileIndex + 1)) != -1) {
            m_files.append(conflictedFilesRE.cap(1));
        }
    }

    void readStdErr(const QString &data)
    {
        static QRegExp couldNotApplyRE(QLatin1String("[Cc]ould not (?:apply|revert) ([^\\n]*)"));
        if (couldNotApplyRE.indexIn(data) != -1)
            m_commit = couldNotApplyRE.cap(1);
    }
private:
    QString m_workingDirectory;
    QString m_command;
    QString m_commit;
    QStringList m_files;
};

class ProgressParser : public VcsBase::ProgressParser
{
public:
    ProgressParser() :
        m_progressExp(QLatin1String("\\((\\d+)/(\\d+)\\)")) // e.g. Rebasing (7/42)
    {
    }

protected:
    void parseProgress(const QString &text)
    {
        if (m_progressExp.lastIndexIn(text) != -1)
            setProgressAndMaximum(m_progressExp.cap(1).toInt(), m_progressExp.cap(2).toInt());
    }

private:
    QRegExp m_progressExp;
};



Core::IEditor *locateEditor(const char *property, const QString &entry)
{
    foreach (Core::IDocument *document, Core::DocumentModel::openedDocuments())
        if (document->property(property).toString() == entry)
            return Core::DocumentModel::editorsForDocument(document).first();
    return 0;
}

// Return converted command output, remove '\r' read on Windows
static inline QString commandOutputFromLocal8Bit(const QByteArray &a)
{
    return Utils::SynchronousProcess::normalizeNewlines(QString::fromLocal8Bit(a));
}

// Return converted command output split into lines
static inline QStringList commandOutputLinesFromLocal8Bit(const QByteArray &a)
{
    QString output = commandOutputFromLocal8Bit(a);
    const QChar newLine = QLatin1Char('\n');
    if (output.endsWith(newLine))
        output.truncate(output.size() - 1);
    if (output.isEmpty())
        return QStringList();
    return output.split(newLine);
}

static inline VcsBase::VcsBaseOutputWindow *outputWindow()
{
    return VcsBase::VcsBaseOutputWindow::instance();
}

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

static inline QString currentDocumentPath()
{
    if (Core::IDocument *document= Core::EditorManager::currentDocument())
        return QFileInfo(document->filePath()).path();
    return QString();
}

static inline QStringList statusArguments()
{
    return QStringList() << QLatin1String("-c") << QLatin1String("color.status=false")
                         << QLatin1String("status");
}

static inline void msgCannotRun(const QString &message, QString *errorMessage)
{
    if (errorMessage)
        *errorMessage = message;
    else
        outputWindow()->appendError(message);
}

static inline void msgCannotRun(const QStringList &args, const QString &workingDirectory,
                                const QByteArray &error, QString *errorMessage)
{
    const QString message = GitClient::tr("Cannot run \"%1 %2\" in \"%2\": %3")
            .arg(QLatin1String("git ") + args.join(QLatin1String(" ")),
                 QDir::toNativeSeparators(workingDirectory),
                 commandOutputFromLocal8Bit(error));

    msgCannotRun(message, errorMessage);
}

// ---------------- GitClient

const char *GitClient::stashNamePrefix = "stash@{";

GitClient::GitClient(GitSettings *settings) :
    m_cachedGitVersion(0),
    m_msgWait(tr("Waiting for data...")),
    m_settings(settings),
    m_disableEditor(false),
    m_contextDiffFileIndex(-1),
    m_contextChunkIndex(-1)
{
    QTC_CHECK(settings);
    connect(Core::ICore::instance(), SIGNAL(saveSettingsRequested()), this, SLOT(saveSettings()));
    m_gitQtcEditor = QString::fromLatin1("\"%1\" -client -block -pid %2")
            .arg(QCoreApplication::applicationFilePath())
            .arg(QCoreApplication::applicationPid());
}

GitClient::~GitClient()
{
}

QString GitClient::findRepositoryForDirectory(const QString &dir)
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
    return fullySynchronousGit(workingDirectory, arguments, &output, 0,
                               VcsBasePlugin::SuppressCommandLogging);
}

VcsBase::VcsBaseEditorWidget *GitClient::findExistingVCSEditor(const char *registerDynamicProperty,
                                                               const QString &dynamicPropertyValue) const
{
    VcsBase::VcsBaseEditorWidget *rc = 0;
    Core::IEditor *outputEditor = locateEditor(registerDynamicProperty, dynamicPropertyValue);
    if (!outputEditor)
        return 0;

    // Exists already
    Core::EditorManager::activateEditor(outputEditor);
    outputEditor->document()->setContents(m_msgWait.toUtf8());
    rc = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(outputEditor);

    return rc;
}

DiffEditor::DiffEditorDocument *GitClient::createDiffEditor(const QString &documentId,
                                                            const QString &source,
                                                            const QString &title) const
{
    DiffEditor::DiffEditorDocument *diffEditorDocument = DiffEditor::DiffEditorManager::findOrCreate(documentId, title);
    QTC_ASSERT(diffEditorDocument, return 0);
    VcsBasePlugin::setSource(diffEditorDocument, source);

    connect(diffEditorDocument->controller(), SIGNAL(chunkActionsRequested(QMenu*,int,int)),
            this, SLOT(slotChunkActionsRequested(QMenu*,int,int)));

    return diffEditorDocument;
}

void GitClient::slotChunkActionsRequested(QMenu *menu, int diffFileIndex, int chunkIndex)
{
    menu->addSeparator();
    QAction *stageChunkAction = menu->addAction(tr("Stage Chunk"));
    connect(stageChunkAction, SIGNAL(triggered()), this, SLOT(slotStageChunk()));
    QAction *unstageChunkAction = menu->addAction(tr("Unstage Chunk"));
    connect(unstageChunkAction, SIGNAL(triggered()), this, SLOT(slotUnstageChunk()));

    m_contextDiffFileIndex = diffFileIndex;
    m_contextChunkIndex = chunkIndex;
    m_contextDocument = qobject_cast<DiffEditor::DiffEditorController *>(sender());

    if (m_contextDiffFileIndex < 0 || m_contextChunkIndex < 0 || !m_contextDocument) {
        stageChunkAction->setEnabled(false);
        unstageChunkAction->setEnabled(false);
    }
}

QString GitClient::makePatch(int diffFileIndex, int chunkIndex, bool revert) const
{
    if (m_contextDocument.isNull())
        return QString();

    if (diffFileIndex < 0 || chunkIndex < 0)
        return QString();

    QList<DiffEditor::FileData> fileDataList = m_contextDocument->diffFiles();

    if (diffFileIndex >= fileDataList.count())
        return QString();

    const DiffEditor::FileData fileData = fileDataList.at(diffFileIndex);
    if (chunkIndex >= fileData.chunks.count())
        return QString();

    const DiffEditor::ChunkData chunkData = fileData.chunks.at(chunkIndex);
    const bool lastChunk = (chunkIndex == fileData.chunks.count() - 1);

    const QString fileName = revert
            ? fileData.rightFileInfo.fileName
            : fileData.leftFileInfo.fileName;

    return DiffEditor::DiffUtils::makePatch(chunkData,
                                QLatin1String("a/") + fileName,
                                QLatin1String("b/") + fileName,
                                lastChunk && fileData.lastChunkAtTheEndOfFile);
}

void GitClient::slotStageChunk()
{
    if (m_contextDocument.isNull())
        return;

    const QString patch = makePatch(m_contextDiffFileIndex,
                                    m_contextChunkIndex, false);
    if (patch.isEmpty())
        return;

    stage(patch, false);
}

void GitClient::slotUnstageChunk()
{
    if (m_contextDocument.isNull())
        return;

    const QString patch = makePatch(m_contextDiffFileIndex,
                                    m_contextChunkIndex, true);
    if (patch.isEmpty())
        return;

    stage(patch, true);
}

void GitClient::stage(const QString &patch, bool revert)
{
    VcsBase::VcsBaseOutputWindow *outwin =
            VcsBase::VcsBaseOutputWindow::instance();
    QTemporaryFile patchFile;
    if (!patchFile.open())
        return;

    const QString baseDir = m_contextDocument->workingDirectory();
    QTextCodec *codec = Core::EditorManager::defaultTextCodec();
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
                outwin->append(tr("Chunk successfully unstaged"));
            else
                outwin->append(tr("Chunk successfully staged"));
        } else {
            outwin->append(errorMessage);
        }
        m_contextDocument->requestReload();
    } else {
        outwin->appendError(errorMessage);
    }
}

/* Create an editor associated to VCS output of a source file/directory
 * (using the file's codec). Makes use of a dynamic property to find an
 * existing instance and to reuse it (in case, say, 'git diff foo' is
 * already open). */
VcsBase::VcsBaseEditorWidget *GitClient::createVcsEditor(
        const Core::Id &id,
        QString title,
        const QString &source, // Source file or directory
        CodecType codecType,
        const char *registerDynamicProperty, // Dynamic property and value to identify that editor
        const QString &dynamicPropertyValue,
        VcsBase::VcsBaseEditorParameterWidget *configWidget) const
{
    VcsBase::VcsBaseEditorWidget *rc = 0;
    QTC_CHECK(!findExistingVCSEditor(registerDynamicProperty, dynamicPropertyValue));

    // Create new, set wait message, set up with source and codec
    Core::IEditor *outputEditor = Core::EditorManager::openEditorWithContents(id, &title,
                                                                              m_msgWait.toUtf8());
    outputEditor->document()->setProperty(registerDynamicProperty, dynamicPropertyValue);
    rc = VcsBase::VcsBaseEditorWidget::getVcsBaseEditor(outputEditor);
    connect(rc, SIGNAL(annotateRevisionRequested(QString,QString,QString,int)),
            this, SLOT(slotBlameRevisionRequested(QString,QString,QString,int)));
    QTC_ASSERT(rc, return 0);
    rc->setSource(source);
    if (codecType == CodecSource)
        rc->setCodec(getSourceCodec(source));
    else if (codecType == CodecLogOutput)
        rc->setCodec(encoding(source, "i18n.logOutputEncoding"));

    rc->setForceReadOnly(true);

    if (configWidget)
        rc->setConfigurationWidget(configWidget);

    return rc;
}

void GitClient::diff(const QString &workingDirectory,
                     const QStringList &unstagedFileNames,
                     const QStringList &stagedFileNames)
{
    GitDiffEditorReloader::DiffType diffType = GitDiffEditorReloader::DiffProjectList;

    if (unstagedFileNames.empty() && stagedFileNames.empty())
        diffType = GitDiffEditorReloader::DiffRepository;
    else if (!stagedFileNames.empty())
        diffType = GitDiffEditorReloader::DiffFileList;

    QString title = tr("Git Diff Projects");
    QString documentTypeId = QLatin1String("Projects:");
    if (diffType ==  GitDiffEditorReloader::DiffRepository) {
        title = tr("Git Diff Repository");
        documentTypeId = QLatin1String("Repository:");
    } else if (diffType == GitDiffEditorReloader::DiffFileList) {
        title = tr("Git Diff Files");
        documentTypeId = QLatin1String("Files:");
    }

    const QString documentId = documentTypeId + workingDirectory;

    DiffEditor::DiffEditorDocument *diffEditorDocument =
            DiffEditor::DiffEditorManager::find(documentId);
    if (!diffEditorDocument) {
        diffEditorDocument = createDiffEditor(documentId, workingDirectory, title);

        GitDiffEditorReloader *reloader =
                new GitDiffEditorReloader(diffEditorDocument->controller());
        reloader->setDiffEditorController(diffEditorDocument->controller());

        reloader->setWorkingDirectory(workingDirectory);
        reloader->setDiffType(diffType);
        if (diffType == GitDiffEditorReloader::DiffFileList)
            reloader->setFileList(stagedFileNames, unstagedFileNames);
        else if (diffType == GitDiffEditorReloader::DiffProjectList)
            reloader->setProjectList(unstagedFileNames);
    }

    diffEditorDocument->controller()->requestReload();

    Core::EditorManager::activateEditorForDocument(diffEditorDocument);
}

void GitClient::diff(const QString &workingDirectory, const QString &fileName)
{
    const QString title = tr("Git Diff \"%1\"").arg(fileName);
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(
                workingDirectory, fileName);
    const QString documentId = QLatin1String("File:") + sourceFile;
    DiffEditor::DiffEditorDocument *diffEditorDocument =
            DiffEditor::DiffEditorManager::find(documentId);
    if (!diffEditorDocument) {
        diffEditorDocument = createDiffEditor(documentId, sourceFile, title);

        GitDiffEditorReloader *reloader =
                new GitDiffEditorReloader(diffEditorDocument->controller());
        reloader->setDiffEditorController(diffEditorDocument->controller());

        reloader->setWorkingDirectory(workingDirectory);
        reloader->setDiffType(GitDiffEditorReloader::DiffFile);
        reloader->setFileName(fileName);
    }

    diffEditorDocument->controller()->requestReload();

    Core::EditorManager::activateEditorForDocument(diffEditorDocument);
}

void GitClient::diffBranch(const QString &workingDirectory,
                           const QString &branchName)
{
    const QString title = tr("Git Diff Branch \"%1\"").arg(branchName);
    const QString documentId = QLatin1String("Branch:") + branchName;
    DiffEditor::DiffEditorDocument *diffEditorDocument =
            DiffEditor::DiffEditorManager::find(documentId);
    if (!diffEditorDocument) {
        diffEditorDocument = createDiffEditor(documentId, workingDirectory, title);

        GitDiffEditorReloader *reloader =
                new GitDiffEditorReloader(diffEditorDocument->controller());
        reloader->setDiffEditorController(diffEditorDocument->controller());

        reloader->setWorkingDirectory(workingDirectory);
        reloader->setDiffType(GitDiffEditorReloader::DiffBranch);
        reloader->setBranchName(branchName);
    }

    diffEditorDocument->controller()->requestReload();

    Core::EditorManager::activateEditorForDocument(diffEditorDocument);
}

void GitClient::merge(const QString &workingDirectory,
                      const QStringList &unmergedFileNames)
{
    MergeTool *mergeTool = new MergeTool(this);
    if (!mergeTool->start(workingDirectory, unmergedFileNames))
        delete mergeTool;
}

void GitClient::status(const QString &workingDirectory)
{
    QStringList statusArgs = statusArguments();
    statusArgs << QLatin1String("-u");
    VcsBase::VcsBaseOutputWindow *outwin = outputWindow();
    outwin->setRepository(workingDirectory);
    VcsBase::Command *command = executeGit(workingDirectory, statusArgs, 0, true);
    connect(command, SIGNAL(finished(bool,int,QVariant)), outwin, SLOT(clearRepository()),
            Qt::QueuedConnection);
}

void GitClient::log(const QString &workingDirectory, const QString &fileName,
                    bool enableAnnotationContextMenu, const QStringList &args)
{
    const QString msgArg = fileName.isEmpty() ? workingDirectory : fileName;
    const QString title = tr("Git Log \"%1\"").arg(msgArg);
    const Core::Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, fileName);
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("logFileName", sourceFile);
    if (!editor)
        editor = createVcsEditor(editorId, title, sourceFile, CodecLogOutput, "logFileName", sourceFile,
                                 new GitLogArgumentsWidget(this, workingDirectory,
                                                           enableAnnotationContextMenu,
                                                           args, fileName));
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);
    editor->setWorkingDirectory(workingDirectory);

    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption)
              << QLatin1String(decorateOption);

    int logCount = settings()->intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << QLatin1String("-n") << QString::number(logCount);

    GitLogArgumentsWidget *argWidget = qobject_cast<GitLogArgumentsWidget *>(editor->configurationWidget());
    argWidget->setBaseArguments(args);
    argWidget->setFileName(fileName);
    QStringList userArgs = argWidget->arguments();

    arguments.append(userArgs);

    if (!fileName.isEmpty())
        arguments << QLatin1String("--follow") << QLatin1String("--") << fileName;

    executeGit(workingDirectory, arguments, editor);
}

void GitClient::reflog(const QString &workingDirectory)
{
    const QString title = tr("Git Reflog \"%1\"").arg(workingDirectory);
    const Core::Id editorId = Git::Constants::GIT_LOG_EDITOR_ID;
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("reflogRepository", workingDirectory);
    if (!editor) {
        editor = createVcsEditor(editorId, title, workingDirectory, CodecLogOutput,
                                 "reflogRepository", workingDirectory, 0);
    }
    editor->setWorkingDirectory(workingDirectory);

    QStringList arguments;
    arguments << QLatin1String("reflog") << QLatin1String(noColorOption)
              << QLatin1String(decorateOption);

    int logCount = settings()->intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << QLatin1String("-n") << QString::number(logCount);

    executeGit(workingDirectory, arguments, editor);
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
        outputWindow()->appendError(msgCannotShow(id));
        return;
    }

    const QString title = tr("Git Show \"%1\"").arg(name.isEmpty() ? id : name);
    const QFileInfo sourceFi(source);
    const QString workingDirectory = sourceFi.isDir()
            ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    const QString documentId = QLatin1String("Show:") + id;
    DiffEditor::DiffEditorDocument *diffEditorDocument =
            DiffEditor::DiffEditorManager::find(documentId);
    if (!diffEditorDocument) {
        diffEditorDocument = createDiffEditor(documentId, source, title);

        connect(diffEditorDocument->controller(), SIGNAL(expandBranchesRequested(QString)),
                this, SLOT(branchesForCommit(QString)));

        diffEditorDocument->controller()->setDescriptionEnabled(true);

        GitDiffEditorReloader *reloader =
                new GitDiffEditorReloader(diffEditorDocument->controller());
        reloader->setDiffEditorController(diffEditorDocument->controller());

        reloader->setWorkingDirectory(workingDirectory);
        reloader->setDiffType(GitDiffEditorReloader::DiffShow);
        reloader->setFileName(source);
        reloader->setId(id);
    }

    diffEditorDocument->controller()->requestReload();

    Core::EditorManager::activateEditorForDocument(diffEditorDocument);
}

void GitClient::saveSettings()
{
    settings()->writeSettings(Core::ICore::settings());
}

void GitClient::slotBlameRevisionRequested(const QString &workingDirectory, const QString &file,
                                           QString change, int lineNumber)
{
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = change.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        change.truncate(blankPos);
    blame(workingDirectory, QStringList(), file, change, lineNumber);
}

QTextCodec *GitClient::getSourceCodec(const QString &file) const
{
    return QFileInfo(file).isFile() ? VcsBase::VcsBaseEditorWidget::getCodec(file)
                                    : encoding(file, "gui.encoding");
}

void GitClient::blame(const QString &workingDirectory,
                      const QStringList &args,
                      const QString &fileName,
                      const QString &revision,
                      int lineNumber)
{
    const Core::Id editorId = Git::Constants::GIT_BLAME_EDITOR_ID;
    const QString id = VcsBase::VcsBaseEditorWidget::getTitleId(workingDirectory, QStringList(fileName), revision);
    const QString title = tr("Git Blame \"%1\"").arg(id);
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, fileName);

    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("blameFileName", id);
    if (!editor) {
        GitBlameArgumentsWidget *argWidget =
                new GitBlameArgumentsWidget(this, workingDirectory, args,
                                            revision, fileName);
        editor = createVcsEditor(editorId, title, sourceFile, CodecSource, "blameFileName", id, argWidget);
        argWidget->setEditor(editor);
    }

    editor->setWorkingDirectory(workingDirectory);
    QStringList arguments(QLatin1String("blame"));
    arguments << QLatin1String("--root");
    arguments.append(editor->configurationWidget()->arguments());
    arguments << QLatin1String("--") << fileName;
    if (!revision.isEmpty())
        arguments << revision;
    executeGit(workingDirectory, arguments, editor, false, 0, lineNumber);
}

bool GitClient::synchronousCheckout(const QString &workingDirectory,
                                          const QString &ref,
                                          QString *errorMessage /* = 0 */)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments = setupCheckoutArguments(workingDirectory, ref);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText,
                                        VcsBasePlugin::ExpectRepoChanges);
    outputWindow()->append(commandOutputFromLocal8Bit(outputText));
    if (!rc) {
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);
        return false;
    }
    updateSubmodulesIfNeeded(workingDirectory, true);
    return true;
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

    if (QMessageBox::question(Core::ICore::mainWindow(), tr("Create Local Branch"),
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

    BranchAddDialog branchAddDialog(localBranches, true, Core::ICore::mainWindow());
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
    if (argument == QLatin1String("--hard"))
        flags |= VcsBasePlugin::ExpectRepoChanges;
    executeGit(workingDirectory, arguments, 0, true, flags);
}

void GitClient::addFile(const QString &workingDirectory, const QString &fileName)
{
    QStringList arguments;
    arguments << QLatin1String("add") << fileName;

    executeGit(workingDirectory, arguments, 0);
}

bool GitClient::synchronousLog(const QString &workingDirectory, const QStringList &arguments,
                               QString *output, QString *errorMessageIn, unsigned flags)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList allArguments;
    allArguments << QLatin1String("log") << QLatin1String(noColorOption);
    allArguments.append(arguments);
    const bool rc = fullySynchronousGit(workingDirectory, allArguments, &outputText, &errorText,
                                        flags);
    if (rc) {
        if (QTextCodec *codec = encoding(workingDirectory, "i18n.logOutputEncoding"))
            *output = codec->toUnicode(outputText);
        else
            *output = commandOutputFromLocal8Bit(outputText);
    } else {
        msgCannotRun(tr("Cannot obtain log of \"%1\": %2")
                     .arg(QDir::toNativeSeparators(workingDirectory),
                         commandOutputFromLocal8Bit(errorText)), errorMessageIn);
    }
    return rc;
}

bool GitClient::synchronousAdd(const QString &workingDirectory, const QStringList &files)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("add") << files;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        msgCannotRun(tr("Cannot add %n file(s) to \"%1\": %2", 0, files.size())
                     .arg(QDir::toNativeSeparators(workingDirectory),
                          commandOutputFromLocal8Bit(errorText)), 0);
    }
    return rc;
}

bool GitClient::synchronousDelete(const QString &workingDirectory,
                                  bool force,
                                  const QStringList &files)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("rm");
    if (force)
        arguments << QLatin1String("--force");
    arguments.append(files);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        msgCannotRun(tr("Cannot remove %n file(s) from \"%1\": %2", 0, files.size())
                     .arg(QDir::toNativeSeparators(workingDirectory),
                          commandOutputFromLocal8Bit(errorText)), 0);
    }
    return rc;
}

bool GitClient::synchronousMove(const QString &workingDirectory,
                                const QString &from,
                                const QString &to)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("mv");
    arguments << (from);
    arguments << (to);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        msgCannotRun(tr("Cannot move from \"%1\" to \"%2\": %3")
                     .arg(from, to, commandOutputFromLocal8Bit(errorText)), 0);
    }
    return rc;
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
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    const QString output = commandOutputFromLocal8Bit(outputText);
    outputWindow()->append(output);
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
    QByteArray errorText;
    const QStringList arguments(QLatin1String("init"));
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    // '[Re]Initialized...'
    outputWindow()->append(commandOutputFromLocal8Bit(outputText));
    if (!rc) {
        outputWindow()->appendError(commandOutputFromLocal8Bit(errorText));
    } else {
        // TODO: Turn this into a VcsBaseClient and use resetCachedVcsInfo(...)
        Core::VcsManager::resetVersionControlForDirectory(workingDirectory);
    }
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
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText,
                                        VcsBasePlugin::ExpectRepoChanges);
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
                                      QString *output, QString *errorMessage)
{
    QByteArray outputTextData;
    QByteArray errorText;

    QStringList args(QLatin1String("rev-list"));
    args << QLatin1String(noColorOption) << arguments;

    const bool rc = fullySynchronousGit(workingDirectory, args, &outputTextData, &errorText,
                                        VcsBasePlugin::SuppressCommandLogging);
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
                                           const QStringList &files /* = QStringList() */,
                                           const QString &revision,
                                           QStringList *parents,
                                           QString *errorMessage)
{
    QString outputText;
    QString errorText;
    QStringList arguments;
    if (parents && !isValidRevision(revision)) { // Not Committed Yet
        *parents = QStringList(QLatin1String(HEAD));
        return true;
    }
    arguments << QLatin1String("--parents") << QLatin1String("--max-count=1") << revision;
    if (!files.isEmpty()) {
        arguments.append(QLatin1String("--"));
        arguments.append(files);
    }

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

QString GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision)
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

QString GitClient::synchronousCurrentLocalBranch(const QString &workingDirectory)
{
    QByteArray outputTextData;
    QStringList arguments;
    arguments << QLatin1String("symbolic-ref") << QLatin1String(HEAD);
    if (fullySynchronousGit(workingDirectory, arguments, &outputTextData, 0,
                            VcsBasePlugin::SuppressCommandLogging)) {
        QString branch = commandOutputFromLocal8Bit(outputTextData.trimmed());
        const QString refsHeadsPrefix = QLatin1String("refs/heads/");
        if (branch.startsWith(refsHeadsPrefix)) {
            branch.remove(0, refsHeadsPrefix.count());
            return branch;
        }
    }
    return QString();
}

bool GitClient::synchronousHeadRefs(const QString &workingDirectory, QStringList *output,
                                    QString *errorMessage)
{
    QStringList args;
    args << QLatin1String("show-ref") << QLatin1String("--head")
         << QLatin1String("--abbrev=10") << QLatin1String("--dereference");
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText,
                                        VcsBasePlugin::SuppressCommandLogging);
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
QString GitClient::synchronousTopic(const QString &workingDirectory)
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
    if (fullySynchronousGit(workingDirectory, arguments, &output, 0, VcsBasePlugin::NoOutput)) {
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
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText,
                                        VcsBasePlugin::SuppressCommandLogging);
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
                                         QString &precedes, QString &follows)
{
    QByteArray pr;
    QStringList arguments;
    arguments << QLatin1String("describe") << QLatin1String("--contains") << revision;
    fullySynchronousGit(workingDirectory, arguments, &pr, 0,
                        VcsBasePlugin::SuppressCommandLogging);
    int tilde = pr.indexOf('~');
    if (tilde != -1)
        pr.truncate(tilde);
    else
        pr = pr.trimmed();
    precedes = QString::fromLocal8Bit(pr);

    QStringList parents;
    QString errorMessage;
    synchronousParentRevisions(workingDirectory, QStringList(), revision, &parents, &errorMessage);
    foreach (const QString &p, parents) {
        QByteArray pf;
        arguments.clear();
        arguments << QLatin1String("describe") << QLatin1String("--tags")
                  << QLatin1String("--abbrev=0") << p;
        fullySynchronousGit(workingDirectory, arguments, &pf, 0,
                            VcsBasePlugin::SuppressCommandLogging);
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

    DiffEditor::DiffEditorController *editorController
            = qobject_cast<DiffEditor::DiffEditorController *>(sender());
    QString workingDirectory = editorController->workingDirectory();
    VcsBase::Command *command = new VcsBase::Command(gitBinaryPath(), workingDirectory,
                                                     processEnvironment());
    command->setCodec(getSourceCodec(currentDocumentPath()));

    connect(command, SIGNAL(output(QString)), editorController,
            SLOT(branchesForCommitReceived(QString)));

    command->addJob(arguments, -1);
    command->execute();
    command->setCookie(workingDirectory);
}

bool GitClient::isRemoteCommit(const QString &workingDirectory, const QString &commit)
{
    QStringList arguments;
    QByteArray outputText;
    arguments << QLatin1String("branch") << QLatin1String("-r")
              << QLatin1String("--contains") << commit;
    fullySynchronousGit(workingDirectory, arguments, &outputText, 0,
                        VcsBasePlugin::SuppressCommandLogging);
    return !outputText.isEmpty();
}

bool GitClient::isFastForwardMerge(const QString &workingDirectory, const QString &branch)
{
    QStringList arguments;
    QByteArray outputText;
    arguments << QLatin1String("merge-base") << QLatin1String(HEAD) << branch;
    fullySynchronousGit(workingDirectory, arguments, &outputText, 0,
                        VcsBasePlugin::SuppressCommandLogging);
    return commandOutputFromLocal8Bit(outputText).trimmed()
            == synchronousTopRevision(workingDirectory);
}

// Format an entry in a one-liner for selection list using git log.
QString GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                            const QString &format)
{
    QString description;
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption)
              << (QLatin1String("--pretty=format:") + format)
              << QLatin1String("--max-count=1") << revision;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText);
    if (!rc) {
        VcsBase::VcsBaseOutputWindow *outputWindow = VcsBase::VcsBaseOutputWindow::instance();
        outputWindow->appendSilently(tr("Cannot describe revision \"%1\" in \"%2\": %3")
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
                                    unsigned flags, bool *unchanged)
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
                if (!inputText(Core::ICore::mainWindow(),
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
            outputWindow()->appendWarning(msgNoChangedFiles());
        break;
    case StatusFailed:
        outputWindow()->appendError(errorMessage);
        break;
    }
    if (!success)
        message.clear();
    return message;
}

bool GitClient::executeSynchronousStash(const QString &workingDirectory,
                                        const QString &message,
                                        QString *errorMessage)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("stash");
    if (!message.isEmpty())
        arguments << QLatin1String("save") << message;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText,
                                        VcsBasePlugin::ExpectRepoChanges);
    if (!rc)
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);

    return rc;
}

// Resolve a stash name from message
bool GitClient::stashNameFromMessage(const QString &workingDirectory,
                                     const QString &message, QString *name,
                                     QString *errorMessage)
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
                                     QString *output, QString *errorMessage)
{
    branchArgs.push_front(QLatin1String("branch"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, branchArgs, &outputText, &errorText);
    *output = commandOutputFromLocal8Bit(outputText);
    if (!rc)
        msgCannotRun(branchArgs, workingDirectory, errorText, errorMessage);

    return rc;
}

bool GitClient::synchronousTagCmd(const QString &workingDirectory, QStringList tagArgs,
                                  QString *output, QString *errorMessage)
{
    tagArgs.push_front(QLatin1String("tag"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, tagArgs, &outputText, &errorText);
    *output = commandOutputFromLocal8Bit(outputText);
    if (!rc)
        msgCannotRun(tagArgs, workingDirectory, errorText, errorMessage);

    return rc;
}

bool GitClient::synchronousForEachRefCmd(const QString &workingDirectory, QStringList args,
                                      QString *output, QString *errorMessage)
{
    args.push_front(QLatin1String("for-each-ref"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText,
                                        VcsBasePlugin::SuppressCommandLogging);
    *output = commandOutputFromLocal8Bit(outputText);
    if (!rc)
        msgCannotRun(args, workingDirectory, errorText, errorMessage);

    return rc;
}

bool GitClient::synchronousRemoteCmd(const QString &workingDirectory, QStringList remoteArgs,
                                     QString *output, QString *errorMessage, bool silent)
{
    remoteArgs.push_front(QLatin1String("remote"));
    QByteArray outputText;
    QByteArray errorText;
    if (!fullySynchronousGit(workingDirectory, remoteArgs, &outputText, &errorText,
                             silent ? VcsBasePlugin::SuppressCommandLogging : 0)) {
        msgCannotRun(remoteArgs, workingDirectory, errorText, errorMessage);
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

QMap<QString,QString> GitClient::synchronousRemotesList(const QString &workingDirectory,
                                                        QString *errorMessage)
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
                                                  QString *errorMessage)
{
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;

    // get submodule status
    arguments << QLatin1String("submodule") << QLatin1String("status");
    if (!fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText)) {
        msgCannotRun(tr("Cannot retrieve submodule status of \"%1\": %2")
                     .arg(QDir::toNativeSeparators(workingDirectory),
                          commandOutputFromLocal8Bit(errorText)), errorMessage);
        return QStringList();
    }
    return commandOutputLinesFromLocal8Bit(outputTextData);
}

SubmoduleDataMap GitClient::submoduleList(const QString &workingDirectory)
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
                                 QString *output, QString *errorMessage)
{
    if (!canShow(id)) {
        *errorMessage = msgCannotShow(id);
        return false;
    }
    QStringList args(QLatin1String("show"));
    args << QLatin1String(decorateOption) << QLatin1String(noColorOption) << id;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
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
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
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
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (rc) {
        if (!errorText.isEmpty())
            *errorMessage = tr("There were warnings while applying \"%1\" to \"%2\":\n%3").arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
    } else {
        *errorMessage = tr("Cannot apply patch \"%1\" to \"%2\": %3").arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    return true;
}

// Factory function to create an asynchronous command
VcsBase::Command *GitClient::createCommand(const QString &workingDirectory,
                                           VcsBase::VcsBaseEditorWidget* editor,
                                           bool useOutputToWindow,
                                           int editorLineNumber)
{
    VcsBase::Command *command = new VcsBase::Command(gitBinaryPath(), workingDirectory, processEnvironment());
    command->setCodec(getSourceCodec(currentDocumentPath()));
    command->setCookie(QVariant(editorLineNumber));
    if (editor) {
        editor->setCommand(command);
        connect(command, SIGNAL(finished(bool,int,QVariant)),
                editor, SLOT(commandFinishedGotoLine(bool,int,QVariant)));
    }
    if (useOutputToWindow) {
        command->addFlags(VcsBasePlugin::ShowStdOutInLogWindow);
        command->addFlags(VcsBasePlugin::ShowSuccessMessage);
        if (editor) // assume that the commands output is the important thing
            command->addFlags(VcsBasePlugin::SilentOutput);
    } else if (editor) {
        connect(command, SIGNAL(output(QString)), editor, SLOT(setPlainTextFiltered(QString)));
    }

    return command;
}

// Execute a single command
VcsBase::Command *GitClient::executeGit(const QString &workingDirectory,
                                        const QStringList &arguments,
                                        VcsBase::VcsBaseEditorWidget* editor,
                                        bool useOutputToWindow,
                                        unsigned additionalFlags,
                                        int editorLineNumber)
{
    outputWindow()->appendCommand(workingDirectory, settings()->stringValue(GitSettings::binaryPathKey), arguments);
    VcsBase::Command *command = createCommand(workingDirectory, editor, useOutputToWindow, editorLineNumber);
    command->addJob(arguments, settings()->intValue(GitSettings::timeoutKey));
    command->addFlags(additionalFlags);
    command->execute();
    return command;
}

QProcessEnvironment GitClient::processEnvironment() const
{
    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    QString gitPath = settings()->stringValue(GitSettings::pathKey);
    if (!gitPath.isEmpty()) {
        gitPath += Utils::HostOsInfo::pathListSeparator();
        gitPath += environment.value(QLatin1String("PATH"));
        environment.insert(QLatin1String("PATH"), gitPath);
    }
    if (Utils::HostOsInfo::isWindowsHost()
            && settings()->boolValue(GitSettings::winSetHomeEnvironmentKey)) {
        environment.insert(QLatin1String("HOME"), QDir::toNativeSeparators(QDir::homePath()));
    }
    environment.insert(QLatin1String("GIT_EDITOR"), m_disableEditor ? QLatin1String("true") : m_gitQtcEditor);
    // Set up SSH and C locale (required by git using perl).
    VcsBasePlugin::setProcessEnvironment(&environment, false);
    return environment;
}

bool GitClient::beginStashScope(const QString &workingDirectory, const QString &command,
                                StashFlag flag, PushAction pushAction)
{
    const QString repoDirectory = findRepositoryForDirectory(workingDirectory);
    QTC_ASSERT(!repoDirectory.isEmpty(), return false);
    StashInfo &stashInfo = m_stashInfo[repoDirectory];
    return stashInfo.init(repoDirectory, command, flag, pushAction);
}

GitClient::StashInfo &GitClient::stashInfo(const QString &workingDirectory)
{
    const QString repoDirectory = findRepositoryForDirectory(workingDirectory);
    QTC_CHECK(m_stashInfo.contains(repoDirectory));
    return m_stashInfo[repoDirectory];
}

void GitClient::endStashScope(const QString &workingDirectory)
{
    const QString repoDirectory = findRepositoryForDirectory(workingDirectory);
    QTC_ASSERT(m_stashInfo.contains(repoDirectory), return);
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

// Synchronous git execution using Utils::SynchronousProcess, with
// log windows updating.
Utils::SynchronousProcessResponse GitClient::synchronousGit(const QString &workingDirectory,
                                                            const QStringList &gitArguments,
                                                            unsigned flags,
                                                            QTextCodec *outputCodec)
{
    return VcsBasePlugin::runVcs(workingDirectory, gitBinaryPath(), gitArguments,
                                 settings()->intValue(GitSettings::timeoutKey) * 1000,
                                 flags, outputCodec, processEnvironment());
}

bool GitClient::fullySynchronousGit(const QString &workingDirectory,
                                    const QStringList &gitArguments,
                                    QByteArray* outputText,
                                    QByteArray* errorText,
                                    unsigned flags) const
{
    VcsBase::Command command(gitBinaryPath(), workingDirectory, processEnvironment());
    command.addFlags(flags);
    return command.runFullySynchronous(gitArguments,
                                       settings()->intValue(GitSettings::timeoutKey) * 1000,
                                       outputText, errorText);
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

    if (prompt && QMessageBox::question(Core::ICore::mainWindow(), tr("Submodules Found"),
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

    VcsBase::Command *cmd = executeGit(workingDirectory, arguments, 0, true,
                                       VcsBasePlugin::ExpectRepoChanges);
    connect(cmd, SIGNAL(finished(bool,int,QVariant)), this, SLOT(finishSubmoduleUpdate()));
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
                                             QString *output, QString *errorMessage)
{
    // Run 'status'. Note that git returns exitcode 1 if there are no added files.
    QByteArray outputText;
    QByteArray errorText;

    QStringList statusArgs = statusArguments();
    if (mode & NoUntracked)
        statusArgs << QLatin1String("--untracked-files=no");
    else
        statusArgs << QLatin1String("--untracked-files=all");
    if (mode & NoSubmodules)
        statusArgs << QLatin1String("--ignore-submodules=all");
    statusArgs << QLatin1String("-s") << QLatin1String("-b");

    const bool statusRc = fullySynchronousGit(workingDirectory, statusArgs,
                                              &outputText, &errorText, false);
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

QString GitClient::commandInProgressDescription(const QString &workingDirectory)
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

GitClient::CommandInProgress GitClient::checkCommandInProgress(const QString &workingDirectory)
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
                       QMessageBox::NoButton, Core::ICore::mainWindow());
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

QString GitClient::extendedShowDescription(const QString &workingDirectory, const QString &text)
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
    return modText;
}

// Quietly retrieve branch list of remote repository URL
//
// The branch HEAD is pointing to is always returned first.
QStringList GitClient::synchronousRepositoryBranches(const QString &repositoryURL)
{
    QStringList arguments(QLatin1String("ls-remote"));
    arguments << repositoryURL << QLatin1String(HEAD) << QLatin1String("refs/heads/*");
    const unsigned flags = VcsBasePlugin::SshPasswordPrompt
            | VcsBasePlugin::SuppressStdErrInLogWindow
            | VcsBasePlugin::SuppressFailMessageInLogWindow;
    const Utils::SynchronousProcessResponse resp = synchronousGit(QString(), arguments, flags);
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
    const QFileInfo binaryInfo(gitBinaryPath());
    QDir foundBinDir(binaryInfo.dir());
    const bool foundBinDirIsCmdDir = foundBinDir.dirName() == QLatin1String("cmd");
    QProcessEnvironment env = processEnvironment();
    if (tryLauchingGitK(env, workingDirectory, fileName, foundBinDir.path()))
        return;

    QString gitkPath = foundBinDir.path() + QLatin1String("/gitk");
    VcsBase::VcsBaseOutputWindow::instance()->appendSilently(msgCannotLaunch(gitkPath));

    if (foundBinDirIsCmdDir) {
        foundBinDir.cdUp();
        if (tryLauchingGitK(env, workingDirectory, fileName,
                            foundBinDir.path() + QLatin1String("/bin"))) {
            return;
        }
        gitkPath = foundBinDir.path() + QLatin1String("/gitk");
        VcsBase::VcsBaseOutputWindow::instance()->appendSilently(msgCannotLaunch(gitkPath));
    }

    Utils::Environment sysEnv = Utils::Environment::systemEnvironment();
    const QString exec = sysEnv.searchInPath(QLatin1String("gitk"));

    if (!exec.isEmpty() && tryLauchingGitK(env, workingDirectory, fileName,
                                           QFileInfo(exec).absolutePath())) {
        return;
    }

    VcsBase::VcsBaseOutputWindow::instance()->appendError(msgCannotLaunch(QLatin1String("gitk")));
}

void GitClient::launchRepositoryBrowser(const QString &workingDirectory)
{
    const QString repBrowserBinary = settings()->stringValue(GitSettings::repositoryBrowserCmd);
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
    if (Utils::HostOsInfo::isWindowsHost()) {
        // If git/bin is in path, use 'wish' shell to run. Otherwise (git/cmd), directly run gitk
        QString wish = gitBinDirectory + QLatin1String("/wish");
        if (QFileInfo(wish + QLatin1String(".exe")).exists()) {
            arguments << binary;
            binary = wish;
        }
    }
    VcsBase::VcsBaseOutputWindow *outwin = VcsBase::VcsBaseOutputWindow::instance();
    const QString gitkOpts = settings()->stringValue(GitSettings::gitkOptionsKey);
    if (!gitkOpts.isEmpty())
        arguments.append(Utils::QtcProcess::splitArgs(gitkOpts, Utils::HostOsInfo::hostOs()));
    if (!fileName.isEmpty())
        arguments << QLatin1String("--") << fileName;
    outwin->appendCommand(workingDirectory, binary, arguments);
    // This should always use QProcess::startDetached (as not to kill
    // the child), but that does not have an environment parameter.
    bool success = false;
    if (!settings()->stringValue(GitSettings::pathKey).isEmpty()) {
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workingDirectory);
        process->setProcessEnvironment(env);
        process->start(binary, arguments);
        success = process->waitForStarted();
        if (success)
            connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
        else
            delete process;
    } else {
        success = QProcess::startDetached(binary, arguments, workingDirectory);
    }

    return success;
}

bool GitClient::launchGitGui(const QString &workingDirectory) {
    bool success;
    QString gitBinary = gitBinaryPath(&success);
    if (success)
        success = QProcess::startDetached(gitBinary, QStringList(QLatin1String("gui")),
                                          workingDirectory);

    if (!success)
        outputWindow()->appendError(msgCannotLaunch(QLatin1String("git gui")));

    return success;
}

QString GitClient::gitBinaryPath(bool *ok, QString *errorMessage) const
{
    return settings()->gitBinaryPath(ok, errorMessage);
}

QTextCodec *GitClient::encoding(const QString &workingDirectory, const QByteArray &configVar) const
{
    QByteArray codecName = readConfigBytes(workingDirectory, QLatin1String(configVar)).trimmed();
    // Set default commit encoding to 'UTF-8', when it's not set,
    // to solve displaying error of commit log with non-latin characters.
    if (codecName.isEmpty())
        codecName = "UTF-8";
    return QTextCodec::codecForName(codecName);
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
    if (!fullySynchronousGit(repoDirectory, args, &outputText, 0,
                             VcsBasePlugin::SuppressCommandLogging)) {
        if (errorMessage)
            *errorMessage = tr("Cannot retrieve last commit data of repository \"%1\".").arg(repoDirectory);
        return false;
    }
    QTextCodec *authorCodec = Utils::HostOsInfo::isWindowsHost()
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
    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
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

        VcsBase::VcsBaseSubmitEditor::filterUntrackedFilesOfProject(repoDirectory, &untrackedFiles);
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
            commitData.panelData.author = readConfigValue(workingDirectory, QLatin1String("user.name"));
            commitData.panelData.email = readConfigValue(workingDirectory, QLatin1String("user.email"));
        }
        // Commit: Get the commit template
        QString templateFilename = gitDirectory.absoluteFilePath(QLatin1String("MERGE_MSG"));
        if (!QFile::exists(templateFilename))
            templateFilename = gitDirectory.absoluteFilePath(QLatin1String("SQUASH_MSG"));
        if (!QFile::exists(templateFilename)) {
            Utils::FileName templateName = Utils::FileName::fromUserInput(
                        readConfigValue(workingDirectory, QLatin1String("commit.template")));
            templateFilename = templateName.toString();
        }
        if (!templateFilename.isEmpty()) {
            // Make relative to repository
            const QFileInfo templateFileInfo(templateFilename);
            if (templateFileInfo.isRelative())
                templateFilename = repoDirectory + QLatin1Char('/') + templateFilename;
            Utils::FileReader reader;
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
                             VcsBase::SubmitFileModel *model)
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

    const bool rc = fullySynchronousGit(repositoryDirectory, args, &outputText, &errorText);
    const QString stdErr = commandOutputFromLocal8Bit(errorText);
    if (rc) {
        outputWindow()->appendMessage(msgCommitted(amendSHA1, commitCount));
        outputWindow()->appendError(stdErr);
    } else {
        outputWindow()->appendError(tr("Cannot commit %n file(s): %1\n", 0, commitCount).arg(stdErr));
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

    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
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
        = QMessageBox::question(Core::ICore::mainWindow(),
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
        outputWindow()->appendWarning(msg);
    }
        break;
    case RevertFailed:
        outputWindow()->appendError(errorMessage);
        break;
    }
}

void GitClient::fetch(const QString &workingDirectory, const QString &remote)
{
    QStringList arguments(QLatin1String("fetch"));
    arguments << (remote.isEmpty() ? QLatin1String("--all") : remote);
    VcsBase::Command *command = executeGit(workingDirectory, arguments, 0, true);
    command->setCookie(workingDirectory);
    connect(command, SIGNAL(success(QVariant)), this, SLOT(fetchFinished(QVariant)));
}

bool GitClient::executeAndHandleConflicts(const QString &workingDirectory,
                                          const QStringList &arguments,
                                          const QString &abortCommand)
{
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsBasePlugin::SshPasswordPrompt
            | VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBasePlugin::ExpectRepoChanges;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, arguments, flags);
    ConflictHandler conflictHandler(0, workingDirectory, abortCommand);
    // Notify about changed files or abort the rebase.
    const bool ok = resp.result == Utils::SynchronousProcessResponse::Finished;
    if (!ok) {
        conflictHandler.readStdOut(resp.stdOut);
        conflictHandler.readStdErr(resp.stdErr);
    }
    return ok;
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
        synchronousCheckoutFiles(findRepositoryForDirectory(workingDir), QStringList(), QString(), 0, false);
        return;
    }
    VcsBase::VcsBaseOutputWindow *outwin = VcsBase::VcsBaseOutputWindow::instance();
    QStringList arguments;
    arguments << abortCommand << QLatin1String("--abort");
    QByteArray stdOut;
    QByteArray stdErr;
    const bool rc = fullySynchronousGit(workingDir, arguments, &stdOut, &stdErr,
                                        VcsBasePlugin::ExpectRepoChanges);
    outwin->append(commandOutputFromLocal8Bit(stdOut));
    if (!rc)
        outwin->appendError(commandOutputFromLocal8Bit(stdErr));
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
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("branch");
    if (gitVersion() >= 0x010800)
        arguments << (QLatin1String("--set-upstream-to=") + tracking) << branch;
    else
        arguments << QLatin1String("--set-upstream") << branch << tracking;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        msgCannotRun(tr("Cannot set tracking branch: %1")
                     .arg(commandOutputFromLocal8Bit(errorText)), 0);
    }
    return rc;
}

void GitClient::handleMergeConflicts(const QString &workingDir, const QString &commit,
                                     const QStringList &files, const QString &abortCommand)
{
    QString message;
    if (!commit.isEmpty())
        message = tr("Conflicts detected with commit %1.").arg(commit);
    else if (!files.isEmpty())
        message = tr("Conflicts detected with files:\n%1").arg(files.join(QLatin1String("\n")));
    else
        message = tr("Conflicts detected.");
    QMessageBox mergeOrAbort(QMessageBox::Question, tr("Conflicts Detected"), message,
                             QMessageBox::NoButton, Core::ICore::mainWindow());
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

// Subversion: git svn
void GitClient::synchronousSubversionFetch(const QString &workingDirectory)
{
    QStringList args;
    args << QLatin1String("svn") << QLatin1String("fetch");
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VcsBasePlugin::SshPasswordPrompt
            | VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBasePlugin::ShowSuccessMessage;
    synchronousGit(workingDirectory, args, flags);
}

void GitClient::subversionLog(const QString &workingDirectory)
{
    QStringList arguments;
    arguments << QLatin1String("svn") << QLatin1String("log");
    int logCount = settings()->intValue(GitSettings::logCountKey);
    if (logCount > 0)
         arguments << (QLatin1String("--limit=") + QString::number(logCount));

    // Create a command editor, no highlighting or interaction.
    const QString title = tr("Git SVN Log");
    const Core::Id editorId = Git::Constants::C_GIT_COMMAND_LOG_EDITOR;
    const QString sourceFile = VcsBase::VcsBaseEditorWidget::getSource(workingDirectory, QStringList());
    VcsBase::VcsBaseEditorWidget *editor = findExistingVCSEditor("svnLog", sourceFile);
    if (!editor)
        editor = createVcsEditor(editorId, title, sourceFile, CodecNone, "svnLog", sourceFile, 0);
    editor->setWorkingDirectory(workingDirectory);
    executeGit(workingDirectory, arguments, editor);
}

void GitClient::push(const QString &workingDirectory, const QStringList &pushArgs)
{
    QStringList arguments(QLatin1String("push"));
    if (!pushArgs.isEmpty())
        arguments += pushArgs;
    executeGit(workingDirectory, arguments, 0, true);
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
    if (QFileInfo(gitDir + QLatin1String("/rebase-apply")).exists()
            || QFileInfo(gitDir + QLatin1String("/rebase-merge")).exists()) {
        VcsBase::VcsBaseOutputWindow::instance()->appendError(
                    tr("Rebase, merge or am is in progress. Finish "
                       "or abort it and then try again."));
        return false;
    }
    return true;
}

void GitClient::rebase(const QString &workingDirectory, const QString &argument)
{
    asyncCommand(workingDirectory, QStringList() << QLatin1String("rebase") << argument, true);
}

void GitClient::cherryPick(const QString &workingDirectory, const QString &argument)
{
    asyncCommand(workingDirectory, QStringList() << QLatin1String("cherry-pick") << argument);
}

void GitClient::revert(const QString &workingDirectory, const QString &argument)
{
    asyncCommand(workingDirectory, QStringList() << QLatin1String("revert") << argument);
}

// Executes a command asynchronously. Work tree is expected to be clean.
// Stashing is handled prior to this call.
void GitClient::asyncCommand(const QString &workingDirectory, const QStringList &arguments,
                             bool hasProgress)
{
    // Git might request an editor, so this must be done asynchronously
    // and without timeout
    QString gitCommand = arguments.first();
    outputWindow()->appendCommand(workingDirectory,
                                  settings()->stringValue(GitSettings::binaryPathKey),
                                  arguments);
    VcsBase::Command *command = createCommand(workingDirectory, 0, true);
    new ConflictHandler(command, workingDirectory, gitCommand);
    if (hasProgress)
        command->setProgressParser(new ProgressParser);
    command->addJob(arguments, -1);
    command->execute();
    command->setCookie(workingDirectory);
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
    outputWindow()->appendCommand(workingDirectory, settings()->stringValue(GitSettings::binaryPathKey), arguments);
    if (fixup)
        m_disableEditor = true;
    asyncCommand(workingDirectory, arguments, true);
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
    VcsBase::Command *cmd = executeGit(workingDirectory, arguments, 0, true,
                                       VcsBasePlugin::ExpectRepoChanges);
    new ConflictHandler(cmd, workingDirectory);
}

void GitClient::stashPop(const QString &workingDirectory)
{
    stashPop(workingDirectory, QString());
}

bool GitClient::synchronousStashRestore(const QString &workingDirectory,
                                        const QString &stash,
                                        bool pop,
                                        const QString &branch /* = QString()*/)
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
                            QString *errorMessage /* = 0 */)
{
    QStringList arguments(QLatin1String("stash"));
    if (stash.isEmpty())
        arguments << QLatin1String("clear");
    else
        arguments << QLatin1String("drop") << stash;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (rc) {
        const QString output = commandOutputFromLocal8Bit(outputText);
        if (!output.isEmpty())
            outputWindow()->append(output);
    } else {
        msgCannotRun(arguments, workingDirectory, errorText, errorMessage);
    }
    return rc;
}

bool GitClient::synchronousStashList(const QString &workingDirectory,
                                     QList<Stash> *stashes,
                                     QString *errorMessage /* = 0 */)
{
    stashes->clear();
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("list") << QLatin1String(noColorOption);
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
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

QByteArray GitClient::readConfigBytes(const QString &workingDirectory, const QString &configVar) const
{
    QStringList arguments;
    arguments << QLatin1String("config") << configVar;

    QByteArray outputText;
    QByteArray errorText;
    if (!fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText,
                             VcsBasePlugin::SuppressCommandLogging))
        return QByteArray();
    if (Utils::HostOsInfo::isWindowsHost())
        outputText.replace("\r\n", "\n");
    return outputText;
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar) const
{
    // msysGit always uses UTF-8 for configuration:
    // https://github.com/msysgit/msysgit/wiki/Git-for-Windows-Unicode-Support#convert-config-files
    static QTextCodec *codec = Utils::HostOsInfo::isWindowsHost()
            ? QTextCodec::codecForName("UTF-8")
            : QTextCodec::codecForLocale();
    const QByteArray value = readConfigBytes(workingDirectory, configVar).trimmed();
    return Utils::SynchronousProcess::normalizeNewlines(codec->toUnicode(value));
}

bool GitClient::cloneRepository(const QString &directory,const QByteArray &url)
{
    QDir workingDirectory(directory);
    const unsigned flags = VcsBasePlugin::SshPasswordPrompt
            | VcsBasePlugin::ShowStdOutInLogWindow
            | VcsBasePlugin::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        if (!synchronousInit(workingDirectory.path()))
            return false;

        QStringList arguments(QLatin1String("remote"));
        arguments << QLatin1String("add") << QLatin1String("origin") << QLatin1String(url);
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0))
            return false;

        arguments.clear();
        arguments << QLatin1String("fetch");
        const Utils::SynchronousProcessResponse resp =
                synchronousGit(workingDirectory.path(), arguments, flags);
        if (resp.result != Utils::SynchronousProcessResponse::Finished)
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.remote")
                  <<  QLatin1String("origin");
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0))
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.merge")
                  << QLatin1String("refs/heads/master");
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0))
            return false;

        return true;
    } else {
        QStringList arguments(QLatin1String("clone"));
        arguments << QLatin1String(url) << workingDirectory.dirName();
        workingDirectory.cdUp();
        const Utils::SynchronousProcessResponse resp =
                synchronousGit(workingDirectory.path(), arguments, flags);
        // TODO: Turn this into a VcsBaseClient and use resetCachedVcsInfo(...)
        Core::VcsManager::resetVersionControlForDirectory(workingDirectory.absolutePath());
        return (resp.result == Utils::SynchronousProcessResponse::Finished);
    }
}

QString GitClient::vcsGetRepositoryURL(const QString &directory)
{
    QStringList arguments(QLatin1String("config"));
    QByteArray outputText;

    arguments << QLatin1String("remote.origin.url");

    if (fullySynchronousGit(directory, arguments, &outputText, 0,
                            VcsBasePlugin::SuppressCommandLogging)) {
        return commandOutputFromLocal8Bit(outputText);
    }
    return QString();
}

GitSettings *GitClient::settings() const
{
    return m_settings;
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::gitVersion(QString *errorMessage) const
{
    const QString newGitBinary = gitBinaryPath();
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
    if (gitBinaryPath().isEmpty())
        return 0;

    // run git --version
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(QString(), QStringList(QLatin1String("--version")),
                                        &outputText, &errorText,
                                        VcsBasePlugin::SuppressCommandLogging);
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
    m_client(GitPlugin::instance()->gitClient()),
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
        VcsBase::VcsBaseOutputWindow::instance()->appendError(errorMessage);
    return !stashingFailed();
}

void GitClient::StashInfo::stashPrompt(const QString &command, const QString &statusOutput,
                                       QString *errorMessage)
{
    QMessageBox msgBox(QMessageBox::Question, tr("Uncommitted Changes Found"),
                       tr("What would you like to do with local changes in:")
                       + QLatin1String("\n\n\"")
                       + QDir::toNativeSeparators(m_workingDir) + QLatin1Char('\"'),
                       QMessageBox::NoButton, Core::ICore::mainWindow());

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
        m_stashResult = m_client->executeSynchronousStash(m_workingDir,
                        creatorStashMessage(command), errorMessage) ? StashUnchanged : StashFailed;
    } else if (msgBox.clickedButton() == stashAndPopButton) {
        executeStash(command, errorMessage);
    }
}

void GitClient::StashInfo::executeStash(const QString &command, QString *errorMessage)
{
    m_message = creatorStashMessage(command);
    if (!m_client->executeSynchronousStash(m_workingDir, m_message, errorMessage))
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
