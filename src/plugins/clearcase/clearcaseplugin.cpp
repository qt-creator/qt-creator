/****************************************************************************
**
** Copyright (C) 2016 AudioCodes Ltd.
** Author: Orgad Shaneh <orgad.shaneh@audiocodes.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clearcaseplugin.h"
#include "activityselector.h"
#include "checkoutdialog.h"
#include "clearcaseconstants.h"
#include "clearcaseeditor.h"
#include "clearcasesubmiteditor.h"
#include "clearcasesubmiteditorwidget.h"
#include "clearcasesync.h"
#include "settingspage.h"
#include "versionselector.h"
#include "ui_undocheckout.h"

#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/locator/commandlocator.h>
#include <coreplugin/messagemanager.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <texteditor/textdocument.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/session.h>

#include <utils/algorithm.h>
#include <utils/fileutils.h>
#include <utils/hostosinfo.h>
#include <utils/infobar.h>
#include <utils/parameteraction.h>
#include <utils/qtcassert.h>
#include <utils/runextensions.h>
#include <utils/stringutils.h>
#include <utils/synchronousprocess.h>
#include <utils/temporarydirectory.h>

#include <vcsbase/basevcseditorfactory.h>
#include <vcsbase/basevcssubmiteditorfactory.h>
#include <vcsbase/vcsbaseeditor.h>
#include <vcsbase/vcscommand.h>
#include <vcsbase/vcsoutputwindow.h>
#include <vcsbase/vcsbasesubmiteditor.h>
#include <vcsbase/vcsbaseplugin.h>

#include <QAction>
#include <QDebug>
#include <QDialog>
#include <QDialogButtonBox>
#include <QDir>
#include <QFileInfo>
#include <QFuture>
#include <QFutureInterface>
#include <QInputDialog>
#include <QList>
#include <QMainWindow>
#include <QMenu>
#include <QMessageBox>
#include <QMetaObject>
#include <QMutex>
#include <QProcess>
#include <QRegularExpression>
#include <QSharedPointer>
#include <QTextCodec>
#include <QUrl>
#include <QUuid>
#include <QVariant>
#include <QVBoxLayout>
#include <QXmlStreamReader>
#ifdef WITH_TESTS
#include <QTest>
#include <coreplugin/vcsmanager.h>
#endif

using namespace Core;
using namespace ProjectExplorer;
using namespace VcsBase;
using namespace Utils;
using namespace std::placeholders;

namespace ClearCase {
namespace Internal {

static const char CLEARCASE_CONTEXT[]         = "ClearCase Context";
static const char CMD_ID_CLEARCASE_MENU[]     = "ClearCase.Menu";
static const char CMD_ID_CHECKOUT[]           = "ClearCase.CheckOut";
static const char CMD_ID_CHECKIN[]            = "ClearCase.CheckInCurrent";
static const char CMD_ID_UNDOCHECKOUT[]       = "ClearCase.UndoCheckOut";
static const char CMD_ID_UNDOHIJACK[]         = "ClearCase.UndoHijack";
static const char CMD_ID_DIFF_CURRENT[]       = "ClearCase.DiffCurrent";
static const char CMD_ID_HISTORY_CURRENT[]    = "ClearCase.HistoryCurrent";
static const char CMD_ID_ANNOTATE[]           = "ClearCase.Annotate";
static const char CMD_ID_ADD_FILE[]           = "ClearCase.AddFile";
static const char CMD_ID_DIFF_ACTIVITY[]      = "ClearCase.DiffActivity";
static const char CMD_ID_CHECKIN_ACTIVITY[]   = "ClearCase.CheckInActivity";
static const char CMD_ID_UPDATEINDEX[]        = "ClearCase.UpdateIndex";
static const char CMD_ID_UPDATE_VIEW[]        = "ClearCase.UpdateView";
static const char CMD_ID_CHECKIN_ALL[]        = "ClearCase.CheckInAll";
static const char CMD_ID_STATUS[]             = "ClearCase.Status";

class ClearCaseResponse
{
public:
    bool error = false;
    QString stdOut;
    QString stdErr;
    QString message;
};

const VcsBaseEditorParameters logEditorParameters {
    LogOutput,
    "ClearCase File Log Editor",   // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase File Log Editor"),   // display_name
    "text/vnd.qtcreator.clearcase.log"
};

const VcsBaseEditorParameters annotateEditorParameters {
    AnnotateOutput,
    "ClearCase Annotation Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase Annotation Editor"),   // display_name
    "text/vnd.qtcreator.clearcase.annotation"
};

const VcsBaseEditorParameters diffEditorParameters {
    DiffOutput,
    "ClearCase Diff Editor",  // id
    QT_TRANSLATE_NOOP("VCS", "ClearCase Diff Editor"),   // display_name
    "text/x-patch"
};

const VcsBaseSubmitEditorParameters submitParameters {
    Constants::CLEARCASE_SUBMIT_MIMETYPE,
    Constants::CLEARCASECHECKINEDITOR_ID,
    Constants::CLEARCASECHECKINEDITOR_DISPLAY_NAME,
    VcsBaseSubmitEditorParameters::DiffFiles
};

static QString debugCodec(const QTextCodec *c)
{
    return c ? QString::fromLatin1(c->name()) : QString::fromLatin1("Null codec");
}

class ClearCasePluginPrivate final : public VcsBase::VcsBasePluginPrivate
{
    Q_OBJECT
    enum { SilentRun = VcsBase::VcsCommand::NoOutput | VcsBase::VcsCommand::FullySynchronously };

public:
    ClearCasePluginPrivate();
    ~ClearCasePluginPrivate() final;

    // IVersionControl
    QString displayName() const final;
    Utils::Id id() const final;

    bool isVcsFileOrDirectory(const Utils::FilePath &fileName) const final;

    bool managesDirectory(const QString &directory, QString *topLevel) const final;
    bool managesFile(const QString &workingDirectory, const QString &fileName) const final;

    bool isConfigured() const final;

    bool supportsOperation(Operation operation) const final;
    OpenSupportMode openSupportMode(const QString &fileName) const final;
    bool vcsOpen(const QString &fileName) final;
    SettingsFlags settingsFlags() const final;
    bool vcsAdd(const QString &fileName) final;
    bool vcsDelete(const QString &filename) final;
    bool vcsMove(const QString &from, const QString &to) final;
    bool vcsCreateRepository(const QString &directory) final;

    void vcsAnnotate(const QString &file, int line) final;
    void vcsDescribe(const QString &source, const QString &changeNr) final;

    QString vcsOpenText() const final;
    QString vcsMakeWritableText() const final;
    QString vcsTopic(const QString &directory) final;

    ///
    ClearCaseSubmitEditor *openClearCaseSubmitEditor(const QString &fileName, bool isUcm);

    const ClearCaseSettings &settings() const;
    void setSettings(const ClearCaseSettings &s);

    // IVersionControl
    bool vcsOpen(const QString &workingDir, const QString &fileName);
    bool vcsAdd(const QString &workingDir, const QString &fileName);
    bool vcsDelete(const QString &workingDir, const QString &fileName);
    bool vcsCheckIn(const QString &workingDir, const QStringList &files, const QString &activity,
                    bool isIdentical, bool isPreserve, bool replaceActivity);
    bool vcsUndoCheckOut(const QString &workingDir, const QString &fileName, bool keep);
    bool vcsUndoHijack(const QString &workingDir, const QString &fileName, bool keep);
    bool vcsMove(const QString &workingDir, const QString &from, const QString &to);
    bool vcsSetActivity(const QString &workingDir, const QString &title, const QString &activity);

    static ClearCasePluginPrivate *instance();

    QString ccGetCurrentActivity() const;
    QList<QStringPair> activities(int *current = nullptr);
    QString ccGetPredecessor(const QString &version) const;
    QStringList ccGetActiveVobs() const;
    ViewData ccGetView(const QString &workingDir) const;
    QString ccGetComment(const QString &workingDir, const QString &fileName) const;
    bool ccFileOp(const QString &workingDir, const QString &title, const QStringList &args,
                  const QString &fileName, const QString &file2 = QString());
    FileStatus vcsStatus(const QString &file) const;
    void checkAndReIndexUnknownFile(const QString &file);
    QString currentView() const { return m_viewData.name; }
    QString viewRoot() const { return m_viewData.root; }
    void refreshActivities();
    inline bool isUcm() const { return m_viewData.isUcm; }
    inline bool isDynamic() const { return m_viewData.isDynamic; }
    void setStatus(const QString &file, FileStatus::Status status, bool update = true);

    bool ccCheckUcm(const QString &viewname, const QString &workingDir) const;
#ifdef WITH_TESTS
    inline void setFakeCleartool(const bool b = true) { m_fakeClearTool = b; }
#endif

    void vcsAnnotateHelper(const QString &workingDir, const QString &file,
                           const QString &revision = QString(), int lineNumber = -1) const;
    bool newActivity();
    void updateStreamAndView();

protected:
    void updateActions(VcsBase::VcsBasePluginPrivate::ActionState) override;
    bool submitEditorAboutToClose() override;
    QString ccGet(const QString &workingDir, const QString &file, const QString &prefix = QString());
    QList<QStringPair> ccGetActivities() const;

private:
    void syncSlot();
    Q_INVOKABLE void updateStatusActions();

    QString commitDisplayName() const final;
    void checkOutCurrentFile();
    void addCurrentFile();
    void undoCheckOutCurrent();
    void undoHijackCurrent();
    void diffActivity();
    void diffCurrentFile();
    void startCheckInAll();
    void startCheckInActivity();
    void startCheckInCurrentFile();
    void historyCurrentFile();
    void annotateCurrentFile();
    void viewStatus();
    void commitFromEditor() override;
    void diffCheckInFiles(const QStringList &);
    void updateIndex();
    void updateView();
    void projectChanged(ProjectExplorer::Project *project);
    void tasksFinished(Utils::Id type);
    void closing();

    inline bool isCheckInEditorOpen() const;
    QStringList getVobList() const;
    QString ccManagesDirectory(const QString &directory) const;
    QString ccViewRoot(const QString &directory) const;
    QString findTopLevel(const QString &directory) const;
    Core::IEditor *showOutputInEditor(const QString& title, const QString &output,
                                      Id id, const QString &source,
                                      QTextCodec *codec) const;
    QString runCleartoolSync(const QString &workingDir, const QStringList &arguments) const;
    ClearCaseResponse runCleartool(const QString &workingDir, const QStringList &arguments,
                                   int timeOutS, unsigned flags,
                                   QTextCodec *outputCodec = nullptr) const;
    static void sync(QFutureInterface<void> &future, QStringList files);

    void history(const QString &workingDir,
                 const QStringList &file = QStringList(),
                 bool enableAnnotationContextMenu = false);
    QString ccGetFileVersion(const QString &workingDir, const QString &file) const;
    void ccUpdate(const QString &workingDir, const QStringList &relativePaths = QStringList());
    void ccDiffWithPred(const QString &workingDir, const QStringList &files);
    void startCheckIn(const QString &workingDir, const QStringList &files = QStringList());
    void cleanCheckInMessageFile();
    QString ccGetFileActivity(const QString &workingDir, const QString &file);
    QStringList ccGetActivityVersions(const QString &workingDir, const QString &activity);
    void diffGraphical(const QString &file1, const QString &file2 = QString());
    QString diffExternal(QString file1, QString file2 = QString(), bool keep = false);
    QString getFile(const QString &nativeFile, const QString &prefix);
    static void rmdir(const QString &path);
    QString runExtDiff(const QString &workingDir, const QStringList &arguments, int timeOutS,
                       QTextCodec *outputCodec = nullptr);
    static QString getDriveLetterOfPath(const QString &directory);

    FileStatus::Status getFileStatus(const QString &fileName) const;
    void updateStatusForFile(const QString &absFile);
    void updateEditDerivedObjectWarning(const QString &fileName, const FileStatus::Status status);

    ClearCaseSettings m_settings;

    QString m_checkInMessageFileName;
    QString m_checkInView;
    QString m_topLevel;
    QString m_stream;
    ViewData m_viewData;
    QString m_intStream;
    QString m_activity;
    QString m_diffPrefix;

    Core::CommandLocator *m_commandLocator = nullptr;
    Utils::ParameterAction *m_checkOutAction = nullptr;
    Utils::ParameterAction *m_checkInCurrentAction = nullptr;
    Utils::ParameterAction *m_undoCheckOutAction = nullptr;
    Utils::ParameterAction *m_undoHijackAction = nullptr;
    Utils::ParameterAction *m_diffCurrentAction = nullptr;
    Utils::ParameterAction *m_historyCurrentAction = nullptr;
    Utils::ParameterAction *m_annotateCurrentAction = nullptr;
    Utils::ParameterAction *m_addFileAction = nullptr;
    QAction *m_diffActivityAction = nullptr;
    QAction *m_updateIndexAction = nullptr;
    Utils::ParameterAction *m_updateViewAction = nullptr;
    Utils::ParameterAction *m_checkInActivityAction = nullptr;
    QAction *m_checkInAllAction = nullptr;
    QAction *m_statusAction = nullptr;

    QAction *m_menuAction = nullptr;
    bool m_submitActionTriggered = false;
    QMutex m_activityMutex;
    QList<QStringPair> m_activities;
    QSharedPointer<StatusMap> m_statusMap;

    ClearCaseSettingsPage m_settingsPage;

    VcsSubmitEditorFactory m_submitEditorFactory {
        submitParameters,
        [] { return new ClearCaseSubmitEditor; },
        this
    };

    VcsEditorFactory logEditorFactory {
        &logEditorParameters,
        [] { return new ClearCaseEditorWidget; },
        std::bind(&ClearCasePluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory annotateEditorFactory {
        &annotateEditorParameters,
        [] { return new ClearCaseEditorWidget; },
        std::bind(&ClearCasePluginPrivate::vcsDescribe, this, _1, _2)
    };

    VcsEditorFactory diffEditorFactory {
        &diffEditorParameters,
        [] { return new ClearCaseEditorWidget; },
        std::bind(&ClearCasePluginPrivate::vcsDescribe, this, _1, _2)
    };

    friend class ClearCasePlugin;
#ifdef WITH_TESTS
    bool m_fakeClearTool = false;
    QString m_tempFile;
#endif
};

// ------------- ClearCasePlugin
static ClearCasePluginPrivate *dd = nullptr;

ClearCasePluginPrivate::~ClearCasePluginPrivate()
{
    cleanCheckInMessageFile();
    // wait for sync thread to finish reading activities
    m_activityMutex.lock();
    m_activityMutex.unlock();
}

void ClearCasePluginPrivate::cleanCheckInMessageFile()
{
    if (!m_checkInMessageFileName.isEmpty()) {
        QFile::remove(m_checkInMessageFileName);
        m_checkInMessageFileName.clear();
        m_checkInView.clear();
    }
}

bool ClearCasePluginPrivate::isCheckInEditorOpen() const
{
    return !m_checkInMessageFileName.isEmpty();
}

/// Files in this directories are under ClearCase control
QStringList ClearCasePluginPrivate::getVobList() const
{
    QStringList args(QLatin1String("lsvob"));
    args << QLatin1String("-s");
    const ClearCaseResponse response =
            runCleartool(currentState().topLevel(), args, m_settings.timeOutS, SilentRun);

    return response.stdOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
}

/// Get the drive letter of a path
/// Necessary since QDir(directory).rootPath() returns C:/ in all cases
QString ClearCasePluginPrivate::getDriveLetterOfPath(const QString &directory)
{
    // cdUp until we get just the drive letter
    QDir dir(directory);
    while (!dir.isRoot() && dir.cdUp())
    { }

    return dir.path();
}

void ClearCasePluginPrivate::updateStatusForFile(const QString &absFile)
{
    setStatus(absFile, getFileStatus(absFile), false);
}

/// Give warning if a derived object is edited
void ClearCasePluginPrivate::updateEditDerivedObjectWarning(const QString &fileName,
                                                     const FileStatus::Status status)
{
    if (!isDynamic())
        return;

    IDocument *curDocument = EditorManager::currentDocument();
    if (!curDocument)
        return;

    InfoBar *infoBar = curDocument->infoBar();
    const Id derivedObjectWarning("ClearCase.DerivedObjectWarning");

    if (status == FileStatus::Derived) {
        if (!infoBar->canInfoBeAdded(derivedObjectWarning))
            return;

        infoBar->addInfo(InfoBarEntry(derivedObjectWarning,
                                      tr("Editing Derived Object: %1").arg(fileName)));
    } else {
        infoBar->removeInfo(derivedObjectWarning);
    }
}

FileStatus::Status ClearCasePluginPrivate::getFileStatus(const QString &fileName) const
{
    QTC_CHECK(!fileName.isEmpty());

    const QDir viewRootDir = QFileInfo(fileName).dir();
    const QString viewRoot = viewRootDir.path();

    QStringList args(QLatin1String("ls"));
    args << fileName;
    QString buffer = runCleartoolSync(viewRoot, args);

    const int atatpos = buffer.indexOf(QLatin1String("@@"));
    if (atatpos != -1) { // probably a managed file
        const QString absFile =
                viewRootDir.absoluteFilePath(
                    QDir::fromNativeSeparators(buffer.left(atatpos)));
        QTC_CHECK(QFileInfo::exists(absFile));
        QTC_CHECK(!absFile.isEmpty());

        // "cleartool ls" of a derived object looks like this:
        // /path/to/file/export/MyFile.h@@--11-13T19:52.266580
        const QChar c = buffer.at(atatpos + 2);
        const bool isDerivedObject = c != QLatin1Char('/') && c != QLatin1Char('\\');
        if (isDerivedObject)
            return FileStatus::Derived;

        // find first whitespace. anything before that is not interesting
        const int wspos = buffer.indexOf(QRegularExpression("\\s"));
        if (buffer.lastIndexOf(QLatin1String("CHECKEDOUT"), wspos) != -1)
            return FileStatus::CheckedOut;
        else
            return FileStatus::CheckedIn;
    } else {
        QTC_CHECK(QFileInfo::exists(fileName));
        QTC_CHECK(!fileName.isEmpty());
        return FileStatus::NotManaged;
    }
}

///
/// Check if the directory is managed by ClearCase.
///
/// There are 6 cases to consider for accessing ClearCase views:
///
/// 1) Windows: dynamic view under M:\<view_tag> (working dir view)
/// 2) Windows: dynamic view under Z:\ (similar to unix "set view" by using "subst" or "net use")
/// 3) Windows: snapshot view
/// 4) Unix: dynamic view under /view/<view_tag> (working dir view)
/// 5) Unix: dynamic view which are set view (transparent access in a shell process)
/// 6) Unix: snapshot view
///
/// Note: the drive letters M: and Z: can be chosen by the user. /view is the "view-root"
///       directory and is not configurable, while VOB names and mount points are configurable
///       by the ClearCase admin.
///
/// Note: All cases except #5 have a root directory, i.e., all files reside under a directory.
///       For #5 files are "mounted" and access is transparent (e.g., under /vobs).
///
/// For a view named "myview" and a VOB named "vobA" topLevels would be:
/// 1) M:/myview/vobA
/// 2) Z:/vobA
/// 3) c:/snapshots/myview/vobA
/// 4) /view/myview/vobs/vobA
/// 5) /vobs/vobA/
/// 6) /home/<username>/snapshots/myview/vobs/vobA
///
/// Note: The VOB directory is used as toplevel although the directory one up could have been
///       used on cases execpt 5. For case 5 it would have been /, which we don't want.
///
/// "cleartool pwv" returns the values for "set view" and "working directory view", also for
/// snapshot views.
///
/// \returns The ClearCase topLevel/VOB directory for this directory
QString ClearCasePluginPrivate::ccManagesDirectory(const QString &directory) const
{
    QStringList args(QLatin1String("pwv"));
    const ClearCaseResponse response =
            runCleartool(directory, args, m_settings.timeOutS, SilentRun);

    if (response.error)
        return QString();

    const QStringList result = response.stdOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    if (result.size() != 2)
        return QString();

    const QByteArray workingDirPattern("Working directory view: ");
    if (!result[0].startsWith(QLatin1String(workingDirPattern)))
        return QString();
    const QString workingDirectoryView = result[0].mid(workingDirPattern.size());

    const QByteArray setViewDirPattern("Set view: ");
    if (!result[1].startsWith(QLatin1String(setViewDirPattern)))
        return QString();
    const QString setView = result[1].mid(setViewDirPattern.size());

    const QString none(QLatin1String("** NONE **"));
    QString rootDir;
    if (setView != none || workingDirectoryView != none)
        rootDir = ccViewRoot(directory);
    else
        return QString();

    // Check if the directory is inside one of the known VOBs.
    static QStringList vobs;
    if (vobs.empty())
        vobs = getVobList();

    foreach (const QString &relativeVobDir, vobs) {
        const QString vobPath = QDir::cleanPath(rootDir + QDir::fromNativeSeparators(relativeVobDir));
        const bool isManaged = (vobPath == directory)
                || FilePath::fromString(directory).isChildOf(FilePath::fromString(vobPath));
        if (isManaged)
            return vobPath;
    }

    return QString();
}

/// Find the root path of a clearcase view. Precondition: This is a clearcase managed dir
QString ClearCasePluginPrivate::ccViewRoot(const QString &directory) const
{
    QStringList args(QLatin1String("pwv"));
    args << QLatin1String("-root");
    const ClearCaseResponse response =
            runCleartool(directory, args, m_settings.timeOutS, SilentRun);

    QString root = response.stdOut.trimmed();

    if (root.isEmpty()) {
        if (HostOsInfo::isWindowsHost())
            root = getDriveLetterOfPath(directory);
        else
            root = QLatin1Char('/');
    }

    return QDir::fromNativeSeparators(root);
}

/*! Find top level for view that contains \a directory
 *
 * Handles both dynamic views and snapshot views.
 */
