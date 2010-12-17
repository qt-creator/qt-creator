/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "gitclient.h"
#include "gitcommand.h"
#include "gitutils.h"

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
#include <coreplugin/iversioncontrol.h>

#include <texteditor/itexteditor.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>
#include <utils/synchronousprocess.h>
#include <utils/environment.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QtCore/QRegExp>
#include <QtCore/QTemporaryFile>
#include <QtCore/QTime>
#include <QtCore/QFileInfo>
#include <QtCore/QDir>
#include <QtCore/QSignalMapper>

#include <QtGui/QComboBox>
#include <QtGui/QMainWindow> // for msg box parent
#include <QtGui/QMessageBox>
#include <QtGui/QToolButton>

static const char *const kGitDirectoryC = ".git";
static const char *const kBranchIndicatorC = "# On branch";

namespace Git {
namespace Internal {

BaseGitArgumentsWidget::BaseGitArgumentsWidget(GitSettings *settings,
                                               Git::Internal::GitClient *client,
                                               const QString &directory,
                                               const QStringList &args) :
    QWidget(0),
    m_client(client),
    m_workingDirectory(directory),
    m_diffArgs(args),
    m_settings(settings)
{
    Q_ASSERT(settings);
    Q_ASSERT(client);
}

} // namespace Internal
} // namespace Git

namespace {

class BaseGitDiffArgumentsWidget : public Git::Internal::BaseGitArgumentsWidget
{
public:
    BaseGitDiffArgumentsWidget(Git::Internal::GitSettings *settings,
                               Git::Internal::GitClient *client,
                               const QString &directory,
                               const QStringList &args) :
        BaseGitArgumentsWidget(settings, client, directory, args),
        m_patience(0),
        m_ignoreSpaces(0)
    {
        QHBoxLayout * layout = new QHBoxLayout(this);
        layout->setContentsMargins(3, 0, 3, 0);

        m_patience = new QToolButton;
        m_patience->setToolTip(tr("Use the patience algorithmn for calculating the diff"));
        m_patience->setText(tr("Patience"));
        layout->addWidget(m_patience);
        m_patience->setCheckable(true);
        m_patience->setChecked(m_settings->diffPatience);
        connect(m_patience, SIGNAL(toggled(bool)), this, SLOT(testForArgumentsChanged()));

        m_ignoreSpaces = new QToolButton;
        m_ignoreSpaces->setToolTip(tr("Ignore whitespaces only changes"));
        m_ignoreSpaces->setText(tr("Ignore Whitespace"));
        layout->addWidget(m_ignoreSpaces);
        m_ignoreSpaces->setCheckable(true);
        m_ignoreSpaces->setChecked(m_settings->ignoreSpaceChangesInDiff);
        connect(m_ignoreSpaces, SIGNAL(toggled(bool)), this, SLOT(testForArgumentsChanged()));

        m_prettyFormat = new QComboBox;
        m_prettyFormat->setToolTip(tr("Select the pretty printing format"));
        m_prettyFormat->addItem(tr("oneline"), QLatin1String("oneline"));
        m_prettyFormat->addItem(tr("short"), QLatin1String("short"));
        m_prettyFormat->addItem(tr("medium"), QLatin1String("medium"));
        m_prettyFormat->addItem(tr("full"), QLatin1String("full"));
        m_prettyFormat->addItem(tr("fuller"), QLatin1String("fuller"));
        m_prettyFormat->addItem(tr("email"), QLatin1String("email"));
        m_prettyFormat->addItem(tr("raw"), QLatin1String("raw"));
        layout->addWidget(m_prettyFormat);
        m_prettyFormat->setCurrentIndex(m_settings->showPrettyFormat);
        m_prettyFormat->setVisible(false);
        connect(m_prettyFormat, SIGNAL(currentIndexChanged(int)), this, SLOT(testForArgumentsChanged()));
    }

    QStringList arguments() const
    {
        QStringList args;
        foreach (const QString &arg, m_diffArgs) {
            if (arg == QLatin1String("--patience")
                    || arg == QLatin1String("--ignore-space-change")
                    || arg.startsWith(QLatin1String("--pretty="))
                    || arg.startsWith(QLatin1String("--format=")))
                continue;
            args.append(arg);
        }

        if (m_patience->isChecked() && m_patience->isVisible())
            args.prepend(QLatin1String("--patience"));
        if (m_ignoreSpaces->isChecked() && m_ignoreSpaces->isVisible())
            args.prepend(QLatin1String("--ignore-space-change"));
        if (m_prettyFormat->isVisible()) {
            args.prepend(QString::fromLatin1("--pretty=")
                         + m_prettyFormat->itemData(m_prettyFormat->currentIndex()).toString());
        }

        return args;
    }

    void testForArgumentsChanged() {
        m_settings->diffPatience = m_patience->isChecked();
        m_settings->ignoreSpaceChangesInDiff = m_ignoreSpaces->isChecked();
        m_settings->showPrettyFormat = m_prettyFormat->currentIndex();

        QStringList newArguments = arguments();

        if (newArguments == m_diffArgs)
            return;

        m_diffArgs = newArguments;
        redoCommand();
    }

protected:
    QToolButton *m_patience;
    QToolButton *m_ignoreSpaces;
    QComboBox *m_prettyFormat;
};

class GitCommitDiffArgumentsWidget : public BaseGitDiffArgumentsWidget
{
public:
    GitCommitDiffArgumentsWidget(Git::Internal::GitSettings *settings,
                                 Git::Internal::GitClient *client, const QString &directory,
                                 const QStringList &args, const QStringList &unstaged,
                                 const QStringList &staged) :
        BaseGitDiffArgumentsWidget(settings, client, directory, args),
        m_unstagedFileNames(unstaged),
        m_stagedFileNames(staged)
    { }

    void redoCommand()
    {
        m_client->diff(m_workingDirectory, m_diffArgs, m_unstagedFileNames, m_stagedFileNames);
    }

private:
    const QStringList m_unstagedFileNames;
    const QStringList m_stagedFileNames;
};

class GitFileDiffArgumentsWidget : public BaseGitDiffArgumentsWidget
{
public:
    GitFileDiffArgumentsWidget(Git::Internal::GitSettings *settings,
                               Git::Internal::GitClient *client, const QString &directory,
                               const QStringList &args, const QString &file) :
        BaseGitDiffArgumentsWidget(settings, client, directory, args),
        m_fileName(file)
    { }

    void redoCommand()
    {
        m_client->diff(m_workingDirectory, m_diffArgs, m_fileName);
    }

private:
    const QString m_fileName;
};

class GitBranchDiffArgumentsWidget : public BaseGitDiffArgumentsWidget
{
public:
    GitBranchDiffArgumentsWidget(Git::Internal::GitSettings *settings,
                                 Git::Internal::GitClient *client, const QString &directory,
                                 const QStringList &args, const QString &branch) :
        BaseGitDiffArgumentsWidget(settings, client, directory, args),
        m_branchName(branch)
    { }

    void redoCommand()
    {
        m_client->diffBranch(m_workingDirectory, m_diffArgs, m_branchName);
    }

private:
    const QString m_branchName;
};

class GitShowArgumentsWidget : public BaseGitDiffArgumentsWidget
{
public:
    GitShowArgumentsWidget(Git::Internal::GitSettings *settings,
                           Git::Internal::GitClient *client, const QString &directory,
                           const QStringList &args, const QString &id) :
        BaseGitDiffArgumentsWidget(settings, client, directory, args),
        m_id(id)
    {
        m_patience->setVisible(false);
        m_ignoreSpaces->setVisible(false);
        m_prettyFormat->setVisible(true);
    }

