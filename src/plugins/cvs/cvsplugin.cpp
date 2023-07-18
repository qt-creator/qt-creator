// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cvsplugin.h"

#include "cvseditor.h"
#include "cvssettings.h"
#include "cvssubmiteditor.h"
#include "cvstr.h"
#include "cvsutils.h"

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseclient.h>
#include <vcsbase/vcsbaseclientsettings.h>
#include <vcsbase/vcsbaseconstants.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcsbaseeditorconfig.h>
#include <vcsbase/vcsbaseplugin.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>

#include <texteditor/textdocument.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/messagebox.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/vcsmanager.h>

#include <utils/commandline.h>
#include <utils/environment.h>
#include <utils/fileutils.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QAction>
#include <QDate>
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QTextCodec>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace Core;
using namespace VcsBase;
using namespace Utils;
using namespace std::placeholders;

namespace Cvs::Internal {

const char CVS_CONTEXT[]               = "CVS Context";
const char CMD_ID_CVS_MENU[]           = "CVS.Menu";
const char CMD_ID_ADD[]                = "CVS.Add";
const char CMD_ID_DELETE_FILE[]        = "CVS.Delete";
const char CMD_ID_EDIT_FILE[]          = "CVS.EditFile";
const char CMD_ID_UNEDIT_FILE[]        = "CVS.UneditFile";
const char CMD_ID_UNEDIT_REPOSITORY[]  = "CVS.UneditRepository";
const char CMD_ID_REVERT[]             = "CVS.Revert";
const char CMD_ID_DIFF_PROJECT[]       = "CVS.DiffAll";
const char CMD_ID_DIFF_CURRENT[]       = "CVS.DiffCurrent";
const char CMD_ID_COMMIT_ALL[]         = "CVS.CommitAll";
const char CMD_ID_REVERT_ALL[]         = "CVS.RevertAll";
const char CMD_ID_COMMIT_CURRENT[]     = "CVS.CommitCurrent";
const char CMD_ID_FILELOG_CURRENT[]    = "CVS.FilelogCurrent";
const char CMD_ID_ANNOTATE_CURRENT[]   = "CVS.AnnotateCurrent";
const char CMD_ID_STATUS[]             = "CVS.Status";
const char CMD_ID_UPDATE_DIRECTORY[]   = "CVS.UpdateDirectory";
const char CMD_ID_COMMIT_DIRECTORY[]   = "CVS.CommitDirectory";
const char CMD_ID_UPDATE[]             = "CVS.Update";
const char CMD_ID_PROJECTLOG[]         = "CVS.ProjectLog";
const char CMD_ID_PROJECTCOMMIT[]      = "CVS.ProjectCommit";
const char CMD_ID_REPOSITORYLOG[]      = "CVS.RepositoryLog";
const char CMD_ID_REPOSITORYDIFF[]     = "CVS.RepositoryDiff";
const char CMD_ID_REPOSITORYSTATUS[]   = "CVS.RepositoryStatus";
const char CMD_ID_REPOSITORYUPDATE[]   = "CVS.RepositoryUpdate";

const char CVS_SUBMIT_MIMETYPE[] = "text/vnd.qtcreator.cvs.submit";
const char CVSCOMMITEDITOR_ID[]  = "CVS Commit Editor";
const char CVSCOMMITEDITOR_DISPLAY_NAME[]  = QT_TRANSLATE_NOOP("QtC::VcsBase", "CVS Commit Editor");

const VcsBaseSubmitEditorParameters submitParameters {
    CVS_SUBMIT_MIMETYPE,
    CVSCOMMITEDITOR_ID,
    CVSCOMMITEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

const VcsBaseEditorParameters commandLogEditorParameters {
    OtherContent,
    "CVS Command Log Editor", // id
    QT_TRANSLATE_NOOP("QtC::VcsBase", "CVS Command Log Editor"), // display name
    "text/vnd.qtcreator.cvs.commandlog"
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    "CVS File Log Editor",   // id
    QT_TRANSLATE_NOOP("QtC::VcsBase", "CVS File Log Editor"),   // display name
    "text/vnd.qtcreator.cvs.log"
};

const VcsBaseEditorParameters annotateEditorParameters {
    AnnotateOutput,
    "CVS Annotation Editor",  // id
    QT_TRANSLATE_NOOP("QtC::VcsBase", "CVS Annotation Editor"),  // display name
    "text/vnd.qtcreator.cvs.annotation"
};

const VcsBaseEditorParameters diffEditorParameters {
    DiffOutput,
    "CVS Diff Editor",  // id
    QT_TRANSLATE_NOOP("QtC::VcsBase", "CVS Diff Editor"),  // display name
    "text/x-patch"
};

static inline bool messageBoxQuestion(const QString &title, const QString &question)
{
    return QMessageBox::question(ICore::dialogParent(), title, question,
                                 QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes;
}

// Parameter widget controlling whitespace diff mode, associated with a parameter
class CvsDiffConfig : public VcsBaseEditorConfig
{
public:
    CvsDiffConfig(QToolBar *toolBar)
        : VcsBaseEditorConfig(toolBar)
    {
        mapSetting(addToggleButton("-w", Tr::tr("Ignore Whitespace")),
                   &settings().diffIgnoreWhiteSpace);
        mapSetting(addToggleButton("-B", Tr::tr("Ignore Blank Lines")),
                   &settings().diffIgnoreBlankLines);
    }

    QStringList arguments() const override
    {
        return settings().diffOptions().split(' ', Qt::SkipEmptyParts)
               + VcsBaseEditorConfig::arguments();
    }
};

class CvsClient : public VcsBaseClient
{
public:
    explicit CvsClient() : VcsBaseClient(&Internal::settings())
    {
        setDiffConfigCreator([](QToolBar *toolBar) { return new CvsDiffConfig(toolBar); });
    }

    ExitCodeInterpreter exitCodeInterpreter(VcsCommandTag cmd) const override
    {
        if (cmd == DiffCommand) {
            return [](int code) {
                return (code < 0 || code > 2) ? ProcessResult::FinishedWithError
                                              : ProcessResult::FinishedWithSuccess;
            };
        }
        return {};
    }

    Utils::Id vcsEditorKind(VcsCommandTag cmd) const override
    {
        switch (cmd) {
        case DiffCommand:
            return "CVS Diff Editor"; // TODO: replace by string from cvsconstants.h
        default:
            return Utils::Id();
        }
    }
};

class CvsPluginPrivate final : public VcsBasePluginPrivate
{
public:
    CvsPluginPrivate();
    ~CvsPluginPrivate() final;

    // IVersionControl
    QString displayName() const final { return QLatin1String("cvs"); }
    Utils::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &filePath) const final;

    bool managesDirectory(const Utils::FilePath &directory, Utils::FilePath *topLevel) const final;
    bool managesFile(const Utils::FilePath &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;
    bool supportsOperation(Operation operation) const final;
    OpenSupportMode openSupportMode(const Utils::FilePath &filePath) const final;
    bool vcsOpen(const Utils::FilePath &filePath) final;
    bool vcsAdd(const Utils::FilePath &filePath) final;
    bool vcsDelete(const Utils::FilePath &filePath) final;
    bool vcsMove(const Utils::FilePath &, const Utils::FilePath &) final { return false; }
    bool vcsCreateRepository(const Utils::FilePath &directory) final;
    void vcsAnnotate(const Utils::FilePath &filePath, int line) final;

    QString vcsOpenText() const final;

    VcsCommand *createInitialCheckoutCommand(const QString &url,
                                             const Utils::FilePath &baseDirectory,
                                             const QString &localName,
                                             const QStringList &extraArgs) final;


    ///
    CvsSubmitEditor *openCVSSubmitEditor(const QString &fileName);

    // IVersionControl
    bool vcsAdd(const FilePath &workingDir, const QString &fileName);
    bool vcsDelete(const FilePath &workingDir, const QString &fileName);
    // cvs 'edit' is used to implement 'open' (cvsnt).
    bool edit(const FilePath &topLevel, const QStringList &files);

    void vcsAnnotate(const FilePath &workingDirectory, const QString &file,
                     const QString &revision, int lineNumber);
    void vcsDescribe(const Utils::FilePath &source, const QString &changeNr) final;

protected:
    void updateActions(ActionState) final;
    bool activateCommit() final;
    void discardCommit() override { cleanCommitMessageFile(); }

private:
    void addCurrentFile();
    void revertCurrentFile();
    void diffProject();
    void diffCurrentFile();
    void revertAll();
    void startCommitAll();
    void startCommitDirectory();
    void startCommitCurrentFile();
    void filelogCurrentFile();
    void annotateCurrentFile();
    void projectStatus();
    void updateDirectory();
    void updateProject();
    void diffCommitFiles(const QStringList &);
    void logProject();
    void logRepository();
    void commitProject();
    void diffRepository();
    void statusRepository();
    void updateRepository();
    void editCurrentFile();
    void uneditCurrentFile();
    void uneditCurrentRepository();

    bool isCommitEditorOpen() const;
    Core::IEditor *showOutputInEditor(const QString& title, const QString &output,
                                      Id id, const FilePath &source, QTextCodec *codec);

    CommandResult runCvs(const FilePath &workingDirectory, const QStringList &arguments,
                         RunFlags flags = RunFlags::None, QTextCodec *outputCodec = nullptr,
                         int timeoutMultiplier = 1) const;

    void annotate(const FilePath &workingDir, const QString &file,
                  const QString &revision = {}, int lineNumber = -1);
    bool describe(const QString &source, const QString &changeNr, QString *errorMessage);
    bool describe(const Utils::FilePath &toplevel, const QString &source,
                  const QString &changeNr, QString *errorMessage);
    bool describe(const Utils::FilePath &repository, QList<CvsLogEntry> entries,
                  QString *errorMessage);
    void filelog(const Utils::FilePath &workingDir, const QString &file = {},
                 bool enableAnnotationContextMenu = false);
    bool unedit(const Utils::FilePath &topLevel, const QStringList &files);
    bool status(const Utils::FilePath &topLevel, const QString &file, const QString &title);
    bool update(const Utils::FilePath &topLevel, const QString &file);
    bool checkCVSDirectory(const QDir &directory) const;
    // Quick check if files are modified
    bool diffCheckModified(const Utils::FilePath &topLevel, const QStringList &files,
                           bool *modified);
    QString findTopLevelForDirectoryI(const QString &directory) const;
    void startCommit(const Utils::FilePath &workingDir, const QString &file = {});
    bool commit(const QString &messageFile, const QStringList &subVersionFileList);
    void cleanCommitMessageFile();

    CvsClient *m_client = nullptr;

    QString m_commitMessageFileName;
    FilePath m_commitRepository;

    Core::CommandLocator *m_commandLocator = nullptr;
    Utils::ParameterAction *m_addAction = nullptr;
    Utils::ParameterAction *m_deleteAction = nullptr;
    Utils::ParameterAction *m_revertAction = nullptr;
    Utils::ParameterAction *m_editCurrentAction = nullptr;
    Utils::ParameterAction *m_uneditCurrentAction = nullptr;
    QAction *m_uneditRepositoryAction = nullptr;
    Utils::ParameterAction *m_diffProjectAction = nullptr;
    Utils::ParameterAction *m_diffCurrentAction = nullptr;
    Utils::ParameterAction *m_logProjectAction = nullptr;
    QAction *m_logRepositoryAction = nullptr;
    QAction *m_commitAllAction = nullptr;
    QAction *m_revertRepositoryAction = nullptr;
    Utils::ParameterAction *m_commitCurrentAction = nullptr;
    Utils::ParameterAction *m_filelogCurrentAction = nullptr;
    Utils::ParameterAction *m_annotateCurrentAction = nullptr;
    Utils::ParameterAction *m_statusProjectAction = nullptr;
    Utils::ParameterAction *m_updateProjectAction = nullptr;
    Utils::ParameterAction *m_commitProjectAction = nullptr;
    Utils::ParameterAction *m_updateDirectoryAction = nullptr;
    Utils::ParameterAction *m_commitDirectoryAction = nullptr;
    QAction *m_diffRepositoryAction = nullptr;
    QAction *m_updateRepositoryAction = nullptr;
    QAction *m_statusRepositoryAction = nullptr;

    QAction *m_menuAction = nullptr;

public:
    VcsSubmitEditorFactory submitEditorFactory {
        submitParameters,
        [] { return new CvsSubmitEditor; },
        this
    };

    VcsEditorFactory commandLogEditorFactory {
        &commandLogEditorParameters,
        [] { return new CvsEditorWidget; },
        std::bind(&CvsPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new CvsEditorWidget; },
        std::bind(&CvsPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory annotateEditorFactory {
        &annotateEditorParameters,
        [] { return new CvsEditorWidget; },
        std::bind(&CvsPluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory diffEditorFactory {
        &diffEditorParameters,
        [] { return new CvsEditorWidget; },
        std::bind(&CvsPluginPrivate::vcsDescribe, this, _1, _2)
    };
};

Utils::Id CvsPluginPrivate::id() const
{
    return Utils::Id(VcsBase::Constants::VCS_ID_CVS);
}

bool CvsPluginPrivate::isVcsFileOrDirectory(const Utils::FilePath &filePath) const
{
    return filePath.isDir()
            && !filePath.fileName().compare("CVS", Utils::HostOsInfo::fileNameCaseSensitivity());
}

bool CvsPluginPrivate::isConfigured() const
{
    const FilePath binary = settings().binaryPath();
    if (binary.isEmpty())
        return false;
    QFileInfo fi = binary.toFileInfo();
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool CvsPluginPrivate::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case AnnotateOperation:
    case InitialCheckoutOperation:
        break;
    case MoveOperation:
    case CreateRepositoryOperation:
    case SnapshotOperations:
        rc = false;
        break;
    }
    return rc;
}

Core::IVersionControl::OpenSupportMode CvsPluginPrivate::openSupportMode(const FilePath &filePath) const
{
    Q_UNUSED(filePath)
    return OpenOptional;
}

bool CvsPluginPrivate::vcsOpen(const FilePath &filePath)
{
    return edit(filePath.parentDir(), QStringList(filePath.fileName()));
}

bool CvsPluginPrivate::vcsAdd(const FilePath &filePath)
{
    return vcsAdd(filePath.parentDir(), filePath.fileName());
}

bool CvsPluginPrivate::vcsDelete(const FilePath &filePath)
{
    return vcsDelete(filePath.parentDir(), filePath.fileName());
}

bool CvsPluginPrivate::vcsCreateRepository(const FilePath &)
{
    return false;
}

void CvsPluginPrivate::vcsAnnotate(const FilePath &filePath, int line)
{
    vcsAnnotate(filePath.parentDir(), filePath.fileName(), {}, line);
}

QString CvsPluginPrivate::vcsOpenText() const
{
    return Tr::tr("&Edit");
}

VcsCommand *CvsPluginPrivate::createInitialCheckoutCommand(const QString &url,
                                                           const Utils::FilePath &baseDirectory,
                                                           const QString &localName,
                                                           const QStringList &extraArgs)
{
    QTC_ASSERT(localName == url, return nullptr);

    QStringList args;
    args << QLatin1String("checkout") << url << extraArgs;

    auto command = VcsBaseClient::createVcsCommand(baseDirectory, Environment::systemEnvironment());
    command->setDisplayName(Tr::tr("CVS Checkout"));
    command->addJob({settings().binaryPath(), settings().addOptions(args)}, -1);
    return command;
}

// ------------- CVSPlugin

static CvsPluginPrivate *dd = nullptr;

CvsPluginPrivate::~CvsPluginPrivate()
{
    delete m_client;
    cleanCommitMessageFile();
}

void CvsPluginPrivate::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}
bool CvsPluginPrivate::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

CvsPlugin::~CvsPlugin()
{
    delete dd;
    dd = nullptr;
}

void CvsPlugin::initialize()
{
    dd = new CvsPluginPrivate;
}

void CvsPlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
}

CvsPluginPrivate::CvsPluginPrivate()
    : VcsBasePluginPrivate(Context(CVS_CONTEXT))
{
    using namespace Core::Constants;
    dd = this;

    Context context(CVS_CONTEXT);
    m_client = new CvsClient;

    const QString prefix = QLatin1String("cvs");
    m_commandLocator = new CommandLocator("CVS", prefix, prefix, this);
    m_commandLocator->setDescription(Tr::tr("Triggers a CVS version control operation."));

    // Register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *cvsMenu = ActionManager::createMenu(Id(CMD_ID_CVS_MENU));
    cvsMenu->menu()->setTitle(Tr::tr("&CVS"));
    toolsContainer->addMenu(cvsMenu);
    m_menuAction = cvsMenu->menu()->menuAction();

    Command *command;

    m_diffCurrentAction = new ParameterAction(Tr::tr("Diff Current File"), Tr::tr("Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+C,Meta+D") : Tr::tr("Alt+C,Alt+D")));
    connect(m_diffCurrentAction, &QAction::triggered, this, &CvsPluginPrivate::diffCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new ParameterAction(Tr::tr("Filelog Current File"), Tr::tr("Filelog \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_filelogCurrentAction, &QAction::triggered, this, &CvsPluginPrivate::filelogCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(Tr::tr("Annotate Current File"), Tr::tr("Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_annotateCurrentAction, &QAction::triggered, this, &CvsPluginPrivate::annotateCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_addAction = new ParameterAction(Tr::tr("Add"), Tr::tr("Add \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addAction, CMD_ID_ADD,
        context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+C,Meta+A") : Tr::tr("Alt+C,Alt+A")));
    connect(m_addAction, &QAction::triggered, this, &CvsPluginPrivate::addCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new ParameterAction(Tr::tr("Commit Current File"), Tr::tr("Commit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? Tr::tr("Meta+C,Meta+C") : Tr::tr("Alt+C,Alt+C")));
    connect(m_commitCurrentAction, &QAction::triggered, this, &CvsPluginPrivate::startCommitCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new ParameterAction(Tr::tr("Delete..."), Tr::tr("Delete \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_deleteAction, &QAction::triggered, this, &CvsPluginPrivate::promptToDeleteCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new ParameterAction(Tr::tr("Revert..."), Tr::tr("Revert \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_revertAction, CMD_ID_REVERT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_revertAction, &QAction::triggered, this, &CvsPluginPrivate::revertCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_editCurrentAction = new ParameterAction(Tr::tr("Edit"), Tr::tr("Edit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_editCurrentAction, CMD_ID_EDIT_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_editCurrentAction, &QAction::triggered, this, &CvsPluginPrivate::editCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditCurrentAction = new ParameterAction(Tr::tr("Unedit"), Tr::tr("Unedit \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_uneditCurrentAction, CMD_ID_UNEDIT_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_uneditCurrentAction, &QAction::triggered, this, &CvsPluginPrivate::uneditCurrentFile);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditRepositoryAction = new QAction(Tr::tr("Unedit Repository"), this);
    command = ActionManager::registerAction(m_uneditRepositoryAction, CMD_ID_UNEDIT_REPOSITORY, context);
    connect(m_uneditRepositoryAction, &QAction::triggered, this, &CvsPluginPrivate::uneditCurrentRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_diffProjectAction = new ParameterAction(Tr::tr("Diff Project"), Tr::tr("Diff Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_diffProjectAction, &QAction::triggered, this, &CvsPluginPrivate::diffProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new ParameterAction(Tr::tr("Project Status"), Tr::tr("Status of Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_statusProjectAction, CMD_ID_STATUS,
        context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_statusProjectAction, &QAction::triggered, this, &CvsPluginPrivate::projectStatus);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new ParameterAction(Tr::tr("Log Project"), Tr::tr("Log Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_logProjectAction, &QAction::triggered, this, &CvsPluginPrivate::logProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new ParameterAction(Tr::tr("Update Project"), Tr::tr("Update Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateProjectAction, CMD_ID_UPDATE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateProjectAction, &QAction::triggered, this, &CvsPluginPrivate::updateProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new ParameterAction(Tr::tr("Commit Project"), Tr::tr("Commit Project \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitProjectAction, CMD_ID_PROJECTCOMMIT, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_commitProjectAction, &QAction::triggered, this, &CvsPluginPrivate::commitProject);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_updateDirectoryAction = new ParameterAction(Tr::tr("Update Directory"), Tr::tr("Update Directory \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateDirectoryAction, CMD_ID_UPDATE_DIRECTORY, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_updateDirectoryAction, &QAction::triggered, this, &CvsPluginPrivate::updateDirectory);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitDirectoryAction = new ParameterAction(Tr::tr("Commit Directory"), Tr::tr("Commit Directory \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_commitDirectoryAction,
        CMD_ID_COMMIT_DIRECTORY, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_commitDirectoryAction, &QAction::triggered, this, &CvsPluginPrivate::startCommitDirectory);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addSeparator(context);

    m_diffRepositoryAction = new QAction(Tr::tr("Diff Repository"), this);
    command = ActionManager::registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, context);
    connect(m_diffRepositoryAction, &QAction::triggered, this, &CvsPluginPrivate::diffRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(Tr::tr("Repository Status"), this);
    command = ActionManager::registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, context);
    connect(m_statusRepositoryAction, &QAction::triggered, this, &CvsPluginPrivate::statusRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(Tr::tr("Repository Log"), this);
    command = ActionManager::registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, context);
    connect(m_logRepositoryAction, &QAction::triggered, this, &CvsPluginPrivate::logRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(Tr::tr("Update Repository"), this);
    command = ActionManager::registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, context);
    connect(m_updateRepositoryAction, &QAction::triggered, this, &CvsPluginPrivate::updateRepository);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(Tr::tr("Commit All Files"), this);
    command = ActionManager::registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        context);
    connect(m_commitAllAction, &QAction::triggered, this, &CvsPluginPrivate::startCommitAll);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertRepositoryAction = new QAction(Tr::tr("Revert Repository..."), this);
    command = ActionManager::registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
                             context);
    connect(m_revertRepositoryAction, &QAction::triggered, this, &CvsPluginPrivate::revertAll);
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    connect(&settings(), &AspectContainer::applied, this, &IVersionControl::configurationChanged);
}

void CvsPluginPrivate::vcsDescribe(const FilePath &source, const QString &changeNr)
{
    QString errorMessage;
    if (!describe(source.toString(), changeNr, &errorMessage))
        VcsOutputWindow::appendError(errorMessage);
};

bool CvsPluginPrivate::activateCommit()
{
    if (!isCommitEditorOpen())
        return true;

    auto editor = qobject_cast<CvsSubmitEditor *>(submitEditor());
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
        closeEditor = DocumentManager::saveDocument(editorDocument);
        if (closeEditor)
            closeEditor = commit(m_commitMessageFileName, fileList);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void CvsPluginPrivate::diffCommitFiles(const QStringList &files)
{
    m_client->diff(m_commitRepository, files);
}

static void setDiffBaseDirectory(IEditor *editor, const FilePath &db)
{
    if (auto ve = qobject_cast<VcsBaseEditorWidget*>(editor->widget()))
        ve->setWorkingDirectory(db);
}

CvsSubmitEditor *CvsPluginPrivate::openCVSSubmitEditor(const QString &fileName)
{
    IEditor *editor = EditorManager::openEditor(FilePath::fromString(fileName), CVSCOMMITEDITOR_ID);
    auto submitEditor = qobject_cast<CvsSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &CvsPluginPrivate::diffCommitFiles);

    return submitEditor;
}

void CvsPluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }

    const bool hasTopLevel = currentState().hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);

    const QString currentFileName = currentState().currentFileName();
    m_addAction->setParameter(currentFileName);
    m_deleteAction->setParameter(currentFileName);
    m_revertAction->setParameter(currentFileName);
    m_diffCurrentAction->setParameter(currentFileName);
    m_commitCurrentAction->setParameter(currentFileName);
    m_filelogCurrentAction->setParameter(currentFileName);
    m_annotateCurrentAction->setParameter(currentFileName);
    m_editCurrentAction->setParameter(currentFileName);
    m_uneditCurrentAction->setParameter(currentFileName);

    const QString currentProjectName = currentState().currentProjectName();
    m_diffProjectAction->setParameter(currentProjectName);
    m_statusProjectAction->setParameter(currentProjectName);
    m_updateProjectAction->setParameter(currentProjectName);
    m_logProjectAction->setParameter(currentProjectName);
    m_commitProjectAction->setParameter(currentProjectName);

    // TODO: Find a more elegant way to shorten the path
    QString currentDirectoryName = currentState().currentFileDirectory().toUserOutput();
    if (currentDirectoryName.size() > 15)
        currentDirectoryName.replace(0, currentDirectoryName.size() - 15, QLatin1String("..."));
    m_updateDirectoryAction->setParameter(currentDirectoryName);
    m_commitDirectoryAction->setParameter(currentDirectoryName);

    m_diffRepositoryAction->setEnabled(hasTopLevel);
    m_statusRepositoryAction->setEnabled(hasTopLevel);
    m_updateRepositoryAction->setEnabled(hasTopLevel);
    m_commitAllAction->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);
    m_uneditRepositoryAction->setEnabled(hasTopLevel);
}

void CvsPluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CvsPluginPrivate::revertAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    const QString title = Tr::tr("Revert Repository");
    if (!messageBoxQuestion(title, Tr::tr("Revert all pending changes to the repository?")))
        return;
    const auto revertResponse = runCvs(state.topLevel(), {"update", "-C",
                                       state.topLevel().toString()}, RunFlags::ShowStdOut);
    if (revertResponse.result() != ProcessResult::FinishedWithSuccess) {
        Core::AsynchronousMessageBox::warning(title, Tr::tr("Revert failed: %1")
                                              .arg(revertResponse.exitMessage()));
        return;
    }
    emit repositoryChanged(state.topLevel());
}

void CvsPluginPrivate::revertCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const auto diffRes = runCvs(state.currentFileTopLevel(), {"diff", state.relativeCurrentFile()});
    switch (diffRes.result()) {
    case ProcessResult::FinishedWithSuccess:
        return; // Not modified, diff exit code 0
    case ProcessResult::FinishedWithError: // Diff exit code != 0
        if (diffRes.cleanedStdOut().isEmpty()) // Paranoia: Something else failed?
            return;
        break;
    default:
        return;
    }

    if (!messageBoxQuestion(QLatin1String("CVS Revert"),
                            Tr::tr("The file has been changed. Do you want to revert it?")))
        return;

    FileChangeBlocker fcb(state.currentFile());

    // revert
    const auto revertRes = runCvs(state.currentFileTopLevel(),
                           {"update", "-C", state.relativeCurrentFile()}, RunFlags::ShowStdOut);
    if (revertRes.result() == ProcessResult::FinishedWithSuccess)
        emit filesChanged(QStringList(state.currentFile().toString()));
}

void CvsPluginPrivate::diffProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    const QString relativeProject = state.relativeCurrentProject();
    m_client->diff(state.currentProjectTopLevel(),
                   relativeProject.isEmpty() ? QStringList() : QStringList(relativeProject));
}

void CvsPluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    m_client->diff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPluginPrivate::startCommitCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    /* The following has the same effect as
       startCommit(state.currentFileTopLevel(), state.relativeCurrentFile()),
       but is faster when the project has multiple directory levels */
    startCommit(state.currentFileDirectory(), state.currentFileName());
}

void CvsPluginPrivate::startCommitDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    startCommit(state.currentFileDirectory());
}

void CvsPluginPrivate::startCommitAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    startCommit(state.topLevel());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void CvsPluginPrivate::startCommit(const FilePath &workingDir, const QString &file)
{
    if (!promptBeforeCommit())
        return;
    if (raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VcsOutputWindow::appendWarning(Tr::tr("Another commit is currently being executed."));
        return;
    }

    // We need the "Examining <subdir>" stderr output to tell
    // where we are, so, have stdout/stderr channels merged.
    const auto response = runCvs(workingDir, {"status"}, RunFlags::MergeOutputChannels);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;
    // Get list of added/modified/deleted files and purge out undesired ones
    // (do not run status with relative arguments as it will omit the directories)
    StateList statusOutput = parseStatusOutput({}, response.cleanedStdOut());
    if (!file.isEmpty()) {
        for (StateList::iterator it = statusOutput.begin(); it != statusOutput.end() ; ) {
            if (file == it->second)
                ++it;
            else
                it = statusOutput.erase(it);
        }
    }
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
    CvsSubmitEditor *editor = openCVSSubmitEditor(m_commitMessageFileName);
    setSubmitEditor(editor);
    editor->setCheckScriptWorkingDirectory(m_commitRepository);
    editor->setStateList(statusOutput);
}

bool CvsPluginPrivate::commit(const QString &messageFile,
                              const QStringList &fileList)
{
    const QStringList args{"commit", "-F", messageFile};
    const auto response = runCvs(m_commitRepository, args + fileList, RunFlags::ShowStdOut, nullptr,
                                 10);
    return response.result() == ProcessResult::FinishedWithSuccess;
}

void CvsPluginPrivate::filelogCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    filelog(state.currentFileTopLevel(), state.relativeCurrentFile(), true);
}

void CvsPluginPrivate::logProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPluginPrivate::logRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    filelog(state.topLevel());
}

void CvsPluginPrivate::filelog(const FilePath &workingDir,
                               const QString &file,
                               bool enableAnnotationContextMenu)
{
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, QStringList(file));
    // no need for temp file
    const QString id = VcsBaseEditor::getTitleId(workingDir, QStringList(file));
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);
    const auto response = runCvs(workingDir, {"log", file}, RunFlags::None, codec);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBaseEditor::editorTag(LogOutput, workingDir, {file});
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.cleanedStdOut().toUtf8());
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cvs log %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.cleanedStdOut(),
                                                logEditorParameters.id, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
}

void CvsPluginPrivate::updateDirectory()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    update(state.currentFileDirectory(), {});
}

void CvsPluginPrivate::updateProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    update(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

bool CvsPluginPrivate::update(const FilePath &topLevel, const QString &file)
{
    QStringList args{"update", "-dR"};
    if (!file.isEmpty())
        args.append(file);
    const auto response = runCvs(topLevel, args, RunFlags::ShowStdOut, nullptr, 10);
    const bool ok = response.result() == ProcessResult::FinishedWithSuccess;
    if (ok)
        emit repositoryChanged(topLevel);
    return ok;
}

void CvsPluginPrivate::editCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    edit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPluginPrivate::uneditCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    unedit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CvsPluginPrivate::uneditCurrentRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    unedit(state.topLevel(), QStringList());
}

void CvsPluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CvsPluginPrivate::vcsAnnotate(const FilePath &workingDirectory, const QString &file,
                                   const QString &revision, int lineNumber)
{
    annotate(workingDirectory, file, revision, lineNumber);
}

bool CvsPluginPrivate::edit(const FilePath &topLevel, const QStringList &files)
{
    const QStringList args{"edit"};
    const auto response = runCvs(topLevel, args + files, RunFlags::ShowStdOut);
    return response.result() == ProcessResult::FinishedWithSuccess;
}

bool CvsPluginPrivate::diffCheckModified(const FilePath &topLevel, const QStringList &files,
                                         bool *modified)
{
    // Quick check for modified files using diff
    *modified = false;
    const QStringList args{"-q", "diff"};
    const auto result = runCvs(topLevel, args + files).result();
    if (result != ProcessResult::FinishedWithSuccess && result != ProcessResult::FinishedWithError)
        return false;

    *modified = result == ProcessResult::FinishedWithError;
    return true;
}

bool CvsPluginPrivate::unedit(const FilePath &topLevel, const QStringList &files)
{
    bool modified;
    // Prompt and use force flag if modified
    if (!diffCheckModified(topLevel, files, &modified))
        return false;
    if (modified) {
        const QString question = files.isEmpty() ?
                Tr::tr("Would you like to discard your changes to the repository \"%1\"?").arg(topLevel.toUserOutput()) :
                Tr::tr("Would you like to discard your changes to the file \"%1\"?").arg(files.front());
        if (!messageBoxQuestion(Tr::tr("Unedit"), question))
            return false;
    }

    QStringList args{"unedit"};
    // Note: Option '-y' to force 'yes'-answer to CVS' 'undo change' prompt,
    // exists in CVSNT only as of 6.8.2010. Standard CVS will otherwise prompt
    if (modified)
        args.append(QLatin1String("-y"));
    const auto response = runCvs(topLevel, args + files, RunFlags::ShowStdOut);
    return response.result() == ProcessResult::FinishedWithSuccess;
}

void CvsPluginPrivate::annotate(const FilePath &workingDir, const QString &file,
                                const QString &revision /* = QString() */,
                                int lineNumber /* = -1 */)
{
    const QStringList files(file);
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, files);
    const QString id = VcsBaseEditor::getTitleId(workingDir, files, revision);
    const FilePath source = VcsBaseEditor::getSource(workingDir, file);
    QStringList args{"annotate"};
    if (!revision.isEmpty())
        args << "-r" << revision;
    args << file;
    const auto response = runCvs(workingDir, args, RunFlags::None, codec);
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber < 1)
        lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(FilePath::fromString(file));

    const QString tag = VcsBaseEditor::editorTag(AnnotateOutput, workingDir, {file}, revision);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.cleanedStdOut().toUtf8());
        VcsBaseEditor::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cvs annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, response.cleanedStdOut(),
                                                annotateEditorParameters.id, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        VcsBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

bool CvsPluginPrivate::status(const FilePath &topLevel, const QString &file, const QString &title)
{
    QStringList args{"status"};
    if (!file.isEmpty())
        args.append(file);
    const auto response = runCvs(topLevel, args);
    const bool ok = response.result() == ProcessResult::FinishedWithSuccess;
    if (ok) {
        showOutputInEditor(title, response.cleanedStdOut(), commandLogEditorParameters.id,
                           topLevel, nullptr);
    }
    return ok;
}

void CvsPluginPrivate::projectStatus()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    status(state.currentProjectTopLevel(), state.relativeCurrentProject(), Tr::tr("Project status"));
}