QString ClearCasePluginPrivate::findTopLevel(const QString &directory) const
{
    // Do not check again if we've already tested that the dir is managed,
    // or if it is a child of a managed dir (top level).
    if ((directory == m_topLevel) ||
           FilePath::fromString(directory).isChildOf(FilePath::fromString(m_topLevel)))
        return m_topLevel;

    return ccManagesDirectory(directory);
}

bool ClearCasePlugin::initialize(const QStringList & /*arguments */, QString *errorMessage)
{
    Q_UNUSED(errorMessage)
    dd = new ClearCasePluginPrivate;
    return true;
}

void ClearCasePlugin::extensionsInitialized()
{
    dd->extensionsInitialized();
}

ClearCasePluginPrivate::ClearCasePluginPrivate()
    : VcsBase::VcsBasePluginPrivate(Context(CLEARCASE_CONTEXT)),
      m_statusMap(new StatusMap)
{
    dd = this;

    qRegisterMetaType<ClearCase::Internal::FileStatus::Status>("ClearCase::Internal::FileStatus::Status");
    connect(qApp, &QApplication::applicationStateChanged,
            this, [this](Qt::ApplicationState state) {
                if (state == Qt::ApplicationActive)
                    syncSlot();
            });

    using namespace Constants;
    using namespace Core::Constants;

    Context context(CLEARCASE_CONTEXT);

    connect(ICore::instance(), &ICore::coreAboutToClose, this, &ClearCasePluginPrivate::closing);
    connect(ProgressManager::instance(), &ProgressManager::allTasksFinished,
            this, &ClearCasePluginPrivate::tasksFinished);

    m_settings.fromSettings(ICore::settings());

    // update view name when changing active project
    connect(SessionManager::instance(), &SessionManager::startupProjectChanged,
            this, &ClearCasePluginPrivate::projectChanged);

    const QString description = QLatin1String("ClearCase");
    const QString prefix = QLatin1String("cc");
    // register cc prefix in Locator
    m_commandLocator = new CommandLocator("cc", description, prefix, this);

    //register actions
    ActionContainer *toolsContainer = ActionManager::actionContainer(M_TOOLS);

    ActionContainer *clearcaseMenu = ActionManager::createMenu(CMD_ID_CLEARCASE_MENU);
    clearcaseMenu->menu()->setTitle(tr("C&learCase"));
    toolsContainer->addMenu(clearcaseMenu);
    m_menuAction = clearcaseMenu->menu()->menuAction();
    Command *command;

    m_checkOutAction = new ParameterAction(tr("Check Out..."), tr("Check &Out \"%1\"..."), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_checkOutAction, CMD_ID_CHECKOUT,
        context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+O") : tr("Alt+L,Alt+O")));
    connect(m_checkOutAction, &QAction::triggered, this, &ClearCasePluginPrivate::checkOutCurrentFile);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_checkInCurrentAction = new ParameterAction(tr("Check &In..."), tr("Check &In \"%1\"..."), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_checkInCurrentAction, CMD_ID_CHECKIN, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+I") : tr("Alt+L,Alt+I")));
    connect(m_checkInCurrentAction, &QAction::triggered, this, &ClearCasePluginPrivate::startCheckInCurrentFile);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_undoCheckOutAction = new ParameterAction(tr("Undo Check Out"), tr("&Undo Check Out \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_undoCheckOutAction, CMD_ID_UNDOCHECKOUT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+U") : tr("Alt+L,Alt+U")));
    connect(m_undoCheckOutAction, &QAction::triggered, this, &ClearCasePluginPrivate::undoCheckOutCurrent);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_undoHijackAction = new ParameterAction(tr("Undo Hijack"), tr("Undo Hi&jack \"%1\""), ParameterAction::AlwaysEnabled, this);
    command = ActionManager::registerAction(m_undoHijackAction, CMD_ID_UNDOHIJACK, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+R") : tr("Alt+L,Alt+R")));
    connect(m_undoHijackAction, &QAction::triggered, this, &ClearCasePluginPrivate::undoHijackCurrent);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    clearcaseMenu->addSeparator(context);

    m_diffCurrentAction = new ParameterAction(tr("Diff Current File"), tr("&Diff \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_diffCurrentAction,
        CMD_ID_DIFF_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+D") : tr("Alt+L,Alt+D")));
    connect(m_diffCurrentAction, &QAction::triggered, this, &ClearCasePluginPrivate::diffCurrentFile);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_historyCurrentAction = new ParameterAction(tr("History Current File"), tr("&History \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_historyCurrentAction,
        CMD_ID_HISTORY_CURRENT, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+H") : tr("Alt+L,Alt+H")));
    connect(m_historyCurrentAction, &QAction::triggered, this,
        &ClearCasePluginPrivate::historyCurrentFile);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_annotateCurrentAction = new ParameterAction(tr("Annotate Current File"), tr("&Annotate \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_annotateCurrentAction,
        CMD_ID_ANNOTATE, context);
    command->setAttribute(Command::CA_UpdateText);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+A") : tr("Alt+L,Alt+A")));
    connect(m_annotateCurrentAction, &QAction::triggered, this,
        &ClearCasePluginPrivate::annotateCurrentFile);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_addFileAction = new ParameterAction(tr("Add File..."), tr("Add File \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_addFileAction, CMD_ID_ADD_FILE, context);
    command->setAttribute(Command::CA_UpdateText);
    connect(m_addFileAction, &QAction::triggered, this, &ClearCasePluginPrivate::addCurrentFile);
    clearcaseMenu->addAction(command);

    clearcaseMenu->addSeparator(context);

    m_diffActivityAction = new QAction(tr("Diff A&ctivity..."), this);
    m_diffActivityAction->setEnabled(false);
    command = ActionManager::registerAction(m_diffActivityAction, CMD_ID_DIFF_ACTIVITY, context);
    connect(m_diffActivityAction, &QAction::triggered, this, &ClearCasePluginPrivate::diffActivity);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_checkInActivityAction = new ParameterAction(tr("Ch&eck In Activity"), tr("Chec&k In Activity \"%1\"..."), ParameterAction::EnabledWithParameter, this);
    m_checkInActivityAction->setEnabled(false);
    command = ActionManager::registerAction(m_checkInActivityAction, CMD_ID_CHECKIN_ACTIVITY, context);
    connect(m_checkInActivityAction, &QAction::triggered, this, &ClearCasePluginPrivate::startCheckInActivity);
    command->setAttribute(Command::CA_UpdateText);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    clearcaseMenu->addSeparator(context);

    m_updateIndexAction = new QAction(tr("Update Index"), this);
    command = ActionManager::registerAction(m_updateIndexAction, CMD_ID_UPDATEINDEX, context);
    connect(m_updateIndexAction, &QAction::triggered, this, &ClearCasePluginPrivate::updateIndex);
    clearcaseMenu->addAction(command);

    m_updateViewAction = new ParameterAction(tr("Update View"), tr("U&pdate View \"%1\""), ParameterAction::EnabledWithParameter, this);
    command = ActionManager::registerAction(m_updateViewAction, CMD_ID_UPDATE_VIEW, context);
    connect(m_updateViewAction, &QAction::triggered, this, &ClearCasePluginPrivate::updateView);
    command->setAttribute(Command::CA_UpdateText);
    clearcaseMenu->addAction(command);

    clearcaseMenu->addSeparator(context);

    m_checkInAllAction = new QAction(tr("Check In All &Files..."), this);
    command = ActionManager::registerAction(m_checkInAllAction, CMD_ID_CHECKIN_ALL, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+F") : tr("Alt+L,Alt+F")));
    connect(m_checkInAllAction, &QAction::triggered, this, &ClearCasePluginPrivate::startCheckInAll);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);

    m_statusAction = new QAction(tr("View &Status"), this);
    command = ActionManager::registerAction(m_statusAction, CMD_ID_STATUS, context);
    command->setDefaultKeySequence(QKeySequence(useMacShortcuts ? tr("Meta+L,Meta+S") : tr("Alt+L,Alt+S")));
    connect(m_statusAction, &QAction::triggered, this, &ClearCasePluginPrivate::viewStatus);
    clearcaseMenu->addAction(command);
    m_commandLocator->appendCommand(command);
}

// called before closing the submit editor
bool ClearCasePluginPrivate::submitEditorAboutToClose()
{
    if (!isCheckInEditorOpen())
        return true;

    auto editor = qobject_cast<ClearCaseSubmitEditor *>(submitEditor());
    QTC_ASSERT(editor, return true);
    IDocument *editorDocument = editor->document();
    QTC_ASSERT(editorDocument, return true);

    // Submit editor closing. Make it write out the check in message
    // and retrieve files
    const QFileInfo editorFile = editorDocument->filePath().toFileInfo();
    const QFileInfo changeFile(m_checkInMessageFileName);
    if (editorFile.absoluteFilePath() != changeFile.absoluteFilePath())
        return true; // Oops?!

    // Prompt user. Force a prompt unless submit was actually invoked (that
    // is, the editor was closed or shutdown).
    bool prompt = m_settings.promptToCheckIn;
    const VcsBaseSubmitEditor::PromptSubmitResult answer =
            editor->promptSubmit(this, &prompt, !m_submitActionTriggered);
    m_submitActionTriggered = false;
    switch (answer) {
    case VcsBaseSubmitEditor::SubmitCanceled:
        return false; // Keep editing and change file
    case VcsBaseSubmitEditor::SubmitDiscarded:
        cleanCheckInMessageFile();
        return true; // Cancel all
    default:
        break;
    }
    // If user changed
    if (prompt != m_settings.promptToCheckIn) {
        m_settings.promptToCheckIn = prompt;
        m_settings.toSettings(ICore::settings());
    }

    const QStringList fileList = editor->checkedFiles();
    bool closeEditor = true;
    if (!fileList.empty()) {
        // get message & check in
        closeEditor = DocumentManager::saveDocument(editorDocument);
        if (closeEditor) {
            ClearCaseSubmitEditorWidget *widget = editor->submitEditorWidget();
            closeEditor = vcsCheckIn(m_checkInMessageFileName, fileList, widget->activity(),
                                   widget->isIdentical(), widget->isPreserve(),
                                   widget->activityChanged());
        }
    }
    // vcsCheckIn might fail if some of the files failed to check-in (though it does check-in
    // those who didn't fail). Therefore, if more than one file was sent, consider it as success
    // anyway (sync will be called from vcsCheckIn for next attempt)
    closeEditor |= (fileList.count() > 1);
    if (closeEditor)
        cleanCheckInMessageFile();
    return closeEditor;
}

void ClearCasePluginPrivate::diffCheckInFiles(const QStringList &files)
{
    ccDiffWithPred(m_checkInView, files);
}

static void setWorkingDirectory(IEditor *editor, const QString &wd)
{
    if (auto ve = qobject_cast<VcsBaseEditorWidget*>(editor->widget()))
        ve->setWorkingDirectory(wd);
}

//! retrieve full location of predecessor of \a version
QString ClearCasePluginPrivate::ccGetPredecessor(const QString &version) const
{
    QStringList args(QLatin1String("describe"));
    args << QLatin1String("-fmt") << QLatin1String("%En@@%PSn") << version;
    const ClearCaseResponse response =
            runCleartool(currentState().topLevel(), args, m_settings.timeOutS, SilentRun);
    if (response.error || response.stdOut.endsWith(QLatin1Char('@'))) // <name-unknown>@@
        return QString();
    else
        return response.stdOut;
}

//! Get a list of paths to active VOBs.
//! Paths are relative to viewRoot
QStringList ClearCasePluginPrivate::ccGetActiveVobs() const
{
    QStringList res;
    QStringList args(QLatin1String("lsvob"));
    const QString theViewRoot = viewRoot();

    const ClearCaseResponse response =
            runCleartool(theViewRoot, args, m_settings.timeOutS, SilentRun);
    if (response.error)
        return res;

    // format of output unix:
    // * /path/to/vob   /path/to/vob/storage.vbs <and some text omitted here>
    // format of output windows:
    // * \vob     \\share\path\to\vob\storage.vbs <and some text omitted here>
    QString prefix = theViewRoot;
    if (!prefix.endsWith(QLatin1Char('/')))
        prefix += QLatin1Char('/');

    const QDir theViewRootDir(theViewRoot);
    foreach (const QString &line, response.stdOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const bool isActive = line.at(0) == QLatin1Char('*');
        if (!isActive)
            continue;

        const QString dir =
                QDir::fromNativeSeparators(line.mid(3, line.indexOf(QLatin1Char(' '), 3) - 3));
        const QString relativeDir = theViewRootDir.relativeFilePath(dir);

        // Snapshot views does not necessarily have all active VOBs loaded, so we'll have to
        // check if the dirs exists as well. Else the command will work, but the output will
        // complain about the element not being loaded.
        if (QFile::exists(prefix + relativeDir))
            res.append(relativeDir);
    }
    return res;
}

void ClearCasePluginPrivate::checkAndReIndexUnknownFile(const QString &file)
{
    if (isDynamic()) {
        // reindex unknown files
        if (m_statusMap->value(file, FileStatus(FileStatus::Unknown)).status == FileStatus::Unknown)
            updateStatusForFile(file);
    }
}

// file must be absolute, and using '/' path separator
FileStatus ClearCasePluginPrivate::vcsStatus(const QString &file) const
{
    return m_statusMap->value(file, FileStatus(FileStatus::Unknown));
}

QString ClearCasePluginPrivate::ccGetFileActivity(const QString &workingDir, const QString &file)
{
    QStringList args(QLatin1String("lscheckout"));
    args << QLatin1String("-fmt") << QLatin1String("%[activity]p");
    args << file;
    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutS, SilentRun);
    return response.stdOut;
}

ClearCaseSubmitEditor *ClearCasePluginPrivate::openClearCaseSubmitEditor(const QString &fileName, bool isUcm)
{
    IEditor *editor =
            EditorManager::openEditor(fileName, Constants::CLEARCASECHECKINEDITOR_ID);
    auto submitEditor = qobject_cast<ClearCaseSubmitEditor*>(editor);
    QTC_ASSERT(submitEditor, return nullptr);
    connect(submitEditor, &VcsBaseSubmitEditor::diffSelectedFiles,
            this, &ClearCasePluginPrivate::diffCheckInFiles);
    submitEditor->setCheckScriptWorkingDirectory(m_checkInView);
    submitEditor->setIsUcm(isUcm);
    return submitEditor;
}

QString fileStatusToText(FileStatus fileStatus)
{
    switch (fileStatus.status)
    {
    case FileStatus::CheckedIn:
        return QLatin1String("CheckedIn");
    case FileStatus::CheckedOut:
        return QLatin1String("CheckedOut");
    case FileStatus::Hijacked:
        return QLatin1String("Hijacked");
    case FileStatus::Missing:
        return QLatin1String("Missing");
    case FileStatus::NotManaged:
        return QLatin1String("ViewPrivate");
    case FileStatus::Unknown:
        return QLatin1String("Unknown");
    default:
        return QLatin1String("default");
    }
}

void ClearCasePluginPrivate::updateStatusActions()
{
    FileStatus fileStatus = FileStatus::Unknown;
    bool hasFile = currentState().hasFile();
    if (hasFile) {
        QString absoluteFileName = currentState().currentFile();
        checkAndReIndexUnknownFile(absoluteFileName);
        fileStatus = vcsStatus(absoluteFileName);

        updateEditDerivedObjectWarning(absoluteFileName, fileStatus.status);

        if (Constants::debug)
            qDebug() << Q_FUNC_INFO << absoluteFileName << ", status = "
                     << fileStatusToText(fileStatus.status) << "(" << fileStatus.status << ")";
    }

    m_checkOutAction->setEnabled(hasFile && (fileStatus.status & (FileStatus::CheckedIn | FileStatus::Hijacked)));
    m_undoCheckOutAction->setEnabled(hasFile && (fileStatus.status & FileStatus::CheckedOut));
    m_undoHijackAction->setEnabled(!m_viewData.isDynamic && hasFile && (fileStatus.status & FileStatus::Hijacked));
    m_checkInCurrentAction->setEnabled(hasFile && (fileStatus.status & FileStatus::CheckedOut));
    m_addFileAction->setEnabled(hasFile && (fileStatus.status & FileStatus::NotManaged));
    m_diffCurrentAction->setEnabled(hasFile && (fileStatus.status != FileStatus::NotManaged));
    m_historyCurrentAction->setEnabled(hasFile && (fileStatus.status != FileStatus::NotManaged));
    m_annotateCurrentAction->setEnabled(hasFile && (fileStatus.status != FileStatus::NotManaged));

    m_checkInActivityAction->setEnabled(m_viewData.isUcm);
    m_diffActivityAction->setEnabled(m_viewData.isUcm);
}

void ClearCasePluginPrivate::updateActions(VcsBasePluginPrivate::ActionState as)
{
    if (!enableMenuAction(as, m_menuAction)) {
        m_commandLocator->setEnabled(false);
        return;
    }
    const VcsBasePluginState state = currentState();
    const bool hasTopLevel = state.hasTopLevel();
    m_commandLocator->setEnabled(hasTopLevel);
    if (hasTopLevel) {
        const QString topLevel = state.topLevel();
        if (m_topLevel != topLevel) {
            m_topLevel = topLevel;
            m_viewData = ccGetView(topLevel);
        }
    }

    m_updateViewAction->setParameter(m_viewData.isDynamic ? QString() : m_viewData.name);

    const QString fileName = state.currentFileName();
    m_checkOutAction->setParameter(fileName);
    m_undoCheckOutAction->setParameter(fileName);
    m_undoHijackAction->setParameter(fileName);
    m_diffCurrentAction->setParameter(fileName);
    m_checkInCurrentAction->setParameter(fileName);
    m_historyCurrentAction->setParameter(fileName);
    m_annotateCurrentAction->setParameter(fileName);
    m_addFileAction->setParameter(fileName);
    m_updateIndexAction->setEnabled(!m_settings.disableIndexer);

    updateStatusActions();
}

QString ClearCasePluginPrivate::commitDisplayName() const
{
    return tr("Check In");
}

void ClearCasePluginPrivate::checkOutCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsOpen(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void ClearCasePluginPrivate::addCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAdd(state.currentFileTopLevel(), state.relativeCurrentFile());
}

// Set the FileStatus of file given in absolute path
void ClearCasePluginPrivate::setStatus(const QString &file, FileStatus::Status status, bool update)
{
    QTC_CHECK(!file.isEmpty());
    m_statusMap->insert(file, FileStatus(status, QFileInfo(file).permissions()));

    if (update && currentState().currentFile() == file)
        QMetaObject::invokeMethod(this, "updateStatusActions");
}

void ClearCasePluginPrivate::undoCheckOutCurrent()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    QString file = state.relativeCurrentFile();
    const QString fileName = QDir::toNativeSeparators(file);

    QStringList args(QLatin1String("diff"));
    args << QLatin1String("-diff_format") << QLatin1String("-predecessor");
    args << fileName;

    const ClearCaseResponse diffResponse =
            runCleartool(state.currentFileTopLevel(), args, m_settings.timeOutS, 0);

    bool different = diffResponse.error; // return value is 1 if there is any difference
    bool keep = false;
    if (different) {
        Ui::UndoCheckOut uncoUi;
        QDialog uncoDlg;
        uncoUi.setupUi(&uncoDlg);
        uncoUi.lblMessage->setText(tr("Do you want to undo the check out of \"%1\"?").arg(fileName));
        uncoUi.chkKeep->setChecked(m_settings.keepFileUndoCheckout);
        if (uncoDlg.exec() != QDialog::Accepted)
            return;
        keep = uncoUi.chkKeep->isChecked();
        if (keep != m_settings.keepFileUndoCheckout) {
            m_settings.keepFileUndoCheckout = keep;
            m_settings.toSettings(ICore::settings());
        }
    }
    vcsUndoCheckOut(state.topLevel(), file, keep);
}

bool ClearCasePluginPrivate::vcsUndoCheckOut(const QString &workingDir, const QString &fileName, bool keep)
{
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDir << fileName << keep;

    FileChangeBlocker fcb(fileName);

    // revert
    QStringList args(QLatin1String("uncheckout"));
    args << QLatin1String(keep ? "-keep" : "-rm");
    args << QDir::toNativeSeparators(fileName);

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutS,
                         VcsCommand::ShowStdOut | VcsCommand::FullySynchronously);

    if (!response.error) {
        const QString absPath = workingDir + QLatin1Char('/') + fileName;

        if (!m_settings.disableIndexer)
            setStatus(absPath, FileStatus::CheckedIn);
        emit filesChanged(QStringList(absPath));
    }
    return !response.error;
}


/*! Undo a hijacked file in a snapshot view
 *
 * Runs cleartool update -overwrite \a fileName in \a workingDir
 * if \a keep is true, renames hijacked files to <filename>.keep. Otherwise it is overwritten
 */
bool ClearCasePluginPrivate::vcsUndoHijack(const QString &workingDir, const QString &fileName, bool keep)
{
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDir << fileName << keep;
    QStringList args(QLatin1String("update"));
    args << QLatin1String(keep ? "-rename" : "-overwrite");
    args << QLatin1String("-log");
    if (HostOsInfo::isWindowsHost())
        args << QLatin1String("NUL");
    else
    args << QLatin1String("/dev/null");
    args << QDir::toNativeSeparators(fileName);

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutS,
                   VcsCommand::ShowStdOut | VcsCommand::FullySynchronously);
    if (!response.error && !m_settings.disableIndexer) {
        const QString absPath = workingDir + QLatin1Char('/') + fileName;
        setStatus(absPath, FileStatus::CheckedIn);
    }
    return !response.error;
}

void ClearCasePluginPrivate::undoHijackCurrent()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    const QString fileName = state.relativeCurrentFile();

    bool keep = false;
    bool askKeep = true;
    if (m_settings.extDiffAvailable) {
        QString diffres = diffExternal(ccGetFileVersion(state.topLevel(), fileName), fileName);
        if (diffres.at(0) == QLatin1Char('F')) // Files are identical
            askKeep = false;
    }
    if (askKeep) {
        Ui::UndoCheckOut unhijackUi;
        QDialog unhijackDlg;
        unhijackUi.setupUi(&unhijackDlg);
        unhijackDlg.setWindowTitle(tr("Undo Hijack File"));
        unhijackUi.lblMessage->setText(tr("Do you want to undo hijack of \"%1\"?")
                                       .arg(QDir::toNativeSeparators(fileName)));
        if (unhijackDlg.exec() != QDialog::Accepted)
            return;
        keep = unhijackUi.chkKeep->isChecked();
    }

    FileChangeBlocker fcb(state.currentFile());

    // revert
    if (vcsUndoHijack(state.currentFileTopLevel(), fileName, keep))
        emit filesChanged(QStringList(state.currentFile()));
}

QString ClearCasePluginPrivate::ccGetFileVersion(const QString &workingDir, const QString &file) const
{
    QStringList args(QLatin1String("ls"));
    args << QLatin1String("-short") << file;
    return runCleartoolSync(workingDir, args).trimmed();
}

void ClearCasePluginPrivate::ccDiffWithPred(const QString &workingDir, const QStringList &files)
{
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << files;
    const QString source = VcsBaseEditor::getSource(workingDir, files);
    QTextCodec *codec = source.isEmpty() ? static_cast<QTextCodec *>(nullptr)
                                         : VcsBaseEditor::getCodec(source);

    if ((m_settings.diffType == GraphicalDiff) && (files.count() == 1)) {
        const QString file = files.first();
        const QString absFilePath = workingDir + QLatin1Char('/') + file;
        if (vcsStatus(absFilePath).status == FileStatus::Hijacked)
            diffGraphical(ccGetFileVersion(workingDir, file), file);
        else
            diffGraphical(file);
        return; // done here, diff is opened in a new window
    }
    if (!m_settings.extDiffAvailable) {
        VcsOutputWindow::appendError(tr("External diff is required to compare multiple files."));
        return;
    }
    QString result;
    foreach (const QString &file, files) {
        const QString absFilePath = workingDir + QLatin1Char('/') + file;
        if (vcsStatus(QDir::fromNativeSeparators(absFilePath)).status == FileStatus::Hijacked)
            result += diffExternal(ccGetFileVersion(workingDir, file), file);
        else
            result += diffExternal(file);
    }

    QString diffname;

    // diff of a single file? re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBaseEditor::editorTag(DiffOutput, workingDir, files);
    if (files.count() == 1) {
        // Show in the same editor if diff has been executed before
        if (IEditor *existingEditor = VcsBaseEditor::locateEditorByTag(tag)) {
            existingEditor->document()->setContents(result.toUtf8());
            EditorManager::activateEditor(existingEditor);
            setWorkingDirectory(existingEditor, workingDir);
            return;
        }
        diffname = QDir::toNativeSeparators(files.first());
    }
    const QString title = QString::fromLatin1("cc diff %1").arg(diffname);
    IEditor *editor = showOutputInEditor(title, result, diffEditorParameters.id, source, codec);
    setWorkingDirectory(editor, workingDir);
    VcsBaseEditor::tagEditor(editor, tag);
    auto diffEditorWidget = qobject_cast<ClearCaseEditorWidget *>(editor->widget());
    QTC_ASSERT(diffEditorWidget, return);
    if (files.count() == 1)
        editor->setProperty("originalFileName", diffname);
}

QStringList ClearCasePluginPrivate::ccGetActivityVersions(const QString &workingDir, const QString &activity)
{
    QStringList args(QLatin1String("lsactivity"));
    args << QLatin1String("-fmt") << QLatin1String("%[versions]Cp") << activity;
    const ClearCaseResponse response =
        runCleartool(workingDir, args, m_settings.timeOutS, SilentRun);
    if (response.error)
        return QStringList();
    QStringList versions = response.stdOut.split(QLatin1String(", "));
    versions.sort();
    return versions;
}

void ClearCasePluginPrivate::rmdir(const QString &path)
{
    QDir dir(path);
    foreach (QFileInfo fi, dir.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot)) {
        if (fi.isDir()) {
            rmdir(fi.canonicalFilePath());
            dir.rmdir(fi.baseName());
        }
        else
            QFile::remove(fi.canonicalFilePath());
    }
}

void ClearCasePluginPrivate::diffActivity()
{
    using FileVerIt = QMap<QString, QStringPair>::Iterator;

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO;
    if (!m_settings.extDiffAvailable) {
        VcsOutputWindow::appendError(tr("External diff is required to compare multiple files."));
        return;
    }
    QString topLevel = state.topLevel();
    QString activity = QInputDialog::getText(ICore::dialogParent(), tr("Enter Activity"),
                                             tr("Activity Name"), QLineEdit::Normal, m_activity);
    if (activity.isEmpty())
        return;
    QStringList versions = ccGetActivityVersions(topLevel, activity);

    QString result;
    // map from fileName to (first, latest) pair
    QMap<QString, QStringPair> filever;
    int topLevelLen = topLevel.length();
    foreach (const QString &version, versions) {
        QString shortver = version.mid(topLevelLen + 1);
        int atatpos = shortver.indexOf(QLatin1String("@@"));
        if (atatpos != -1) {
            QString file = shortver.left(atatpos);
            // latest version - updated each line
            filever[file].second = shortver;

            // pre-first version. only for the first occurrence
            if (filever[file].first.isEmpty()) {
                int verpos = shortver.lastIndexOf(QRegularExpression("[^0-9]")) + 1;
                int vernum = shortver.midRef(verpos).toInt();
                if (vernum)
                    --vernum;
                shortver.replace(verpos, shortver.length() - verpos, QString::number(vernum));
                // first version
                filever[file].first = shortver;
            }
        }
    }

    if ((m_settings.diffType == GraphicalDiff) && (filever.count() == 1)) {
        QStringPair pair(filever.values().at(0));
        diffGraphical(pair.first, pair.second);
        return;
    }
    rmdir(Utils::TemporaryDirectory::masterDirectoryPath() + QLatin1String("/ccdiff/") + activity);
    QDir(Utils::TemporaryDirectory::masterDirectoryPath()).rmpath(QLatin1String("ccdiff/") + activity);
    m_diffPrefix = activity;
    const FileVerIt fend = filever.end();
    for (FileVerIt it = filever.begin(); it != fend; ++it) {
        QStringPair &pair(it.value());
        if (pair.first.contains(QLatin1String("CHECKEDOUT")))
            pair.first = ccGetPredecessor(pair.first.left(pair.first.indexOf(QLatin1String("@@"))));
        result += diffExternal(pair.first, pair.second, true);
    }
    m_diffPrefix.clear();
    const QString title = QString::fromLatin1("%1.patch").arg(activity);
    IEditor *editor = showOutputInEditor(title, result, diffEditorParameters.id, activity, nullptr);
    setWorkingDirectory(editor, topLevel);
}

void ClearCasePluginPrivate::diffCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    ccDiffWithPred(state.topLevel(), QStringList(state.relativeCurrentFile()));
}

void ClearCasePluginPrivate::startCheckInCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    QString nativeFile = QDir::toNativeSeparators(state.relativeCurrentFile());
    startCheckIn(state.currentFileTopLevel(), QStringList(nativeFile));
}

void ClearCasePluginPrivate::startCheckInAll()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    QString topLevel = state.topLevel();
    QStringList files;
    for (StatusMap::ConstIterator iterator = m_statusMap->constBegin();
         iterator != m_statusMap->constEnd();
         ++iterator)
    {
        if (iterator.value().status == FileStatus::CheckedOut)
            files.append(QDir::toNativeSeparators(iterator.key()));
    }
    files.sort();
    startCheckIn(topLevel, files);
}

