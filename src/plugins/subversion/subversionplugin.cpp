// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "subversionplugin.h"

#include "subversionclient.h"
#include "subversionconstants.h"
#include "subversioneditor.h"
#include "subversionsettings.h"
#include "subversionsubmiteditor.h"
#include "subversiontr.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/messagemanager.h>

#include <texteditor/textdocument.h>

#include <utils/algorithm.h>
#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <QAction>
#include <QDebug>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QInputDialog>
#include <QMenu>
#include <QMessageBox>
#include <QProcessEnvironment>
#include <QTextCodec>
#include <QUrl>
#include <QXmlStreamReader>

#include <climits>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace Utils;
using namespace VcsBase;
using namespace std::placeholders;

namespace Subversion {
namespace Internal {

const char CMD_ID_SUBVERSION_MENU[]    = "Subversion.Menu";
const char CMD_ID_ADD[]                = "Subversion.Add";
const char CMD_ID_DELETE_FILE[]        = "Subversion.Delete";
const char CMD_ID_REVERT[]             = "Subversion.Revert";
const char CMD_ID_DIFF_PROJECT[]       = "Subversion.DiffAll";
const char CMD_ID_DIFF_CURRENT[]       = "Subversion.DiffCurrent";
const char CMD_ID_COMMIT_ALL[]         = "Subversion.CommitAll";
const char CMD_ID_REVERT_ALL[]         = "Subversion.RevertAll";
const char CMD_ID_COMMIT_CURRENT[]     = "Subversion.CommitCurrent";
const char CMD_ID_FILELOG_CURRENT[]    = "Subversion.FilelogCurrent";
const char CMD_ID_ANNOTATE_CURRENT[]   = "Subversion.AnnotateCurrent";
const char CMD_ID_STATUS[]             = "Subversion.Status";
const char CMD_ID_PROJECTLOG[]         = "Subversion.ProjectLog";
const char CMD_ID_REPOSITORYLOG[]      = "Subversion.RepositoryLog";
const char CMD_ID_REPOSITORYUPDATE[]   = "Subversion.RepositoryUpdate";
const char CMD_ID_REPOSITORYDIFF[]     = "Subversion.RepositoryDiff";
const char CMD_ID_REPOSITORYSTATUS[]   = "Subversion.RepositoryStatus";
const char CMD_ID_UPDATE[]             = "Subversion.Update";
const char CMD_ID_COMMIT_PROJECT[]     = "Subversion.CommitProject";
const char CMD_ID_DESCRIBE[]           = "Subversion.Describe";

const VcsBaseSubmitEditorParameters submitParameters {
    Constants::SUBVERSION_SUBMIT_MIMETYPE,
    Constants::SUBVERSION_COMMIT_EDITOR_ID,
    Constants::SUBVERSION_COMMIT_EDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    Constants::SUBVERSION_LOG_EDITOR_ID,
    Constants::SUBVERSION_LOG_EDITOR_DISPLAY_NAME,
    Constants::SUBVERSION_LOG_MIMETYPE
};

const VcsBaseEditorParameters blameEditorParameters {
    AnnotateOutput,
    Constants::SUBVERSION_BLAME_EDITOR_ID,
    Constants::SUBVERSION_BLAME_EDITOR_DISPLAY_NAME,
    Constants::SUBVERSION_BLAME_MIMETYPE
};

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
}

// Parse "svn status" output for added/conflicted/deleted/modified files
// "M<7blanks>file"
using StatusList = QList<SubversionSubmitEditor::StatusFilePair>;

StatusList parseStatusOutput(const QString &output)
{
    StatusList changeSet;
    const QString newLine = QString(QLatin1Char('\n'));
    const QStringList list = output.split(newLine, Qt::SkipEmptyParts);
    for (const QString &l : list) {
        const QString line =l.trimmed();
        if (line.size() > 8) {
            const QByteArray state = line.left(1).toLatin1();
            if (state == FileAddedC || state == FileConflictedC
                    || state == FileDeletedC || state == FileModifiedC) {
                const QString fileName = line.mid(7); // Column 8 starting from svn 1.6
                changeSet.push_back(SubversionSubmitEditor::StatusFilePair(QLatin1String(state),
                                                                           fileName.trimmed()));
            }

        }
    }
    return changeSet;
}

// Return a list of names for the internal svn directories
static inline QStringList svnDirectories()
{
    QStringList rc(QLatin1String(".svn"));
    if (HostOsInfo::isWindowsHost())
        // Option on Windows systems to avoid hassle with some IDEs
        rc.push_back(QLatin1String("_svn"));
    return rc;
}

class SubversionPluginPrivate;

class SubversionTopicCache : public Core::IVersionControl::TopicCache
{
public:
    SubversionTopicCache(SubversionPluginPrivate *plugin) :
        m_plugin(plugin)
    { }

protected:
    FilePath trackFile(const FilePath &repository) override;