    void redoCommand()
    {
        m_client->show(m_workingDirectory, m_id, m_diffArgs);
    }

private:
    const QString m_id;
};


class GitBlameArgumentsWidget : public Git::Internal::BaseGitArgumentsWidget
{
public:
    GitBlameArgumentsWidget(Git::Internal::GitSettings *settings,
                            Git::Internal::GitClient *client, const QString &directory,
                            const QStringList &args, const QString &revision,
                            const QString &fileName) :
        Git::Internal::BaseGitArgumentsWidget(settings, client, directory, args),
        m_omitDate(0),
        m_ignoreSpaces(0),
        m_editor(0),
        m_revision(revision),
        m_fileName(fileName)
    {
        QHBoxLayout * layout = new QHBoxLayout(this);
        layout->setContentsMargins(3, 0, 3, 0);

        m_omitDate = new QToolButton;
        m_omitDate->setToolTip(tr("Do not show the date a change was made in the output"));
        m_omitDate->setText(tr("Omit Date"));
        layout->addWidget(m_omitDate);
        m_omitDate->setCheckable(true);
        m_omitDate->setChecked(m_settings->omitAnnotationDate);
        m_omitDate->setMinimumHeight(16);
        m_omitDate->setMaximumHeight(16);
        connect(m_omitDate, SIGNAL(toggled(bool)), this, SLOT(testForArgumentsChanged()));

        m_ignoreSpaces = new QToolButton;
        m_ignoreSpaces->setToolTip(tr("Ignore whitespace only changes"));
        m_ignoreSpaces->setText(tr("Ignore Whitespace"));
        layout->addWidget(m_ignoreSpaces);
        m_ignoreSpaces->setCheckable(true);
        m_ignoreSpaces->setChecked(m_settings->ignoreSpaceChangesInBlame);
        m_ignoreSpaces->setMinimumHeight(16);
        m_ignoreSpaces->setMaximumHeight(16);
        connect(m_ignoreSpaces, SIGNAL(toggled(bool)), this, SLOT(testForArgumentsChanged()));
    }

    void setEditor(VCSBase::VCSBaseEditor *editor)
    {
        Q_ASSERT(editor);
        m_editor = editor;
    }

    QStringList arguments() const
    {
        QStringList args = m_diffArgs;

        args.removeAll(QLatin1String("-w"));

        if (m_ignoreSpaces->isChecked())
            args.prepend(QLatin1String("-w"));

        return args;
    }

    void testForArgumentsChanged() {
        m_settings->omitAnnotationDate = m_omitDate->isChecked();
        m_settings->ignoreSpaceChangesInBlame = m_ignoreSpaces->isChecked();

        m_diffArgs = arguments();
        redoCommand(); // always redo for omit date
    }

    void redoCommand()
    {
        m_client->blame(m_workingDirectory, m_diffArgs, m_fileName,
                        m_revision, m_editor->lineNumberOfCurrentEditor());
    }

private:
    QToolButton *m_omitDate;
    QToolButton *m_ignoreSpaces;
    VCSBase::VCSBaseEditor *m_editor;
    QString m_revision;
    QString m_fileName;
};

} // namespace

inline Core::IEditor* locateEditor(const Core::ICore *core, const char *property, const QString &entry)
{
    foreach (Core::IEditor *ed, core->editorManager()->openedEditors())
        if (ed->file()->property(property).toString() == entry)
            return ed;
    return 0;
}

// Return converted command output, remove '\r' read on Windows
static inline QString commandOutputFromLocal8Bit(const QByteArray &a)
{
    QString output = QString::fromLocal8Bit(a);
    output.remove(QLatin1Char('\r'));
    return output;
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

static inline VCSBase::VCSBaseOutputWindow *outputWindow()
{
    return VCSBase::VCSBaseOutputWindow::instance();
}

namespace Git {
namespace Internal {

static inline QString msgRepositoryNotFound(const QString &dir)
{
    return GitClient::tr("Unable to determine the repository for %1.").arg(dir);
}

static inline QString msgParseFilesFailed()
{
    return  GitClient::tr("Unable to parse the file output.");
}

// ---------------- GitClient

const char *GitClient::stashNamePrefix = "stash@{";

GitClient::GitClient(GitPlugin* plugin)
  : m_msgWait(tr("Waiting for data...")),
    m_plugin(plugin),
    m_core(Core::ICore::instance()),
    m_repositoryChangedSignalMapper(0),
    m_cachedGitVersion(0),
    m_hasCachedGitVersion(false)
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

QString GitClient::findRepositoryForDirectory(const QString &dir)
{
    // Check for ".git/config"
    const QString checkFile = QLatin1String(kGitDirectoryC) + QLatin1String("/config");
    return VCSBase::VCSBasePlugin::findRepositoryForDirectory(dir, checkFile);
}

VCSBase::VCSBaseEditor *GitClient::findExistingVCSEditor(const char *registerDynamicProperty,
                                                         const QString &dynamicPropertyValue) const
{
    VCSBase::VCSBaseEditor *rc = 0;
    Core::IEditor *outputEditor = locateEditor(m_core, registerDynamicProperty, dynamicPropertyValue);
    if (!outputEditor)
        return 0;

    // Exists already
    Core::EditorManager::instance()->activateEditor(outputEditor, Core::EditorManager::ModeSwitch);
    outputEditor->createNew(m_msgWait);
    rc = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);

    return rc;
}


/* Create an editor associated to VCS output of a source file/directory
 * (using the file's codec). Makes use of a dynamic property to find an
 * existing instance and to reuse it (in case, say, 'git diff foo' is
 * already open). */
VCSBase::VCSBaseEditor *GitClient::createVCSEditor(const QString &id,
                                                   QString title,
                                                   // Source file or directory
                                                   const QString &source,
                                                   bool setSourceCodec,
                                                   // Dynamic property and value to identify that editor
                                                   const char *registerDynamicProperty,
                                                   const QString &dynamicPropertyValue,
                                                   QWidget *configWidget) const
{
    VCSBase::VCSBaseEditor *rc = 0;
    Q_ASSERT(!findExistingVCSEditor(registerDynamicProperty, dynamicPropertyValue));

    // Create new, set wait message, set up with source and codec
    Core::IEditor *outputEditor = m_core->editorManager()->openEditorWithContents(id, &title, m_msgWait);
    outputEditor->file()->setProperty(registerDynamicProperty, dynamicPropertyValue);
    rc = VCSBase::VCSBaseEditor::getVcsBaseEditor(outputEditor);
    connect(rc, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            this, SLOT(slotBlameRevisionRequested(QString,QString,int)));
    QTC_ASSERT(rc, return 0);
    rc->setSource(source);
    if (setSourceCodec)
        rc->setCodec(VCSBase::VCSBaseEditor::getCodec(source));

    m_core->editorManager()->activateEditor(outputEditor, Core::EditorManager::ModeSwitch);
    rc->setForceReadOnly(true);

    if (configWidget)
        rc->setConfigurationWidget(configWidget);

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
    const QString editorId = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_ID);
    const QString title = tr("Git Diff");

    QStringList userDiffArgs = diffArgs;
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("originalFileName", workingDirectory);
    if (!editor) {
        GitCommitDiffArgumentsWidget *argWidget =
                new GitCommitDiffArgumentsWidget(&m_settings, this, workingDirectory, diffArgs,
                                                 unstagedFileNames, stagedFileNames);
        userDiffArgs = argWidget->arguments();

        editor = createVCSEditor(editorId, title,
                                 workingDirectory, true, "originalFileName", workingDirectory, argWidget);
    }
    editor->setDiffBaseDirectory(workingDirectory);

    // Create a batch of 2 commands to be run after each other in case
    // we have a mixture of staged/unstaged files as is the case
    // when using the submit dialog.
    GitCommand *command = createCommand(workingDirectory, editor);
    // Directory diff?

    QStringList cmdArgs;
    cmdArgs << QLatin1String("diff") << QLatin1String(noColorOption);

    if (unstagedFileNames.empty() && stagedFileNames.empty()) {
       QStringList arguments(cmdArgs);
       arguments << userDiffArgs;
       outputWindow()->appendCommand(workingDirectory, binary, arguments);
       command->addJob(arguments, m_settings.timeoutSeconds);
    } else {
        // Files diff.
        if (!unstagedFileNames.empty()) {
           QStringList arguments(cmdArgs);
           arguments << userDiffArgs;
           arguments << QLatin1String("--") << unstagedFileNames;
           outputWindow()->appendCommand(workingDirectory, binary, arguments);
           command->addJob(arguments, m_settings.timeoutSeconds);
        }
        if (!stagedFileNames.empty()) {
           QStringList arguments(cmdArgs);
           arguments << userDiffArgs;
           arguments << QLatin1String("--cached") << diffArgs << QLatin1String("--") << stagedFileNames;
           outputWindow()->appendCommand(workingDirectory, binary, arguments);
           command->addJob(arguments, m_settings.timeoutSeconds);
        }
    }
    command->execute();
}

void GitClient::diff(const QString &workingDirectory,
                     const QStringList &diffArgs,
                     const QString &fileName)
{
    const QString editorId = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_ID);
    const QString title = tr("Git Diff %1").arg(fileName);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, fileName);

    QStringList userDiffArgs = diffArgs;
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("originalFileName", sourceFile);
    if (!editor) {
        GitFileDiffArgumentsWidget *argWidget =
                new GitFileDiffArgumentsWidget(&m_settings, this, workingDirectory,
                                               diffArgs, fileName);
        userDiffArgs = argWidget->arguments();

        editor = createVCSEditor(editorId, title, sourceFile, true, "originalFileName", sourceFile, argWidget);
    }

    QStringList cmdArgs;
    cmdArgs << QLatin1String("diff") << QLatin1String(noColorOption)
              << userDiffArgs;

    if (!fileName.isEmpty())
        cmdArgs << QLatin1String("--") << fileName;
    executeGit(workingDirectory, cmdArgs, editor);
}