void CvsPluginPrivate::commitProject()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);
    startCommit(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CvsPluginPrivate::diffRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    m_client->diff(state.topLevel(), QStringList());
}

void CvsPluginPrivate::statusRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    status(state.topLevel(), {}, Tr::tr("Repository status"));
}

void CvsPluginPrivate::updateRepository()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    update(state.topLevel(), {});

}

bool CvsPluginPrivate::describe(const QString &file, const QString &changeNr, QString *errorMessage)
{
    FilePath toplevel;
    const bool manages = managesDirectory(FilePath::fromString(QFileInfo(file).absolutePath()), &toplevel);
    if (!manages || toplevel.isEmpty()) {
        *errorMessage = Tr::tr("Cannot find repository for \"%1\".")
                .arg(QDir::toNativeSeparators(file));
        return false;
    }
    return describe(toplevel, QDir(toplevel.toString()).relativeFilePath(file), changeNr, errorMessage);
}

bool CvsPluginPrivate::describe(const FilePath &toplevel, const QString &file,
                                const QString &changeNr, QString *errorMessage)
{

    // In CVS, revisions of files are normally unrelated, there is
    // no global revision/change number. The only thing that groups
    // a commit is the "commit-id" (as shown in the log).
    // This function makes use of it to find all files related to
    // a commit in order to emulate a "describe global change" functionality
    // if desired.
    // Number must be > 1
    if (isFirstRevision(changeNr)) {
        *errorMessage = Tr::tr("The initial revision %1 cannot be described.").arg(changeNr);
        return false;
    }
    // Run log to obtain commit id and details
    const auto logResponse = runCvs(toplevel, {"log", QString("-r%1").arg(changeNr), file});
    if (logResponse.result() != ProcessResult::FinishedWithSuccess) {
        *errorMessage = logResponse.exitMessage();
        return false;
    }
    const QList<CvsLogEntry> fileLog = parseLogEntries(logResponse.cleanedStdOut());
    if (fileLog.empty() || fileLog.front().revisions.empty()) {
        *errorMessage = Tr::tr("Parsing of the log output failed.");
        return false;
    }
    if (settings().describeByCommitId()) {
        // Run a log command over the repo, filtering by the commit date
        // and commit id, collecting all files touched by the commit.
        const QString commitId = fileLog.front().revisions.front().commitId;
        // Date range "D1<D2" in ISO format "YYYY-MM-DD"
        const QString dateS = fileLog.front().revisions.front().date;
        const QDate date = QDate::fromString(dateS, Qt::ISODate);
        const QString nextDayS = date.addDays(1).toString(Qt::ISODate);
        const QStringList args{"log", "-d", dateS + '<' + nextDayS};
        const auto repoLogResponse = runCvs(toplevel, args, RunFlags::None, nullptr, 10);
        if (repoLogResponse.result() != ProcessResult::FinishedWithSuccess) {
            *errorMessage = repoLogResponse.exitMessage();
            return false;
        }
        // Describe all files found, pass on dir to obtain correct absolute paths.
        const QList<CvsLogEntry> repoEntries = parseLogEntries(repoLogResponse.cleanedStdOut(),
                                                               {}, commitId);
        if (repoEntries.empty()) {
            *errorMessage = Tr::tr("Could not find commits of id \"%1\" on %2.").arg(commitId, dateS);
            return false;
        }
        return describe(toplevel, repoEntries, errorMessage);
    } else {
        // Just describe that single entry
        return describe(toplevel, fileLog, errorMessage);
    }
    return false;
}