void ClearCasePluginPrivate::startCheckInActivity()
{
    QTC_ASSERT(isUcm(), return);

    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasProject(), return);

    QDialog dlg;
    auto layout = new QVBoxLayout(&dlg);
    auto actSelector = new ActivitySelector(&dlg);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal, &dlg);
    connect(buttonBox, &QDialogButtonBox::accepted, &dlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &dlg, &QDialog::reject);
    layout->addWidget(actSelector);
    layout->addWidget(buttonBox);
    dlg.setWindowTitle(tr("Check In Activity"));
    if (!dlg.exec())
        return;

    QString topLevel = state.topLevel();
    int topLevelLen = topLevel.length();
    QStringList versions = ccGetActivityVersions(topLevel, actSelector->activity());
    QStringList files;
    QString last;
    foreach (const QString &version, versions) {
        int atatpos = version.indexOf(QLatin1String("@@"));
        if ((atatpos != -1) && (version.indexOf(QLatin1String("CHECKEDOUT"), atatpos) != -1)) {
            QString file = version.left(atatpos);
            if (file != last)
                files.append(file.mid(topLevelLen+1));
            last = file;
        }
    }
    files.sort();
    startCheckIn(topLevel, files);
}

/* Start check in of files of a single repository by displaying
 * template and files in a submit editor. On closing, the real
 * check in will start. */