void GitClient::diffBranch(const QString &workingDirectory,
                           const QStringList &diffArgs,
                           const QString &branchName)
{
    if (Git::Constants::debug)
        qDebug() << "diffBranch" << workingDirectory << branchName;

    const QString editorId = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_ID);
    const QString title = tr("Git Diff Branch %1").arg(branchName);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, QStringList());

    QStringList userDiffArgs = diffArgs;
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("BranchName", branchName);
    if (!editor) {
        GitBranchDiffArgumentsWidget *argWidget =
                new GitBranchDiffArgumentsWidget(&m_settings, this, workingDirectory,
                                                 diffArgs, branchName);
        userDiffArgs = argWidget->arguments();

        editor = createVCSEditor(editorId, title, sourceFile, true, "BranchName", branchName, argWidget);
    }

    QStringList cmdArgs;
    cmdArgs << QLatin1String("diff") << QLatin1String(noColorOption)
              << diffArgs  << branchName;

    executeGit(workingDirectory, cmdArgs, editor);
}

void GitClient::status(const QString &workingDirectory)
{
    // @TODO: Use "--no-color" once it is supported
    QStringList statusArgs(QLatin1String("status"));
    statusArgs << QLatin1String("-u");
    VCSBase::VCSBaseOutputWindow *outwin = outputWindow();
    outwin->setRepository(workingDirectory);
    GitCommand *command = executeGit(workingDirectory, statusArgs, 0, true);
    connect(command, SIGNAL(finished(bool,int,QVariant)), outwin, SLOT(clearRepository()),
            Qt::QueuedConnection);
}

static const char graphLogFormatC[] = "%h %an %s %ci";

// Create a graphical log.
void GitClient::graphLog(const QString &workingDirectory, const QString & branch)
{
    if (Git::Constants::debug)
        qDebug() << "log" << workingDirectory;

    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(noColorOption);

    if (m_settings.logCount > 0)
         arguments << QLatin1String("-n") << QString::number(m_settings.logCount);
    arguments << (QLatin1String("--pretty=format:") +  QLatin1String(graphLogFormatC))
              << QLatin1String("--topo-order") <<  QLatin1String("--graph");

    QString title;
    if (branch.isEmpty()) {
        title = tr("Git Log");
    } else {
        title = tr("Git Log %1").arg(branch);
        arguments << branch;
    }
    const QString editorId = QLatin1String(Git::Constants::GIT_LOG_EDITOR_ID);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, QStringList());
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("logFileName", sourceFile);
    if (!editor)
        editor = createVCSEditor(editorId, title, sourceFile, false, "logFileName", sourceFile, 0);
    executeGit(workingDirectory, arguments, editor);
}

void GitClient::log(const QString &workingDirectory, const QStringList &fileNames, bool enableAnnotationContextMenu)
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
    const QString editorId = QLatin1String(Git::Constants::GIT_LOG_EDITOR_ID);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, fileNames);
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("logFileName", sourceFile);
    if (!editor)
        editor = createVCSEditor(editorId, title, sourceFile, false, "logFileName", sourceFile, 0);
    editor->setFileLogAnnotateEnabled(enableAnnotationContextMenu);
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
    return GitClient::tr("Cannot describe '%1'.").arg(sha);
}

void GitClient::show(const QString &source, const QString &id, const QStringList &args)
{
    if (Git::Constants::debug)
        qDebug() << "show" << source << id;
    if (!canShow(id)) {
        outputWindow()->append(msgCannotShow(id));
        return;
    }

    QStringList userArgs = args;
    const QString title =  tr("Git Show %1").arg(id);
    const QString editorId = QLatin1String(Git::Constants::GIT_DIFF_EDITOR_ID);
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("show", id);
    if (!editor) {
        GitShowArgumentsWidget *argWidget =
                new GitShowArgumentsWidget(&m_settings, this, source,
                                           QStringList(), id);
        userArgs = argWidget->arguments();
        editor = createVCSEditor(editorId, title, source, true, "show", id, argWidget);
    }

    QStringList arguments;
    arguments << QLatin1String("show") << QLatin1String(noColorOption);
    arguments.append(userArgs);
    arguments << id;

    const QFileInfo sourceFi(source);
    const QString workDir = sourceFi.isDir() ? sourceFi.absoluteFilePath() : sourceFi.absolutePath();
    executeGit(workDir, arguments, editor);
}

void GitClient::slotBlameRevisionRequested(const QString &source, QString change, int lineNumber)
{
    // This might be invoked with a verbose revision description
    // "SHA1 author subject" from the annotation context menu. Strip the rest.
    const int blankPos = change.indexOf(QLatin1Char(' '));
    if (blankPos != -1)
        change.truncate(blankPos);
    const QFileInfo fi(source);
    blame(fi.absolutePath(), QStringList(), fi.fileName(), change, lineNumber);
}

void GitClient::blame(const QString &workingDirectory,
                      const QStringList &args,
                      const QString &fileName,
                      const QString &revision /* = QString() */,
                      int lineNumber /* = -1 */)
{
    if (Git::Constants::debug)
        qDebug() << "blame" << workingDirectory << fileName << lineNumber << args;

    const QString editorId = QLatin1String(Git::Constants::GIT_BLAME_EDITOR_ID);
    const QString id = VCSBase::VCSBaseEditor::getTitleId(workingDirectory, QStringList(fileName), revision);
    const QString title = tr("Git Blame %1").arg(id);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, fileName);

    QStringList userBlameArgs = args;
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("blameFileName", id);
    if (!editor) {
        GitBlameArgumentsWidget *argWidget =
                new GitBlameArgumentsWidget(&m_settings, this, workingDirectory, userBlameArgs,
                                            revision, fileName);
        editor = createVCSEditor(editorId, title, sourceFile, true, "blameFileName", id, argWidget);
        argWidget->setEditor(editor);

        userBlameArgs = argWidget->arguments();
    }

    QStringList arguments(QLatin1String("blame"));
    arguments << QLatin1String("--root");
    arguments.append(userBlameArgs);
    arguments << QLatin1String("--") << fileName;
    if (!revision.isEmpty())
        arguments << revision;
    executeGit(workingDirectory, arguments, editor, false, GitCommand::NoReport, lineNumber);
}

void GitClient::checkoutBranch(const QString &workingDirectory, const QString &branch)
{
    QStringList arguments(QLatin1String("checkout"));
    arguments <<  branch;
    GitCommand *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

bool GitClient::synchronousCheckoutBranch(const QString &workingDirectory,
                                    const QString &branch,
                                    QString *errorMessage /* = 0 */)
{
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("checkout") << branch;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    const QString output = commandOutputFromLocal8Bit(outputText);
    outputWindow()->append(output);
    if (!rc) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        //: Meaning of the arguments: %1: Branch, %2: Repository, %3: Error message
        const QString msg = tr("Unable to checkout %1 of %2: %3").arg(branch, workingDirectory, stdErr);
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->appendError(msg);
        }
        return false;
    }
    return true;
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

// Warning: 'intendToAdd' works only from 1.6.1 onwards
bool GitClient::synchronousAdd(const QString &workingDirectory,
                               bool intendToAdd,
                               const QStringList &files)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << files;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("add");
    if (intendToAdd)
        arguments << QLatin1String("--intent-to-add");
    arguments.append(files);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Unable to add %n file(s) to %1: %2", 0, files.size()).
                                     arg(QDir::toNativeSeparators(workingDirectory),
                                     commandOutputFromLocal8Bit(errorText));
        outputWindow()->appendError(errorMessage);
    }
    return rc;
}

