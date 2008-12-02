/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** 
** Non-Open Source Usage  
** 
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.  
** 
** GNU General Public License Usage 
** 
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#include "gitclient.h"
#include "gitplugin.h"
#include "gitconstants.h"
#include "commitdata.h"

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/actionmanager/actionmanagerinterface.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/progressmanager/progressmanagerinterface.h>
#include <vcsbase/vcsbaseeditor.h>
#include <texteditor/itexteditor.h>

#include <QtCore/QRegExp>
#include <QtCore/QTemporaryFile>
#include <QtCore/QFuture>

#include <QtGui/QErrorMessage>

using namespace Git;
using namespace Git::Internal;

const char* const kGitCommand = "git";
const char* const kGitDirectoryC = ".git";
const char* const kBranchIndicatorC = "# On branch";

static inline QString msgServerFailure()
{
    return GitClient::tr(
"Note that the git plugin for QtCreator is not able to interact with the server "
"so far. Thus, manual ssh-identification etc. will not work.");
}

inline Core::IEditor* locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->property(property).toString() == entry)
            return ed;
    return 0;
}

GitClient::GitClient(GitPlugin* plugin, Core::ICore *core) :
    m_msgWait(tr("Waiting for data...")),
    m_plugin(plugin),
    m_core(core)
{
}

GitClient::~GitClient()
{
}

bool GitClient::vcsOpen(const QString &fileName)
{
    return m_plugin->vcsOpen(fileName);
}

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

// Return source file or directory string depending on parameters
// ('git diff XX' -> 'XX' , 'git diff XX file' -> 'XX/file').
static QString source(const QString &workingDirectory, const QString &fileName)
{
    if (fileName.isEmpty())
        return workingDirectory;
    QString rc = workingDirectory;
    if (!rc.isEmpty() && !rc.endsWith(QDir::separator()))
        rc += QDir::separator();
    rc += fileName;
    return rc;
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
        Q_ASSERT(rc);
        m_core->editorManager()->setCurrentEditor(outputEditor);
    } else {
        // Create new, set wait message, set up with source and codec
        outputEditor = m_core->editorManager()->newFile(kind, &title, m_msgWait);
        outputEditor->setProperty(registerDynamicProperty, dynamicPropertyValue);
        rc = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);
        Q_ASSERT(rc);
        rc->setSource(source);
        if (setSourceCodec)
            rc->setCodec(VCSBase::VCSBaseEditor::getCodec(m_core, source));
    }
    return rc;
}

void GitClient::diff(const QString &workingDirectory, const QStringList &fileNames)
{
      if (Git::Constants::debug)
        qDebug() << "diff" << workingDirectory << fileNames;
    QStringList arguments;
    arguments << QLatin1String("diff") << fileNames;

    const QString kind = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_KIND);
    const QString title = tr("Git Diff");

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, workingDirectory, true, "originalFileName", workingDirectory);
    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, editor);

}

void GitClient::diff(const QString &workingDirectory, const QString &fileName)
{
    if (Git::Constants::debug)
        qDebug() << "diff" << workingDirectory << fileName;
    QStringList arguments;
    arguments << QLatin1String("diff");
    if (!fileName.isEmpty())
            arguments << fileName;

    const QString kind = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_KIND);
    const QString title = tr("Git Diff %1").arg(fileName);
    const QString sourceFile = source(workingDirectory, fileName);

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, sourceFile, true, "originalFileName", sourceFile);
    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, editor);
}

void GitClient::status(const QString &workingDirectory)
{
    executeGit(workingDirectory, QStringList(QLatin1String("status")), m_plugin->m_outputWindow, 0,true);
}