void ClearCasePluginPrivate::startCheckIn(const QString &workingDir, const QStringList &files)
{
    if (!promptBeforeCommit())
        return;

    if (raiseSubmitEditor())
        return;

    if (isCheckInEditorOpen()) {
        VcsOutputWindow::appendWarning(tr("Another check in is currently being executed."));
        return;
    }

    // Get list of added/modified/deleted files
    if (files.empty()) {
        VcsOutputWindow::appendWarning(tr("There are no modified files."));
        return;
    }
    // Create a new submit change file containing the submit template
    TempFileSaver saver;
    saver.setAutoRemove(false);
    QString submitTemplate;
    if (files.count() == 1)
        submitTemplate = ccGetComment(workingDir, files.first());
    // Create a submit
    saver.write(submitTemplate.toUtf8());
    if (!saver.finalize()) {
        VcsOutputWindow::appendError(saver.errorString());
        return;
    }
    m_checkInMessageFileName = saver.fileName();
    m_checkInView = workingDir;
    // Create a submit editor and set file list
    ClearCaseSubmitEditor *editor = openClearCaseSubmitEditor(m_checkInMessageFileName, m_viewData.isUcm);
    setSubmitEditor(editor);
    editor->setStatusList(files);

    if (m_viewData.isUcm && (files.size() == 1)) {
        QString activity = ccGetFileActivity(workingDir, files.first());
        editor->submitEditorWidget()->setActivity(activity);
    }
}