bool GitClient::synchronousDelete(const QString &workingDirectory,
                                  bool force,
                                  const QStringList &files)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << files;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("rm");
    if (force)
        arguments << QLatin1String("--force");
    arguments.append(files);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Unable to remove %n file(s) from %1: %2", 0, files.size()).
                                     arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        outputWindow()->appendError(errorMessage);
    }
    return rc;
}

bool GitClient::synchronousMove(const QString &workingDirectory,
                                const QString &from,
                                const QString &to)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << from << to;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("mv");
    arguments << (from);
    arguments << (to);
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString errorMessage = tr("Unable to move from %1 to %2: %3").
                                     arg(from, to, commandOutputFromLocal8Bit(errorText));
        outputWindow()->appendError(errorMessage);
    }
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
    arguments << QLatin1String("reset");
    if (files.isEmpty()) {
        arguments << QLatin1String("--hard");
    } else {
        arguments << QLatin1String("HEAD") << QLatin1String("--") << files;
    }
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    const QString output = commandOutputFromLocal8Bit(outputText);
    outputWindow()->append(output);
    // Note that git exits with 1 even if the operation is successful
    // Assume real failure if the output does not contain "foo.cpp modified"
    // or "Unstaged changes after reset" (git 1.7.0).
    if (!rc &&
        (!output.contains(QLatin1String("modified"))
         && !output.contains(QLatin1String("Unstaged changes after reset")))) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        const QString msg = files.isEmpty() ?
                            tr("Unable to reset %1: %2").arg(QDir::toNativeSeparators(workingDirectory), stdErr) :
                            tr("Unable to reset %n file(s) in %1: %2", 0, files.size()).
                            arg(QDir::toNativeSeparators(workingDirectory), stdErr);
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->appendError(msg);
        }
        return false;
    }
    return true;
}

// Initialize repository
bool GitClient::synchronousInit(const QString &workingDirectory)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;
    QByteArray outputText;
    QByteArray errorText;
    const QStringList arguments(QLatin1String("init"));
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    // '[Re]Initialized...'
    outputWindow()->append(commandOutputFromLocal8Bit(outputText));
    if (!rc)
        outputWindow()->appendError(commandOutputFromLocal8Bit(errorText));
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
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << files;
    if (revertStaging && revision.isEmpty())
        revision = QLatin1String("HEAD");
    if (files.isEmpty())
        files = QStringList(QString(QLatin1Char('.')));
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("checkout");
    if (revertStaging)
        arguments << revision;
    arguments << QLatin1String("--") << files;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString fileArg = files.join(QLatin1String(", "));
        //: Meaning of the arguments: %1: revision, %2: files, %3: repository,
        //: %4: Error message
        const QString msg = tr("Unable to checkout %1 of %2 in %3: %4").
                            arg(revision, fileArg, workingDirectory, commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->appendError(msg);
        }
        return false;
    }
    return true;
}

static inline QString msgParentRevisionFailed(const QString &workingDirectory,
                                              const QString &revision,
                                              const QString &why)
{
    //: Failed to find parent revisions of a SHA1 for "annotate previous"
    return GitClient::tr("Unable to find parent revisions of %1 in %2: %3").arg(revision, workingDirectory, why);
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

// Find out the immediate parent revisions of a revision of the repository.
// Might be several in case of merges.
bool GitClient::synchronousParentRevisions(const QString &workingDirectory,
                                           const QStringList &files /* = QStringList() */,
                                           const QString &revision,
                                           QStringList *parents,
                                           QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << revision;
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("rev-list") << QLatin1String(GitClient::noColorOption)
              << QLatin1String("--parents") << QLatin1String("--max-count=1") << revision;
    if (!files.isEmpty()) {
        arguments.append(QLatin1String("--"));
        arguments.append(files);
    }
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText);
    if (!rc) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    // Should result in one line of blank-delimited revisions, specifying current first
    // unless it is top.
    QString outputText = commandOutputFromLocal8Bit(outputTextData);
    outputText.remove(QLatin1Char('\n'));
    if (!splitCommitParents(outputText, 0, parents)) {
        *errorMessage = msgParentRevisionFailed(workingDirectory, revision, msgInvalidRevision());
        return false;
    }
    if (Git::Constants::debug)
        qDebug() << workingDirectory << files << revision << "->" << *parents;
    return true;
}

// Short SHA1, author, subject
static const char defaultShortLogFormatC[] = "%h (%an \"%s\")";

bool GitClient::synchronousShortDescription(const QString &workingDirectory, const QString &revision,
                                    QString *description, QString *errorMessage)
{
    // Short SHA 1, author, subject
    return synchronousShortDescription(workingDirectory, revision,
                               QLatin1String(defaultShortLogFormatC),
                               description, errorMessage);
}

// Convenience working on a list of revisions
bool GitClient::synchronousShortDescriptions(const QString &workingDirectory, const QStringList &revisions,
                                            QStringList *descriptions, QString *errorMessage)
{
    descriptions->clear();
    foreach (const QString &revision, revisions) {
        QString description;
        if (!synchronousShortDescription(workingDirectory, revision, &description, errorMessage)) {
            descriptions->clear();
            return false;
        }
        descriptions->push_back(description);
    }
    return true;
}

static inline QString msgCannotDetermineBranch(const QString &workingDirectory, const QString &why)
{
    return GitClient::tr("Unable to retrieve branch of %1: %2").arg(QDir::toNativeSeparators(workingDirectory), why);
}

// Retrieve head revision/branch
bool GitClient::synchronousTopRevision(const QString &workingDirectory,
                                       QString *revision /* = 0 */,
                                       QString *branch /* = 0 */,
                                       QString *errorMessageIn /* = 0 */)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    QString errorMessage;
    do {
        // get revision
        if (revision) {
            revision->clear();
            arguments << QLatin1String("log") << QLatin1String(noColorOption)
                    <<  QLatin1String("--max-count=1") << QLatin1String("--pretty=format:%H");
            if (!fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText)) {
                errorMessage =  tr("Unable to retrieve top revision of %1: %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
                break;
            }
            *revision = commandOutputFromLocal8Bit(outputTextData);
            revision->remove(QLatin1Char('\n'));
        } // revision desired
        // get branch
        if (branch) {
            branch->clear();
            arguments.clear();
            arguments << QLatin1String("branch") << QLatin1String(noColorOption);
            if (!fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText)) {
                errorMessage = msgCannotDetermineBranch(workingDirectory, commandOutputFromLocal8Bit(errorText));
                break;
            }
            /* parse output for current branch: \code
* master
  branch2
\endcode */
            const QString branchPrefix = QLatin1String("* ");
            foreach(const QString &line, commandOutputLinesFromLocal8Bit(outputTextData)) {
                if (line.startsWith(branchPrefix)) {
                    *branch = line;
                    branch->remove(0, branchPrefix.size());
                    break;
                }
            }
            if (branch->isEmpty()) {
                errorMessage = msgCannotDetermineBranch(workingDirectory,
                                                        QString::fromLatin1("Internal error: Failed to parse output: %1").arg(commandOutputFromLocal8Bit(outputTextData)));
                break;
            }
        } // branch
    } while (false);
    const bool failed = (revision && revision->isEmpty()) || (branch && branch->isEmpty());
    if (failed && !errorMessage.isEmpty()) {
        if (errorMessageIn) {
            *errorMessageIn = errorMessage;
        } else {
            outputWindow()->appendError(errorMessage);
        }
    }
    return !failed;
}