// Describe a set of files and revisions by
// concatenating log and diffs to previous revisions
bool CvsPluginPrivate::describe(const FilePath &repositoryPath,
                                QList<CvsLogEntry> entries,
                                QString *errorMessage)
{
    // Collect logs
    QString output;
    QTextCodec *codec = nullptr;
    const QList<CvsLogEntry>::iterator lend = entries.end();
    for (QList<CvsLogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        // Before fiddling file names, try to find codec
        if (!codec)
            codec = VcsBaseEditor::getCodec(repositoryPath, QStringList(it->file));
        // Run log
        const QStringList args{"log", "-r", it->revisions.front().revision, it->file};
        const auto logResponse = runCvs(repositoryPath, args);
        if (logResponse.result() != ProcessResult::FinishedWithSuccess) {
            *errorMessage =  logResponse.exitMessage();
            return false;
        }
        output += logResponse.cleanedStdOut();
    }
    // Collect diffs relative to repository
    for (QList<CvsLogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        const QString &revision = it->revisions.front().revision;
        if (!isFirstRevision(revision)) {
            const QStringList args{"diff", settings().diffOptions(),
                                   "-r", previousRevision(revision),
                                   "-r", it->revisions.front().revision, it->file};
            const auto diffResponse = runCvs(repositoryPath, args, RunFlags::None, codec);
            switch (diffResponse.result()) {
            case ProcessResult::FinishedWithSuccess:
            case ProcessResult::FinishedWithError: // Diff exit code != 0
                if (diffResponse.cleanedStdOut().isEmpty()) {
                    *errorMessage = diffResponse.exitMessage();
                    return false; // Something else failed.
                }
                break;
            default:
                *errorMessage = diffResponse.exitMessage();
                return false;
            }
            output += fixDiffOutput(diffResponse.cleanedStdOut());
        }
    }

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString commitId = entries.front().revisions.front().commitId;
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(commitId)) {
        editor->document()->setContents(output.toUtf8());
        EditorManager::activateEditor(editor);
        setDiffBaseDirectory(editor, repositoryPath);
    } else {
        const QString title = QString::fromLatin1("cvs describe %1").arg(commitId);
        IEditor *newEditor = showOutputInEditor(title, output, diffEditorParameters.id,
                                                FilePath::fromString(entries.front().file), codec);
        VcsBaseEditor::tagEditor(newEditor, commitId);
        setDiffBaseDirectory(newEditor, repositoryPath);
    }
    return true;
}

