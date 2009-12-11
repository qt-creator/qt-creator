/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "gitclient.h"
#include "gitcommand.h"

#include "commitdata.h"
#include "gitconstants.h"
#include "gitplugin.h"
#include "gitsubmiteditor.h"
#include "gitversioncontrol.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/iversioncontrol.h>

#include <texteditor/itexteditor.h>
#include <utils/qtcassert.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>

#include <projectexplorer/environment.h>

#include <QtCore/QRegExp>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSignalMapper>

#include <QtGui/QMainWindow> // for msg box parent
#include <QtGui/QMessageBox>
#include <QtGui/QPushButton>

using namespace Git;
using namespace Git::Internal;

static const char *const kGitDirectoryC = ".git";
static const char *const kBranchIndicatorC = "# On branch";

static inline QString msgServerFailure()
{
    return GitClient::tr(
"Note that the git plugin for QtCreator is not able to interact with the server "
"so far. Thus, manual ssh-identification etc. will not work.");
}

inline Core::IEditor* locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->file()->property(property).toString() == entry)
            return ed;
    return 0;
}

static inline QString msgRepositoryNotFound(const QString &dir)
{
    return GitClient::tr("Unable to determine the repository for %1.").arg(dir);
}

static inline QString msgParseFilesFailed()
{
    return  GitClient::tr("Unable to parse the file output.");
}

// Format a command for the status window
static QString formatCommand(const QString &binary, const QStringList &args)
{
    //: Executing: <executable> <arguments>
    return GitClient::tr("Executing: %1 %2\n").arg(binary, args.join(QString(QLatin1Char(' '))));
}

// ---------------- GitClient
GitClient::GitClient(GitPlugin* plugin)
  : m_msgWait(tr("Waiting for data...")),
    m_plugin(plugin),
    m_core(Core::ICore::instance()),
    m_repositoryChangedSignalMapper(0)
{
    if (QSettings *s = m_core->settings()) {
        m_settings.fromSettings(s);
        m_binaryPath = m_settings.gitBinaryPath();
    }
}

GitClient::~GitClient()
{
}

const char *GitClient::noColorOption = "--no-color";

QString GitClient::findRepositoryForFile(const QString &fileName)
{
    const QString gitDirectory = QLatin1String(kGitDirectoryC);
    const QFileInfo info(fileName);
    QDir dir = info.absoluteDir();
    do {
        if (dir.entryList(QDir::AllDirs|QDir::Hidden).contains(gitDirectory))
            return dir.absolutePath();
    } while (dir.cdUp());

    return QString();
}

QString GitClient::findRepositoryForDirectory(const QString &dir)
{
    const QString gitDirectory = QLatin1String(kGitDirectoryC);
    QDir directory(dir);
    do {
        if (directory.entryList(QDir::AllDirs|QDir::Hidden).contains(gitDirectory))
            return directory.absolutePath();
    } while (directory.cdUp());

    return QString();
}

/* Create an editor associated to VCS output of a source file/directory
 * (using the file's codec). Makes use of a dynamic property to find an
 * existing instance and to reuse it (in case, say, 'git diff foo' is
 * already open). */
VCSBase::VCSBaseEditor
    *GitClient::createVCSEditor(const QString &kind,
                                QString title,
                                // Source file or directory
                                const QString &source,
                                bool setSourceCodec,
                                // Dynamic property and value to identify that editor
                                const char *registerDynamicProperty,
                                const QString &dynamicPropertyValue) const
{
    VCSBase::VCSBaseEditor *rc = 0;
    Core::IEditor* outputEditor = locateEditor(m_core, registerDynamicProperty, dynamicPropertyValue);
    if (outputEditor) {
         // Exists already
        outputEditor->createNew(m_msgWait);
        rc = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(rc, return 0);
    } else {
        // Create new, set wait message, set up with source and codec
        outputEditor = m_core->editorManager()->openEditorWithContents(kind, &title, m_msgWait);
        outputEditor->file()->setProperty(registerDynamicProperty, dynamicPropertyValue);
        rc = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);
        QTC_ASSERT(rc, return 0);
        rc->setSource(source);
        if (setSourceCodec)
            rc->setCodec(VCSBase::VCSBaseEditor::getCodec(source));
    }
    m_core->editorManager()->activateEditor(outputEditor);
    return rc;
}