    QString refreshTopic(const FilePath &repository) override;

private:
    SubversionPluginPrivate *m_plugin;
};

class SubversionPluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
public:
    SubversionPluginPrivate();
    ~SubversionPluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Utils::Id id() const final;
    bool isVcsFileOrDirectory(const FilePath &filePath) const final;

    bool managesDirectory(const FilePath &directory, FilePath *topLevel) const final;
    bool managesFile(const FilePath &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    bool vcsOpen(const FilePath &filePath) final;
    bool vcsAdd(const FilePath &filePath) final;
    bool vcsDelete(const FilePath &filePath) final;
    bool vcsMove(const FilePath &from, const FilePath &to) final;
    bool vcsCreateRepository(const FilePath &directory) final;

    void vcsAnnotate(const FilePath &file, int line) final;
    void vcsDescribe(const FilePath &source, const QString &changeNr) final;

    VcsCommand *createInitialCheckoutCommand(const QString &url,
                                             const Utils::FilePath &baseDirectory,
                                             const QString &localName,
                                             const QStringList &extraArgs) final;

    bool isVcsDirectory(const Utils::FilePath &fileName) const;

    ///
    SubversionClient *client();

    SubversionSubmitEditor *openSubversionSubmitEditor(const QString &fileName);

    // IVersionControl
    bool vcsAdd(const FilePath &workingDir, const QString &fileName);
    bool vcsDelete(const FilePath &workingDir, const QString &fileName);
    bool vcsMove(const FilePath &workingDir, const QString &from, const QString &to);
    bool vcsCheckout(const FilePath &directory, const QByteArray &url);

    static SubversionPluginPrivate *instance();

    QString monitorFile(const FilePath &repository) const;
    QString synchronousTopic(const FilePath &repository) const;
    CommandResult runSvn(const FilePath &workingDir, const CommandLine &command,
                         RunFlags flags = RunFlags::None, QTextCodec *outputCodec = nullptr,
                         int timeoutMutiplier = 1) const;
    void vcsAnnotateHelper(const FilePath &workingDir, const QString &file,
                           const QString &revision = {}, int lineNumber = -1);

protected:
    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) override;
    bool activateCommit() override;
    void discardCommit() override { cleanCommitMessageFile(); }

private:
    void addCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void cleanCommitMessageFile();
    void startCommitAll();
    void startCommitProject();
    void startCommitCurrentFile();
    void revertAll();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void projectStatus();
    void slotDescribe();
    void updateProject();
    void diffCommitFiles(const QStringList &);
    void logProject();
    void logRepository();
    void diffRepository();
    void statusRepository();
    void updateRepository();

    inline bool isCommitEditorOpen() const;
    Core::IEditor *showOutputInEditor(const QString &title, const QString &output,
                                      Id id, const FilePath &source,
                                      QTextCodec *codec);

    void filelog(const FilePath &workingDir,
                 const QString &file = {},
                 bool enableAnnotationContextMenu = false);
    void svnStatus(const FilePath &workingDir, const QString &relativePath = {});
    void svnUpdate(const FilePath &workingDir, const QString &relativePath = {});
    bool checkSVNSubDir(const QDir &directory) const;
    void startCommit(const FilePath &workingDir, const QStringList &files = {});

    const QStringList m_svnDirectories;

    SubversionClient *m_client = nullptr;
    QString m_commitMessageFileName;
    FilePath m_commitRepository;

    Core::CommandLocator *m_commandLocator = nullptr;
    Utils::ParameterAction *m_addAction = nullptr;
    Utils::ParameterAction *m_deleteAction = nullptr;
    Utils::ParameterAction *m_revertAction = nullptr;
    Utils::ParameterAction *m_diffProjectAction = nullptr;
    Utils::ParameterAction *m_diffCurrentAction = nullptr;
    Utils::ParameterAction *m_logProjectAction = nullptr;
    QAction *m_logRepositoryAction = nullptr;
    QAction *m_commitAllAction = nullptr;
    QAction *m_revertRepositoryAction = nullptr;
    QAction *m_diffRepositoryAction = nullptr;
    QAction *m_statusRepositoryAction = nullptr;
    QAction *m_updateRepositoryAction = nullptr;
    Utils::ParameterAction *m_commitCurrentAction = nullptr;
    Utils::ParameterAction *m_filelogCurrentAction = nullptr;
    Utils::ParameterAction *m_annotateCurrentAction = nullptr;
    Utils::ParameterAction *m_statusProjectAction = nullptr;
    Utils::ParameterAction *m_updateProjectAction = nullptr;
    Utils::ParameterAction *m_commitProjectAction = nullptr;
    QAction *m_describeAction = nullptr;