// Format an entry in a one-liner for selection list using git log.
bool GitClient::synchronousShortDescription(const QString &workingDirectory,
                                    const QString &revision,
                                    const QString &format,
                                    QString *description,
                                    QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << revision;
    QByteArray outputTextData;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("log") << QLatin1String(GitClient::noColorOption)
              << (QLatin1String("--pretty=format:") + format)
              << QLatin1String("--max-count=1") << revision;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputTextData, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to describe revision %1 in %2: %3").arg(revision, workingDirectory, commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *description = commandOutputFromLocal8Bit(outputTextData);
    if (description->endsWith(QLatin1Char('\n')))
        description->truncate(description->size() - 1);
    return true;
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

QString GitClient::synchronousStash(const QString &workingDirectory,
                                    const QString &messageKeyword /*  = QString() */,
                                    unsigned flags,
                                    bool *unchanged /* =0 */)
{
    if (unchanged)
        *unchanged = false;
    QString message;
    bool success = false;
    // Check for changes and stash
    QString errorMessage;
    switch (gitStatus(workingDirectory, false, 0, &errorMessage)) {
    case  StatusChanged: {
            message = creatorStashMessage(messageKeyword);
            do {
                if ((flags & StashPromptDescription)) {
                    if (!inputText(Core::ICore::instance()->mainWindow(),
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
        }
        break;
    case StatusUnchanged:
        if (unchanged)
            *unchanged = true;
        if (!(flags & StashIgnoreUnchanged))
            outputWindow()->append(msgNoChangedFiles());
        break;
    case StatusFailed:
        outputWindow()->append(errorMessage);
        break;
    }
    if (!success)
        message.clear();
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << '\n' << workingDirectory << messageKeyword << "returns" << message;
    return message;
}

bool GitClient::executeSynchronousStash(const QString &workingDirectory,
                                 const QString &message,
                                 QString *errorMessage /* = 0*/)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;
    QByteArray outputText;
    QByteArray errorText;
    QStringList arguments;
    arguments << QLatin1String("stash");
    if (!message.isEmpty())
        arguments << QLatin1String("save") << message;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString msg = tr("Unable stash in %1: %2").
                            arg(QDir::toNativeSeparators(workingDirectory),
                                commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->append(msg);
        }
        return false;
    }
    return true;
}

// Resolve a stash name from message
bool GitClient::stashNameFromMessage(const QString &workingDirectory,
                                     const QString &message, QString *name,
                                     QString *errorMessage /* = 0 */)
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
    const QString msg = tr("Unable to resolve stash message '%1' in %2").arg(message, workingDirectory);
    if (errorMessage) {
        *errorMessage = msg;
    } else {
        outputWindow()->append(msg);
    }
    return  false;
}

bool GitClient::synchronousBranchCmd(const QString &workingDirectory, QStringList branchArgs,
                                     QString *output, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << branchArgs;
    branchArgs.push_front(QLatin1String("branch"));
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, branchArgs, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to run a 'git branch' command in %1: %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

bool GitClient::synchronousShow(const QString &workingDirectory, const QString &id,
                                 QString *output, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory << id;
    if (!canShow(id)) {
        *errorMessage = msgCannotShow(id);
        return false;
    }
    QStringList args(QLatin1String("show"));
    args << QLatin1String(noColorOption) << id;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to run 'git show' in %1: %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    *output = commandOutputFromLocal8Bit(outputText);
    return true;
}

// Retrieve list of files to be cleaned
bool GitClient::synchronousCleanList(const QString &workingDirectory,
                                     QStringList *files, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;
    files->clear();
    QStringList args;
    args << QLatin1String("clean") << QLatin1String("--dry-run") << QLatin1String("-dxf");
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (!rc) {
        *errorMessage = tr("Unable to run 'git clean' in %1: %2").arg(QDir::toNativeSeparators(workingDirectory), commandOutputFromLocal8Bit(errorText));
        return false;
    }
    // Filter files that git would remove
    const QString prefix = QLatin1String("Would remove ");
    foreach(const QString &line, commandOutputLinesFromLocal8Bit(outputText))
        if (line.startsWith(prefix))
            files->push_back(line.mid(prefix.size()));
    return true;
}

bool GitClient::synchronousApplyPatch(const QString &workingDirectory,
                                      const QString &file, QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;
    QStringList args;
    args << QLatin1String("apply") << QLatin1String("--whitespace=fix") << file;
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, args, &outputText, &errorText);
    if (rc) {
        if (!errorText.isEmpty())
            *errorMessage = tr("There were warnings while applying %1 to %2:\n%3").arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
    } else {
        *errorMessage = tr("Unable apply patch %1 to %2: %3").arg(file, workingDirectory, commandOutputFromLocal8Bit(errorText));
        return false;
    }
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
        connect(command, SIGNAL(finished(bool,int,QVariant)), editor, SLOT(commandFinishedGotoLine(bool,int,QVariant)));
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
                                  int editorLineNumber,
                                  bool unixTerminalDisabled)
{
    outputWindow()->appendCommand(workingDirectory, QLatin1String(Constants::GIT_BINARY), arguments);
    GitCommand *command = createCommand(workingDirectory, editor, outputToWindow, editorLineNumber);
    command->addJob(arguments, m_settings.timeoutSeconds);
    command->setTerminationReportMode(tm);
    command->setUnixTerminalDisabled(unixTerminalDisabled);
    command->execute();
    return command;
}

// Return fixed arguments required to run
QString GitClient::binary() const
{
    return m_binaryPath;
}

// Determine a value for the HOME variable on Windows
// working around MSys git not finding it when run outside git bash
// (then looking for the SSH keys under "\program files\git").

QString GitClient::fakeWinHome(const QProcessEnvironment &e)
{
    const QString homeDrive = e.value("HOMEDRIVE");
    const QString homePath = e.value("HOMEPATH");
    QTC_ASSERT(!homeDrive.isEmpty() && !homePath.isEmpty(), return QString())
    return homeDrive + homePath;
}

QProcessEnvironment GitClient::processEnvironment() const
{

    QProcessEnvironment environment = QProcessEnvironment::systemEnvironment();
    if (m_settings.adoptPath)
        environment.insert(QLatin1String("PATH"), m_settings.path);
#ifdef Q_OS_WIN
    if (m_settings.winSetHomeEnvironment) {
        const QString home = fakeWinHome(environment);
        if (Constants::debug)
            qDebug("Setting home '%s'", qPrintable(home));
        environment.insert(QLatin1String("HOME"), home);
    }
#endif // Q_OS_WIN
    // Set up SSH and C locale (required by git using perl).
    VCSBase::VCSBasePlugin::setProcessEnvironment(&environment, false);
    return environment;
}

// Synchronous git execution using Utils::SynchronousProcess, with
// log windows updating.
Utils::SynchronousProcessResponse
        GitClient::synchronousGit(const QString &workingDirectory,
                                  const QStringList &gitArguments,
                                  unsigned flags,
                                  QTextCodec *stdOutCodec)
{
    if (Git::Constants::debug)
        qDebug() << "synchronousGit" << workingDirectory << gitArguments;
    return VCSBase::VCSBasePlugin::runVCS(workingDirectory, binary(), gitArguments,
                                          m_settings.timeoutSeconds * 1000,
                                          processEnvironment(),
                                          flags, stdOutCodec);
}

bool GitClient::fullySynchronousGit(const QString &workingDirectory,
                                    const QStringList &gitArguments,
                                    QByteArray* outputText,
                                    QByteArray* errorText,
                                    bool logCommandToWindow)
{
    if (Git::Constants::debug)
        qDebug() << "fullySynchronousGit" << workingDirectory << gitArguments;

    if (logCommandToWindow)
        outputWindow()->appendCommand(workingDirectory, m_binaryPath, gitArguments);

    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.setProcessEnvironment(processEnvironment());

    process.start(binary(), gitArguments);
    process.closeWriteChannel();
    if (!process.waitForStarted()) {
        if (errorText) {
            const QString msg = QString::fromLatin1("Unable to execute '%1': %2:")
                                .arg(binary(), process.errorString());
            *errorText = msg.toLocal8Bit();
        }
        return false;
    }

    if (!Utils::SynchronousProcess::readDataFromProcess(process, m_settings.timeoutSeconds * 1000,
                                                        outputText, errorText, true)) {
        errorText->append(GitCommand::msgTimeout(m_settings.timeoutSeconds).toLocal8Bit());
        Utils::SynchronousProcess::stopProcess(process);
        return false;
    }

    if (Git::Constants::debug)
        qDebug() << "synchronousGit ex=" << process.exitStatus() << process.exitCode();
    return process.exitStatus() == QProcess::NormalExit && process.exitCode() == 0;
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
        outputWindow()->appendError(errorMessage);
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
            if (!executeSynchronousStash(workingDirectory, creatorStashMessage(QLatin1String("push")), errorMessage))
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
                                             QString *errorMessage,
                                             bool *onBranch)
{
    // Run 'status'. Note that git returns exitcode 1 if there are no added files.
    QByteArray outputText;
    QByteArray errorText;
    // @TODO: Use "--no-color" once it is supported
    QStringList statusArgs(QLatin1String("status"));
    if (untracked)
        statusArgs << QLatin1String("-u");
    const bool statusRc = fullySynchronousGit(workingDirectory, statusArgs, &outputText, &errorText);
    GitCommand::removeColorCodes(&outputText);
    if (output)
        *output = commandOutputFromLocal8Bit(outputText);
    const bool branchKnown = outputText.contains(kBranchIndicatorC);
    if (onBranch)
        *onBranch = branchKnown;
    // Is it something really fatal?
    if (!statusRc && !branchKnown && !outputText.contains("# Not currently on any branch.")) {
        if (errorMessage) {
            const QString error = commandOutputFromLocal8Bit(errorText);
            *errorMessage = tr("Unable to obtain the status: %1").arg(error);
        }
        return StatusFailed;
    }
    // Unchanged (output text depending on whether -u was passed)
    if (outputText.contains("nothing to commit"))
        return StatusUnchanged;
    if (outputText.contains("nothing added to commit but untracked files present"))
        return untracked ? StatusChanged : StatusUnchanged;
    return StatusChanged;
}

// Quietly retrieve branch list of remote repository URL
//
// The branch HEAD is pointing to is always returned first.
QStringList GitClient::synchronousRepositoryBranches(const QString &repositoryURL)
{
    QStringList arguments(QLatin1String("ls-remote"));
    arguments << repositoryURL << QLatin1String("HEAD") << QLatin1String("refs/heads/*");
    const unsigned flags =
            VCSBase::VCSBasePlugin::SshPasswordPrompt|
            VCSBase::VCSBasePlugin::SuppressStdErrInLogWindow|
            VCSBase::VCSBasePlugin::SuppressFailMessageInLogWindow;
    const Utils::SynchronousProcessResponse resp = synchronousGit(QString(), arguments, flags);
    QStringList branches;
    branches << "<detached HEAD>";
    QString headSha;
    if (resp.result == Utils::SynchronousProcessResponse::Finished) {
        // split "82bfad2f51d34e98b18982211c82220b8db049b<tab>refs/heads/master"
        foreach(const QString &line, resp.stdOut.split(QLatin1Char('\n'))) {
            if (line.endsWith("\tHEAD")) {
                Q_ASSERT(headSha.isNull());
                headSha = line.left(line.indexOf(QChar('\t')));
                continue;
            }

            const int slashPos = line.lastIndexOf(QLatin1Char('/'));
            const QString branchName = line.mid(slashPos + 1);
            if (slashPos != -1) {
                if (line.startsWith(headSha))
                    branches[0] = branchName;
                else
                    branches.push_back(branchName);
            }
        }
    }
    return branches;
}

void GitClient::launchGitK(const QString &workingDirectory)
{
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    // Locate git in (potentially) custom path. m_binaryPath can be absolute,
    // which will be handled correctly.
    QTC_ASSERT(!m_binaryPath.isEmpty(), return);
    const QString gitBinary = QLatin1String(Constants::GIT_BINARY);
    const QProcessEnvironment env = processEnvironment();
    const QString path = env.value(QLatin1String("PATH"));
    const QString fullGitBinary = Utils::SynchronousProcess::locateBinary(path, m_binaryPath);
    if (fullGitBinary.isEmpty()) {
        outwin->appendError(tr("Cannot locate %1.").arg(gitBinary));
        return;
    }
    const QString gitBinDirectory = QFileInfo(fullGitBinary).absolutePath();
    QDir foundBinDir = gitBinDirectory;
    const bool foundBinDirIsCmdDir = foundBinDir.dirName() == "cmd";
    if (!tryLauchingGitK(env, workingDirectory, gitBinDirectory, foundBinDirIsCmdDir)) {
        if (foundBinDirIsCmdDir) {
            foundBinDir.cdUp();
            tryLauchingGitK(env, workingDirectory, foundBinDir.path() + "/bin", false);
        }
    }
}

bool GitClient::tryLauchingGitK(const QProcessEnvironment &env,
                                const QString &workingDirectory,
                                const QString &gitBinDirectory,
                                bool silent)
{
#ifdef Q_OS_WIN
    // Launch 'wish' shell from git binary directory with the gitk located there
    const QString binary = gitBinDirectory + QLatin1String("/wish");
    QStringList arguments(gitBinDirectory + QLatin1String("/gitk"));
#else
    // Simple: Run gitk from binary path
    const QString binary = gitBinDirectory + QLatin1String("/gitk");
    QStringList arguments;
#endif
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    if (!m_settings.gitkOptions.isEmpty())
        arguments.append(Utils::QtcProcess::splitArgs(m_settings.gitkOptions));
    outwin->appendCommand(workingDirectory, binary, arguments);
    // This should always use QProcess::startDetached (as not to kill
    // the child), but that does not have an environment parameter.
    bool success = false;
    if (m_settings.adoptPath) {
        QProcess *process = new QProcess(this);
        process->setWorkingDirectory(workingDirectory);
        process->setProcessEnvironment(env);
        process->start(binary, arguments);
        success = process->waitForStarted();
        if (success) {
            connect(process, SIGNAL(finished(int)), process, SLOT(deleteLater()));
        } else {
            delete process;
        }
    } else {
        success = QProcess::startDetached(binary, arguments, workingDirectory);
    }
    if (!success) {
        const QString error = tr("Unable to launch %1.").arg(binary);
        if (silent)
            outwin->appendSilently(error);
        else
            outwin->appendError(error);
    }
    return success;
}

bool GitClient::getCommitData(const QString &workingDirectory,
                              bool amend,
                              QString *commitTemplate,
                              CommitData *commitData,
                              QString *errorMessage)
{
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDirectory;

    commitData->clear();

    // Find repo
    const QString repoDirectory = GitClient::findRepositoryForDirectory(workingDirectory);
    if (repoDirectory.isEmpty()) {
        *errorMessage = msgRepositoryNotFound(workingDirectory);
        return false;
    }

    commitData->panelInfo.repository = repoDirectory;

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
            commitData->panelInfo.description = commandOutputFromLocal8Bit(file.readAll()).trimmed();
    }

    // Run status. Note that it has exitcode 1 if there are no added files.
    bool onBranch;
    QString output;
    const StatusResult status = gitStatus(repoDirectory, true, &output, errorMessage, &onBranch);
    switch (status) {
    case  StatusChanged:
        if (!onBranch) {
            *errorMessage = tr("You did not checkout a branch.");
            return false;
        }
        break;
    case StatusUnchanged:
        if (amend)
            break;
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

    if (status != StatusUnchanged) {
        if (!commitData->parseFilesFromStatus(output)) {
            *errorMessage = msgParseFilesFailed();
            return false;
        }
        // Filter out untracked files that are not part of the project
        VCSBase::VCSBaseSubmitEditor::filterUntrackedFilesOfProject(repoDirectory, &commitData->untrackedFiles);
        if (commitData->filesEmpty()) {
            *errorMessage = msgNoChangedFiles();
            return false;
        }
    }

    commitData->panelData.author = readConfigValue(workingDirectory, QLatin1String("user.name"));
    commitData->panelData.email = readConfigValue(workingDirectory, QLatin1String("user.email"));

    // Get the commit template or the last commit message
    if (amend) {
        // Amend: get last commit data as "SHA1@message". TODO: Figure out codec.
        QStringList args(QLatin1String("log"));
        const QString format = synchronousGitVersion(true) > 0x010701 ? "%h@%B" : "%h@%s%n%n%b";
        args << QLatin1String("--max-count=1") << QLatin1String("--pretty=format:") + format;
        const Utils::SynchronousProcessResponse sp = synchronousGit(repoDirectory, args);
        if (sp.result != Utils::SynchronousProcessResponse::Finished) {
            *errorMessage = tr("Unable to retrieve the last commit data of the repository %1.").arg(repoDirectory);
            return false;
        }
        const int separatorPos = sp.stdOut.indexOf(QLatin1Char('@'));
        QTC_ASSERT(separatorPos != -1, return false)
        commitData->amendSHA1= sp.stdOut.left(separatorPos);
        *commitTemplate = sp.stdOut.mid(separatorPos + 1);
    } else {
        // Commit: Get the commit template
        QString templateFilename = readConfigValue(workingDirectory, QLatin1String("commit.template"));
        if (!templateFilename.isEmpty()) {
            // Make relative to repository
            const QFileInfo templateFileInfo(templateFilename);
            if (templateFileInfo.isRelative())
                templateFilename = repoDirectory + QLatin1Char('/') + templateFilename;
            QFile templateFile(templateFilename);
            if (templateFile.open(QIODevice::ReadOnly|QIODevice::Text)) {
                *commitTemplate = QString::fromLocal8Bit(templateFile.readAll());
            } else {
                qWarning("Unable to read commit template %s: %s",
                         qPrintable(templateFilename),
                         qPrintable(templateFile.errorString()));
            }
        }
    }
    return true;
}

// Log message for commits/amended commits to go to output window
static inline QString msgCommitted(const QString &amendSHA1, int fileCount)
{
    if (amendSHA1.isEmpty())
        return GitClient::tr("Committed %n file(s).\n", 0, fileCount);
    if (fileCount)
        return GitClient::tr("Amended %1 (%n file(s)).\n", 0, fileCount).arg(amendSHA1);
    return GitClient::tr("Amended %1.").arg(amendSHA1);
}

// addAndCommit:
bool GitClient::addAndCommit(const QString &repositoryDirectory,
                             const GitSubmitEditorPanelData &data,
                             const QString &amendSHA1,
                             const QString &messageFile,
                             const QStringList &checkedFiles,
                             const QStringList &origCommitFiles,
                             const QStringList &origDeletedFiles)
{
    if (Git::Constants::debug)
        qDebug() << "GitClient::addAndCommit:" << repositoryDirectory << checkedFiles << origCommitFiles;
    const QString renamedSeparator = QLatin1String(" -> ");
    const bool amend = !amendSHA1.isEmpty();

    // Do we need to reset any files that had been added before
    // (did the user uncheck any previously added files)
    // Split up  renamed files ('foo.cpp -> foo2.cpp').
    QStringList resetFiles = origCommitFiles.toSet().subtract(checkedFiles.toSet()).toList();
    for (QStringList::iterator it = resetFiles.begin(); it != resetFiles.end(); ++it) {
        const int renamedPos = it->indexOf(renamedSeparator);
        if (renamedPos != -1) {
            const QString newFile = it->mid(renamedPos + renamedSeparator.size());
            it->truncate(renamedPos);
            it = resetFiles.insert(++it, newFile);
        }
    }

    if (!resetFiles.isEmpty())
        if (!synchronousReset(repositoryDirectory, resetFiles))
            return false;

    // Re-add all to make sure we have the latest changes, but only add those that aren't marked
    // for deletion. Purge out renamed files ('foo.cpp -> foo2.cpp').
    QStringList addFiles = checkedFiles.toSet().subtract(origDeletedFiles.toSet()).toList();
    for (QStringList::iterator it = addFiles.begin(); it != addFiles.end(); ) {
        if (it->contains(renamedSeparator)) {
            it = addFiles.erase(it);
        } else {
            ++it;
        }
    }
    if (!addFiles.isEmpty())
        if (!synchronousAdd(repositoryDirectory, false, addFiles))
            return false;

    // Do the final commit
    QStringList args;
    args << QLatin1String("commit")
         << QLatin1String("-F") << QDir::toNativeSeparators(messageFile);
    if (amend)
        args << QLatin1String("--amend");
    const QString &authorString =  data.authorString();
    if (!authorString.isEmpty())
         args << QLatin1String("--author") << authorString;

    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(repositoryDirectory, args, &outputText, &errorText);
    if (rc) {
        outputWindow()->append(msgCommitted(amendSHA1, checkedFiles.size()));
    } else {
        outputWindow()->appendError(tr("Unable to commit %n file(s): %1\n", 0, checkedFiles.size()).arg(commandOutputFromLocal8Bit(errorText)));
    }
    return rc;
}

/* Revert: This function can be called with a file list (to revert single
 * files)  or a single directory (revert all). Qt Creator currently has only
 * 'revert single' in its VCS menus, but the code is prepared to deal with
 * reverting a directory pending a sophisticated selection dialog in the
 * VCSBase plugin. */

GitClient::RevertResult GitClient::revertI(QStringList files,
                                           bool *ptrToIsDirectory,
                                           QString *errorMessage,
                                           bool revertStaging)
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

    if ((!revertStaging || stagedFiles.empty()) && unstagedFiles.empty())
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
        m_plugin->gitVersionControl()->emitFilesChanged(files);
        break;
    case RevertCanceled:
        break;
    case RevertUnchanged: {
        const QString msg = (isDirectory || files.size() > 1) ? msgNoChangedFiles() : tr("The file is not modified.");
        outputWindow()->append(msg);
    }
        break;
    case RevertFailed:
        outputWindow()->append(errorMessage);
        break;
    }
}