void GitClient::diff(const QString &workingDirectory,
                     const QStringList &diffArgs,
                     const QStringList &unstagedFileNames,
                     const QStringList &stagedFileNames)
{

    if (Git::Constants::debug)
        qDebug() << "diff" << workingDirectory << unstagedFileNames << stagedFileNames;

    const QString binary = QLatin1String(Constants::GIT_BINARY);
    const QString kind = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_KIND);
    const QString title = tr("Git Diff");

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, workingDirectory, true, "originalFileName", workingDirectory);
    editor->setDiffBaseDirectory(workingDirectory);

    // Create a batch of 2 commands to be run after each other in case
    // we have a mixture of staged/unstaged files as is the case
    // when using the submit dialog.
    GitCommand *command = createCommand(workingDirectory, editor);
    // Directory diff?
    QStringList commonDiffArgs;
    commonDiffArgs << QLatin1String("diff") << QLatin1String(noColorOption);
    if (unstagedFileNames.empty() && stagedFileNames.empty()) {
       QStringList arguments(commonDiffArgs);
       arguments << diffArgs;
       VCSBase::VCSBaseOutputWindow::instance()->appendCommand(formatCommand(binary, arguments));
       command->addJob(arguments, m_settings.timeout);
    } else {
        // Files diff.
        if (!unstagedFileNames.empty()) {
           QStringList arguments(commonDiffArgs);
           arguments << QLatin1String("--") << unstagedFileNames;
           VCSBase::VCSBaseOutputWindow::instance()->appendCommand(formatCommand(binary, arguments));
           command->addJob(arguments, m_settings.timeout);
        }
        if (!stagedFileNames.empty()) {
           QStringList arguments(commonDiffArgs);
           arguments << QLatin1String("--cached") << diffArgs << QLatin1String("--") << stagedFileNames;
           VCSBase::VCSBaseOutputWindow::instance()->appendCommand(formatCommand(binary, arguments));
           command->addJob(arguments, m_settings.timeout);
        }
    }
    command->execute();
}

void GitClient::diff(const QString &workingDirectory,
                     const QStringList &diffArgs,
                     const QString &fileName)
{
    if (Git::Constants::debug)
        qDebug() << "diff" << workingDirectory << fileName;
    QStringList arguments;
    arguments << QLatin1String("diff") << QLatin1String(noColorOption);
    if (!fileName.isEmpty())
        arguments << diffArgs  << QLatin1String("--") << fileName;

    const QString kind = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_KIND);
    const QString title = tr("Git Diff %1").arg(fileName);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, fileName);
    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, sourceFile, true, "originalFileName", sourceFile);
    executeGit(workingDirectory, arguments, editor);
}

void GitClient::status(const QString &workingDirectory)
{
    // @TODO: Use "--no-color" once it is supported
    QStringList statusArgs(QLatin1String("status"));
    statusArgs << QLatin1String("-u");
    executeGit(workingDirectory, statusArgs, 0, true);
}

void GitClient::log(const QString &workingDirectory, const QStringList &fileNames)
{
    if (Git::Constants::debug)
        qDebug() << "log" << workingDirectory << fileNames;

    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption);

    if (m_settings.logCount > 0)
         arguments << QLatin1String("-n") << QString::number(m_settings.logCount);

    if (!fileNames.isEmpty())
        arguments.append(fileNames);

    const QString msgArg = fileNames.empty() ? workingDirectory :
                           fileNames.join(QString(", "));
    const QString title = tr("Git Log %1").arg(msgArg);
    const QString kind = QLatin1String(Git::Constants::GIT_LOG_EDITOR_KIND);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, fileNames);
    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, sourceFile, false, "logFileName", sourceFile);
    executeGit(workingDirectory, arguments, editor);
}