// Run CVS. At this point, file arguments must be relative to
// the working directory (see above).
CommandResult CvsPluginPrivate::runCvs(const FilePath &workingDirectory,
                                       const QStringList &arguments, RunFlags flags,
                                       QTextCodec *outputCodec, int timeoutMultiplier) const
{
    const FilePath executable = settings().binaryPath();
    if (executable.isEmpty())
        return CommandResult(ProcessResult::StartFailed, Tr::tr("No CVS executable specified."));

    const int timeoutS = settings().timeout() * timeoutMultiplier;
    return m_client->vcsSynchronousExec(workingDirectory,
                                        {executable, settings().addOptions(arguments)},
                                        flags, timeoutS, outputCodec);
}

IEditor *CvsPluginPrivate::showOutputInEditor(const QString& title, const QString &output,
                                              Utils::Id id, const FilePath &source,
                                              QTextCodec *codec)
{
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    auto e = qobject_cast<CvsEditorWidget*>(editor->widget());
    if (!e)
        return nullptr;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested, this, &CvsPluginPrivate::annotate);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    e->setForceReadOnly(true);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    return editor;
}

bool CvsPluginPrivate::vcsAdd(const FilePath &workingDir, const QString &rawFileName)
{
    const auto response = runCvs(workingDir, {"add", rawFileName}, RunFlags::ShowStdOut);
    return response.result() == ProcessResult::FinishedWithSuccess;
}