bool GitClient::synchronousFetch(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("fetch"));
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VCSBase::VCSBasePlugin::SshPasswordPrompt|VCSBase::VCSBasePlugin::ShowStdOutInLogWindow
                           |VCSBase::VCSBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, arguments, flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

bool GitClient::synchronousPull(const QString &workingDirectory)
{
    return synchronousPull(workingDirectory, m_settings.pullRebase);
}

bool GitClient::synchronousPull(const QString &workingDirectory, bool rebase)
{
    QStringList arguments(QLatin1String("pull"));
    if (rebase)
        arguments << QLatin1String("--rebase");
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VCSBase::VCSBasePlugin::SshPasswordPrompt|VCSBase::VCSBasePlugin::ShowStdOutInLogWindow;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, arguments, flags);
    // Notify about changed files or abort the rebase.
    const bool ok = resp.result == Utils::SynchronousProcessResponse::Finished;
    if (ok) {
        GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
    } else {
        if (rebase)
            syncAbortPullRebase(workingDirectory);
    }
    return ok;
}

void GitClient::syncAbortPullRebase(const QString &workingDir)
{
    // Abort rebase to clean if something goes wrong
    VCSBase::VCSBaseOutputWindow *outwin = VCSBase::VCSBaseOutputWindow::instance();
    outwin->appendError(tr("The command 'git pull --rebase' failed, aborting rebase."));
    QStringList arguments;
    arguments << QLatin1String("rebase") << QLatin1String("--abort");
    QByteArray stdOut;
    QByteArray stdErr;
    const bool rc = fullySynchronousGit(workingDir, arguments, &stdOut, &stdErr, true);
    outwin->append(commandOutputFromLocal8Bit(stdOut));
    if (!rc)
        outwin->appendError(commandOutputFromLocal8Bit(stdErr));
}