void GitClient::show(const QString &source, const QString &id)
{
    if (Git::Constants::debug)
        qDebug() << "show" << source << id;
    QStringList arguments;
    arguments << QLatin1String("show") << QLatin1String(noColorOption) << id;

    const QString title =  tr("Git Show %1").arg(id);
    const QString kind = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_KIND);
    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, source, true, "show", id);

    const QFileInfo sourceFi(source);
    const QString workDir = sourceFi.isDir() ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    executeGit(workDir, arguments, editor);
}

void GitClient::blame(const QString &workingDirectory, const QString &fileName, int lineNumber /* = -1 */)
{
    if (Git::Constants::debug)
        qDebug() << "blame" << workingDirectory << fileName << lineNumber;
    QStringList arguments(QLatin1String("blame"));
    arguments << QLatin1String("--") << fileName;

    const QString kind = QLatin1String(Git::Constants::GIT_BLAME_EDITOR_KIND);
    const QString title = tr("Git Blame %1").arg(fileName);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, fileName);

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, sourceFile, true, "blameFileName", sourceFile);
    executeGit(workingDirectory, arguments, editor, false, GitCommand::NoReport, lineNumber);
}

void GitClient::checkoutBranch(const QString &workingDirectory, const QString &branch)
{
    QStringList arguments(QLatin1String("checkout"));
    arguments <<  branch;
    GitCommand *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

void GitClient::checkout(const QString &workingDirectory, const QString &fileName)
{
    // Passing an empty argument as the file name is very dangereous, since this makes
    // git checkout apply to all files. Almost looks like a bug in git.
    if (fileName.isEmpty())
        return;

    QStringList arguments;
    arguments << QLatin1String("checkout") << QLatin1String("HEAD") << QLatin1String("--")
            << fileName;

    executeGit(workingDirectory, arguments, 0, true);
}

void GitClient::hardReset(const QString &workingDirectory, const QString &commit)
{
    QStringList arguments;
    arguments << QLatin1String("reset") << QLatin1String("--hard");
    if (!commit.isEmpty())
        arguments << commit;

    GitCommand *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

void GitClient::addFile(const QString &workingDirectory, const QString &fileName)
{
    QStringList arguments;
    arguments << QLatin1String("add") << fileName;

    executeGit(workingDirectory, arguments, 0, true);
}

bool GitClient::synchronousAdd(const QString &workingDirectory, const QStringList &files)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << files;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("add") << files;
    const bool rc = synchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Unable to add %n file(s) to %1: %2", 0, files.size()).
                                     arg(workingDirectory, QString::fromLocal8Bit(errorText));
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
    }
    return rc;
}

bool GitClient::synchronousReset(const QString &workingDirectory,
                                 const QStringList &files)
{
    QString errorMessage;
    const bool rc = synchronousReset(workingDirectory, files, &errorMessage);
    if (!rc)
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
    return rc;
}

bool GitClient::synchronousReset(const QString &workingDirectory,
                                 const QStringList &files,
                                 QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << files;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("reset") << QLatin1String("HEAD") << QLatin1String("--") << files;
    const bool rc = synchronousGit(workingDirectory, arguments, &outputText, &errorText);
    const QString output = QString::fromLocal8Bit(outputText);
    VCSBase::VCSBaseOutputWindow::instance()->append(output);
    // Note that git exits with 1 even if the operation is successful
    // Assume real failure if the output does not contain "foo.cpp modified"
    if (!rc && !output.contains(QLatin1String("modified"))) {
        *errorMessage = tr("Unable to reset %n file(s) in %1: %2", 0, files.size()).arg(workingDirectory, QString::fromLocal8Bit(errorText));
        return false;
    }
    return true;
}