bool CvsPluginPrivate::vcsDelete(const FilePath &workingDir, const QString &rawFileName)
{
    const auto response = runCvs(workingDir, {"remove", "-f", rawFileName}, RunFlags::ShowStdOut);
    return response.result() == ProcessResult::FinishedWithSuccess;
}

/* CVS has a "CVS" directory in each directory it manages. The top level
 * is the first directory under the directory that does not have it. */
bool CvsPluginPrivate::managesDirectory(const FilePath &directory, FilePath *topLevel /* = 0 */) const
{
    if (topLevel)
        topLevel->clear();
    bool manages = false;
    const QDir dir(directory.toString());
    do {
        if (!dir.exists() || !checkCVSDirectory(dir))
            break;
        manages = true;
        if (!topLevel)
            break;
        /* Recursing up, the top level is a child of the first directory that does
         * not have a  "CVS" directory. The starting directory must be a managed
         * one. Go up and try to find the first unmanaged parent dir. */
        QDir lastDirectory = dir;
        for (QDir parentDir = lastDirectory;
             !parentDir.isRoot() && parentDir.cdUp();
             lastDirectory = parentDir) {
            if (!checkCVSDirectory(parentDir)) {
                *topLevel = FilePath::fromString(lastDirectory.absolutePath());
                break;
            }
        }
    } while (false);

    return manages;
}