void ClearCasePluginPrivate::historyCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    history(state.currentFileTopLevel(), QStringList(state.relativeCurrentFile()), true);
}

void ClearCasePluginPrivate::updateView()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasTopLevel(), return);
    ccUpdate(state.topLevel());
}

void ClearCasePluginPrivate::history(const QString &workingDir,
                               const QStringList &files,
                               bool enableAnnotationContextMenu)
{
    QTextCodec *codec = VcsBaseEditor::getCodec(workingDir, files);
    // no need for temp file
    QStringList args(QLatin1String("lshistory"));
    if (m_settings.historyCount > 0)
        args << QLatin1String("-last") << QString::number(m_settings.historyCount);
    if (!m_intStream.isEmpty())
        args << QLatin1String("-branch") << m_intStream;
    foreach (const QString &file, files)
        args.append(QDir::toNativeSeparators(file));

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutS, 0, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file

    const QString id = VcsBaseEditor::getTitleId(workingDir, files);
    const QString tag = VcsBaseEditor::editorTag(LogOutput, workingDir, files);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(response.stdOut.toUtf8());
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cc history %1").arg(id);
        const QString source = VcsBaseEditor::getSource(workingDir, files);
        IEditor *newEditor = showOutputInEditor(title, response.stdOut, logEditorParameters.id, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        if (enableAnnotationContextMenu)
            VcsBaseEditor::getVcsBaseEditor(newEditor)->setFileLogAnnotateEnabled(true);
    }
}

void ClearCasePluginPrivate::viewStatus()
{
    if (m_viewData.name.isEmpty())
        m_viewData = ccGetView(m_topLevel);
    QTC_ASSERT(!m_viewData.name.isEmpty() && !m_settings.disableIndexer, return);
    VcsOutputWindow::append(QLatin1String("Indexed files status (C=Checked Out, "
                                          "H=Hijacked, ?=Missing)"),
                            VcsOutputWindow::Command, true);
    bool anymod = false;
    for (StatusMap::ConstIterator it = m_statusMap->constBegin();
         it != m_statusMap->constEnd();
         ++it)
    {
        char cstat = 0;
        switch (it.value().status) {
            case FileStatus::CheckedOut: cstat = 'C'; break;
            case FileStatus::Hijacked:   cstat = 'H'; break;
            case FileStatus::Missing:    cstat = '?'; break;
            default: break;
        }
        if (cstat) {
            VcsOutputWindow::append(QString::fromLatin1("%1    %2\n")
                           .arg(cstat)
                           .arg(QDir::toNativeSeparators(it.key())));
            anymod = true;
        }
    }
    if (!anymod)
        VcsOutputWindow::appendWarning(QLatin1String("No modified files found."));
}

void ClearCasePluginPrivate::ccUpdate(const QString &workingDir, const QStringList &relativePaths)
{
    QStringList args(QLatin1String("update"));
    args << QLatin1String("-noverwrite");
    if (!relativePaths.isEmpty())
        args.append(relativePaths);
    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.longTimeOutS(), VcsCommand::ShowStdOut);
    if (!response.error)
        emit repositoryChanged(workingDir);
}

void ClearCasePluginPrivate::annotateCurrentFile()
{
    const VcsBasePluginState state = currentState();
    QTC_ASSERT(state.hasFile(), return);
    vcsAnnotateHelper(state.currentFileTopLevel(), state.relativeCurrentFile());
}

void ClearCasePluginPrivate::vcsAnnotateHelper(const QString &workingDir, const QString &file,
                                               const QString &revision /* = QString() */,
                                               int lineNumber /* = -1 */) const
{
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << file;

    QTextCodec *codec = VcsBaseEditor::getCodec(file);

    // Determine id
    QString id = file;
    if (!revision.isEmpty())
        id += QLatin1String("@@") + revision;

    QStringList args(QLatin1String("annotate"));
    args << QLatin1String("-nco") << QLatin1String("-f");
    args << QLatin1String("-fmt") << QLatin1String("%-14.14Sd %-8.8u | ");
    args << QLatin1String("-out") << QLatin1String("-");
    args.append(QDir::toNativeSeparators(id));

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutS, 0, codec);
    if (response.error)
        return;

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString source = workingDir + QLatin1Char('/') + file;
    if (lineNumber <= 0)
        lineNumber = VcsBaseEditor::lineNumberOfCurrentEditor(source);

    QString headerSep(QLatin1String("-------------------------------------------------"));
    int pos = qMax(0, response.stdOut.indexOf(headerSep));
    // there are 2 identical headerSep lines - skip them
    int dataStart = response.stdOut.indexOf(QLatin1Char('\n'), pos) + 1;
    dataStart = response.stdOut.indexOf(QLatin1Char('\n'), dataStart) + 1;
    QString res;
    QTextStream stream(&res, QIODevice::WriteOnly | QIODevice::Text);
    stream << response.stdOut.mid(dataStart) << headerSep << QLatin1Char('\n')
           << headerSep << QLatin1Char('\n') << response.stdOut.left(pos);
    const QStringList files = QStringList(file);
    const QString tag = VcsBaseEditor::editorTag(AnnotateOutput, workingDir, files);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(res.toUtf8());
        VcsBaseEditor::gotoLineOfEditor(editor, lineNumber);
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cc annotate %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, res, annotateEditorParameters.id, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
        VcsBaseEditor::gotoLineOfEditor(newEditor, lineNumber);
    }
}

void ClearCasePluginPrivate::vcsDescribe(const QString &source, const QString &changeNr)
{
    const QFileInfo fi(source);
    QString topLevel;
    const bool manages = managesDirectory(fi.isDir() ? source : fi.absolutePath(), &topLevel);
    if (!manages || topLevel.isEmpty())
        return;
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << source << topLevel << changeNr;
    QString description;
    QString relPath = QDir::toNativeSeparators(QDir(topLevel).relativeFilePath(source));
    QString id = QString::fromLatin1("%1@@%2").arg(relPath).arg(changeNr);

    QStringList args(QLatin1String("describe"));
    args.push_back(id);
    QTextCodec *codec = VcsBaseEditor::getCodec(source);
    const ClearCaseResponse response = runCleartool(topLevel, args, m_settings.timeOutS, 0, codec);
    description = response.stdOut;
    if (m_settings.extDiffAvailable)
        description += diffExternal(id);

    // Re-use an existing view if possible to support
    // the common usage pattern of continuously changing and diffing a file
    const QString tag = VcsBaseEditor::editorTag(DiffOutput, source, QStringList(), changeNr);
    if (IEditor *editor = VcsBaseEditor::locateEditorByTag(tag)) {
        editor->document()->setContents(description.toUtf8());
        EditorManager::activateEditor(editor);
    } else {
        const QString title = QString::fromLatin1("cc describe %1").arg(id);
        IEditor *newEditor = showOutputInEditor(title, description, diffEditorParameters.id, source, codec);
        VcsBaseEditor::tagEditor(newEditor, tag);
    }
}

void ClearCasePluginPrivate::commitFromEditor()
{
    m_submitActionTriggered = true;
    QTC_ASSERT(submitEditor(), return);
    EditorManager::closeDocument(submitEditor()->document());
}

QString ClearCasePluginPrivate::runCleartoolSync(const QString &workingDir,
                                          const QStringList &arguments) const
{
    return runCleartool(workingDir, arguments, m_settings.timeOutS, SilentRun).stdOut;
}

ClearCaseResponse
ClearCasePluginPrivate::runCleartool(const QString &workingDir,
                              const QStringList &arguments,
                              int timeOutS,
                              unsigned flags,
                              QTextCodec *outputCodec) const
{
    const QString executable = m_settings.ccBinaryPath;
    ClearCaseResponse response;
    if (executable.isEmpty()) {
        response.error = true;
        response.message = tr("No ClearCase executable specified.");
        return response;
    }

    const SynchronousProcessResponse sp_resp =
                  VcsBase::runVcs(workingDir,
                                  {executable, arguments},
                                  timeOutS,
                                  flags, outputCodec);

    response.error = sp_resp.result != SynchronousProcessResponse::Finished;
    if (response.error)
        response.message = sp_resp.exitMessage(executable, timeOutS);
    response.stdErr = sp_resp.stdErr();
    response.stdOut = sp_resp.stdOut();
    return response;
}

IEditor *ClearCasePluginPrivate::showOutputInEditor(const QString& title, const QString &output,
                                                   Utils::Id id, const QString &source,
                                                   QTextCodec *codec) const
{
    if (Constants::debug)
        qDebug() << "ClearCasePlugin::showOutputInEditor" << title << id.name()
                 <<  "Size= " << output.size() << debugCodec(codec);
    QString s = title;
    IEditor *editor = EditorManager::openEditorWithContents(id, &s, output.toUtf8());
    auto e = qobject_cast<ClearCaseEditorWidget*>(editor->widget());
    if (!e)
        return nullptr;
    connect(e, &VcsBaseEditorWidget::annotateRevisionRequested,
            this, &ClearCasePluginPrivate::vcsAnnotateHelper);
    e->setForceReadOnly(true);
    s.replace(QLatin1Char(' '), QLatin1Char('_'));
    e->textDocument()->setFallbackSaveAsFileName(s);
    if (!source.isEmpty())
        e->setSource(source);
    if (codec)
        e->setCodec(codec);
    return editor;
}

const ClearCaseSettings &ClearCasePluginPrivate::settings() const
{
    return m_settings;
}

void ClearCasePluginPrivate::setSettings(const ClearCaseSettings &s)
{
    if (s != m_settings) {
        m_settings = s;
        m_settings.toSettings(ICore::settings());
        emit configurationChanged();
    }
}

ClearCasePluginPrivate *ClearCasePluginPrivate::instance()
{
    QTC_ASSERT(dd, return dd);
    return dd;
}