bool GitClient::synchronousCheckout(const QString &workingDirectory,
                                    const QStringList &files,
                                    QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << files;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("checkout") << QLatin1String("--") << files;
    const bool rc = synchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to checkout %n file(s) in %1: %2", 0, files.size()).arg(workingDirectory, QString::fromLocal8Bit(errorText));
        return false;
    }
    return true;
}

bool GitClient::synchronousStash(const QString &workingDirectory, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("stash");
    const bool rc = synchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable stash in %1: %2").arg(workingDirectory, QString::fromLocal8Bit(errorText));
        return false;
    }
    return true;
}

bool GitClient::synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                                     QString *output, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << branchArgs;
    branchArgs.push_front(QLatin1String("branch"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = synchronousGit(workingDirectory, branchArgs, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to run branch command: %1: %2").arg(workingDirectory, QString::fromLocal8Bit(errorText));
        return false;
    }
    *output = QString::fromLocal8Bit(outputText).remove(QLatin1Char('\r'));
    return true;
}

bool GitClient::synchronousShow(const QString &workingDirectory, const QString &id,
                                 QString *output, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << id;
    QStringList args(QLatin1String("show"));
    args << QLatin1String(noColorOption) << id;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = synchronousGit(workingDirectory, args, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to run show: %1: %2").arg(workingDirectory, QString::fromLocal8Bit(errorText));
        return false;
    }
    *output = QString::fromLocal8Bit(outputText).remove(QLatin1Char('\r'));
    return true;
}

// Factory function to create an asynchronous command
GitCommand *GitClient::createCommand(const QString &workingDirectory,
                             VCSBase::VCSBaseEditor* editor,
                             bool outputToWindow,
                             int editorLineNumber)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << editor;

    VCSBase::VCSBaseOutputWindow *outputWindow = VCSBase::VCSBaseOutputWindow::instance();
    GitCommand* command = new GitCommand(binary(), workingDirectory, processEnvironment(), QVariant(editorLineNumber));
    if (editor)
        connect(command, SIGNAL(finished(bool,QVariant)), editor, SLOT(commandFinishedGotoLine(bool,QVariant)));
    if (outputToWindow) {
        if (editor) { // assume that the commands output is the important thing
            connect(command, SIGNAL(outputData(QByteArray)), outputWindow, SLOT(appendDataSilently(QByteArray)));
        } else {
            connect(command, SIGNAL(outputData(QByteArray)), outputWindow, SLOT(appendData(QByteArray)));
        }
    } else {
        QTC_ASSERT(editor, /**/);
        connect(command, SIGNAL(outputData(QByteArray)), editor, SLOT(setPlainTextDataFiltered(QByteArray)));
    }

    if (outputWindow)
        connect(command, SIGNAL(errorText(QString)), outputWindow, SLOT(appendError(QString)));
    return command;
}

// Execute a single command
GitCommand *GitClient::executeGit(const QString &workingDirectory,
                                  const QStringList &arguments,
                                  VCSBase::VCSBaseEditor* editor,
                                  bool outputToWindow,
                                  GitCommand::TerminationReportMode tm,
                                  int editorLineNumber)
{
    VCSBase::VCSBaseOutputWindow::instance()->appendCommand(formatCommand(QLatin1String(Constants::GIT_BINARY), arguments));
    GitCommand *command = createCommand(workingDirectory, editor, outputToWindow, editorLineNumber);
    command->addJob(arguments, m_settings.timeout);
    command->setTerminationReportMode(tm);
    command->execute();
    return command;
}

// Return fixed arguments required to run
QStringList GitClient::binary() const
{
#ifdef Q_OS_WIN
        QStringList args;
        args << QLatin1String("cmd.exe") << QLatin1String("/c") << m_binaryPath;
        return args;
#else
        return QStringList(m_binaryPath);
#endif
}