// Subversion: git svn
void GitClient::synchronousSubversionFetch(const QString &workingDirectory)
{
    QStringList args;
    args << QLatin1String("svn") << QLatin1String("fetch");
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VCSBase::VCSBasePlugin::SshPasswordPrompt|VCSBase::VCSBasePlugin::ShowStdOutInLogWindow
                           |VCSBase::VCSBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp = synchronousGit(workingDirectory, args, flags);
    // Notify about changes.
    if (resp.result == Utils::SynchronousProcessResponse::Finished)
        GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
}

void GitClient::subversionLog(const QString &workingDirectory)
{
    if (Git::Constants::debug)
        qDebug() << "subversionLog" << workingDirectory;

    QStringList arguments;
    arguments << QLatin1String("svn") << QLatin1String("log");
    if (m_settings.logCount > 0)
         arguments << (QLatin1String("--limit=") + QString::number(m_settings.logCount));

    // Create a command editor, no highlighting or interaction.
    const QString title = tr("Git SVN Log");
    const QString editorId = QLatin1String(Git::Constants::C_GIT_COMMAND_LOG_EDITOR);
    const QString sourceFile = VCSBase::VCSBaseEditor::getSource(workingDirectory, QStringList());
    VCSBase::VCSBaseEditor *editor = findExistingVCSEditor("svnLog", sourceFile);
    if (!editor)
        editor = createVCSEditor(editorId, title, sourceFile, false, "svnLog", sourceFile, 0);
    executeGit(workingDirectory, arguments, editor);
}