    QAction *m_menuAction = nullptr;

public:
    VcsSubmitEditorFactory submitEditorFactory {
        submitParameters,
        [] { return new SubversionSubmitEditor; },
        this
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new SubversionEditorWidget; },
        std::bind(&SubversionPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory blameEditorFactory {
        &blameEditorParameters,
        [] { return new SubversionEditorWidget; },
        std::bind(&SubversionPluginPrivate::vcsDescribe, this, _1, _2)
    };
};


// ------------- SubversionPlugin

static SubversionPluginPrivate *dd = nullptr;

SubversionPlugin::~SubversionPlugin()
{
    delete dd;
    dd = nullptr;
}

SubversionPluginPrivate::~SubversionPluginPrivate()
{
    cleanCommitMessageFile();
    delete m_client;
}

void SubversionPluginPrivate::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}

bool SubversionPluginPrivate::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

void SubversionPlugin::initialize()
{
    dd = new SubversionPluginPrivate;
}

void SubversionPlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
}

SubversionPluginPrivate::SubversionPluginPrivate()
    : VcsBasePluginPrivate(Context(Constants::SUBVERSION_CONTEXT)),
      m_svnDirectories(svnDirectories())
{
    dd = this;

    m_client = new SubversionClient();

    setTopicCache(new SubversionTopicCache(this));

    using namespace Constants;
    using namespace Core::Constants;
    Context context(SUBVERSION_CONTEXT);

    const QString prefix = QLatin1String("svn");
    m_commandLocator = new CommandLocator("Subversion", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a Subversion version control operation."));

    // Register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *subversionMenu = ActionManager::createMenu(Id(CMD_ID_SUBVERSION_MENU));
    subversionMenu->menu()->setTitle(Tr::tr("&Subversion"));
    toolsContainer->addMenu(subversionMenu);
    m_menuAction = subversionMenu->menu()->menuAction();
    Command *command;

    m_diffCurrentAction = new ParameterAction(Tr::tr("Diff Current File"), Tr::tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+S,Meta+D") : Tr::tr("Alt+S,Alt+D")));
    connect(m_diffCurrentAction, &QAction::triggered, this, &SubversionPluginPrivate::diffCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(Tr::tr("Filelog Current File"), Tr::tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_filelogCurrentAction, &QAction::triggered, this, &SubversionPluginPrivate::filelogCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(Tr::tr("Annotate Current File"), Tr::tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateCurrentAction, &QAction::triggered, this, &SubversionPluginPrivate::annotateCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(context);

    m_addAction = new ParameterAction(Tr::tr("Add"), Tr::tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD,
        context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+S,Meta+A") : Tr::tr("Alt+S,Alt+A")));
    connect(m_addAction, &QAction::triggered, this, &SubversionPluginPrivate::addCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new ParameterAction(Tr::tr("Commit Current File"), Tr::tr("Commit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+S,Meta+C") : Tr::tr("Alt+S,Alt+C")));
    connect(m_commitCurrentAction, &QAction::triggered, this, &SubversionPluginPrivate::startCommitCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(Tr::tr("Delete..."), Tr::tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &SubversionPluginPrivate::promptToDeleteCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new ParameterAction(Tr::tr("Revert..."), Tr::tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertAction, CMD_ID_REVERT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertAction, &QAction::triggered, this, &SubversionPluginPrivate::revertCurrentFile);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(context);

    m_diffProjectAction = new ParameterAction(Tr::tr("Diff Project"), Tr::tr("Diff Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_diffProjectAction, &QAction::triggered, this, &SubversionPluginPrivate::diffProject);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new ParameterAction(Tr::tr("Project Status"), Tr::tr("Status of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusProjectAction, CMD_ID_STATUS,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_statusProjectAction, &QAction::triggered, this, &SubversionPluginPrivate::projectStatus);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(Tr::tr("Log Project"), Tr::tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &SubversionPluginPrivate::logProject);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new ParameterAction(Tr::tr("Update Project"), Tr::tr("Update Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE, context);
    connect(m_updateProjectAction, &QAction::triggered, this, &SubversionPluginPrivate::updateProject);
    command->setAttribute(Command::CA_UpdateText);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new ParameterAction(Tr::tr("Commit Project"), Tr::tr("Commit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitProjectAction, CMD_ID_COMMIT_PROJECT, context);
    connect(m_commitProjectAction, &QAction::triggered, this, &SubversionPluginPrivate::startCommitProject);
    command->setAttribute(Command::CA_UpdateText);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    subversionMenu->addSeparator(context);

    m_diffRepositoryAction = new QAction(Tr::tr("Diff Repository"), this);
    command = ActionManager::registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, context);
    connect(m_diffRepositoryAction, &QAction::triggered, this, &SubversionPluginPrivate::diffRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(Tr::tr("Repository Status"), this);
    command = ActionManager::registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, context);
    connect(m_statusRepositoryAction, &QAction::triggered, this, &SubversionPluginPrivate::statusRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(Tr::tr("Log Repository"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &SubversionPluginPrivate::logRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(Tr::tr("Update Repository"), this);
    command = ActionManager::registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, context);
    connect(m_updateRepositoryAction, &QAction::triggered, this, &SubversionPluginPrivate::updateRepository);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(Tr::tr("Commit All Files"), this);
    command = ActionManager::registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        context);
    connect(m_commitAllAction, &QAction::triggered, this, &SubversionPluginPrivate::startCommitAll);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_describeAction = new QAction(Tr::tr("Describe..."), this);
    command = ActionManager::registerAction(m_describeAction, CMD_ID_DESCRIBE, context);
    connect(m_describeAction, &QAction::triggered, this, &SubversionPluginPrivate::slotDescribe);
    subversionMenu->addAction(command);

    m_revertRepositoryAction = new QAction(Tr::tr("Revert Repository..."), this);
    command = ActionManager::registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
        context);
    connect(m_revertRepositoryAction, &QAction::triggered, this, &SubversionPluginPrivate::revertAll);
    subversionMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    connect(&settings(), &AspectContainer::applied, this, &IVersionControl::configurationChanged);
}

bool SubversionPluginPrivate::isVcsDirectory(const FilePath &fileName) const
{
    const QString baseName = fileName.fileName();
    return fileName.isDir() && contains(m_svnDirectories, [baseName](const QString &s) {
        return !baseName.compare(s, HostOsInfo::fileNameCaseSensitivity());
    });
}

SubversionClient *SubversionPluginPrivate::client()
{
    return m_client;
}

bool SubversionPluginPrivate::activateCommit()
{
    if (!isCommitEditorOpen())
        return true;

    auto editor = qobject_cast<SubversionSubmitEditor *>(submitEditor());
    QTC_ASSERT(editor, return true);
    IDocument *editorDocument = editor->document();
    QTC_ASSERT(editorDocument, return true);

    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile = editorDocument->filePath().toFileInfo();
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        closeEditor = DocumentManager::saveDocument(editorDocument)
                && m_client->doCommit(m_commitRepository, fileList, m_commitMessageFileName);
        if (closeEditor)
            cleanCommitMessageFile();
    }
    return closeEditor;
}

void SubversionPluginPrivate::diffCommitFiles(const QStringList &files)
{
    m_client->diff(m_commitRepository, files, QStringList());
}

SubversionSubmitEditor *SubversionPluginPrivate::openSubversionSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(FilePath::fromString(fileName),
                                                Constants::SUBVERSION_COMMIT_EDITOR_ID);
    auto submitEditor = qobject_cast<SubversionSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    setSubmitEditor(submitEditor);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &SubversionPluginPrivate::diffCommitFiles);
    submitEditor->setCheckScriptWorkingDirectory(m_commitRepository);
    return submitEditor;
}

void SubversionPluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);

    const QString projectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(projectName);
    m_statusProjectAction->setParameter(projectName);
    m_updateProjectAction->setParameter(projectName);
    m_logProjectAction->setParameter(projectName);
    m_commitProjectAction->setParameter(projectName);

    const bool repoEnabled = currentState().hasTopLevel();
    m_commitAllAction->setEnabled(repoEnabled);
    m_describeAction->setEnabled(repoEnabled);
    m_revertRepositoryAction->setEnabled(repoEnabled);
    m_diffRepositoryAction->setEnabled(repoEnabled);
    m_statusRepositoryAction->setEnabled(repoEnabled);
    m_updateRepositoryAction->setEnabled(repoEnabled);

    const QString fileName = currentState().currentFileName();

    m_addAction->setParameter(fileName);
    m_deleteAction->setParameter(fileName);
    m_revertAction->setParameter(fileName);
    m_diffCurrentAction->setParameter(fileName);
    m_commitCurrentAction->setParameter(fileName);
    m_filelogCurrentAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
}

void SubversionPluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPluginPrivate::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString title = Tr::tr("Revert repository");
    if (QMessageBox::warning(ICore::dialogParent(), title,
                             Tr::tr("Revert all pending changes to the repository?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No)
        return;
    // NoteL: Svn "revert ." doesn not work.
    CommandLine args{settings().binaryPath(), {"revert"}};
    args << SubversionClient::AddAuthOptions();
    args << QLatin1String("--recursive") << state.topLevel().toString();
    const auto revertResponse = runSvn(state.topLevel(), args, RunFlags::ShowStdOut);
    if (revertResponse.result() != ProcessResult::FinishedWithSuccess) {
        QMessageBox::warning(ICore::dialogParent(), title, Tr::tr("Revert failed: %1")
                             .arg(revertResponse.exitMessage()), QMessageBox::Ok);
        return;
    }
    emit repositoryChanged(state.topLevel());
}

void SubversionPluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);

    CommandLine diffArgs{settings().binaryPath(), {"diff"}};
    diffArgs << SubversionClient::AddAuthOptions();
    diffArgs << SubversionClient::escapeFile(state.relativeCurrentFile());

    const auto diffResponse = runSvn(state.currentFileTopLevel(), diffArgs);
    if (diffResponse.result() != ProcessResult::FinishedWithSuccess)
        return;
    if (diffResponse.cleanedStdOut().isEmpty())
        return;
    if (QMessageBox::warning(ICore::dialogParent(), QLatin1String("svn revert"),
                             Tr::tr("The file has been changed. Do you want to revert it?"),
                             QMessageBox::Yes, QMessageBox::No) == QMessageBox::No) {
        return;
    }

    FileChangeBlocker fcb(state.currentFile());

    // revert
    CommandLine args{settings().binaryPath(), {"revert"}};
    args << SubversionClient::AddAuthOptions();
    args << SubversionClient::escapeFile(state.relativeCurrentFile());

    const auto revertResponse = runSvn(state.currentFileTopLevel(), args, RunFlags::ShowStdOut);
    if (revertResponse.result() == ProcessResult::FinishedWithSuccess)
        emit filesChanged(QStringList(state.currentFile().toString()));
}

void SubversionPluginPrivate::diffProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    m_client->diff(state.currentProjectTopLevel(),
                   relativeProject.isEmpty() ? QStringList() : QStringList(relativeProject), {});
}

void SubversionPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), {});
}

void SubversionPluginPrivate::startCommitCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    startCommit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void SubversionPluginPrivate::startCommitAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

void SubversionPluginPrivate::startCommitProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    startCommit(state.currentProjectPath());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void SubversionPluginPrivate::startCommit(const FilePath &workingDir, const QStringList &files)
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(Tr::tr("Another commit is currently being executed."));
        return;
    }

    CommandLine args{settings().binaryPath(), {"status"}};
    args << SubversionClient::AddAuthOptions();
    args << SubversionClient::escapeFiles(files);

    const auto response = runSvn(workingDir, args);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;

    // Get list of added/modified/deleted files
    const StatusList statusOutput = parseStatusOutput(response.cleanedStdOut());
    if (statusOutput.empty()) {
        VcsOutputWindow::appendWarning(Tr::tr("There are no modified files."));
        return;
    }
    m_commitRepository = workingDir;
    // Create a new submit change file containing the submit template
    TempFileSaver saver;
    saver.setAutoRemove(false);
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.filePath().toString();
    // Create a submit editor and set file list
    SubversionSubmitEditor *editor = openSubversionSubmitEditor(m_commitMessageFileName);
    QTC_ASSERT(editor, return);
    editor->setStatusList(statusOutput);
}

void SubversionPluginPrivate::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void SubversionPluginPrivate::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel());
}