bool CvsPluginPrivate::managesFile(const FilePath &workingDirectory, const QString &fileName) const
{
    const auto response = runCvs(workingDirectory, {"status", fileName});
    if (response.result() != ProcessResult::FinishedWithSuccess)
        return false;
    return !response.cleanedStdOut().contains(QLatin1String("Status: Unknown"));
}

bool CvsPluginPrivate::checkCVSDirectory(const QDir &directory) const
{
    const QString cvsDir = directory.absoluteFilePath(QLatin1String("CVS"));
    return QFileInfo(cvsDir).isDir();
}

#ifdef WITH_TESTS
void CvsPlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("Modified") << QByteArray(
            "Index: src/plugins/cvs/cvseditor.cpp\n"
            "===================================================================\n"
            "--- src/plugins/cvs/cvseditor.cpp\t21 Jan 2013 20:34:20 -0000\t1.1\n"
            "+++ src/plugins/cvs/cvseditor.cpp\t21 Jan 2013 20:34:28 -0000\n"
            "@@ -120,7 +120,7 @@\n\n")
        << QByteArray("src/plugins/cvs/cvseditor.cpp");
}

void CvsPlugin::testDiffFileResolving()
{
    VcsBaseEditorWidget::testDiffFileResolving(dd->diffEditorFactory);
}

void CvsPlugin::testLogResolving()
{
    QByteArray data(
                "RCS file: /sources/cvs/ccvs/Attic/FIXED-BUGS,v\n"
                "Working file: FIXED-BUGS\n"
                "head: 1.3\n"
                "branch:\n"
                "locks: strict\n"
                "access list:\n"
                "symbolic names:\n"
                "keyword substitution: kv\n"
                "total revisions: 3;     selected revisions: 3\n"
                "description:\n"
                "----------------------------\n"
                "revision 1.3\n"
                "date: 1995-04-29 06:22:41 +0300;  author: jimb;  state: dead;  lines: +0 -0;\n"
                "*** empty log message ***\n"
                "----------------------------\n"
                "revision 1.2\n"
                "date: 1995-04-28 18:52:24 +0300;  author: noel;  state: Exp;  lines: +6 -0;\n"
                "added latest commentary\n"
                "----------------------------\n"
                );
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data, "1.3", "1.2");
}
#endif

} // namespace Cvs::Internal