bool GitClient::synchronousPush(const QString &workingDirectory)
{
    // Disable UNIX terminals to suppress SSH prompting.
    const unsigned flags = VCSBase::VCSBasePlugin::SshPasswordPrompt|VCSBase::VCSBasePlugin::ShowStdOutInLogWindow
                           |VCSBase::VCSBasePlugin::ShowSuccessMessage;
    const Utils::SynchronousProcessResponse resp =
            synchronousGit(workingDirectory, QStringList(QLatin1String("push")), flags);
    return resp.result == Utils::SynchronousProcessResponse::Finished;
}

QString GitClient::msgNoChangedFiles()
{
    return tr("There are no modified files.");
}

void GitClient::stashPop(const QString &workingDirectory)
{
    QStringList arguments(QLatin1String("stash"));
    arguments << QLatin1String("pop");
    GitCommand *cmd = executeGit(workingDirectory, arguments, 0, true);
    connectRepositoryChanged(workingDirectory, cmd);
}

bool GitClient::synchronousStashRestore(const QString &workingDirectory,
                                        const QString &stash,
                                        const QString &branch /* = QString()*/,
                                        QString *errorMessage)
{
    QStringList arguments(QLatin1String("stash"));
    if (branch.isEmpty()) {
        arguments << QLatin1String("apply") << stash;
    } else {
        arguments << QLatin1String("branch") << branch << stash;
    }
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        const QString nativeWorkingDir = QDir::toNativeSeparators(workingDirectory);
        const QString msg = branch.isEmpty() ?
                            tr("Unable to restore stash %1: %2").
                            arg(nativeWorkingDir, stdErr) :
                            tr("Unable to restore stash %1 to branch %2: %3").
                            arg(nativeWorkingDir, branch, stdErr);
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->append(msg);
        }
        return false;
    }
    QString output = commandOutputFromLocal8Bit(outputText);
    if (!output.isEmpty())
        outputWindow()->append(output);
    GitPlugin::instance()->gitVersionControl()->emitRepositoryChanged(workingDirectory);
    return true;
}

bool GitClient::synchronousStashRemove(const QString &workingDirectory,
                            const QString &stash /* = QString() */,
                            QString *errorMessage /* = 0 */)
{
    QStringList arguments(QLatin1String("stash"));
    if (stash.isEmpty()) {
        arguments << QLatin1String("clear");
    } else {
        arguments << QLatin1String("drop") << stash;
    }
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText);
    if (!rc) {
        const QString stdErr = commandOutputFromLocal8Bit(errorText);
        const QString nativeWorkingDir = QDir::toNativeSeparators(workingDirectory);
        const QString msg = stash.isEmpty() ?
                            tr("Unable to remove stashes of %1: %2").
                            arg(nativeWorkingDir, stdErr) :
                            tr("Unable to remove stash %1 of %2: %3").
                            arg(stash, nativeWorkingDir, stdErr);
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->append(msg);
        }
        return false;
    }
    QString output = commandOutputFromLocal8Bit(outputText);
    if (!output.isEmpty())
        outputWindow()->append(output);
    return true;
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
        const QString msg = tr("Unable retrieve stash list of %1: %2").
                            arg(QDir::toNativeSeparators(workingDirectory),
                                commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            outputWindow()->append(msg);
        }
        return false;
    }
    Stash stash;
    foreach(const QString &line, commandOutputLinesFromLocal8Bit(outputText))
        if (stash.parseStashLine(line))
            stashes->push_back(stash);
    if (Git::Constants::debug)
        qDebug() << Q_FUNC_INFO << *stashes;
    return true;
}

QString GitClient::readConfig(const QString &workingDirectory, const QStringList &configVar)
{
    QStringList arguments;
    arguments << QLatin1String("config") << configVar;

    QByteArray outputText;
    QByteArray errorText;
    if (fullySynchronousGit(workingDirectory, arguments, &outputText, &errorText, false))
        return commandOutputFromLocal8Bit(outputText);
    return QString();
}

// Read a single-line config value, return trimmed
QString GitClient::readConfigValue(const QString &workingDirectory, const QString &configVar)
{
    return readConfig(workingDirectory, QStringList(configVar)).remove(QLatin1Char('\n'));
}

bool GitClient::cloneRepository(const QString &directory,const QByteArray &url)
{
    QDir workingDirectory(directory);
    const unsigned flags = VCSBase::VCSBasePlugin::SshPasswordPrompt |
            VCSBase::VCSBasePlugin::ShowStdOutInLogWindow|
            VCSBase::VCSBasePlugin::ShowSuccessMessage;

    if (workingDirectory.exists()) {
        if (!synchronousInit(workingDirectory.path()))
            return false;

        QStringList arguments(QLatin1String("remote"));
        arguments << QLatin1String("add") << QLatin1String("origin") << url;
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0, 0, true))
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
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0, 0, true))
            return false;

        arguments.clear();
        arguments << QLatin1String("config")
                  << QLatin1String("branch.master.merge")
                  << QLatin1String("refs/heads/master");
        if (!fullySynchronousGit(workingDirectory.path(), arguments, 0, 0, true))
            return false;

        return true;
    } else {
        QStringList arguments(QLatin1String("clone"));
        arguments << url << workingDirectory.dirName();
        workingDirectory.cdUp();
        const Utils::SynchronousProcessResponse resp =
                synchronousGit(workingDirectory.path(), arguments, flags);
        return resp.result == Utils::SynchronousProcessResponse::Finished;
    }
}

QString GitClient::vcsGetRepositoryURL(const QString &directory)
{
    QStringList arguments(QLatin1String("config"));
    QByteArray outputText;

    arguments << QLatin1String("remote.origin.url");

    if (fullySynchronousGit(directory, arguments, &outputText, 0, false)) {
        return commandOutputFromLocal8Bit(outputText);
    }
    return QString();
}

GitSettings GitClient::settings() const
{
    return m_settings;
}

void GitClient::setSettings(const GitSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *coreSettings = m_core->settings())
            m_settings.toSettings(coreSettings);
        m_binaryPath = m_settings.gitBinaryPath();
        m_cachedGitVersion = 0u;
        m_hasCachedGitVersion = false;
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

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::gitVersion(bool silent, QString *errorMessage  /* = 0 */)
{
    if (!m_hasCachedGitVersion) {
        // Do not execute repeatedly if that fails (due to git
        // not being installed) until settings are changed.
        m_cachedGitVersion = synchronousGitVersion(silent, errorMessage);
        m_hasCachedGitVersion = true;
    }
    return m_cachedGitVersion;
}

QString GitClient::gitVersionString(bool silent, QString *errorMessage)
{
    if (const unsigned version = gitVersion(silent, errorMessage)) {
        QString rc;
        QTextStream(&rc) << (version >> 16) << '.'
                << (0xFF & (version >> 8)) << '.'
                << (version & 0xFF);
        return rc;
    }
    return QString();
}

// determine version as '(major << 16) + (minor << 8) + patch' or 0.
unsigned GitClient::synchronousGitVersion(bool silent, QString *errorMessage /* = 0 */)
{
    // run git --version
    QByteArray outputText;
    QByteArray errorText;
    const bool rc = fullySynchronousGit(QString(), QStringList("--version"), &outputText, &errorText);
    if (!rc) {
        const QString msg = tr("Unable to determine git version: %1").arg(commandOutputFromLocal8Bit(errorText));
        if (errorMessage) {
            *errorMessage = msg;
        } else {
            if (silent) {
                outputWindow()->append(msg);
            } else {
                outputWindow()->appendError(msg);
            }
        }
        return 0;
    }
    // cut 'git version 1.6.5.1.sha'
    const QString output = commandOutputFromLocal8Bit(outputText);
    const QRegExp versionPattern(QLatin1String("^[^\\d]+([\\d])\\.([\\d])\\.([\\d]).*$"));
    QTC_ASSERT(versionPattern.isValid(), return 0);
    QTC_ASSERT(versionPattern.exactMatch(output), return 0);
    const unsigned major = versionPattern.cap(1).toUInt();
    const unsigned minor = versionPattern.cap(2).toUInt();
    const unsigned patch = versionPattern.cap(3).toUInt();
    return version(major, minor, patch);
}

} // namespace Internal
} // namespace Git