void SubversionPluginPrivate::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel(), QStringList(), QStringList());
}

void SubversionPluginPrivate::statusRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    svnStatus(state.topLevel());
}

void SubversionPluginPrivate::updateRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    svnUpdate(state.topLevel());
}

void SubversionPluginPrivate::svnStatus(const FilePath &workingDir, const QString &relativePath)
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    CommandLine args{settings().binaryPath(), {"status"}};
    args << SubversionClient::AddAuthOptions();
    if (!relativePath.isEmpty())
        args << SubversionClient::escapeFile(relativePath);
    VcsOutputWindow::setRepository(workingDir);
    runSvn(workingDir, args, RunFlags::ShowStdOut | RunFlags::ShowSuccessMessage);
    VcsOutputWindow::clearRepository();
}

void SubversionPluginPrivate::filelog(const FilePath &workingDir,
                                      const QString &file,
                                      bool enableAnnotationContextMenu)
{
    m_client->log(workingDir, QStringList(file), QStringList(), enableAnnotationContextMenu,
                  [](CommandLine &command) { command << SubversionClient::AddAuthOptions(); });
}

void SubversionPluginPrivate::updateProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    svnUpdate(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPluginPrivate::svnUpdate(const FilePath &workingDir, const QString &relativePath)
{
    CommandLine args{settings().binaryPath(), {"update"}};
    args << SubversionClient::AddAuthOptions();
    args << Constants::NON_INTERACTIVE_OPTION;
    if (!relativePath.isEmpty())
        args << relativePath;
    const auto response = runSvn(workingDir, args, RunFlags::ShowStdOut, nullptr, 10);
    if (response.result() == ProcessResult::FinishedWithSuccess)
        emit repositoryChanged(workingDir);
}

void SubversionPluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAnnotateHelper(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void SubversionPluginPrivate::vcsAnnotateHelper(const FilePath &workingDir, const QString &file,
                                                const QString &revision /* = QString() */,
                                                int lineNumber /* = -1 */)
{
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);
    QTextCodec *codec = VcsBaseEditor::getCodec(source);

    CommandLine args{settings().binaryPath(), {"annotate"}};
    args << SubversionClient::AddAuthOptions();
    if (settings().spaceIgnorantAnnotation())
        args << "-x" << "-uw";
    if (!revision.isEmpty())
        args << "-r" << revision;
    args << "-v" << QDir::toNativeSeparators(SubversionClient::escapeFile(file));

    const auto response = runSvn(workingDir, args, RunFlags::ForceCLocale, codec);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber <= 0)
        lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(source);
    // Determine id
    const QStringList files = QStringList(file);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, revision);
    const QString tag = VcsBaseEditor::editorTag(AnnotateOutput, workingDir, files);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.cleanedStdOut().toUtf8());
        VcsBaseEditor::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("svn annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.cleanedStdOut(),
                                                blameEditorParameters.id, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        VcsBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void SubversionPluginPrivate::projectStatus()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    svnStatus(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void SubversionPluginPrivate::vcsDescribe(const FilePath &source, const QString &changeNr)
{
    // To describe a complete change, find the top level and then do
    //svn diff -r 472958:472959 <top level>
    const QFileInfo fi = source.toFileInfo();
    FilePath topLevel;
    const bool manages = managesDirectory(fi.isDir() ? source : FilePath::fromString(fi.absolutePath()), &topLevel);
    if (!manages || topLevel.isEmpty())
        return;
    if (Subversion::Constants::debug)
        qDebug() << Q_FUNC_INFO << source << topLevel << changeNr;
    // Number must be >= 1
    bool ok;

    const int number = changeNr.toInt(&ok);
    if (!ok || number < 1)
        return;

    const QString title = QString::fromLatin1("svn describe %1#%2").arg(fi.fileName(), changeNr);

    m_client->describe(topLevel, number, title);
}

void SubversionPluginPrivate::slotDescribe()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);

    QInputDialog inputDialog(ICore::dialogParent());
    inputDialog.setInputMode(QInputDialog::IntInput);
    inputDialog.setIntRange(1, INT_MAX);
    inputDialog.setWindowTitle(Tr::tr("Describe"));
    inputDialog.setLabelText(Tr::tr("Revision number:"));
    if (inputDialog.exec() != QDialog::Accepted)
        return;

    const int revision = inputDialog.intValue();
    vcsDescribe(state.topLevel(), QString::number(revision));
}