QStringList GitClient::processEnvironment() const
{
    ProjectExplorer::Environment environment = ProjectExplorer::Environment::systemEnvironment();
    if (m_settings.adoptPath)
        environment.set(QLatin1String("PATH"), m_settings.path);
    return environment.toStringList();
}

bool GitClient::synchronousGit(const QString &workingDirectory,
                               const QStringList &gitArguments,
                               QByteArray* outputText,
                               QByteArray* errorText,
                               bool logCommandToWindow)
{
    if (Git::Constants::debug)
        qDebug() << "synchronousGit" << workingDirectory << gitArguments;

    if (logCommandToWindow)
        VCSBase::VCSBaseOutputWindow::instance()->appendCommand(formatCommand(m_binaryPath, gitArguments));

    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.setEnvironment(processEnvironment());

    QStringList args = binary();
    const QString executable = args.front();
    args.pop_front();
    args.append(gitArguments);
    process.start(executable, args);
    process.closeWriteChannel();

    if (!process.waitForFinished()) {
        if (errorText)
            *errorText = "Error: Git timed out";
        process.kill();
        return false;
    }

    if (outputText)
        *outputText = process.readAllStandardOutput();

    if (errorText)
        *errorText = process.readAllStandardError();

    if (Git::Constants::debug)
        qDebug() << "synchronousGit ex=" << process.exitCode();
    return process.exitCode() == 0;
}

static inline int
        askWithDetailedText(QWidget *parent,
                            const QString &title, const QString &msg,
                            const QString &inf,
                            QMessageBox::StandardButton defaultButton,
                            QMessageBox::StandardButtons buttons = QMessageBox::Yes|QMessageBox::No)
{
    QMessageBox msgBox(QMessageBox::Question, title, msg, buttons, parent);
    msgBox.setDetailedText(inf);
    msgBox.setDefaultButton(defaultButton);
    return msgBox.exec();
}

// Convenience that pops up an msg box.
GitClient::StashResult GitClient::ensureStash(const QString &workingDirectory)
{
    QString errorMessage;
    const StashResult sr = ensureStash(workingDirectory, &errorMessage);
    if (sr == StashFailed)
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
    return sr;
}

// Ensure that changed files are stashed before a pull or similar
GitClient::StashResult GitClient::ensureStash(const QString &workingDirectory, QString *errorMessage)
{
    QString statusOutput;
    switch (gitStatus(workingDirectory, false, &statusOutput, errorMessage)) {
        case StatusChanged:
        break;
        case StatusUnchanged:
        return StashUnchanged;
        case StatusFailed:
        return StashFailed;
    }

    const int answer = askWithDetailedText(m_core->mainWindow(), tr("Changes"),
                             tr("You have modified files. Would you like to stash your changes?"),
                             statusOutput, QMessageBox::Yes, QMessageBox::Yes|QMessageBox::No|QMessageBox::Cancel);
    switch (answer) {
        case QMessageBox::Cancel:
            return StashCanceled;
        case QMessageBox::Yes:
            if (!synchronousStash(workingDirectory, errorMessage))
                return StashFailed;
            break;
        case QMessageBox::No: // At your own risk, so.
            return NotStashed;
        }

    return Stashed;
 }

// Trim a git status file spec: "modified:    foo .cpp" -> "modified: foo .cpp"
static inline QString trimFileSpecification(QString fileSpec)
{
    const int colonIndex = fileSpec.indexOf(QLatin1Char(':'));
    if (colonIndex != -1) {
        // Collapse the sequence of spaces
        const int filePos = colonIndex + 2;
        int nonBlankPos = filePos;
        for ( ; fileSpec.at(nonBlankPos).isSpace(); nonBlankPos++) ;
        if (nonBlankPos > filePos)
            fileSpec.remove(filePos, nonBlankPos - filePos);
    }
    return fileSpec;
}

