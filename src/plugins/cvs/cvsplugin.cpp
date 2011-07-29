/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#include "cvsplugin.h"
#include "settingspage.h"
#include "cvseditor.h"
#include "cvssubmiteditor.h"
#include "cvsconstants.h"
#include "cvscontrol.h"
#include "checkoutwizard.h"

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseoutputwindow.h>
#include <vcsbase/vcsbaseeditorparameterwidget.h>
#include <locator/commandlocator.h>
#include <utils/synchronousprocess.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>

#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/filemanager.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/vcsmanager.h>
#include <utils/stringutils.h>
#include <utils/fileutils.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QDate>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTextCodec>
#include <QtCore/QtPlugin>
#include <QtGui/QAction>
#include <QtGui/QMainWindow>
#include <QtGui/QMenu>
#include <QtGui/QMessageBox>

namespace CVS {
    namespace Internal {

static inline QString msgCannotFindTopLevel(const QString &f)
{
    return CVSPlugin::tr("Cannot find repository for '%1'").
            arg(QDir::toNativeSeparators(f));
}

static inline QString msgLogParsingFailed()
{
    return CVSPlugin::tr("Parsing of the log output failed");
}

static const char * const CMD_ID_CVS_MENU    = "CVS.Menu";
static const char * const CMD_ID_ADD                = "CVS.Add";
static const char * const CMD_ID_DELETE_FILE        = "CVS.Delete";
static const char * const CMD_ID_EDIT_FILE          = "CVS.EditFile";
static const char * const CMD_ID_UNEDIT_FILE        = "CVS.UneditFile";
static const char * const CMD_ID_UNEDIT_REPOSITORY  = "CVS.UneditRepository";
static const char * const CMD_ID_REVERT             = "CVS.Revert";
static const char * const CMD_ID_SEPARATOR0         = "CVS.Separator0";
static const char * const CMD_ID_DIFF_PROJECT       = "CVS.DiffAll";
static const char * const CMD_ID_DIFF_CURRENT       = "CVS.DiffCurrent";
static const char * const CMD_ID_SEPARATOR1         = "CVS.Separator1";
static const char * const CMD_ID_COMMIT_ALL         = "CVS.CommitAll";
static const char * const CMD_ID_REVERT_ALL         = "CVS.RevertAll";
static const char * const CMD_ID_COMMIT_CURRENT     = "CVS.CommitCurrent";
static const char * const CMD_ID_SEPARATOR2         = "CVS.Separator2";
static const char * const CMD_ID_FILELOG_CURRENT    = "CVS.FilelogCurrent";
static const char * const CMD_ID_ANNOTATE_CURRENT   = "CVS.AnnotateCurrent";
static const char * const CMD_ID_STATUS             = "CVS.Status";
static const char * const CMD_ID_UPDATE             = "CVS.Update";
static const char * const CMD_ID_PROJECTLOG         = "CVS.ProjectLog";
static const char * const CMD_ID_PROJECTCOMMIT      = "CVS.ProjectCommit";
static const char * const CMD_ID_REPOSITORYLOG      = "CVS.RepositoryLog";
static const char * const CMD_ID_REPOSITORYDIFF     = "CVS.RepositoryDiff";
static const char * const CMD_ID_REPOSITORYSTATUS   = "CVS.RepositoryStatus";
static const char * const CMD_ID_REPOSITORYUPDATE   = "CVS.RepositoryUpdate";
static const char * const CMD_ID_SEPARATOR3         = "CVS.Separator3";

static const VCSBase::VCSBaseEditorParameters editorParameters[] = {
{
    VCSBase::RegularCommandOutput,
    "CVS Command Log Editor", // id
    QT_TRANSLATE_NOOP("VCS", "CVS Command Log Editor"), // display name
    "CVS Command Log Editor", // context
    "application/vnd.nokia.text.scs_cvs_commandlog",
    "scslog"},
{   VCSBase::LogOutput,
    "CVS File Log Editor",   // id
    QT_TRANSLATE_NOOP("VCS", "CVS File Log Editor"),   // display name
    "CVS File Log Editor",   // context
    "application/vnd.nokia.text.scs_cvs_filelog",
    "scsfilelog"},
{    VCSBase::AnnotateOutput,
    "CVS Annotation Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "CVS Annotation Editor"),  // display name
    "CVS Annotation Editor",  // context
    "application/vnd.nokia.text.scs_cvs_annotation",
    "scsannotate"},
{   VCSBase::DiffOutput,
    "CVS Diff Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "CVS Diff Editor"),  // display name
    "CVS Diff Editor",  // context
    "text/x-patch","diff"}
};

// Utility to find a parameter set by type
static inline const VCSBase::VCSBaseEditorParameters *findType(int ie)
{
    const VCSBase::EditorContentType et = static_cast<VCSBase::EditorContentType>(ie);
    return  VCSBase::VCSBaseEditorWidget::findType(editorParameters, sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters), et);
}

static inline QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromAscii(c->name()) : QString::fromAscii("Null codec");
}

static inline bool messageBoxQuestion(const QString &title, const QString &question, QWidget *parent = 0)
{
    return QMessageBox::question(parent, title, question, QMessageBox::Yes|QMessageBox::No) == QMessageBox::Yes;
}

// ------------- CVSPlugin
CVSPlugin *CVSPlugin::m_cvsPluginInstance = 0;