CommandResult SubversionPluginPrivate::runSvn(const FilePath &workingDir,
                                              const CommandLine &command, RunFlags flags,
                                              QTextCodec *outputCodec, int timeoutMutiplier) const
{
    if (settings().binaryPath().isEmpty())
        return CommandResult(ProcessResult::StartFailed, Tr::tr("No subversion executable specified."));

    const int timeoutS = settings().timeout() * timeoutMutiplier;
    return m_client->vcsSynchronousExec(workingDir, command, flags, timeoutS, outputCodec);
}

IEditor *SubversionPluginPrivate::showOutputInEditor(const QString &title, const QString &output,
                                                     Id id, const FilePath &source,
                                                     QTextCodec *codec)
{
    if (Subversion::Constants::debug)
        qDebug() << "SubversionPlugin::showOutputInEditor" << title << id.toString()
                 <<  "Size= " << output.size() <<  " Type=" << id << debugCodec(codec);
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    auto e = qobject_cast<SubversionEditorWidget*>(editor->widget());
    if (!e)
        return nullptr;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested,
            this, &SubversionPluginPrivate::vcsAnnotateHelper);
    e->setForceReadOnly(true);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    return editor;
}

SubversionPluginPrivate *SubversionPluginPrivate::instance()
{
    QTC_ASSERT(dd, return dd);
    return dd;
}