GitClient::StatusResult GitClient::gitStatus(const QString &workingDirectory,
                                             bool untracked,
                                             QString *output,
                                             QString *errorMessage)
{
    // Run 'status'. Note that git returns exitcode 1 if there are no added files.
    QByteArray outputText;
    QByteArray errorText;
    // @TODO: Use "--no-color" once it is supported
    QStringList statusArgs(QLatin1String("status"));
    if (untracked)
        statusArgs << QLatin1String("-u");
    const bool statusRc = synchronousGit(workingDirectory, statusArgs, &outputText, &errorText);
    GitCommand::removeColorCodes(&outputText);
    if (output)
        *output = QString::fromLocal8Bit(outputText).remove(QLatin1Char('\r'));
    // Is it something really fatal?
    if (!statusRc && !outputText.contains(kBranchIndicatorC)) {
        if (errorMessage) {
            const QString error = QString::fromLocal8Bit(errorText).remove(QLatin1Char('\r'));
            *errorMessage = tr("Unable to obtain the status: %1").arg(error);
        }
        return StatusFailed;
    }
    // Unchanged?
    if (outputText.contains("nothing to commit"))
        return StatusUnchanged;
    return StatusChanged;
}

bool GitClient::getCommitData(const QString &workingDirectory,
                              QString *commitTemplate,
                              CommitData *d,
                              QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;

    d->clear();

    // Find repo
    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return false;
    }

    d->panelInfo.repository = repoDirectory;

    QDir gitDir(repoDirectory);
    if (!gitDir.cd(QLatin1String(kGitDirectoryC))) {
        *errorMessage = tr("The repository %1 is not initialized yet.").arg(repoDirectory);
        return false;
    }

    // Read description
    const QString descriptionFile = gitDir.absoluteFilePath(QLatin1String("description"));
    if (QFileInfo(descriptionFile).isFile()) {
        QFile file(descriptionFile);
        if (file.open(QIODevice::ReadOnly|QIODevice::Text))
            d->panelInfo.description = QString::fromLocal8Bit(file.readAll()).trimmed();
    }

    // Run status. Note that it has exitcode 1 if there are no added files.
    QString output;
    switch (gitStatus(repoDirectory, true, &output, errorMessage)) {
    case  StatusChanged:
        break;
    case StatusUnchanged:
        *errorMessage = msgNoChangedFiles();
        return false;
    case StatusFailed:
        return false;
    }

    //    Output looks like:
    //    # On branch [branchname]
    //    # Changes to be committed:
    //    #   (use "git reset HEAD <file>..." to unstage)
    //    #
    //    #       modified:   somefile.cpp
    //    #       new File:   somenew.h
    //    #
    //    # Changed but not updated:
    //    #   (use "git add <file>..." to update what will be committed)
    //    #
    //    #       modified:   someother.cpp
    //    #
    //    # Untracked files:
    //    #   (use "git add <file>..." to include in what will be committed)
    //    #
    //    #       list of files...

    if (!d->parseFilesFromStatus(output)) {
        *errorMessage = msgParseFilesFailed();
        return false;
    }
    // Filter out untracked files that are not part of the project
    VCSBase::VCSBaseSubmitEditor::filterUntrackedFilesOfProject(repoDirectory, &d->untrackedFiles);
    if (d->filesEmpty()) {
        *errorMessage = msgNoChangedFiles();
        return false;
    }

    d->panelData.author = readConfigValue(workingDirectory, QLatin1String("user.name"));
    d->panelData.email = readConfigValue(workingDirectory, QLatin1String("user.email"));

    // Get the commit template
    const QString templateFilename = readConfigValue(workingDirectory, QLatin1String("commit.template"));
    if (!templateFilename.isEmpty()) {
        QFile templateFile(templateFilename);
        if (templateFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
            *commitTemplate = QString::fromLocal8Bit(templateFile.readAll());
        } else {
            qWarning("Unable to read commit template %s: %s",
                     qPrintable(templateFilename),
                     qPrintable(templateFile.errorString()));
        }
    }
    return true;
}