bool ClearCasePluginPrivate::vcsOpen(const QString &workingDir, const QString &fileName)
{
    QTC_ASSERT(currentState().hasTopLevel(), return false);

    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << workingDir << fileName;

    QFileInfo fi(workingDir, fileName);
    QString topLevel = currentState().topLevel();
    QString absPath = fi.absoluteFilePath();

    if (!m_settings.disableIndexer &&
            (fi.isWritable() || vcsStatus(absPath).status == FileStatus::Unknown))
        Utils::runAsync(sync, QStringList(absPath)).waitForFinished();
    if (vcsStatus(absPath).status == FileStatus::CheckedOut) {
        QMessageBox::information(ICore::dialogParent(), tr("ClearCase Checkout"),
                                 tr("File is already checked out."));
        return true;
    }

    const QString relFile = QDir(topLevel).relativeFilePath(absPath);
    const QString file = QDir::toNativeSeparators(relFile);
    const QString title = QString::fromLatin1("Checkout %1").arg(file);
    CheckOutDialog coDialog(title, m_viewData.isUcm, !m_settings.noComment);

    // Only snapshot views can have hijacked files
    bool isHijacked = (!m_viewData.isDynamic && (vcsStatus(absPath).status & FileStatus::Hijacked));
    if (!isHijacked)
        coDialog.hideHijack();
    if (coDialog.exec() == QDialog::Accepted) {
        if (m_viewData.isUcm && !vcsSetActivity(topLevel, title, coDialog.activity()))
            return false;

        FileChangeBlocker fcb(absPath);
        QStringList args(QLatin1String("checkout"));

        const QString comment = coDialog.comment();
        if (m_settings.noComment || comment.isEmpty())
            args << QLatin1String("-nc");
        else
            args << QLatin1String("-c") << comment;

        args << QLatin1String("-query");
        const bool reserved = coDialog.isReserved();
        const bool unreserved = !reserved || coDialog.isUnreserved();
        if (reserved)
            args << QLatin1String("-reserved");
        if (unreserved)
            args << QLatin1String("-unreserved");
        if (coDialog.isPreserveTime())
            args << QLatin1String("-ptime");
        if (isHijacked) {
            if (Constants::debug)
                qDebug() << Q_FUNC_INFO << file << " seems to be hijacked";

            // A hijacked files means that the file is modified but was
            // not checked out. By checking it out now changes will
            // be lost, unless handled. This can be done by renaming
            // the hijacked file, undoing the hijack and updating the file

            // -usehijack not supported in old cleartool versions...
            // args << QLatin1String("-usehijack");
            if (coDialog.isUseHijacked())
                QFile::rename(absPath, absPath + QLatin1String(".hijack"));
            vcsUndoHijack(topLevel, relFile, false); // don't keep, we've already kept a copy
        }
        args << file;
        ClearCaseResponse response =
                runCleartool(topLevel, args, m_settings.timeOutS,
                             VcsCommand::ShowStdOut
                             | VcsCommand::SuppressStdErr
                             | VcsCommand::FullySynchronously);
        if (response.error) {
            if (response.stdErr.contains(QLatin1String("Versions other than the selected version"))) {
                VersionSelector selector(file, response.stdErr);
                if (selector.exec() == QDialog::Accepted) {
                    if (selector.isUpdate())
                        ccUpdate(workingDir, QStringList(file));
                    else
                        args.removeOne(QLatin1String("-query"));
                    response = runCleartool(topLevel, args, m_settings.timeOutS,
                                            VcsCommand::ShowStdOut
                                            | VcsCommand::FullySynchronously);
                }
            } else {
                VcsOutputWindow::append(response.stdOut);
                VcsOutputWindow::appendError(response.stdErr);
            }
        }

        if (!response.error && isHijacked && coDialog.isUseHijacked()) { // rename back
            QFile::remove(absPath);
            QFile::rename(absPath + QLatin1String(".hijack"), absPath);
        }

        if ((!response.error || response.stdErr.contains(QLatin1String("already checked out")))
                && !m_settings.disableIndexer) {
            setStatus(absPath, FileStatus::CheckedOut);
        }

        foreach (DocumentModel::Entry *e, DocumentModel::entries()) {
            if (e->fileName().toString() == absPath) {
                e->document->checkPermissions();
                break;
            }
        }

        return !response.error;
    }
    return true;
}

bool ClearCasePluginPrivate::vcsSetActivity(const QString &workingDir, const QString &title, const QString &activity)
{
    QStringList args;
    args << QLatin1String("setactivity") << activity;
    const ClearCaseResponse actResponse =
            runCleartool(workingDir, args, m_settings.timeOutS, VcsCommand::ShowStdOut);
    if (actResponse.error) {
        QMessageBox::warning(ICore::dialogParent(), title,
                             tr("Set current activity failed: %1").arg(actResponse.message), QMessageBox::Ok);
        return false;
    }
    m_activity = activity;
    return true;
}

// files are received using native separators
bool ClearCasePluginPrivate::vcsCheckIn(const QString &messageFile, const QStringList &files, const QString &activity,
                                 bool isIdentical, bool isPreserve, bool replaceActivity)
{
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << messageFile << files << activity;
    if (files.isEmpty())
        return true;
    const QString title = QString::fromLatin1("Checkin %1").arg(files.join(QLatin1String("; ")));
    using FCBPointer = QSharedPointer<FileChangeBlocker>;
    replaceActivity &= (activity != QLatin1String(Constants::KEEP_ACTIVITY));
    if (replaceActivity && !vcsSetActivity(m_checkInView, title, activity))
        return false;
    QString message;
    QFile msgFile(messageFile);
    if (msgFile.open(QFile::ReadOnly | QFile::Text)) {
        message = QString::fromLocal8Bit(msgFile.readAll().trimmed());
        msgFile.close();
    }
    QStringList args;
    args << QLatin1String("checkin");
    if (message.isEmpty())
        args << QLatin1String("-nc");
    else
        args << QLatin1String("-cfile") << messageFile;
    if (isIdentical)
        args << QLatin1String("-identical");
    if (isPreserve)
        args << QLatin1String("-ptime");
    args << files;
    QList<FCBPointer> blockers;
    foreach (const QString &fileName, files) {
        FCBPointer fcb(new FileChangeBlocker(QFileInfo(m_checkInView, fileName).canonicalFilePath()));
        blockers.append(fcb);
    }
    const ClearCaseResponse response =
            runCleartool(m_checkInView, args, m_settings.longTimeOutS(),
                         VcsCommand::ShowStdOut);
    const QRegularExpression checkedIn("Checked in \\\"([^\"]*)\\\"");
    QRegularExpressionMatch match = checkedIn.match(response.stdOut);
    bool anySucceeded = false;
    int offset = match.capturedStart();
    while (match.hasMatch()) {
        QString file = match.captured(1);
        QFileInfo fi(m_checkInView, file);
        QString absPath = fi.absoluteFilePath();

        if (!m_settings.disableIndexer)
            setStatus(QDir::fromNativeSeparators(absPath), FileStatus::CheckedIn);
        emit filesChanged(files);
        anySucceeded = true;
        match = checkedIn.match(response.stdOut, offset + 12);
        offset = match.capturedStart();
    }
    return anySucceeded;
}