QString SubversionPluginPrivate::monitorFile(const FilePath &repository) const
{
    QTC_ASSERT(!repository.isEmpty(), return QString());
    QDir repoDir(repository.toString());
    for (const QString &svnDir : std::as_const(m_svnDirectories)) {
        if (repoDir.exists(svnDir)) {
            QFileInfo fi(repoDir.absoluteFilePath(svnDir + QLatin1String("/wc.db")));
            if (fi.exists() && fi.isFile())
                return fi.absoluteFilePath();
        }
    }
    return {};
}

QString SubversionPluginPrivate::synchronousTopic(const FilePath &repository) const
{
    return m_client->synchronousTopic(repository);
}

bool SubversionPluginPrivate::vcsAdd(const FilePath &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(SubversionClient::escapeFile(rawFileName));
    CommandLine args{settings().binaryPath()};
    args << "add" << SubversionClient::AddAuthOptions() << "--parents" << file;
    return runSvn(workingDir, args, RunFlags::ShowStdOut).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::vcsDelete(const FilePath &workingDir, const QString &rawFileName)
{
    const QString file = QDir::toNativeSeparators(SubversionClient::escapeFile(rawFileName));

    CommandLine args{settings().binaryPath()};
    args << "delete" << SubversionClient::AddAuthOptions() << "--force" << file;

    return runSvn(workingDir, args, RunFlags::ShowStdOut).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::vcsMove(const FilePath &workingDir, const QString &from, const QString &to)
{
    CommandLine args{settings().binaryPath(), {"move"}};
    args << SubversionClient::AddAuthOptions()
         << QDir::toNativeSeparators(SubversionClient::escapeFile(from))
         << QDir::toNativeSeparators(SubversionClient::escapeFile(to));
    return runSvn(workingDir, args, RunFlags::ShowStdOut).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::vcsCheckout(const FilePath &directory, const QByteArray &url)
{
    QUrl tempUrl = QUrl::fromEncoded(url);
    const QString username = tempUrl.userName();
    const QString password = tempUrl.password();
    CommandLine args{settings().binaryPath(), {"checkout"}};
    args << Constants::NON_INTERACTIVE_OPTION;

    if (!username.isEmpty()) {
        // If url contains username and password we have to use separate username and password
        // arguments instead of passing those in the url. Otherwise the subversion 'non-interactive'
        // authentication will always fail (if the username and password data are not stored locally),
        // if for example we are logging into a new host for the first time using svn. There seems to
        // be a bug in subversion, so this might get fixed in the future.
        tempUrl.setUserInfo({});
        args << "--username" << username;
        if (!password.isEmpty())
            args << "--password";
        args.addMaskedArg(password);
    }

    args << QString::fromLatin1(tempUrl.toEncoded()) << directory.toString();

    return runSvn(directory, args, RunFlags::None, nullptr, 10).result()
            == ProcessResult::FinishedWithSuccess;
}

bool SubversionPluginPrivate::managesDirectory(const FilePath &directory, FilePath *topLevel /* = 0 */) const
{
    const QDir dir(directory.toString());
    if (topLevel)
        topLevel->clear();

    /* Subversion >= 1.7 has ".svn" directory in the root of the working copy. Check for
     * furthest parent containing ".svn/wc.db". Need to check for furthest parent as closer
     * parents may be svn:externals. */
    QDir parentDir = dir;
    while (!parentDir.isRoot()) {
        if (checkSVNSubDir(parentDir)) {
            if (topLevel)
                *topLevel = FilePath::fromString(parentDir.absolutePath());
            return true;
        }
        if (!parentDir.cdUp())
            break;
    }

    return false;
}

bool SubversionPluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    CommandLine args{settings().binaryPath()};
    args << "status" << SubversionClient::AddAuthOptions()
         << QDir::toNativeSeparators(SubversionClient::escapeFile(fileName));
    const QString output = runSvn(workingDirectory, args).cleanedStdOut();
    return output.isEmpty() || output.front() != QLatin1Char('?');
}

// Check whether SVN management subdirs exist.
bool SubversionPluginPrivate::checkSVNSubDir(const QDir &directory) const
{
    const int dirCount = m_svnDirectories.size();
    for (int i = 0; i < dirCount; i++) {
        const QDir svnDir(directory.absoluteFilePath(m_svnDirectories.at(i)));
        if (!svnDir.exists())
            continue;
        if (!svnDir.exists(QLatin1String("wc.db")))
            continue;
        return true;
    }
    return false;
}

QString SubversionPluginPrivate::displayName() const
{
    return QLatin1String("subversion");
}

Utils::Id SubversionPluginPrivate::id() const
{
    return Utils::Id(VcsBase::Constants::VCS_ID_SUBVERSION);
}

bool SubversionPluginPrivate::isVcsFileOrDirectory(const FilePath &filePath) const
{
    return isVcsDirectory(filePath);
}

bool SubversionPluginPrivate::isConfigured() const
{
    const FilePath binary = settings().binaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool SubversionPluginPrivate::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        break;
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

bool SubversionPluginPrivate::vcsOpen(const FilePath & /* filePath */)
{
    // Open for edit: N/A
    return true;
}

bool SubversionPluginPrivate::vcsAdd(const FilePath &filePath)
{
    return vcsAdd(filePath.parentDir(), filePath.fileName());
}

bool SubversionPluginPrivate::vcsDelete(const FilePath &filePath)
{
    return vcsDelete(filePath.parentDir(), filePath.fileName());
}

bool SubversionPluginPrivate::vcsMove(const FilePath &from, const FilePath &to)
{
    const QFileInfo fromInfo = from.toFileInfo();
    const QFileInfo toInfo = to.toFileInfo();
    return vcsMove(from.parentDir(), fromInfo.absoluteFilePath(), toInfo.absoluteFilePath());
}

bool SubversionPluginPrivate::vcsCreateRepository(const FilePath &)
{
    return false;
}

void SubversionPluginPrivate::vcsAnnotate(const FilePath &filePath, int line)
{
    vcsAnnotateHelper(filePath.parentDir(), filePath.fileName(), QString(), line);
}

VcsCommand *SubversionPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                                  const Utils::FilePath &baseDirectory,
                                                                  const QString &localName,
                                                                  const QStringList &extraArgs)
{
    CommandLine args{settings().binaryPath()};
    args << "checkout";
    args << SubversionClient::AddAuthOptions();
    args << Subversion::Constants::NON_INTERACTIVE_OPTION << extraArgs << url << localName;

    auto command = VcsBaseClient::createVcsCommand(baseDirectory, m_client->processEnvironment());
    command->addJob(args, -1);
    return command;
}

FilePath SubversionTopicCache::trackFile(const FilePath &repository)
{
    return FilePath::fromString(m_plugin->monitorFile(repository));
}

QString SubversionTopicCache::refreshTopic(const FilePath &repository)
{
    return m_plugin->synchronousTopic(repository);
}


#ifdef WITH_TESTS
void SubversionPlugin::testLogResolving()
{
    QByteArray data(
                "------------------------------------------------------------------------\n"
                "r1439551 | philip | 2013-01-28 20:19:55 +0200 (Mon, 28 Jan 2013) | 4 lines\n"
                "\n"
                "* subversion/tests/cmdline/update_tests.py\n"
                "  (update_moved_dir_file_move): Resolve conflict, adjust expectations,\n"
                "   remove XFail.\n"
                "\n"
                "------------------------------------------------------------------------\n"
                "r1439540 | philip | 2013-01-28 20:06:36 +0200 (Mon, 28 Jan 2013) | 4 lines\n"
                "\n"
                "* subversion/tests/cmdline/update_tests.py\n"
                "  (update_moved_dir_edited_leaf_del): Do non-recursive resolution, adjust\n"
                "   expectations, remove XFail.\n"
                "\n"
                );
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data, "r1439551", "r1439540");
}

#endif

} // Internal
} // Subversion