// addAndCommit:
bool GitClient::addAndCommit(const QString &repositoryDirectory,
                             const GitSubmitEditorPanelData &data,
                             const QString &messageFile,
                             const QStringList &checkedFiles,
                             const QStringList &origCommitFiles,
                             const QStringList &origDeletedFiles)
{
    if (Git::Constants::debug)
        qDebug() << "GitClient::addAndCommit:" << repositoryDirectory << checkedFiles << origCommitFiles;

    // Do we need to reset any files that had been added before
    // (did the user uncheck any previously added files)
    const QSet<QString> resetFiles = origCommitFiles.toSet().subtract(checkedFiles.toSet());
    if (!resetFiles.empty())
        if (!synchronousReset(repositoryDirectory, resetFiles.toList()))
            return false;

    // Re-add all to make sure we have the latest changes, but only add those that aren't marked
    // for deletion
    QStringList addFiles = checkedFiles.toSet().subtract(origDeletedFiles.toSet()).toList();
    if (!addFiles.isEmpty())
        if (!synchronousAdd(repositoryDirectory, addFiles))
            return false;

    // Do the final commit
    QStringList args;
    args << QLatin1String("commit")
         << QLatin1String("-F") << QDir::toNativeSeparators(messageFile);

    const QString &authorString =  data.authorString();
    if (!authorString.isEmpty())
         args << QLatin1String("--author") << authorString;

    QByteArray outputText;
    QByteArray errorText;
    const bool rc = synchronousGit(repositoryDirectory, args, &outputText, &errorText);
    if (rc) {
        VCSBase::VCSBaseOutputWindow::instance()->append(tr("Committed %n file(s).\n", 0, checkedFiles.size()));
    } else {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(tr("Unable to commit %n file(s): %1\n", 0, checkedFiles.size()).arg(QString::fromLocal8Bit(errorText)));
    }
    return rc;
}

/* Revert: This function can be called with a file list (to revert single
 * files)  or a single directory (revert all). Qt Creator currently has only
 * 'revert single' in its VCS menus, but the code is prepared to deal with
 * reverting a directory pending a sophisticated selection dialog in the
 * VCSBase plugin. */