CVSPlugin::CVSPlugin() :
    VCSBase::VCSBasePlugin(QLatin1String(CVS::Constants::CVSCOMMITEDITOR_ID)),
    m_commandLocator(0),
    m_addAction(0),
    m_deleteAction(0),
    m_revertAction(0),
    m_editCurrentAction(0),
    m_uneditCurrentAction(0),
    m_uneditRepositoryAction(0),
    m_diffProjectAction(0),
    m_diffCurrentAction(0),
    m_logProjectAction(0),
    m_logRepositoryAction(0),
    m_commitAllAction(0),
    m_revertRepositoryAction(0),
    m_commitCurrentAction(0),
    m_filelogCurrentAction(0),
    m_annotateCurrentAction(0),
    m_statusProjectAction(0),
    m_updateProjectAction(0),
    m_commitProjectAction(0),
    m_diffRepositoryAction(0),
    m_updateRepositoryAction(0),
    m_statusRepositoryAction(0),
    m_submitCurrentLogAction(0),
    m_submitDiffAction(0),
    m_submitUndoAction(0),
    m_submitRedoAction(0),
    m_menuAction(0),
    m_submitActionTriggered(false)
{
}

CVSPlugin::~CVSPlugin()
{
    cleanCommitMessageFile();
}

void CVSPlugin::cleanCommitMessageFile()
{
    if (!m_commitMessageFileName.isEmpty()) {
        QFile::remove(m_commitMessageFileName);
        m_commitMessageFileName.clear();
        m_commitRepository.clear();
    }
}
bool CVSPlugin::isCommitEditorOpen() const
{
    return !m_commitMessageFileName.isEmpty();
}

static const VCSBase::VCSBaseSubmitEditorParameters submitParameters = {
    CVS::Constants::CVS_SUBMIT_MIMETYPE,
    CVS::Constants::CVSCOMMITEDITOR_ID,
    CVS::Constants::CVSCOMMITEDITOR_DISPLAY_NAME,
    CVS::Constants::CVSCOMMITEDITOR
};

static inline Core::Command *createSeparator(QObject *parent,
                                             Core::ActionManager *ami,
                                             const char *id,
                                             const Core::Context &globalcontext)
{
    QAction *tmpaction = new QAction(parent);
    tmpaction->setSeparator(true);
    return ami->registerAction(tmpaction, id, globalcontext);
}