bool ClearCasePluginPrivate::ccFileOp(const QString &workingDir, const QString &title, const QStringList &opArgs,
                               const QString &fileName, const QString &file2)
{
    const QString file = QDir::toNativeSeparators(fileName);
    bool noCheckout = false;
    ActivitySelector *actSelector = nullptr;
    QDialog fileOpDlg;
    fileOpDlg.setWindowTitle(title);

    auto verticalLayout = new QVBoxLayout(&fileOpDlg);
    if (m_viewData.isUcm) {
        actSelector = new ActivitySelector;
        verticalLayout->addWidget(actSelector);
    }

    auto commentLabel = new QLabel(tr("Enter &comment:"));
    verticalLayout->addWidget(commentLabel);

    auto commentEdit = new QTextEdit;
    verticalLayout->addWidget(commentEdit);

    auto buttonBox = new QDialogButtonBox;
    buttonBox->setStandardButtons(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    verticalLayout->addWidget(buttonBox);

    commentLabel->setBuddy(commentEdit);

    connect(buttonBox, &QDialogButtonBox::accepted, &fileOpDlg, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, &fileOpDlg, &QDialog::reject);

    if (!fileOpDlg.exec())
        return false;

    QString comment = commentEdit->toPlainText();
    if (m_viewData.isUcm && actSelector->changed())
        vcsSetActivity(workingDir, fileOpDlg.windowTitle(), actSelector->activity());

    QString dirName = QDir::toNativeSeparators(QFileInfo(workingDir, fileName).absolutePath());
    QStringList commentArg;
    if (comment.isEmpty())
        commentArg << QLatin1String("-nc");
    else
        commentArg << QLatin1String("-c") << comment;

    // check out directory
    QStringList args;
    args << QLatin1String("checkout") << commentArg << dirName;
    const ClearCaseResponse coResponse =
        runCleartool(workingDir, args, m_settings.timeOutS,
                     VcsCommand::ShowStdOut | VcsCommand::FullySynchronously);
    if (coResponse.error) {
        if (coResponse.stdErr.contains(QLatin1String("already checked out")))
            noCheckout = true;
        else
            return false;
    }

    // do the file operation
    args.clear();
    args << opArgs << commentArg << file;
    if (!file2.isEmpty())
        args << QDir::toNativeSeparators(file2);
    const ClearCaseResponse opResponse =
            runCleartool(workingDir, args, m_settings.timeOutS,
                         VcsCommand::ShowStdOut | VcsCommand::FullySynchronously);
    if (opResponse.error) {
        // on failure - undo checkout for the directory
        if (!noCheckout)
            vcsUndoCheckOut(workingDir, dirName, false);
        return false;
    }

    if (!noCheckout) {
        // check in the directory
        args.clear();
        args << QLatin1String("checkin") << commentArg << dirName;
        const ClearCaseResponse ciResponse =
            runCleartool(workingDir, args, m_settings.timeOutS,
                         VcsCommand::ShowStdOut | VcsCommand::FullySynchronously);
        return !ciResponse.error;
    }
    return true;
}

static QString baseName(const QString &fileName)
{
    return fileName.mid(fileName.lastIndexOf(QLatin1Char('/')) + 1);
}

bool ClearCasePluginPrivate::vcsAdd(const QString &workingDir, const QString &fileName)
{
    return ccFileOp(workingDir, tr("ClearCase Add File %1").arg(baseName(fileName)),
                    QStringList({"mkelem", "-ci"}), fileName);
}

bool ClearCasePluginPrivate::vcsDelete(const QString &workingDir, const QString &fileName)
{
    const QString title(tr("ClearCase Remove Element %1").arg(baseName(fileName)));
    if (QMessageBox::warning(ICore::dialogParent(), title, tr("This operation is irreversible. Are you sure?"),
                         QMessageBox::Yes | QMessageBox::No) == QMessageBox::No)
        return true;

    return ccFileOp(workingDir, tr("ClearCase Remove File %1").arg(baseName(fileName)),
                    QStringList({"rmname", "-force"}), fileName);
}

bool ClearCasePluginPrivate::vcsMove(const QString &workingDir, const QString &from, const QString &to)
{
    return ccFileOp(workingDir, tr("ClearCase Rename File %1 -> %2")
                    .arg(baseName(from)).arg(baseName(to)),
                    QStringList("move"), from, to);
}

///
/// Check if the directory is managed under ClearCase control.
///
bool ClearCasePluginPrivate::managesDirectory(const QString &directory, QString *topLevel /* = 0 */) const
{
#ifdef WITH_TESTS
    // If running with tests and fake ClearTool is enabled, then pretend we manage every directory
    QString topLevelFound = m_fakeClearTool ? directory : findTopLevel(directory);
#else
    QString topLevelFound = findTopLevel(directory);
#endif

    if (topLevel)
        *topLevel = topLevelFound;
    return !topLevelFound.isEmpty();
}

QString ClearCasePluginPrivate::ccGetCurrentActivity() const
{
    QStringList args(QLatin1String("lsactivity"));
    args << QLatin1String("-cact");
    args << QLatin1String("-fmt") << QLatin1String("%n");
    return runCleartoolSync(currentState().topLevel(), args);
}

QList<QStringPair> ClearCasePluginPrivate::ccGetActivities() const
{
    QList<QStringPair> result;
    // Maintain latest deliver and rebase activities only
    QStringPair rebaseAct;
    QStringPair deliverAct;
    // Retrieve all activities
    QStringList args(QLatin1String("lsactivity"));
    args << QLatin1String("-fmt") << QLatin1String("%n\\t%[headline]p\\n");
    const QString response = runCleartoolSync(currentState().topLevel(), args);
    QStringList acts = response.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    foreach (const QString &activity, acts) {
        QStringList act = activity.split(QLatin1Char('\t'));
        if (act.size() >= 2)
        {
            QString actName = act.at(0);
            // include only latest deliver/rebase activities. Activities are sorted
            // by creation time
            if (actName.startsWith(QLatin1String("rebase.")))
                rebaseAct = QStringPair(actName, act.at(1));
            else if (actName.startsWith(QLatin1String("deliver.")))
                deliverAct = QStringPair(actName, act.at(1));
            else
                result.append(QStringPair(actName, act.at(1).trimmed()));
        }
    }
    Utils::sort(result);
    if (!rebaseAct.first.isEmpty())
        result.append(rebaseAct);
    if (!deliverAct.first.isEmpty())
        result.append(deliverAct);
    return result;
}

void ClearCasePluginPrivate::refreshActivities()
{
    QMutexLocker locker(&m_activityMutex);
    m_activity = ccGetCurrentActivity();
    m_activities = ccGetActivities();
}

QList<QStringPair> ClearCasePluginPrivate::activities(int *current)
{
    QList<QStringPair> activitiesList;
    QString curActivity;
    const VcsBasePluginState state = currentState();
    if (state.topLevel() == state.currentProjectTopLevel()) {
        QMutexLocker locker(&m_activityMutex);
        activitiesList = m_activities;
        curActivity = m_activity;
    } else {
        activitiesList = ccGetActivities();
        curActivity = ccGetCurrentActivity();
    }
    if (current) {
        int nActivities = activitiesList.size();
        *current = -1;
        for (int i = 0; i < nActivities && (*current == -1); ++i) {
            if (activitiesList[i].first == curActivity)
                *current = i;
        }
    }
    return activitiesList;
}

bool ClearCasePluginPrivate::newActivity()
{
    QString workingDir = currentState().topLevel();
    QStringList args;
    args << QLatin1String("mkactivity") << QLatin1String("-f");
    if (!m_settings.autoAssignActivityName) {
        QString headline = QInputDialog::getText(ICore::dialogParent(), tr("Activity Headline"),
                                                 tr("Enter activity headline"));
        if (headline.isEmpty())
            return false;
        args << QLatin1String("-headline") << headline;
    }

    const ClearCaseResponse response =
            runCleartool(workingDir, args, m_settings.timeOutS, 0);

    if (!response.error)
        refreshActivities();
    return (!response.error);
}

// check if the view is UCM
bool ClearCasePluginPrivate::ccCheckUcm(const QString &viewname, const QString &workingDir) const
{
    QStringList catcsArgs(QLatin1String("catcs"));
    catcsArgs << QLatin1String("-tag") << viewname;
    QString catcsData = runCleartoolSync(workingDir, catcsArgs);

    // check output for the word "ucm"
    return catcsData.indexOf(QRegularExpression("(^|\\n)ucm\\n")) != -1;
}

bool ClearCasePluginPrivate::managesFile(const QString &workingDirectory, const QString &fileName) const
{
    QString absFile = QFileInfo(QDir(workingDirectory), fileName).absoluteFilePath();
    const FileStatus::Status status = getFileStatus(absFile);
    return status != FileStatus::NotManaged && status != FileStatus::Derived;
}

ViewData ClearCasePluginPrivate::ccGetView(const QString &workingDir) const
{
    static QHash<QString, ViewData> viewCache;

    bool inCache = viewCache.contains(workingDir);
    ViewData &res = viewCache[workingDir];
    if (!inCache) {
        QStringList args(QLatin1String("lsview"));
        args << QLatin1String("-cview");
        QString data = runCleartoolSync(workingDir, args);
        res.isDynamic = !data.isEmpty() && (data.at(0) == QLatin1Char('*'));
        res.name = data.mid(2, data.indexOf(QLatin1Char(' '), 2) - 2);
        res.isUcm = ccCheckUcm(res.name, workingDir);
        res.root = ccViewRoot(workingDir);
    }

    return res;
}

QString ClearCasePluginPrivate::ccGetComment(const QString &workingDir, const QString &fileName) const
{
    QStringList args(QLatin1String("describe"));
    args << QLatin1String("-fmt") << QLatin1String("%c") << fileName;
    return runCleartoolSync(workingDir, args);
}

void ClearCasePluginPrivate::updateStreamAndView()
{
    QStringList args(QLatin1String("lsstream"));
    args << QLatin1String("-fmt") << QLatin1String("%n\\t%[def_deliver_tgt]Xp");
    const QString sresponse = runCleartoolSync(m_topLevel, args);
    int tabPos = sresponse.indexOf(QLatin1Char('\t'));
    m_stream = sresponse.left(tabPos);
    const QRegularExpression intStreamExp("stream:([^@]*)");
    const QRegularExpressionMatch match = intStreamExp.match(sresponse.mid(tabPos + 1));
    if (match.hasMatch())
        m_intStream = match.captured(1);
    m_viewData = ccGetView(m_topLevel);
    m_updateViewAction->setParameter(m_viewData.isDynamic ? QString() : m_viewData.name);
}

void ClearCasePluginPrivate::projectChanged(Project *project)
{
    if (m_viewData.name == ccGetView(m_topLevel).name) // New project on same view as old project
        return;
    m_viewData = ViewData();
    m_stream.clear();
    m_intStream.clear();
    ProgressManager::cancelTasks(ClearCase::Constants::TASK_INDEX);
    if (project) {
        QString projDir = project->projectDirectory().toString();
        QString topLevel = findTopLevel(projDir);
        m_topLevel = topLevel;
        if (topLevel.isEmpty())
            return;
        connect(qApp, &QApplication::applicationStateChanged,
                this, [this](Qt::ApplicationState state) {
                    if (state == Qt::ApplicationActive)
                        syncSlot();
                });
        updateStreamAndView();
        if (m_viewData.name.isEmpty())
            return;
        updateIndex();
    }
    if (Constants::debug)
        qDebug() << "stream: " << m_stream << "; intStream: " << m_intStream << "view: " << m_viewData.name;
}

void ClearCasePluginPrivate::tasksFinished(Id type)
{
    if (type == ClearCase::Constants::TASK_INDEX)
        m_checkInAllAction->setEnabled(true);
}

void ClearCasePluginPrivate::updateIndex()
{
    QTC_ASSERT(currentState().hasTopLevel(), return);
    ProgressManager::cancelTasks(ClearCase::Constants::TASK_INDEX);
    Project *project = SessionManager::startupProject();
    if (!project)
        return;
    m_checkInAllAction->setEnabled(false);
    m_statusMap->clear();
    QFuture<void> result = Utils::runAsync(sync, transform(project->files(Project::SourceFiles), &FilePath::toString));
    if (!m_settings.disableIndexer)
        ProgressManager::addTask(result, tr("Updating ClearCase Index"), ClearCase::Constants::TASK_INDEX);
}

/*! retrieve a \a file (usually of the form path\to\filename.cpp@@\main\ver)
 *  from cc and save it to a temporary location which is returned
 */
QString ClearCasePluginPrivate::getFile(const QString &nativeFile, const QString &prefix)
{
    QString tempFile;
    QDir tempDir = QDir::temp();
    tempDir.mkdir(QLatin1String("ccdiff"));
    tempDir.cd(QLatin1String("ccdiff"));
    int atatpos = nativeFile.indexOf(QLatin1String("@@"));
    QString file = QDir::fromNativeSeparators(nativeFile.left(atatpos));
    if (prefix.isEmpty()) {
        tempFile = tempDir.absoluteFilePath(QString::number(QUuid::createUuid().data1, 16));
    } else {
        tempDir.mkpath(prefix);
        tempDir.cd(prefix);
        int slash = file.lastIndexOf(QLatin1Char('/'));
        if (slash != -1)
            tempDir.mkpath(file.left(slash));
        tempFile = tempDir.absoluteFilePath(file);
    }
    if (Constants::debug)
        qDebug() << Q_FUNC_INFO << nativeFile;
    if ((atatpos != -1) && (nativeFile.indexOf(QLatin1String("CHECKEDOUT"), atatpos) != -1)) {
        bool res = QFile::copy(QDir(m_topLevel).absoluteFilePath(file), tempFile);
        return res ? tempFile : QString();
    }
    QStringList args(QLatin1String("get"));
    args << QLatin1String("-to") << tempFile << nativeFile;
    const ClearCaseResponse response =
            runCleartool(m_topLevel, args, m_settings.timeOutS, SilentRun);
    if (response.error)
        return QString();
    QFile::setPermissions(tempFile, QFile::ReadOwner | QFile::ReadUser |
                          QFile::WriteOwner | QFile::WriteUser);
    return tempFile;
}

// runs external (GNU) diff, and returns the stdout result
QString ClearCasePluginPrivate::diffExternal(QString file1, QString file2, bool keep)
{
    QTextCodec *codec = VcsBaseEditor::getCodec(file1);

    // if file2 is empty, we should compare to predecessor
    if (file2.isEmpty()) {
        QString predVer = ccGetPredecessor(file1);
        return (predVer.isEmpty() ? QString() : diffExternal(predVer, file1, keep));
    }

    file1 = QDir::toNativeSeparators(file1);
    file2 = QDir::toNativeSeparators(file2);
    QString tempFile1, tempFile2;
    QString prefix = m_diffPrefix;
    if (!prefix.isEmpty())
        prefix.append(QLatin1Char('/'));

    if (file1.contains(QLatin1String("@@")))
        tempFile1 = getFile(file1, prefix + QLatin1String("old"));
    if (file2.contains(QLatin1String("@@")))
        tempFile2 = getFile(file2, prefix + QLatin1String("new"));
    QStringList args;
    if (!tempFile1.isEmpty()) {
        args << QLatin1String("-L") << file1;
        args << tempFile1;
    } else {
        args << file1;
    }
    if (!tempFile2.isEmpty()) {
        args << QLatin1String("-L") << file2;
        args << tempFile2;
    } else {
        args << file2;
    }
    const QString diffResponse = runExtDiff(m_topLevel, args, m_settings.timeOutS, codec);
    if (!keep && !tempFile1.isEmpty()) {
        QFile::remove(tempFile1);
        QFileInfo(tempFile1).dir().rmpath(QLatin1String("."));
    }
    if (!keep && !tempFile2.isEmpty()) {
        QFile::remove(tempFile2);
        QFileInfo(tempFile2).dir().rmpath(QLatin1String("."));
    }
    if (diffResponse.isEmpty())
        return QLatin1String("Files are identical");
    QString header = QString::fromLatin1("diff %1 old/%2 new/%2\n")
            .arg(m_settings.diffArgs)
            .arg(QDir::fromNativeSeparators(file2.left(file2.indexOf(QLatin1String("@@")))));
    return header + diffResponse;
}

// runs builtin diff (either graphical or diff_format)
void ClearCasePluginPrivate::diffGraphical(const QString &file1, const QString &file2)
{
    QStringList args;
    bool pred = file2.isEmpty();
    args.push_back(QLatin1String("diff"));
    if (pred)
        args.push_back(QLatin1String("-predecessor"));
    args.push_back(QLatin1String("-graphical"));
    args << file1;
    if (!pred)
        args << file2;
    QProcess::startDetached(m_settings.ccBinaryPath, args, m_topLevel);
}

QString ClearCasePluginPrivate::runExtDiff(const QString &workingDir, const QStringList &arguments,
                                    int timeOutS, QTextCodec *outputCodec)
{
    CommandLine diff("diff");
    diff.addArgs(m_settings.diffArgs.split(' ', Qt::SkipEmptyParts));
    diff.addArgs(arguments);

    SynchronousProcess process;
    process.setTimeoutS(timeOutS);
    process.setWorkingDirectory(workingDir);
    process.setCodec(outputCodec ? outputCodec : QTextCodec::codecForName("UTF-8"));
    SynchronousProcessResponse response = process.run(diff);
    if (response.result != SynchronousProcessResponse::Finished)
        return QString();
    return response.allOutput();
}

void ClearCasePluginPrivate::syncSlot()
{
    VcsBasePluginState state = currentState();
    if (!state.hasProject() || !state.hasTopLevel())
        return;
    QString topLevel = state.topLevel();
    if (topLevel != state.currentProjectTopLevel())
        return;
    Utils::runAsync(sync, QStringList());
}

void ClearCasePluginPrivate::closing()
{
    // prevent syncSlot from being called on shutdown
    ProgressManager::cancelTasks(ClearCase::Constants::TASK_INDEX);
    disconnect(qApp, &QApplication::applicationStateChanged, nullptr, nullptr);
}

void ClearCasePluginPrivate::sync(QFutureInterface<void> &future, QStringList files)
{
    ClearCasePluginPrivate *plugin = ClearCasePluginPrivate::instance();
    ClearCaseSync ccSync(plugin->m_statusMap);
    connect(&ccSync, &ClearCaseSync::updateStreamAndView, plugin, &ClearCasePluginPrivate::updateStreamAndView);
    ccSync.run(future, files);
}

QString ClearCasePluginPrivate::displayName() const
{
    return QLatin1String("ClearCase");
}

Utils::Id ClearCasePluginPrivate::id() const
{
    return Constants::VCS_ID_CLEARCASE;
}

bool ClearCasePluginPrivate::isVcsFileOrDirectory(const Utils::FilePath &fileName) const
{
    Q_UNUSED(fileName)
    return false; // ClearCase has no files/directories littering the sources
}

bool ClearCasePluginPrivate::isConfigured() const
{
#ifdef WITH_TESTS
    if (m_fakeClearTool)
        return true;
#endif
    const QString binary = m_settings.ccBinaryPath;
    if (binary.isEmpty())
        return false;
    QFileInfo fi(binary);
    return fi.exists() && fi.isFile() && fi.isExecutable();
}

bool ClearCasePluginPrivate::supportsOperation(Operation operation) const
{
    bool rc = isConfigured();
    switch (operation) {
    case AddOperation:
    case DeleteOperation:
    case MoveOperation:
    case AnnotateOperation:
        break;
    case CreateRepositoryOperation:
    case SnapshotOperations:
    case Core::IVersionControl::InitialCheckoutOperation:
        rc = false;
        break;
    }
    return rc;
}

Core::IVersionControl::OpenSupportMode ClearCasePluginPrivate::openSupportMode(const QString &fileName) const
{
    if (isDynamic()) {
        // NB! Has to use managesFile() and not vcsStatus() since the index can only be guaranteed
        // to be up to date if the file has been explicitly opened, which is not the case when
        // doing a search and replace as a part of a refactoring.
        if (managesFile(QFileInfo(fileName).absolutePath(), fileName)) {
            // Checkout is the only option for managed files in dynamic views
            return IVersionControl::OpenMandatory;
        } else {
            // Not managed files can be edited without noticing the VCS
            return IVersionControl::NoOpen;
        }

    } else {
        return IVersionControl::OpenOptional; // Snapshot views supports Hijack and check out
    }
}

bool ClearCasePluginPrivate::vcsOpen(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return vcsOpen(fi.absolutePath(), fi.fileName());
}

Core::IVersionControl::SettingsFlags ClearCasePluginPrivate::settingsFlags() const
{
    SettingsFlags rc;
    if (m_settings.autoCheckOut)
        rc|= AutoOpen;
    return rc;
}

bool ClearCasePluginPrivate::vcsAdd(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return vcsAdd(fi.absolutePath(), fi.fileName());
}

bool ClearCasePluginPrivate::vcsDelete(const QString &fileName)
{
    const QFileInfo fi(fileName);
    return vcsDelete(fi.absolutePath(), fi.fileName());
}

bool ClearCasePluginPrivate::vcsMove(const QString &from, const QString &to)
{
    const QFileInfo ifrom(from);
    const QFileInfo ito(to);
    return vcsMove(ifrom.absolutePath(), ifrom.fileName(), ito.fileName());
}

void ClearCasePluginPrivate::vcsAnnotate(const QString &file, int line)
{
    const QFileInfo fi(file);
    vcsAnnotateHelper(fi.absolutePath(), fi.fileName(), QString(), line);
}

QString ClearCasePluginPrivate::vcsOpenText() const
{
    return tr("Check &Out");
}

QString ClearCasePluginPrivate::vcsMakeWritableText() const
{
    if (isDynamic())
        return QString();
    return tr("&Hijack");
}

QString ClearCasePluginPrivate::vcsTopic(const QString &directory)
{
    return ccGetView(directory).name;
}

bool ClearCasePluginPrivate::vcsCreateRepository(const QString &)
{
    return false;
}

// ClearCasePlugin

ClearCasePlugin::~ClearCasePlugin()
{
    delete dd;
    dd = nullptr;
}

bool ClearCasePlugin::newActivity()
{
    return dd->newActivity();
}

const QList<QStringPair> ClearCasePlugin::activities(int *current)
{
    return dd->activities(current);
}

QStringList ClearCasePlugin::ccGetActiveVobs()
{
    return dd->ccGetActiveVobs();
}

void ClearCasePlugin::refreshActivities()
{
    dd->refreshActivities();
}

const ViewData ClearCasePlugin::viewData()
{
    return dd->m_viewData;
}

void ClearCasePlugin::setStatus(const QString &file, FileStatus::Status status, bool update)
{
    dd->setStatus(file, status, update);
}

const ClearCaseSettings &ClearCasePlugin::settings()
{
    return dd->m_settings;
}

void ClearCasePlugin::setSettings(const ClearCaseSettings &s)
{
    dd->setSettings(s);
}

QSharedPointer<StatusMap> ClearCasePlugin::statusMap()
{
    return dd->m_statusMap;
}

#ifdef WITH_TESTS

void ClearCasePlugin::testDiffFileResolving_data()
{
    QTest::addColumn<QByteArray>("header");
    QTest::addColumn<QByteArray>("fileName");

    QTest::newRow("Modified") << QByteArray(
            "--- src/plugins/clearcase/clearcaseeditor.cpp@@/main/1\t2013-01-20 23:45:48.549615210 +0200\n"
            "+++ src/plugins/clearcase/clearcaseeditor.cpp@@/main/2\t2013-01-20 23:45:53.217604679 +0200\n"
            "@@ -58,6 +58,10 @@\n\n")
        << QByteArray("src/plugins/clearcase/clearcaseeditor.cpp");
}

void ClearCasePlugin::testDiffFileResolving()
{
    VcsBaseEditorWidget::testDiffFileResolving(dd->diffEditorFactory);
}

void ClearCasePlugin::testLogResolving()
{
    QByteArray data(
                "13-Sep.17:41   user1      create version \"src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/9\" (baseline1, baseline2, ...)\n"
                "22-Aug.14:13   user2      create version \"src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/8\" (baseline3, baseline4, ...)\n"
                );
    VcsBaseEditorWidget::testLogResolving(dd->logEditorFactory, data,
                            "src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/9",
                            "src/plugins/clearcase/clearcaseeditor.h@@/main/branch1/branch2/8");
}

void ClearCasePlugin::initTestCase()
{
    dd->m_tempFile = QDir::currentPath() + QLatin1String("/cc_file.cpp");
    FileSaver srcSaver(dd->m_tempFile);
    srcSaver.write(QByteArray());
    srcSaver.finalize();
}

void ClearCasePlugin::cleanupTestCase()
{
    QVERIFY(QFile::remove(dd->m_tempFile));
}

void ClearCasePlugin::testFileStatusParsing_data()
{
    QTest::addColumn<QString>("filename");
    QTest::addColumn<QString>("cleartoolLsLine");
    QTest::addColumn<int>("status");

    QTest::newRow("CheckedOut")
            << dd->m_tempFile
            << QString(dd->m_tempFile + QLatin1String("@@/main/branch1/CHECKEDOUT from /main/branch1/0  Rule: CHECKEDOUT"))
            << static_cast<int>(FileStatus::CheckedOut);

    QTest::newRow("CheckedIn")
            << dd->m_tempFile
            << QString(dd->m_tempFile + QLatin1String("@@/main/9  Rule: MY_LABEL_1.6.4 [-mkbranch branch1]"))
            << static_cast<int>(FileStatus::CheckedIn);

    QTest::newRow("Hijacked")
            << dd->m_tempFile
            << QString(dd->m_tempFile + QLatin1String("@@/main/9 [hijacked]        Rule: MY_LABEL_1.5.33 [-mkbranch myview1]"))
            << static_cast<int>(FileStatus::Hijacked);


    QTest::newRow("Missing")
            << dd->m_tempFile
            << QString(dd->m_tempFile + QLatin1String("@@/main/9 [loaded but missing]              Rule: MY_LABEL_1.5.33 [-mkbranch myview1]"))
            << static_cast<int>(FileStatus::Missing);
}

void ClearCasePlugin::testFileStatusParsing()
{
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);

    QFETCH(QString, filename);
    QFETCH(QString, cleartoolLsLine);
    QFETCH(int, status);

    ClearCaseSync ccSync(dd->m_statusMap);
    ccSync.verifyParseStatus(filename, cleartoolLsLine, static_cast<FileStatus::Status>(status));
}