GitClient::RevertResult GitClient::revertI(QStringList files, bool *ptrToIsDirectory, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << files;

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
    switch (gitStatus(repoDirectory, false, &output, errorMessage)) {
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
    const QString modifiedState = QLatin1String("modified");
    const QStringList allStagedFiles = data.stagedFileNames(modifiedState);
    const QStringList allUnstagedFiles = data.unstagedFileNames(modifiedState);
    // Unless a directory was passed, filter all modified files for the
    // argument file list.
    QStringList stagedFiles = allStagedFiles;
    QStringList unstagedFiles = allUnstagedFiles;
    if (!isDirectory) {
        const QSet<QString> filesSet = files.toSet();
        stagedFiles = allStagedFiles.toSet().intersect(filesSet).toList();
        unstagedFiles = allUnstagedFiles.toSet().intersect(filesSet).toList();
    }
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << data.stagedFiles << data.unstagedFiles << allStagedFiles << allUnstagedFiles << stagedFiles << unstagedFiles;

    if (stagedFiles.empty() && unstagedFiles.empty())
        return RevertUnchanged;

    // Ask to revert (to do: Handle lists with a selection dialog)
    const QMessageBox::StandardButton answer
        = QMessageBox::question(m_core->mainWindow(),
                                tr("Revert"),
                                tr("The file has been changed. Do you want to revert it?"),
                                QMessageBox::Yes|QMessageBox::No,
                                QMessageBox::No);
    if (answer == QMessageBox::No)
        return RevertCanceled;

    // Unstage the staged files
    if (!stagedFiles.empty() && !synchronousReset(repoDirectory, stagedFiles, errorMessage))
        return RevertFailed;
    // Finally revert!
    if (!synchronousCheckout(repoDirectory, stagedFiles + unstagedFiles, errorMessage))
        return RevertFailed;
    return RevertOk;
}

void GitClient::revert(const QStringList &files)
{
    bool isDirectory;
    QString errorMessage;
    switch (revertI(files, &isDirectory, &errorMessage)) {
    case RevertOk:
        m_plugin->gitVersionControl()->emitFilesChanged(files);
        break;
    case RevertCanceled:
        break;
    case RevertUnchanged: {
        const QString msg = (isDirectory || files.size() > 1) ? msgNoChangedFiles() : tr("The file is not modified.");
        VCSBase::VCSBaseOutputWindow::instance()->append(msg);
    }
        break;
    case RevertFailed:
        VCSBase::VCSBaseOutputWindow::instance()->append(errorMessage);
        break;
    }
}

void GitClient::pull(const QString &workingDirectory)
{
    GitCommand *cmd = executeGit(workingDirectory, QStringList(QLatin1String("pull")), 0, true, GitCommand::ReportStderr);
    connectRepositoryChanged(workingDirectory, cmd);
}

void GitClient::push(const QString &workingDirectory)
{
    executeGit(workingDirectory, QStringList(QLatin1String("push")), 0, true, GitCommand::ReportStderr);
}

QString GitClient::msgNoChangedFiles()
{
    return tr("There are no modified files.");
}

void GitClient::stash(const QString &workingDirectory)
{
    // Check for changes and stash
    QString errorMessage;
    switch (gitStatus(workingDirectory, false, 0, &errorMessage)) {
    case  StatusChanged:
        executeGit(workingDirectory, QStringList(QLatin1String("stash")), 0, true);
        break;
    case StatusUnchanged:
        VCSBase::VCSBaseOutputWindow::instance()->append(msgNoChangedFiles());
        break;
    case StatusFailed:
        VCSBase::VCSBaseOutputWindow::instance()->append(errorMessage);
        break;
    }
}

void GitClient::stashPop(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("pop");
    GitCommand *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

void GitClient::branchList(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("branch"));
    arguments << QLatin1String("-r") << QLatin1String(noColorOption);
    executeGit(workingDirectory, arguments, 0, true);
}

void GitClient::stashList(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("list") << QLatin1String(noColorOption);
    executeGit(workingDirectory, arguments, 0, true);
}

QString GitClient::readConfig(const QString &workingDirectory, const QStringList &configVar)
{
    QStringList arguments;
    arguments << QLatin1String("config") << configVar;

    QByteArray outputText;
    if (synchronousGit(workingDirectory, arguments, &outputText, 0, false))
        return QString::fromLocal8Bit(outputText).remove(QLatin1Char('\r'));
    return QString();
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar)
{
    return readConfig(workingDirectory, QStringList(configVar)).remove(QLatin1Char('\n'));
}

GitSettings GitClient::settings() const
{
    return m_settings;
}

void GitClient::setSettings(const GitSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *s = m_core->settings())
            m_settings.toSettings(s);
        m_binaryPath = m_settings.gitBinaryPath();
    }
}

void GitClient::connectRepositoryChanged(const QString & repository, GitCommand *cmd)
{
    // Bind command success termination with repository to changed signal
    if (!m_repositoryChangedSignalMapper) {
        m_repositoryChangedSignalMapper = new QSignalMapper(this);
        connect(m_repositoryChangedSignalMapper, SIGNAL(mapped(QString)),
                m_plugin->gitVersionControl(), SIGNAL(repositoryChanged(QString)));
    }
    m_repositoryChangedSignalMapper->setMapping(cmd, repository);
    connect(cmd, SIGNAL(success()), m_repositoryChangedSignalMapper, SLOT(map()),
            Qt::QueuedConnection);
}