bool CVSPlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    typedef VCSBase::VCSSubmitEditorFactory<CVSSubmitEditor> CVSSubmitEditorFactory;
    typedef VCSBase::VCSEditorFactory<CVSEditor> CVSEditorFactory;
    using namespace Constants;

    using namespace Core::Constants;
    using namespace ExtensionSystem;

    initializeVcs(new CVSControl(this));

    m_cvsPluginInstance = this;
    Core::ICore *core = Core::ICore::instance();

    if (!core->mimeDatabase()->addMimeTypes(QLatin1String(":/trolltech.cvs/CVS.mimetypes.xml"), errorMessage))
        return false;

    if (QSettings *settings = core->settings())
        m_settings.fromSettings(settings);

    addAutoReleasedObject(new SettingsPage);

    addAutoReleasedObject(new CVSSubmitEditorFactory(&submitParameters));

    static const char *describeSlotC = SLOT(slotDescribe(QString,QString));
    const int editorCount = sizeof(editorParameters)/sizeof(VCSBase::VCSBaseEditorParameters);
    for (int i = 0; i < editorCount; i++)
        addAutoReleasedObject(new CVSEditorFactory(editorParameters + i, this, describeSlotC));

    addAutoReleasedObject(new CheckoutWizard);

    const QString prefix = QLatin1String("cvs");
    m_commandLocator = new Locator::CommandLocator(QLatin1String("CVS"), prefix, prefix);
    addAutoReleasedObject(m_commandLocator);

    //register actions
    Core::ActionManager *ami = core->actionManager();
    Core::ActionContainer *toolsContainer = ami->actionContainer(M_TOOLS);

    Core::ActionContainer *cvsMenu = ami->createMenu(Core::Id(CMD_ID_CVS_MENU));
    cvsMenu->menu()->setTitle(tr("&CVS"));
    toolsContainer->addMenu(cvsMenu);
    m_menuAction = cvsMenu->menu()->menuAction();

    Core::Context globalcontext(C_GLOBAL);

    Core::Command *command;

    m_diffCurrentAction = new Utils::ParameterAction(tr("Diff Current File"), tr("Diff \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+D")));
    connect(m_diffCurrentAction, SIGNAL(triggered()), this, SLOT(diffCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_filelogCurrentAction = new Utils::ParameterAction(tr("Filelog Current File"), tr("Filelog \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_filelogCurrentAction,
        CMD_ID_FILELOG_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_filelogCurrentAction, SIGNAL(triggered()), this,
        SLOT(filelogCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new Utils::ParameterAction(tr("Annotate Current File"), tr("Annotate \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_annotateCurrentAction, SIGNAL(triggered()), this,
        SLOT(annotateCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR0, globalcontext));

    m_addAction = new Utils::ParameterAction(tr("Add"), tr("Add \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_addAction, CMD_ID_ADD,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+A")));
    connect(m_addAction, SIGNAL(triggered()), this, SLOT(addCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitCurrentAction = new Utils::ParameterAction(tr("Commit Current File"), tr("Commit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_commitCurrentAction,
        CMD_ID_COMMIT_CURRENT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(tr("Alt+C,Alt+C")));
    connect(m_commitCurrentAction, SIGNAL(triggered()), this, SLOT(startCommitCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_deleteAction = new Utils::ParameterAction(tr("Delete..."), tr("Delete \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_deleteAction, CMD_ID_DELETE_FILE,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_deleteAction, SIGNAL(triggered()), this, SLOT(promptToDeleteCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertAction = new Utils::ParameterAction(tr("Revert..."), tr("Revert \"%1\"..."), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_revertAction, CMD_ID_REVERT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_revertAction, SIGNAL(triggered()), this, SLOT(revertCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR1, globalcontext));

    m_editCurrentAction = new Utils::ParameterAction(tr("Edit"), tr("Edit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_editCurrentAction, CMD_ID_EDIT_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_editCurrentAction, SIGNAL(triggered()), this, SLOT(editCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditCurrentAction = new Utils::ParameterAction(tr("Unedit"), tr("Unedit \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_uneditCurrentAction, CMD_ID_UNEDIT_FILE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_uneditCurrentAction, SIGNAL(triggered()), this, SLOT(uneditCurrentFile()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_uneditRepositoryAction = new QAction(tr("Unedit Repository"), this);
    command = ami->registerAction(m_uneditRepositoryAction, CMD_ID_UNEDIT_REPOSITORY, globalcontext);
    connect(m_uneditRepositoryAction, SIGNAL(triggered()), this, SLOT(uneditCurrentRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR2, globalcontext));

    m_diffProjectAction = new Utils::ParameterAction(tr("Diff Project"), tr("Diff Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_diffProjectAction, CMD_ID_DIFF_PROJECT,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_diffProjectAction, SIGNAL(triggered()), this, SLOT(diffProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusProjectAction = new Utils::ParameterAction(tr("Project Status"), tr("Status of Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_statusProjectAction, CMD_ID_STATUS,
        globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_statusProjectAction, SIGNAL(triggered()), this, SLOT(projectStatus()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logProjectAction = new Utils::ParameterAction(tr("Log Project"), tr("Log Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_logProjectAction, CMD_ID_PROJECTLOG, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_logProjectAction, SIGNAL(triggered()), this, SLOT(logProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateProjectAction = new Utils::ParameterAction(tr("Update Project"), tr("Update Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_updateProjectAction, CMD_ID_UPDATE, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_updateProjectAction, SIGNAL(triggered()), this, SLOT(updateProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitProjectAction = new Utils::ParameterAction(tr("Commit Project"), tr("Commit Project \"%1\""), Utils::ParameterAction::EnabledWithParameter, this);
    command = ami->registerAction(m_commitProjectAction, CMD_ID_PROJECTCOMMIT, globalcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_commitProjectAction, SIGNAL(triggered()), this, SLOT(commitProject()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    cvsMenu->addAction(createSeparator(this, ami, CMD_ID_SEPARATOR3, globalcontext));

    m_diffRepositoryAction = new QAction(tr("Diff Repository"), this);
    command = ami->registerAction(m_diffRepositoryAction, CMD_ID_REPOSITORYDIFF, globalcontext);
    connect(m_diffRepositoryAction, SIGNAL(triggered()), this, SLOT(diffRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusRepositoryAction = new QAction(tr("Repository Status"), this);
    command = ami->registerAction(m_statusRepositoryAction, CMD_ID_REPOSITORYSTATUS, globalcontext);
    connect(m_statusRepositoryAction, SIGNAL(triggered()), this, SLOT(statusRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_logRepositoryAction = new QAction(tr("Repository Log"), this);
    command = ami->registerAction(m_logRepositoryAction, CMD_ID_REPOSITORYLOG, globalcontext);
    connect(m_logRepositoryAction, SIGNAL(triggered()), this, SLOT(logRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_updateRepositoryAction = new QAction(tr("Update Repository"), this);
    command = ami->registerAction(m_updateRepositoryAction, CMD_ID_REPOSITORYUPDATE, globalcontext);
    connect(m_updateRepositoryAction, SIGNAL(triggered()), this, SLOT(updateRepository()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_commitAllAction = new QAction(tr("Commit All Files"), this);
    command = ami->registerAction(m_commitAllAction, CMD_ID_COMMIT_ALL,
        globalcontext);
    connect(m_commitAllAction, SIGNAL(triggered()), this, SLOT(startCommitAll()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_revertRepositoryAction = new QAction(tr("Revert Repository..."), this);
    command = ami->registerAction(m_revertRepositoryAction, CMD_ID_REVERT_ALL,
                                  globalcontext);
    connect(m_revertRepositoryAction, SIGNAL(triggered()), this, SLOT(revertAll()));
    cvsMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    // Actions of the submit editor
    Core::Context cvscommitcontext(Constants::CVSCOMMITEDITOR);

    m_submitCurrentLogAction = new QAction(VCSBase::VCSBaseSubmitEditor::submitIcon(), tr("Commit"), this);
    command = ami->registerAction(m_submitCurrentLogAction, Constants::SUBMIT_CURRENT, cvscommitcontext);
    command->setAttribute(Core::Command::CA_UpdateText);
    connect(m_submitCurrentLogAction, SIGNAL(triggered()), this, SLOT(submitCurrentLog()));

    m_submitDiffAction = new QAction(VCSBase::VCSBaseSubmitEditor::diffIcon(), tr("Diff &Selected Files"), this);
    command = ami->registerAction(m_submitDiffAction , Constants::DIFF_SELECTED, cvscommitcontext);

    m_submitUndoAction = new QAction(tr("&Undo"), this);
    command = ami->registerAction(m_submitUndoAction, Core::Constants::UNDO, cvscommitcontext);

    m_submitRedoAction = new QAction(tr("&Redo"), this);
    command = ami->registerAction(m_submitRedoAction, Core::Constants::REDO, cvscommitcontext);
    return true;
}

bool CVSPlugin::submitEditorAboutToClose(VCSBase::VCSBaseSubmitEditor *submitEditor)
{
    if (!isCommitEditorOpen())
        return true;

    Core::IFile *fileIFace = submitEditor->file();
    const CVSSubmitEditor *editor = qobject_cast<CVSSubmitEditor *>(submitEditor);
    if (!fileIFace || !editor)
        return true;

    // Submit editor closing. Make it write out the commit message
    // and retrieve files
    const QFileInfo editorFile(fileIFace->fileName());
    const QFileInfo changeFile(m_commitMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    CVSSettings newSettings = m_settings;
    const VCSBase::VCSBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(tr("Closing CVS Editor"),
                                 tr("Do you want to commit the change?"),
                                 tr("The commit message check failed. Do you want to commit the change?"),
                                 &newSettings.promptToSubmit, !m_submitActionTriggered);
    m_submitActionTriggered = false;
    switch (answer) {
    case VCSBase::VCSBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VCSBase::VCSBaseSubmitEditor::SubmitDiscarded:
        cleanCommitMessageFile();
        return true; // Cancel all
    default:
        break;
    }
    setSettings(newSettings); // in case someone turned prompting off
    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & commit
        closeEditor = Core::ICore::instance()->fileManager()->saveFile(fileIFace);
        if (closeEditor)
            closeEditor = commit(m_commitMessageFileName, fileList);
    }
    if (closeEditor)
        cleanCommitMessageFile();
    return closeEditor;
}

void CVSPlugin::diffCommitFiles(const QStringList &files)
{
    cvsDiff(m_commitRepository, files);
}

static inline void setDiffBaseDirectory(Core::IEditor *editor, const QString &db)
{
    if (VCSBase::VCSBaseEditorWidget *ve = qobject_cast<VCSBase::VCSBaseEditorWidget*>(editor->widget()))
        ve->setDiffBaseDirectory(db);
}

// Collect all parameters required for a diff to be able to associate them
// with a diff editor and re-run the diff with parameters.
struct CvsDiffParameters
{
    QString workingDir;
    QStringList arguments;
    QStringList files;
};

// Parameter widget controlling whitespace diff mode, associated with a parameter
// struct.
class CvsDiffParameterWidget : public VCSBase::VCSBaseEditorParameterWidget
{
    Q_OBJECT
public:
    explicit CvsDiffParameterWidget(const CvsDiffParameters &p, QWidget *parent = 0);

signals:
    void reRunDiff(const CVS::Internal::CvsDiffParameters &);

public slots:
    void triggerReRun();

private:
    const CvsDiffParameters m_parameters;
};

CvsDiffParameterWidget::CvsDiffParameterWidget(const CvsDiffParameters &p, QWidget *parent) :
    VCSBase::VCSBaseEditorParameterWidget(parent), m_parameters(p)
{
    setBaseArguments(p.arguments);
    addToggleButton(QLatin1String("-w"), tr("Ignore whitespace"));
    addToggleButton(QLatin1String("-B"), tr("Ignore blank lines"));
    connect(this, SIGNAL(argumentsChanged()),
            this, SLOT(triggerReRun()));
}

void CvsDiffParameterWidget::triggerReRun()
{
    CvsDiffParameters effectiveParameters = m_parameters;
    effectiveParameters.arguments = arguments();
    emit reRunDiff(effectiveParameters);
}

void CVSPlugin::cvsDiff(const QString &workingDir, const QStringList &files)
{
    CvsDiffParameters p;
    p.workingDir = workingDir;
    p.files = files;
    p.arguments = m_settings.cvsDiffOptions.split(QLatin1Char(' '), QString::SkipEmptyParts);
    cvsDiff(p);
}

void CVSPlugin::cvsDiff(const CvsDiffParameters &p)
{
    if (CVS::Constants::debug)
        qDebug() << Q_FUNC_INFO << p.files;
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(p.workingDir, p.files);
    QTextCodec *codec = VCSBase::VCSBaseEditorWidget::getCodec(p.workingDir, p.files);
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(p.workingDir, p.files);

    QStringList args(QLatin1String("diff"));
    args.append(p.arguments);
    args.append(p.files);

    // CVS returns the diff exit code (1 if files differ), which is
    // undistinguishable from a "file not found" error, unfortunately.
    const CVSResponse response =
            runCVS(p.workingDir, args, m_settings.timeOutMS(), 0, codec);
    switch (response.result) {
    case CVSResponse::NonNullExitCode:
    case CVSResponse::Ok:
        break;
    case CVSResponse::OtherError:
        return;
    }

    QString output = fixDiffOutput(response.stdOut);
    if (output.isEmpty())
        output = tr("The files do not differ.");
    // diff of a single file? re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    // Show in the same editor if diff has been executed before
    const QString tag = VCSBase::VCSBaseEditorWidget::editorTag(VCSBase::DiffOutput, p.workingDir, p.files);
    if (Core::IEditor *existingEditor = VCSBase::VCSBaseEditorWidget::locateEditorByTag(tag)) {
        existingEditor->createNew(output);
        Core::EditorManager::instance()->activateEditor(existingEditor, Core::EditorManager::ModeSwitch);
        setDiffBaseDirectory(existingEditor, p.workingDir);
        return;
    }
    const QString title = QString::fromLatin1("cvs diff %1").arg(id);
    Core::IEditor *editor = showOutputInEditor(title, output, VCSBase::DiffOutput, source, codec);
    VCSBase::VCSBaseEditorWidget::tagEditor(editor, tag);
    setDiffBaseDirectory(editor, p.workingDir);
    CVSEditor *diffEditorWidget = qobject_cast<CVSEditor*>(editor->widget());
    QTC_ASSERT(diffEditorWidget, return ; )

    // Wire up the parameter widget to trigger a re-run on
    // parameter change and 'revert' from inside the diff editor.
    diffEditorWidget->setRevertDiffChunkEnabled(true);
    CvsDiffParameterWidget *pw = new CvsDiffParameterWidget(p);
    connect(pw, SIGNAL(reRunDiff(CVS::Internal::CvsDiffParameters)),
            this, SLOT(cvsDiff(CVS::Internal::CvsDiffParameters)));
    connect(diffEditorWidget, SIGNAL(diffChunkReverted(VCSBase::DiffChunk)),
            pw, SLOT(triggerReRun()));
    diffEditorWidget->setConfigurationWidget(pw);
}

CVSSubmitEditor *CVSPlugin::openCVSSubmitEditor(const QString &fileName)
{
    Core::IEditor *editor = Core::EditorManager::instance()->openEditor(fileName, QLatin1String(Constants::CVSCOMMITEDITOR_ID),
                                                                        Core::EditorManager::ModeSwitch);
    CVSSubmitEditor *submitEditor = qobject_cast<CVSSubmitEditor*>(editor);
    QTC_CHECK(submitEditor);
    submitEditor->registerActions(m_submitUndoAction, m_submitRedoAction, m_submitCurrentLogAction, m_submitDiffAction);
    connect(submitEditor, SIGNAL(diffSelectedFiles(QStringList)), this, SLOT(diffCommitFiles(QStringList)));

    return submitEditor;
}

void CVSPlugin::updateActions(VCSBase::VCSBasePlugin::ActionState as)
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

    m_diffRepositoryAction->setEnabled(hasTopLevel);
    m_statusRepositoryAction->setEnabled(hasTopLevel);
    m_updateRepositoryAction->setEnabled(hasTopLevel);
    m_commitAllAction->setEnabled(hasTopLevel);
    m_logRepositoryAction->setEnabled(hasTopLevel);
    m_uneditRepositoryAction->setEnabled(hasTopLevel);
}

void CVSPlugin::addCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CVSPlugin::revertAll()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    const QString title = tr("Revert repository");
    if (!messageBoxQuestion(title, tr("Revert all pending changes to the repository?")))
        return;
    QStringList args;
    args << QLatin1String("update") << QLatin1String("-C") << state.topLevel();
    const CVSResponse revertResponse =
            runCVS(state.topLevel(), args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    if (revertResponse.result == CVSResponse::Ok) {
        cvsVersionControl()->emitRepositoryChanged(state.topLevel());
    } else {
        QMessageBox::warning(0, title, tr("Revert failed: %1").arg(revertResponse.message), QMessageBox::Ok);
    }
}

void CVSPlugin::revertCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    QStringList args;
    args << QLatin1String("diff") << state.relativeCurrentFile();
    const CVSResponse diffResponse =
            runCVS(state.currentFileTopLevel(), args, m_settings.timeOutMS(), 0);
    switch (diffResponse.result) {
    case CVSResponse::Ok:
        return; // Not modified, diff exit code 0
    case CVSResponse::NonNullExitCode: // Diff exit code != 0
        if (diffResponse.stdOut.isEmpty()) // Paranoia: Something else failed?
            return;
        break;
    case CVSResponse::OtherError:
        return;
    }

    if (!messageBoxQuestion(QLatin1String("CVS Revert"),
                            tr("The file has been changed. Do you want to revert it?")))
        return;

    Core::FileChangeBlocker fcb(state.currentFile());

    // revert
    args.clear();
    args << QLatin1String("update") << QLatin1String("-C") << state.relativeCurrentFile();
    const CVSResponse revertResponse =
            runCVS(state.currentFileTopLevel(), args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    if (revertResponse.result == CVSResponse::Ok) {
        cvsVersionControl()->emitFilesChanged(QStringList(state.currentFile()));
    }
}

void CVSPlugin::diffProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    cvsDiff(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CVSPlugin::diffCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    cvsDiff(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::startCommitCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    startCommit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::startCommitAll()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    startCommit(state.topLevel());
}

/* Start commit of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * commit will start. */
void CVSPlugin::startCommit(const QString &workingDir, const QStringList &files)
{
    if (VCSBase::VCSBaseSubmitEditor::raiseSubmitEditor())
        return;
    if (isCommitEditorOpen()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendWarning(tr("Another commit is currently being executed."));
        return;
    }

    // We need the "Examining <subdir>" stderr output to tell
    // where we are, so, have stdout/stderr channels merged.
    QStringList args = QStringList(QLatin1String("status"));
    const CVSResponse response =
            runCVS(workingDir, args, m_settings.timeOutMS(), MergeOutputChannels);
    if (response.result != CVSResponse::Ok)
        return;
    // Get list of added/modified/deleted files and purge out undesired ones
    // (do not run status with relative arguments as it will omit the directories)
    StateList statusOutput = parseStatusOutput(QString(), response.stdOut);
    if (!files.isEmpty()) {
        for (StateList::iterator it = statusOutput.begin(); it != statusOutput.end() ; ) {
            if (files.contains(it->second)) {
                ++it;
            } else {
                it = statusOutput.erase(it);
            }
        }
    }
    if (statusOutput.empty()) {
        VCSBase::VCSBaseOutputWindow::instance()->append(tr("There are no modified files."));
        return;
    }
    m_commitRepository = workingDir;

    // Create a new submit change file containing the submit template
    Utils::TempFileSaver saver;
    saver.setAutoRemove(false);
    // TODO: Retrieve submit template from
    const QString submitTemplate;
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (!saver.finalize()) {
        VCSBase::VCSBaseOutputWindow::instance()->appendError(saver.errorString());
        return;
    }
    m_commitMessageFileName = saver.fileName();
    // Create a submit editor and set file list
    CVSSubmitEditor *editor = openCVSSubmitEditor(m_commitMessageFileName);
    editor->setCheckScriptWorkingDirectory(m_commitRepository);
    editor->setStateList(statusOutput);
}

bool CVSPlugin::commit(const QString &messageFile,
                              const QStringList &fileList)
{
    if (CVS::Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << fileList;
    QStringList args = QStringList(QLatin1String("commit"));
    args << QLatin1String("-F") << messageFile;
    args.append(fileList);
    const CVSResponse response =
            runCVS(m_commitRepository, args, m_settings.longTimeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return response.result == CVSResponse::Ok ;
}

void CVSPlugin::filelogCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    filelog(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void CVSPlugin::logProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    filelog(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CVSPlugin::logRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    filelog(state.topLevel());
}

void CVSPlugin::filelog(const QString &workingDir,
                        const QStringList &files,
                        bool enableAnnotationContextMenu)
{
    QTextCodec *codec = VCSBase::VCSBaseEditorWidget::getCodec(workingDir, files);
    // no need for temp file
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(workingDir, files);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, files);
    QStringList args;
    args << QLatin1String("log");
    args.append(files);
    const CVSResponse response =
            runCVS(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt, codec);
    if (response.result != CVSResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VCSBase::VCSBaseEditorWidget::editorTag(VCSBase::LogOutput, workingDir, files);
    if (Core::IEditor *editor = VCSBase::VCSBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(response.stdOut);
        Core::EditorManager::instance()->activateEditor(editor, Core::EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cvs log %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VCSBase::LogOutput, source, codec);
        VCSBase::VCSBaseEditorWidget::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VCSBase::VCSBaseEditorWidget::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
}

void CVSPlugin::updateProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    update(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

bool CVSPlugin::update(const QString &topLevel, const QStringList &files)
{
    QStringList args(QLatin1String("update"));
    args.push_back(QLatin1String("-dR"));
    args.append(files);
    const CVSResponse response =
            runCVS(topLevel, args, m_settings.longTimeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    const bool ok = response.result == CVSResponse::Ok;
    if (ok)
        cvsVersionControl()->emitRepositoryChanged(topLevel);
    return ok;
}

void CVSPlugin::editCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    edit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::uneditCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    unedit(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()));
}

void CVSPlugin::uneditCurrentRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    unedit(state.topLevel(), QStringList());
}

void CVSPlugin::annotateCurrentFile()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return)
    annotate(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void CVSPlugin::vcsAnnotate(const QString &file, const QString &revision, int lineNumber)
{
    const QFileInfo fi(file);
    annotate(fi.absolutePath(), fi.fileName(), revision, lineNumber);
}

bool CVSPlugin::edit(const QString &topLevel, const QStringList &files)
{
    QStringList args(QLatin1String("edit"));
    args.append(files);
    const CVSResponse response =
            runCVS(topLevel, args, m_settings.timeOutMS(),
                   ShowStdOutInLogWindow|SshPasswordPrompt);
    return response.result == CVSResponse::Ok;
}

bool CVSPlugin::diffCheckModified(const QString &topLevel, const QStringList &files, bool *modified)
{
    // Quick check for modified files using diff
    *modified = false;
    QStringList args(QLatin1String("-q"));
    args << QLatin1String("diff");
    args.append(files);
    const CVSResponse response = runCVS(topLevel, args, m_settings.timeOutMS(), 0);
    if (response.result == CVSResponse::OtherError)
        return false;
    *modified = response.result == CVSResponse::NonNullExitCode;
    return true;
}

bool CVSPlugin::unedit(const QString &topLevel, const QStringList &files)
{
    bool modified;
    // Prompt and use force flag if modified
    if (!diffCheckModified(topLevel, files, &modified))
        return false;
    if (modified) {
        const QString question = files.isEmpty() ?
                      tr("Would you like to discard your changes to the repository '%1'?").arg(topLevel) :
                      tr("Would you like to discard your changes to the file '%1'?").arg(files.front());
        if (!messageBoxQuestion(tr("Unedit"), question))
            return false;
    }

    QStringList args(QLatin1String("unedit"));
    // Note: Option '-y' to force 'yes'-answer to CVS' 'undo change' prompt,
    // exists in CVSNT only as of 6.8.2010. Standard CVS will otherwise prompt
    if (modified)
        args.append(QLatin1String("-y"));
    args.append(files);
    const CVSResponse response =
            runCVS(topLevel, args, m_settings.timeOutMS(),
                   ShowStdOutInLogWindow|SshPasswordPrompt);
    return response.result == CVSResponse::Ok;
}

void CVSPlugin::annotate(const QString &workingDir, const QString &file,
                         const QString &revision /* = QString() */,
                         int lineNumber /* = -1 */)
{
    const QStringList files(file);
    QTextCodec *codec = VCSBase::VCSBaseEditorWidget::getCodec(workingDir, files);
    const QString id = VCSBase::VCSBaseEditorWidget::getTitleId(workingDir, files, revision);
    const QString source = VCSBase::VCSBaseEditorWidget::getSource(workingDir, file);
    QStringList args;
    args << QLatin1String("annotate");
    if (!revision.isEmpty())
        args << QLatin1String("-r") << revision;
    args << file;
    const CVSResponse response =
            runCVS(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt, codec);
    if (response.result != CVSResponse::Ok)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    if (lineNumber < 1)
        lineNumber = VCSBase::VCSBaseEditorWidget::lineNumberOfCurrentEditor(file);

    const QString tag = VCSBase::VCSBaseEditorWidget::editorTag(VCSBase::AnnotateOutput, workingDir, QStringList(file), revision);
    if (Core::IEditor *editor = VCSBase::VCSBaseEditorWidget::locateEditorByTag(tag)) {
        editor->createNew(response.stdOut);
        VCSBase::VCSBaseEditorWidget::gotoLineOfEditor(editor, lineNumber);
        Core::EditorManager::instance()->activateEditor(editor, Core::EditorManager::ModeSwitch);
    } else {
        const QString title = QString::fromLatin1("cvs annotate %1").arg(id);
        Core::IEditor *newEditor = showOutputInEditor(title, response.stdOut, VCSBase::AnnotateOutput, source, codec);
        VCSBase::VCSBaseEditorWidget::tagEditor(newEditor, tag);
        VCSBase::VCSBaseEditorWidget::gotoLineOfEditor(newEditor, lineNumber);
    }
}

bool CVSPlugin::status(const QString &topLevel, const QStringList &files, const QString &title)
{
    QStringList args(QLatin1String("status"));
    args.append(files);
    const CVSResponse response =
            runCVS(topLevel, args, m_settings.timeOutMS(), 0);
    const bool ok = response.result == CVSResponse::Ok;
    if (ok)
        showOutputInEditor(title, response.stdOut, VCSBase::RegularCommandOutput, topLevel, 0);
    return ok;
}

void CVSPlugin::projectStatus()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    status(state.currentProjectTopLevel(), state.relativeCurrentProject(), tr("Project status"));
}

void CVSPlugin::commitProject()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return)
    startCommit(state.currentProjectTopLevel(), state.relativeCurrentProject());
}

void CVSPlugin::diffRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    cvsDiff(state.topLevel(), QStringList());
}

void CVSPlugin::statusRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    status(state.topLevel(), QStringList(), tr("Repository status"));
}

void CVSPlugin::updateRepository()
{
    const VCSBase::VCSBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return)
    update(state.topLevel(), QStringList());

}

void CVSPlugin::slotDescribe(const QString &source, const QString &changeNr)
{
    QString errorMessage;
    if (!describe(source, changeNr, &errorMessage))
        VCSBase::VCSBaseOutputWindow::instance()->appendError(errorMessage);
}

bool CVSPlugin::describe(const QString &file, const QString &changeNr, QString *errorMessage)
{

    QString toplevel;
    const bool manages = managesDirectory(QFileInfo(file).absolutePath(), &toplevel);
    if (!manages || toplevel.isEmpty()) {
        *errorMessage = msgCannotFindTopLevel(file);
        return false;
    }
    return describe(toplevel, QDir(toplevel).relativeFilePath(file), changeNr, errorMessage);
}

bool CVSPlugin::describe(const QString &toplevel, const QString &file, const
                         QString &changeNr, QString *errorMessage)
{

    // In CVS, revisions of files are normally unrelated, there is
    // no global revision/change number. The only thing that groups
    // a commit is the "commit-id" (as shown in the log).
    // This function makes use of it to find all files related to
    // a commit in order to emulate a "describe global change" functionality
    // if desired.
    if (CVS::Constants::debug)
        qDebug() << Q_FUNC_INFO << file << changeNr;
    // Number must be > 1
    if (isFirstRevision(changeNr)) {
        *errorMessage = tr("The initial revision %1 cannot be described.").arg(changeNr);
        return false;
    }
    // Run log to obtain commit id and details
    QStringList args;
    args << QLatin1String("log") << (QLatin1String("-r") + changeNr) << file;
    const CVSResponse logResponse =
            runCVS(toplevel, args, m_settings.timeOutMS(), SshPasswordPrompt);
    if (logResponse.result != CVSResponse::Ok) {
        *errorMessage = logResponse.message;
        return false;
    }
    const QList<CVS_LogEntry> fileLog = parseLogEntries(logResponse.stdOut);
    if (fileLog.empty() || fileLog.front().revisions.empty()) {
        *errorMessage = msgLogParsingFailed();
        return false;
    }
    if (m_settings.describeByCommitId) {
        // Run a log command over the repo, filtering by the commit date
        // and commit id, collecting all files touched by the commit.
        const QString commitId = fileLog.front().revisions.front().commitId;
        // Date range "D1<D2" in ISO format "YYYY-MM-DD"
        const QString dateS = fileLog.front().revisions.front().date;
        const QDate date = QDate::fromString(dateS, Qt::ISODate);
        const QString nextDayS = date.addDays(1).toString(Qt::ISODate);
        args.clear();
        args << QLatin1String("log") << QLatin1String("-d") << (dateS  + QLatin1Char('<') + nextDayS);

        const CVSResponse repoLogResponse =
                runCVS(toplevel, args, m_settings.longTimeOutMS(), SshPasswordPrompt);
        if (repoLogResponse.result != CVSResponse::Ok) {
            *errorMessage = repoLogResponse.message;
            return false;
        }
        // Describe all files found, pass on dir to obtain correct absolute paths.
        const QList<CVS_LogEntry> repoEntries = parseLogEntries(repoLogResponse.stdOut, QString(), commitId);
        if (repoEntries.empty()) {
            *errorMessage = tr("Could not find commits of id '%1' on %2.").arg(commitId, dateS);
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
bool CVSPlugin::describe(const QString &repositoryPath,
                         QList<CVS_LogEntry> entries,
                         QString *errorMessage)
{
    // Collect logs
    QString output;
    QTextCodec *codec = 0;
    const QList<CVS_LogEntry>::iterator lend = entries.end();
    for (QList<CVS_LogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        // Before fiddling file names, try to find codec
        if (!codec)
            codec = VCSBase::VCSBaseEditorWidget::getCodec(repositoryPath, QStringList(it->file));
        // Run log
        QStringList args(QLatin1String("log"));
        args << (QLatin1String("-r") + it->revisions.front().revision) << it->file;
        const CVSResponse logResponse =
                runCVS(repositoryPath, args, m_settings.timeOutMS(), SshPasswordPrompt);
        if (logResponse.result != CVSResponse::Ok) {
            *errorMessage =  logResponse.message;
            return false;
        }
        output += logResponse.stdOut;
    }
    // Collect diffs relative to repository
    for (QList<CVS_LogEntry>::iterator it = entries.begin(); it != lend; ++it) {
        const QString &revision = it->revisions.front().revision;
        if (!isFirstRevision(revision)) {
            const QString previousRev = previousRevision(revision);
            QStringList args(QLatin1String("diff"));
            args << m_settings.cvsDiffOptions << QLatin1String("-r") << previousRev
                    << QLatin1String("-r") << it->revisions.front().revision
                    << it->file;
            const CVSResponse diffResponse =
                    runCVS(repositoryPath, args, m_settings.timeOutMS(), 0, codec);
            switch (diffResponse.result) {
            case CVSResponse::Ok:
            case CVSResponse::NonNullExitCode: // Diff exit code != 0
                if (diffResponse.stdOut.isEmpty()) {
                    *errorMessage = diffResponse.message;
                    return false; // Something else failed.
                }
                break;
            case CVSResponse::OtherError:
                *errorMessage = diffResponse.message;
                return false;
            }
            output += fixDiffOutput(diffResponse.stdOut);
        }
    }

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString commitId = entries.front().revisions.front().commitId;
    if (Core::IEditor *editor = VCSBase::VCSBaseEditorWidget::locateEditorByTag(commitId)) {
        editor->createNew(output);
        Core::EditorManager::instance()->activateEditor(editor, Core::EditorManager::ModeSwitch);
        setDiffBaseDirectory(editor, repositoryPath);
    } else {
        const QString title = QString::fromLatin1("cvs describe %1").arg(commitId);
        Core::IEditor *newEditor = showOutputInEditor(title, output, VCSBase::DiffOutput, entries.front().file, codec);
        VCSBase::VCSBaseEditorWidget::tagEditor(newEditor, commitId);
        setDiffBaseDirectory(newEditor, repositoryPath);
    }
    return true;
}

void CVSPlugin::submitCurrentLog()
{
    m_submitActionTriggered = true;
    Core::EditorManager::instance()->closeEditors(QList<Core::IEditor*>()
        << Core::EditorManager::instance()->currentEditor());
}

// Run CVS. At this point, file arguments must be relative to
// the working directory (see above).
CVSResponse CVSPlugin::runCVS(const QString &workingDirectory,
                              const QStringList &arguments,
                              int timeOut,
                              unsigned flags,
                              QTextCodec *outputCodec)
{
    const QString executable = m_settings.cvsCommand;
    CVSResponse response;
    if (executable.isEmpty()) {
        response.result = CVSResponse::OtherError;
        response.message =tr("No cvs executable specified!");
        return response;
    }
    // Run, connect stderr to the output window
    const Utils::SynchronousProcessResponse sp_resp =
            runVCS(workingDirectory, executable,
                   m_settings.addOptions(arguments),
                   timeOut, flags, outputCodec);

    response.result = CVSResponse::OtherError;
    response.stdErr = sp_resp.stdErr;
    response.stdOut = sp_resp.stdOut;
    switch (sp_resp.result) {
    case Utils::SynchronousProcessResponse::Finished:
        response.result = CVSResponse::Ok;
        break;
    case Utils::SynchronousProcessResponse::FinishedError:
        response.result = CVSResponse::NonNullExitCode;
        break;
    case Utils::SynchronousProcessResponse::TerminatedAbnormally:
    case Utils::SynchronousProcessResponse::StartFailed:
    case Utils::SynchronousProcessResponse::Hang:
        break;
    }

    if (response.result != CVSResponse::Ok)
        response.message = sp_resp.exitMessage(executable, timeOut);

    return response;
}

Core::IEditor * CVSPlugin::showOutputInEditor(const QString& title, const QString &output,
                                                     int editorType, const QString &source,
                                                     QTextCodec *codec)
{
    const VCSBase::VCSBaseEditorParameters *params = findType(editorType);
    QTC_ASSERT(params, return 0);
    const QString id = params->id;
    if (CVS::Constants::debug)
        qDebug() << "CVSPlugin::showOutputInEditor" << title << id <<  "source=" << source << "Size= " << output.size() <<  " Type=" << editorType << debugCodec(codec);
    QString s = title;
    Core::IEditor *editor = Core::EditorManager::instance()->openEditorWithContents(id, &s, output.toLocal8Bit());
    connect(editor, SIGNAL(annotateRevisionRequested(QString,QString,int)),
            this, SLOT(vcsAnnotate(QString,QString,int)));
    CVSEditor *e = qobject_cast<CVSEditor*>(editor->widget());
    if (!e)
        return 0;
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->setSuggestedFileName(s);
    e->setForceReadOnly(true);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    Core::IEditor *ie = e->editor();
    Core::EditorManager::instance()->activateEditor(ie, Core::EditorManager::ModeSwitch);
    return ie;
}

CVSSettings CVSPlugin::settings() const
{
    return m_settings;
}

void CVSPlugin::setSettings(const CVSSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        if (QSettings *settings = Core::ICore::instance()->settings())
            m_settings.toSettings(settings);
        cvsVersionControl()->emitConfigurationChanged();
    }
}

CVSPlugin *CVSPlugin::instance()
{
    QTC_ASSERT(m_cvsPluginInstance, return m_cvsPluginInstance);
    return m_cvsPluginInstance;
}

bool CVSPlugin::vcsAdd(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("add") << rawFileName;
    const CVSResponse response =
            runCVS(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return response.result == CVSResponse::Ok;
}

bool CVSPlugin::vcsDelete(const QString &workingDir, const QString &rawFileName)
{
    QStringList args;
    args << QLatin1String("remove") << QLatin1String("-f") << rawFileName;
    const CVSResponse response =
            runCVS(workingDir, args, m_settings.timeOutMS(),
                   SshPasswordPrompt|ShowStdOutInLogWindow);
    return response.result == CVSResponse::Ok;
}

/* CVS has a "CVS" directory in each directory it manages. The top level
 * is the first directory under the directory that does not have it. */
bool CVSPlugin::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
    if (topLevel)
        topLevel->clear();
    bool manages = false;
    const QDir dir(directory);
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
        for (QDir parentDir = lastDirectory; parentDir.cdUp() ; lastDirectory = parentDir) {
            if (!checkCVSDirectory(parentDir)) {
                *topLevel = lastDirectory.absolutePath();
                break;
            }
        }
    } while (false);
    if (CVS::Constants::debug) {
        QDebug nsp = qDebug().nospace();
        nsp << "CVSPlugin::managesDirectory" << directory << manages;
        if (topLevel)
            nsp << *topLevel;
    }
    return manages;
}

bool CVSPlugin::checkCVSDirectory(const QDir &directory) const
{
    const QString cvsDir = directory.absoluteFilePath(QLatin1String("CVS"));
    return QFileInfo(cvsDir).isDir();
}

CVSControl *CVSPlugin::cvsVersionControl() const
{
    return static_cast<CVSControl *>(versionControl());
}

}
}
Q_EXPORT_PLUGIN(CVS::Internal::CVSPlugin)

#include "cvsplugin.moc"