void GitClient::log(const QString &workingDirectory, const QString &fileName)
{
    if (Git::Constants::debug)
        qDebug() << "log" << workingDirectory << fileName;
    QStringList arguments;
    int logCount = 10;
    if (m_plugin->m_settingsPage && m_plugin->m_settingsPage->logCount() > 0)
        logCount = m_plugin->m_settingsPage->logCount();

    arguments << QLatin1String("log") << QLatin1String("-n")
        << QString::number(logCount);
    if (!fileName.isEmpty())
        arguments << fileName;

    const QString title = tr("Git Log %1").arg(fileName);
    const QString kind = QLatin1String(Git::Constants::GIT_LOG_EDITOR_KIND);
    const QString sourceFile = source(workingDirectory, fileName);
    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, sourceFile, false, "logFileName", sourceFile);
    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, editor);
}

void GitClient::show(const QString &source, const QString &id)
{
    if (Git::Constants::debug)
        qDebug() << "show" << source << id;
    QStringList arguments(QLatin1String("show"));
    arguments << id;

    const QString title =  tr("Git Show %1").arg(id);
    const QString kind = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_KIND);
    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, source, true, "show", id);

    const QFileInfo sourceFi(source);
    const QString workDir = sourceFi.isDir() ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    executeGit(workDir, arguments, m_plugin->m_outputWindow, editor);
}

void GitClient::blame(const QString &workingDirectory, const QString &fileName)
{
    if (Git::Constants::debug)
        qDebug() << "blame" << workingDirectory << fileName;
    QStringList arguments(QLatin1String("blame"));
    arguments << fileName;

    const QString kind = QLatin1String(Git::Constants::GIT_BLAME_EDITOR_KIND);
    const QString title = tr("Git Blame %1").arg(fileName);
    const QString sourceFile = source(workingDirectory, fileName);

    VCSBase::VCSBaseEditor *editor = createVCSEditor(kind, title, sourceFile, true, "blameFileName", sourceFile);
    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, editor);
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

    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, 0,true);
}

void GitClient::hardReset(const QString &workingDirectory, const QString &commit)
{
    QStringList arguments;
    arguments << QLatin1String("reset") << QLatin1String("--hard");
    if (!commit.isEmpty())
        arguments << commit;

    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, 0,true);
}

void GitClient::addFile(const QString &workingDirectory, const QString &fileName)
{
    QStringList arguments;
    arguments << QLatin1String("add") << fileName;

    executeGit(workingDirectory, arguments, m_plugin->m_outputWindow, 0,true);
}

bool GitClient::synchronousAdd(const QString &workingDirectory, const QStringList &files)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << "add" << files;
    const bool rc = synchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Unable to add %n file(s) to %1: %2", 0, files.size()).
                                     arg(workingDirectory, QString::fromLocal8Bit(errorText));
        m_plugin->m_outputWindow->append(errorMessage);
        m_plugin->m_outputWindow->popup(false);
    }
    return rc;
}

void GitClient::executeGit(const QString &workingDirectory, const QStringList &arguments,
                           GitOutputWindow *outputWindow, VCSBase::VCSBaseEditor* editor,
                           bool outputToWindow)
{
    if (Git::Constants::debug)
        qDebug() << "executeGit" << workingDirectory << arguments << editor;
    outputWindow->clearContents();

    QProcess process;
    ProjectExplorer::Environment environment = ProjectExplorer::Environment::systemEnvironment();

    if (m_plugin->m_settingsPage && !m_plugin->m_settingsPage->adoptEnvironment())
        environment.set(QLatin1String("PATH"), m_plugin->m_settingsPage->path());

    GitCommand* command = new GitCommand();
    if (outputToWindow) {
        Q_ASSERT(outputWindow);
        connect(command, SIGNAL(outputText(QString)), outputWindow, SLOT(append(QString)));
        connect(command, SIGNAL(outputData(QByteArray)), outputWindow, SLOT(appendData(QByteArray)));
    } else {
        Q_ASSERT(editor);
        connect(command, SIGNAL(outputText(QString)), editor, SLOT(setPlainText(QString)));
        connect(command, SIGNAL(outputData(QByteArray)), editor, SLOT(setPlainTextData(QByteArray)));
    }

    if (outputWindow)
        connect(command, SIGNAL(errorText(QString)), outputWindow, SLOT(append(QString)));

    command->execute(arguments, workingDirectory, environment);
}