void ClearCasePlugin::testFileNotManaged()
{
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);
    ClearCaseSync ccSync(dd->m_statusMap);
    ccSync.verifyFileNotManaged();
}

void ClearCasePlugin::testFileCheckedOutDynamicView()
{
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);

    ClearCaseSync ccSync(dd->m_statusMap);
    ccSync.verifyFileCheckedOutDynamicView();
}

void ClearCasePlugin::testFileCheckedInDynamicView()
{
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);
    ClearCaseSync ccSync(dd->m_statusMap);
    ccSync.verifyFileCheckedInDynamicView();
}

void ClearCasePlugin::testFileNotManagedDynamicView()
{
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);
    ClearCaseSync ccSync(dd->m_statusMap);
    ccSync.verifyFileNotManagedDynamicView();
}

namespace {
/**
 * @brief Convenience class which also properly cleans up editors and temp files
 */
class TestCase
{
public:
    TestCase(const QString &fileName) :
        m_fileName(fileName)
    {
        ClearCasePluginPrivate::instance()->setFakeCleartool(true);
        VcsManager::clearVersionControlCache();

        FileSaver srcSaver(fileName);
        srcSaver.write(QByteArray());
        srcSaver.finalize();
        m_editor = EditorManager::openEditor(fileName);

        QCoreApplication::processEvents(); // process any pending events
    }

    ViewData dummyViewData() const
    {
        ViewData viewData;
        viewData.name = QLatin1String("fake_view");
        viewData.root = QDir::currentPath();
        viewData.isUcm = false;
        return viewData;
    }

    ~TestCase()
    {
        EditorManager::closeDocument(m_editor->document(), false);
        QCoreApplication::processEvents(); // process any pending events

        QFile file(m_fileName);
        if (!file.isWritable()) // Windows can't delete read only files
            file.setPermissions(file.permissions() | QFile::WriteUser);
        QVERIFY(file.remove());
        ClearCasePluginPrivate::instance()->setFakeCleartool(false);
    }

private:
    QString m_fileName;
    IEditor *m_editor;
};
}

void ClearCasePlugin::testStatusActions_data()
{
    QTest::addColumn<int>("status");
    QTest::addColumn<bool>("checkOutAction");
    QTest::addColumn<bool>("undoCheckOutAction");
    QTest::addColumn<bool>("undoHijackAction");
    QTest::addColumn<bool>("checkInCurrentAction");
    QTest::addColumn<bool>("addFileAction");
    QTest::addColumn<bool>("checkInActivityAction");
    QTest::addColumn<bool>("diffActivityAction");

    QTest::newRow("Unknown")    << static_cast<int>(FileStatus::Unknown)
                                << true  << true  << true  << true  << true  << false << false;
    QTest::newRow("CheckedOut") << static_cast<int>(FileStatus::CheckedOut)
                                << false << true  << false << true  << false << false << false;
    QTest::newRow("CheckedIn")  << static_cast<int>(FileStatus::CheckedIn)
                                << true  << false << false << false << false << false << false;
    QTest::newRow("NotManaged") << static_cast<int>(FileStatus::NotManaged)
                                << false << false << false << false << true  << false << false;
}

void ClearCasePlugin::testStatusActions()
{
    const QString fileName = QDir::currentPath() + QLatin1String("/clearcase_file.cpp");
    TestCase testCase(fileName);

    dd->m_viewData = testCase.dummyViewData();

    QFETCH(int, status);
    auto tempStatus = static_cast<FileStatus::Status>(status);

    // special case: file should appear as "Unknown" since there is no entry in the index
    // and we don't want to explicitly set the status for this test case
    if (tempStatus != FileStatus::Unknown)
        dd->setStatus(fileName, tempStatus, true);

    QFETCH(bool, checkOutAction);
    QFETCH(bool, undoCheckOutAction);
    QFETCH(bool, undoHijackAction);
    QFETCH(bool, checkInCurrentAction);
    QFETCH(bool, addFileAction);
    QFETCH(bool, checkInActivityAction);
    QFETCH(bool, diffActivityAction);

    QCOMPARE(dd->m_checkOutAction->isEnabled(), checkOutAction);
    QCOMPARE(dd->m_undoCheckOutAction->isEnabled(), undoCheckOutAction);
    QCOMPARE(dd->m_undoHijackAction->isEnabled(), undoHijackAction);
    QCOMPARE(dd->m_checkInCurrentAction->isEnabled(), checkInCurrentAction);
    QCOMPARE(dd->m_addFileAction->isEnabled(), addFileAction);
    QCOMPARE(dd->m_checkInActivityAction->isEnabled(), checkInActivityAction);
    QCOMPARE(dd->m_diffActivityAction->isEnabled(), diffActivityAction);
}

void ClearCasePlugin::testVcsStatusDynamicReadonlyNotManaged()
{
    // File is not in map, and is read-only
    ClearCasePluginPrivate::instance();
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);

    const QString fileName = QDir::currentPath() + QLatin1String("/readonly_notmanaged_file.cpp");

    dd->m_viewData.isDynamic = true;
    TestCase testCase(fileName);

    QFile::setPermissions(fileName, QFile::ReadOwner |
                          QFile::ReadUser |
                          QFile::ReadGroup |
                          QFile::ReadOther);

    dd->m_viewData = testCase.dummyViewData();
    dd->m_viewData.isDynamic = true;

    QCOMPARE(dd->vcsStatus(fileName).status, FileStatus::NotManaged);

}

void ClearCasePlugin::testVcsStatusDynamicNotManaged()
{
    ClearCasePluginPrivate::instance();
    dd->m_statusMap = QSharedPointer<StatusMap>(new StatusMap);

    const QString fileName = QDir::currentPath() + QLatin1String("/notmanaged_file.cpp");

    dd->m_viewData.isDynamic = true;
    TestCase testCase(fileName);

    dd->m_viewData = testCase.dummyViewData();
    dd->m_viewData.isDynamic = true;

    QCOMPARE(dd->vcsStatus(fileName).status, FileStatus::NotManaged);
}
#endif

} // namespace Internal
} // namespace ClearCase

#include "clearcaseplugin.moc"