bool GitClient::synchronousGit(const QString &workingDirectory
                                , const QStringList &arguments
                                , QByteArray* outputText
                                , QByteArray* errorText)
{
    if (Git::Constants::debug)
        qDebug() << "synchronousGit" << workingDirectory << arguments;
    QProcess process;

    process.setWorkingDirectory(workingDirectory);

    ProjectExplorer::Environment environment = ProjectExplorer::Environment::systemEnvironment();
    if (m_plugin->m_settingsPage && !m_plugin->m_settingsPage->adoptEnvironment())
        environment.set(QLatin1String("PATH"), m_plugin->m_settingsPage->path());
    process.setEnvironment(environment.toStringList());

    process.start(QLatin1String(kGitCommand), arguments);
    if (!process.waitForFinished()) {
        if (errorText)
            *errorText = "Error: Git timed out";
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

/* Parse a git status file list:
 * \code
    # Changes to be committed:
    #<tab>modified:<blanks>git.pro
    # Changed but not updated:
    #<tab>modified:<blanks>git.pro
    # Untracked files:
    #<tab>modified:<blanks>git.pro
    \endcode
*/
static bool parseFiles(const QStringList &lines, CommitData *d)
{
    enum State { None, CommitFiles, NotUpdatedFiles, UntrackedFiles };

    const QString branchIndicator = QLatin1String(kBranchIndicatorC);
    const QString commitIndicator = QLatin1String("# Changes to be committed:");
    const QString notUpdatedIndicator = QLatin1String("# Changed but not updated:");
    const QString untrackedIndicator = QLatin1String("# Untracked files:");

    State s = None;

    const QRegExp filesPattern(QLatin1String("#\\t[^:]+:\\s+[^ ]+"));
    Q_ASSERT(filesPattern.isValid());

    const QStringList::const_iterator cend = lines.constEnd();
    for (QStringList::const_iterator it =  lines.constBegin(); it != cend; ++it) {
        const QString line = *it;
        if (line.startsWith(branchIndicator)) {
            d->panelInfo.branch = line.mid(branchIndicator.size() + 1);
        } else {
            if (line.startsWith(commitIndicator)) {
                s = CommitFiles;
            } else {
                if (line.startsWith(notUpdatedIndicator)) {
                    s = NotUpdatedFiles;
                } else {
                    if (line.startsWith(untrackedIndicator)) {
                        s = UntrackedFiles;
                    } else {
                        if (filesPattern.exactMatch(line)) {
                            const QString fileSpec = line.mid(2).simplified();
                            switch (s) {
                            case CommitFiles:
                                d->commitFiles.push_back(fileSpec);
                            break;
                            case NotUpdatedFiles:
                                d->notUpdatedFiles.push_back(fileSpec);
                                break;
                            case UntrackedFiles:
                                d->untrackedFiles.push_back(fileSpec);
                                break;
                            case None:
                                break;
                            }
                        }
                    }
                }
            }
        }
    }
    return !d->commitFiles.empty() || !d->notUpdatedFiles.empty() || !d->untrackedFiles.empty();
}

bool GitClient::getCommitData(const QString &workingDirectory,
                              QString *commitTemplate,
                              CommitData *d,
                              QString *errorMessage)
{
    d->clear();

    // Find repo
    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = tr("Unable to determine the repository for %1.").arg(workingDirectory);
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
    QByteArray outputText;
    QByteArray errorText;
    const bool statusRc = synchronousGit(workingDirectory, QStringList(QLatin1String("status")), &outputText, &errorText);
    if (!statusRc) {
        // Something fatal
        if (!outputText.contains(kBranchIndicatorC)) {
            *errorMessage = tr("Unable to obtain the project status: %1").arg(QString::fromLocal8Bit(errorText));
            return false;
        }
        // All unchanged
        if (outputText.contains("nothing to commit")) {
            *errorMessage = tr("There are no modified files.");
            return false;
        }
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

    const QStringList lines = QString::fromLocal8Bit(outputText).remove(QLatin1Char('\r')).split(QLatin1Char('\n'));
    if (!parseFiles(lines, d)) {
        *errorMessage = tr("Unable to parse the file output.");
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

bool GitClient::addAndCommit(const QString &workingDirectory,
                             const GitSubmitEditorPanelData &data,
                             const QString &messageFile,
                             const QStringList &files)
{
    // Re-add all to make sure we have the latest changes
    if (!synchronousAdd(workingDirectory, files))
        return false;

    // Do the final commit
    QStringList args;
    args << QLatin1String("commit")
         << QLatin1String("-F") << QDir::toNativeSeparators(messageFile)
         << QLatin1String("--author") << data.authorString();

    QByteArray outputText;
    QByteArray errorText;
    const bool rc = synchronousGit(workingDirectory, args, &outputText, &errorText);
    const QString message = rc ?
        tr("Committed %n file(s).", 0, files.size()) :
        tr("Unable to commit %n file(s): %1", 0, files.size()).arg(QString::fromLocal8Bit(errorText));

    m_plugin->m_outputWindow->append(message);
    m_plugin->m_outputWindow->popup(false);
    return rc;
}

void GitClient::pull(const QString &workingDirectory)
{
    executeGit(workingDirectory, QStringList(QLatin1String("pull")), m_plugin->m_outputWindow, 0,true);
}

void GitClient::push(const QString &workingDirectory)
{
    executeGit(workingDirectory, QStringList(QLatin1String("push")), m_plugin->m_outputWindow, 0,true);
}

QString GitClient::readConfig(const QString &workingDirectory, const QStringList &configVar)
{
    QStringList arguments;
    arguments << QLatin1String("config") << configVar;

    QByteArray outputText;
    if (synchronousGit(workingDirectory, arguments, &outputText))
        return outputText;
    return QString();
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar)
{
    return readConfig(workingDirectory, QStringList(configVar)).remove(QLatin1Char('\n'));
}

GitCommand::GitCommand()
{
}

GitCommand::~GitCommand()
{
}

void GitCommand::execute(const QStringList &arguments
                 , const QString &workingDirectory
                 , const ProjectExplorer::Environment &environment)
{
    if (Git::Constants::debug)
        qDebug() << "GitCommand::execute" << workingDirectory << arguments;

    // For some reason QtConcurrent::run() only works on this
    QFuture<void> task = QtConcurrent::run(this, &GitCommand::run
                                           , arguments
                                           , workingDirectory
                                           , environment);
    QString taskName = QLatin1String("Git ") + arguments[0];

    Core::ICore *core = ExtensionSystem::PluginManager::instance()->getObject<Core::ICore>();
    core->progressManager()->addTask(task, taskName
                            , QLatin1String("Git.action")
                            , Core::ProgressManagerInterface::CloseOnSuccess);
}

void GitCommand::run(const QStringList &arguments
                 , const QString &workingDirectory
                 , const ProjectExplorer::Environment &environment)
{
    if (Git::Constants::debug)
        qDebug() << "GitCommand::run" << workingDirectory << arguments;
    QProcess process;
    if (!workingDirectory.isEmpty())
        process.setWorkingDirectory(workingDirectory);

    ProjectExplorer::Environment env = environment;
    if (env.toStringList().isEmpty())
        env = ProjectExplorer::Environment::systemEnvironment();
    process.setEnvironment(env.toStringList());

    process.start(QLatin1String(kGitCommand), arguments);
    if (!process.waitForFinished()) {
        emit errorText(QLatin1String("Error: Git timed out"));
        return;
    }

    const QByteArray output = process.readAllStandardOutput();
    if (output.isEmpty()) {
        if (arguments.at(0) == QLatin1String("diff"))
            emit outputText(tr("The file does not differ from HEAD"));
    } else {
        emit outputData(output);
    }
    const QByteArray error = process.readAllStandardError();
    if (!error.isEmpty())
        emit errorText(QString::fromLocal8Bit(error));

    // As it is used asynchronously, we need to delete ourselves
    this->deleteLater();
}
