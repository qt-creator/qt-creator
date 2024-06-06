// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "icore.h"

#include "actionmanager/actioncontainer.h"
#include "actionmanager/actionmanager.h"
#include "actionmanager/command.h"
#include "coreicons.h"
#include "coreplugintr.h"
#include "coreplugintr.h"
#include "dialogs/externaltoolconfig.h"
#include "dialogs/settingsdialog.h"
#include "dialogs/shortcutsettings.h"
#include "documentmanager.h"
#include "editormanager/documentmodel_p.h"
#include "editormanager/editormanager.h"
#include "editormanager/editormanager_p.h"
#include "editormanager/ieditor.h"
#include "editormanager/ieditorfactory.h"
#include "editormanager/systemeditor.h"
#include "externaltoolmanager.h"
#include "fancytabwidget.h"
#include "fileutils.h"
#include "find/basetextfind.h"
#include "findplaceholder.h"
#include "helpmanager.h"
#include "icore.h"
#include "idocumentfactory.h"
#include "inavigationwidgetfactory.h"
#include "iwizardfactory.h"
#include "jsexpander.h"
#include "loggingviewer.h"
#include "manhattanstyle.h"
#include "messagemanager.h"
#include "mimetypesettings.h"
#include "modemanager.h"
#include "navigationwidget.h"
#include "outputpanemanager.h"
#include "plugindialog.h"
#include "progressmanager/progressmanager_p.h"
#include "progressmanager/progressview.h"
#include "rightpane.h"
#include "statusbarmanager.h"
#include "systemsettings.h"
#include "vcsmanager.h"
#include "versiondialog.h"
#include "windowsupport.h"

#include <extensionsystem/pluginmanager.h>

#include <utils/algorithm.h>
#include <utils/appinfo.h>
#include <utils/checkablemessagebox.h>
#include <utils/dropsupport.h>
#include <utils/environment.h>
#include <utils/fsengine/fileiconprovider.h>
#include <utils/fsengine/fsengine.h>
#include <utils/guiutils.h>
#include <utils/historycompleter.h>
#include <utils/hostosinfo.h>
#include <utils/mimeutils.h>
#include <utils/proxyaction.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>
#include <utils/stylehelper.h>
#include <utils/terminalcommand.h>
#include <utils/theme/theme.h>
#include <utils/touchbar/touchbar.h>
#include <utils/utilsicons.h>

#include <nanotrace/nanotrace.h>

#include <QActionGroup>
#include <QApplication>
#include <QCloseEvent>
#include <QColorDialog>
#include <QComboBox>
#include <QDebug>
#include <QDialogButtonBox>
#include <QLibraryInfo>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPrinter>
#include <QStandardPaths>
#include <QStatusBar>
#include <QStyleFactory>
#include <QSysInfo>
#include <QTextBrowser>
#include <QTimer>
#include <QToolButton>
#include <QVersionNumber>

#ifdef Q_OS_LINUX
#include <malloc.h>
#endif

/*!
    \namespace Core
    \inmodule QtCreator
    \brief The Core namespace contains all classes that make up the Core plugin
    which constitute the basic functionality of \QC.
*/

/*!
    \enum Utils::FindFlag
    This enum holds the find flags.

    \value FindBackward
           Searches backwards.
    \value FindCaseSensitively
           Considers case when searching.
    \value FindWholeWords
           Finds only whole words.
    \value FindRegularExpression
           Uses a regular epression as a search term.
    \value FindPreserveCase
           Preserves the case when replacing search terms.
*/

/*!
    \enum Core::ICore::ContextPriority

    This enum defines the priority of additional contexts.

    \value High
           Additional contexts that have higher priority than contexts from
           Core::IContext instances.
    \value Low
           Additional contexts that have lower priority than contexts from
           Core::IContext instances.

    \sa Core::ICore::updateAdditionalContexts()
*/

/*!
    \enum Core::SaveSettingsReason
    \internal
*/

/*!
    \namespace Core::Internal
    \internal
*/

/*!
    \class Core::ICore
    \inheaderfile coreplugin/icore.h
    \inmodule QtCreator
    \ingroup mainclasses

    \brief The ICore class allows access to the different parts that make up
    the basic functionality of \QC.

    You should never create a subclass of this interface. The one and only
    instance is created by the Core plugin. You can access this instance
    from your plugin through instance().
*/

/*!
    \fn void Core::ICore::coreAboutToOpen()

    Indicates that all plugins have been loaded and the main window is about to
    be shown.
*/

/*!
    \fn void Core::ICore::coreOpened()
    Indicates that all plugins have been loaded and the main window is shown.
*/

/*!
    \fn void Core::ICore::saveSettingsRequested(Core::ICore::SaveSettingsReason reason)
    Signals that the user has requested that the global settings
    should be saved to disk for a \a reason.

    At the moment that happens when the application is closed, and on \uicontrol{Save All}.
*/

/*!
    \fn void Core::ICore::coreAboutToClose()
    Enables plugins to perform some pre-end-of-life actions.

    The application is guaranteed to shut down after this signal is emitted.
    It is there as an addition to the usual plugin lifecycle functions, namely
    \c IPlugin::aboutToShutdown(), just for convenience.
*/

/*!
    \fn void Core::ICore::contextAboutToChange(const QList<Core::IContext *> &context)
    Indicates that a new \a context will shortly become the current context
    (meaning that its widget got focus).
*/

/*!
    \fn void Core::ICore::contextChanged(const Core::Context &context)
    Indicates that a new \a context just became the current context. This includes the context
    from the focus object as well as the additional context.
*/

#include "dialogs/newdialogwidget.h"
#include "dialogs/newdialog.h"
#include "iwizardfactory.h"
#include "documentmanager.h"

#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QMessageBox>
#include <QPushButton>
#include <QStatusBar>

using namespace Core::Internal;
using namespace ExtensionSystem;
using namespace Utils;

namespace Core {

const char settingsGroup[] = "MainWindow";
const char colorKey[] = "Color";
const char windowGeometryKey[] = "WindowGeometry";
const char windowStateKey[] = "WindowState";
const char modeSelectorLayoutKey[] = "ModeSelectorLayout";
const char menubarVisibleKey[] = "MenubarVisible";

namespace Internal {

class MainWindow : public AppMainWindow
{
public:
    MainWindow()
    {
        setWindowTitle(QGuiApplication::applicationDisplayName());
        setDockNestingEnabled(true);
        setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);
        setCorner(Qt::BottomRightCorner, Qt::BottomDockWidgetArea);
    }

private:
    void closeEvent(QCloseEvent *event) override;
    void keyPressEvent(QKeyEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
};

static QColor s_overrideColor;

// The Core Singleton
static ICore *m_core = nullptr;

static NewDialog *defaultDialogFactory(QWidget *parent)
{
    return new NewDialogWidget(parent);
}
class ICorePrivate : public QObject
{
public:
    ICorePrivate() {}

    ~ICorePrivate();

    void init();

    static void openFile();
    void aboutToShowRecentFiles();

    static void setFocusToEditor();
    void aboutQtCreator();
    void aboutPlugins();
    void changeLog();
    void contact();
    void updateFocusWidget(QWidget *old, QWidget *now);
    NavigationWidget *navigationWidget(Side side) const;
    void setSidebarVisible(bool visible, Side side);
    void destroyVersionDialog();
    void openDroppedFiles(const QList<Utils::DropSupport::FileSpec> &files);
    void restoreWindowState();

    void openFileFromDevice();

    void updateContextObject(const QList<IContext *> &context);
    void updateContext();

    void registerDefaultContainers();
    void registerDefaultActions();
    void registerModeSelectorStyleActions();

    void readSettings();
    void saveWindowSettings();

    void updateModeSelectorStyleMenu();

    MainWindow *m_mainwindow = nullptr;
    QTimer m_trimTimer;
    QStringList m_aboutInformation;
    Context m_highPrioAdditionalContexts;
    Context m_lowPrioAdditionalContexts{Constants::C_GLOBAL};
    mutable QPrinter *m_printer = nullptr;
    WindowSupport *m_windowSupport = nullptr;
    EditorManager *m_editorManager = nullptr;
    ExternalToolManager *m_externalToolManager = nullptr;
    ProgressManagerPrivate *m_progressManager = nullptr;
    JsExpander *m_jsExpander = nullptr;
    VcsManager *m_vcsManager = nullptr;
    ModeManager *m_modeManager = nullptr;
    FancyTabWidget *m_modeStack = nullptr;
    NavigationWidget *m_leftNavigationWidget = nullptr;
    NavigationWidget *m_rightNavigationWidget = nullptr;
    RightPaneWidget *m_rightPaneWidget = nullptr;
    VersionDialog *m_versionDialog = nullptr;

    QList<IContext *> m_activeContext;

    std::unordered_map<QWidget *, QList<IContext *>> m_contextWidgets;

    ShortcutSettings *m_shortcutSettings = nullptr;
    ToolSettings *m_toolSettings = nullptr;
    MimeTypeSettings *m_mimeTypeSettings = nullptr;
    SystemEditor *m_systemEditor = nullptr;

    // actions
    QAction *m_toggleLeftSideBarAction = nullptr;
    QAction *m_toggleRightSideBarAction = nullptr;
    QAction *m_toggleMenubarAction = nullptr;
    QAction *m_setModeSelectorStyleIconsAndTextAction = nullptr;
    QAction *m_setModeSelectorStyleHiddenAction = nullptr;
    QAction *m_setModeSelectorStyleIconsOnlyAction = nullptr;

    QToolButton *m_toggleLeftSideBarButton = nullptr;
    QToolButton *m_toggleRightSideBarButton = nullptr;
    QList<std::function<bool()>> m_preCloseListeners;
};

static QMenuBar *globalMenuBar()
{
    return ActionManager::actionContainer(Constants::MENU_BAR)->menuBar();
}

} // Internal

static ICorePrivate *d = nullptr;

static std::function<NewDialog *(QWidget *)> m_newDialogFactory = defaultDialogFactory;

/*!
    Returns the pointer to the instance. Only use for connecting to signals.
*/
ICore *ICore::instance()
{
    return m_core;
}

/*!
    Returns whether the new item dialog is currently open.
*/
bool ICore::isNewItemDialogRunning()
{
    return NewDialog::currentDialog() || IWizardFactory::isWizardRunning();
}

/*!
    Returns the currently open new item dialog widget, or \c nullptr if there is none.

    \sa isNewItemDialogRunning()
    \sa showNewItemDialog()
*/
QWidget *ICore::newItemDialog()
{
    if (NewDialog::currentDialog())
        return NewDialog::currentDialog();
    return IWizardFactory::currentWizard();
}

/*!
    \internal
*/
ICore::ICore()
{
    m_core = this;

    d = new ICorePrivate;
    d->init(); // Separation needed for now as the call triggers other MainWindow calls.

    connect(PluginManager::instance(), &PluginManager::testsFinished,
            this, [this](int failedTests) {
        emit coreAboutToClose();
        if (failedTests != 0)
            qWarning("Test run was not successful: %d test(s) failed.", failedTests);
        QCoreApplication::exit(failedTests);
    });
    connect(PluginManager::instance(), &PluginManager::scenarioFinished,
            this, [this](int exitCode) {
        emit coreAboutToClose();
        QCoreApplication::exit(exitCode);
    });

    Utils::setDialogParentGetter(&ICore::dialogParent);

    d->m_progressManager->init(); // needs the status bar manager
    MessageManager::init();
    OutputPaneManager::create();
}

/*!
    \internal
*/
ICore::~ICore()
{
    delete d;
    m_core = nullptr;
}

/*!
    Opens a dialog where the user can choose from a set of \a factories that
    create new files or projects.

    The \a title argument is shown as the dialog title. The path where the
    files will be created (if the user does not change it) is set
    in \a defaultLocation. Defaults to DocumentManager::projectsDirectory()
    or DocumentManager::fileDialogLastVisitedDirectory(), depending on wizard
    kind.

    Additional variables for the wizards are set in \a extraVariables.

    \sa Core::DocumentManager
    \sa isNewItemDialogRunning()
    \sa newItemDialog()
*/
void ICore::showNewItemDialog(const QString &title,
                              const QList<IWizardFactory *> &factories,
                              const FilePath &defaultLocation,
                              const QVariantMap &extraVariables)
{
    QTC_ASSERT(!isNewItemDialogRunning(), return);

    /* This is a workaround for QDS: In QDS, we currently have a "New Project" dialog box but we do
     * not also have a "New file" dialog box (yet). Therefore, when requested to add a new file, we
     * need to use QtCreator's dialog box. In QDS, if `factories` contains project wizard factories
     * (even though it may contain file wizard factories as well), then we consider it to be a
     * request for "New Project". Otherwise, if we only have file wizard factories, we defer to
     * QtCreator's dialog and request "New File"
     */
    auto dialogFactory = m_newDialogFactory;
    bool haveProjectWizards = Utils::anyOf(factories, [](IWizardFactory *f) {
        return f->kind() == IWizardFactory::ProjectWizard;
    });

    if (!haveProjectWizards)
        dialogFactory = defaultDialogFactory;

    NewDialog *newDialog = dialogFactory(dialogParent());
    connect(newDialog->widget(), &QObject::destroyed, m_core, &ICore::updateNewItemDialogState);
    newDialog->setWizardFactories(factories, defaultLocation, extraVariables);
    newDialog->setWindowTitle(title);
    newDialog->showDialog();

    updateNewItemDialogState();
}

/*!
    Opens the options dialog on the specified \a page. The dialog's \a parent
    defaults to dialogParent(). If the dialog is already shown when this method
    is called, it is just switched to the specified \a page.

    Returns whether the user accepted the dialog.

    \sa msgShowOptionsDialog()
    \sa msgShowOptionsDialogToolTip()
*/
bool ICore::showOptionsDialog(const Id page, QWidget *parent)
{
    return executeSettingsDialog(parent ? parent : dialogParent(), page);
}

/*!
    Opens the options dialog on the specified \a page. The dialog's \a parent
    defaults to dialogParent(). If the dialog is already shown when this method
    is called, it is just switched to the specified \a page.
    Pre-selects some part of the dialog specified by \a item which the dialog
    knows how to interpret.

    Returns whether the user accepted the dialog.

    \sa msgShowOptionsDialog()
    \sa msgShowOptionsDialogToolTip()
*/
bool ICore::showOptionsDialog(const Utils::Id page, Utils::Id item, QWidget *parent)
{
    setPreselectedOptionsPageItem(page, item);
    return showOptionsDialog(page, parent);
}

/*!
    Returns the text to use on buttons that open the options dialog.

    \sa showOptionsDialog()
    \sa msgShowOptionsDialogToolTip()
*/
QString ICore::msgShowOptionsDialog()
{
    return Tr::tr("Configure...", "msgShowOptionsDialog");
}

/*!
    Returns the tool tip to use on buttons that open the options dialog.

    \sa showOptionsDialog()
    \sa msgShowOptionsDialog()
*/
QString ICore::msgShowOptionsDialogToolTip()
{
    if (Utils::HostOsInfo::isMacHost())
        return Tr::tr("Open Preferences dialog.", "msgShowOptionsDialogToolTip (mac version)");
    else
        return Tr::tr("Open Options dialog.", "msgShowOptionsDialogToolTip (non-mac version)");
}

/*!
    Creates a message box with \a parent that contains a \uicontrol Configure
    button for opening the settings page specified by \a settingsId.

    The dialog has \a title and displays the message \a text and detailed
    information specified by \a details.

    Use this function to display configuration errors and to point users to the
    setting they should fix.

    Returns \c true if the user accepted the settings dialog.

    \sa showOptionsDialog()
*/
bool ICore::showWarningWithOptions(const QString &title, const QString &text,
                                   const QString &details, Id settingsId, QWidget *parent)
{
    if (!parent)
        parent = d->m_mainwindow;
    QMessageBox msgBox(QMessageBox::Warning, title, text,
                       QMessageBox::Ok, parent);
    msgBox.setEscapeButton(QMessageBox::Ok);
    if (!details.isEmpty())
        msgBox.setDetailedText(details);
    QAbstractButton *settingsButton = nullptr;
    if (settingsId.isValid())
        settingsButton = msgBox.addButton(msgShowOptionsDialog(), QMessageBox::AcceptRole);
    msgBox.exec();
    if (settingsButton && msgBox.clickedButton() == settingsButton)
        return showOptionsDialog(settingsId);
    return false;
}

 bool ICore::isQtDesignStudio()
{
    QtcSettings *settings = Core::ICore::settings();
    return settings->value("QML/Designer/StandAloneMode", false).toBool();
}

/*!
    Returns the application's main settings object.

    You can use it to retrieve or set application-wide settings
    (in contrast to session or project specific settings).

    If \a scope is \c QSettings::UserScope (the default), the
    settings will be read from the user's settings, with
    a fallback to global settings provided with \QC.

    If \a scope is \c QSettings::SystemScope, only the installation settings
    shipped with the current version of \QC will be read. This
    functionality exists for internal purposes only.
*/
QtcSettings *ICore::settings(QSettings::Scope scope)
{
    if (scope == QSettings::UserScope)
        return PluginManager::settings();
    else
        return PluginManager::globalSettings();
}

/*!
    Returns the application's printer object.

    Always use this printer object for printing, so the different parts of the
    application re-use its settings.
*/
QPrinter *ICore::printer()
{
    if (!d->m_printer)
        d->m_printer = new QPrinter(QPrinter::HighResolution);
    return d->m_printer;
}

/*!
    Returns the locale string for the user interface language that is currently
    configured in \QC. Use this to install your plugin's translation file with
    QTranslator.
*/
QString ICore::userInterfaceLanguage()
{
    return qApp->property("qtc_locale").toString();
}

static QString pathHelper(const QString &rel)
{
    if (rel.isEmpty())
        return rel;
    if (rel.startsWith('/'))
        return rel;
    return '/' + rel;
}
/*!
    Returns the absolute path for the relative path \a rel that is used for resources like
    project templates and the debugger macros.

    This abstraction is needed to avoid platform-specific code all over
    the place, since on \macos, for example, the resources are part of the
    application bundle.

    \sa userResourcePath()
*/
FilePath ICore::resourcePath(const QString &rel)
{
    return FilePath::fromString(
               QDir::cleanPath(QCoreApplication::applicationDirPath() + '/' + RELATIVE_DATA_PATH))
           / rel;
}

/*!
    Returns the absolute path for the relative path \a rel in the users directory that is used for
    resources like project templates.

    Use this function for finding the place for resources that the user may
    write to, for example, to allow for custom palettes or templates.

    \sa resourcePath()
*/

FilePath ICore::userResourcePath(const QString &rel)
{
    // Create qtcreator dir if it doesn't yet exist
    const QString configDir = QFileInfo(settings(QSettings::UserScope)->fileName()).path();
    const QString urp = configDir + '/' + appInfo().id;

    if (!QFileInfo::exists(urp + QLatin1Char('/'))) {
        QDir dir;
        if (!dir.mkpath(urp))
            qWarning() << "could not create" << urp;
    }

    return FilePath::fromString(urp + pathHelper(rel));
}

/*!
    Returns a writable path for the relative path \a rel that can be used for persistent cache files.
*/
FilePath ICore::cacheResourcePath(const QString &rel)
{
    return FilePath::fromString(QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                                + pathHelper(rel));
}

/*!
    Returns the path, based on the relative path \a rel, to resources written by the installer,
    for example pre-defined kits and toolchains.
*/
FilePath ICore::installerResourcePath(const QString &rel)
{
    return FilePath::fromString(settings(QSettings::SystemScope)->fileName()).parentDir()
           / appInfo().id / rel;
}

/*!
    Returns the path to the plugins that are included in the \QC installation.

    \internal
*/
QString ICore::pluginPath()
{
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + '/' + RELATIVE_PLUGIN_PATH);
}

/*!
    Returns the path where user-specific plugins should be written.

    \internal
*/
QString ICore::userPluginPath()
{
    const QVersionNumber appVersion = QVersionNumber::fromString(
        QCoreApplication::applicationVersion());
    QString pluginPath = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation);
    if (Utils::HostOsInfo::isAnyUnixHost() && !Utils::HostOsInfo::isMacHost())
        pluginPath += "/data";
    pluginPath += '/' + QCoreApplication::organizationName() + '/';
    pluginPath += Utils::HostOsInfo::isMacHost() ? QGuiApplication::applicationDisplayName()
                                                 : appInfo().id;
    pluginPath += "/plugins/";
    pluginPath += QString::number(appVersion.majorVersion()) + '.'
                  + QString::number(appVersion.minorVersion()) + '.'
                  + QString::number(appVersion.microVersion());
    return pluginPath;
}

/*!
    Returns the path, based on the relative path \a rel, to the command line tools that are
    included in the \QC installation.
 */
FilePath ICore::libexecPath(const QString &rel)
{
    return FilePath::fromString(QDir::cleanPath(QApplication::applicationDirPath()
                                                + pathHelper(RELATIVE_LIBEXEC_PATH)))
           / rel;
}

FilePath ICore::crashReportsPath()
{
    if (Utils::HostOsInfo::isMacHost())
        return Core::ICore::userResourcePath("crashpad_reports/completed");
    else
        return libexecPath("crashpad_reports/reports");
}

static QString clangIncludePath(const QString &clangVersion)
{
    return "/lib/clang/" + clangVersion + "/include";
}

/*!
    \internal
*/
FilePath ICore::clangIncludeDirectory(const QString &clangVersion,
                                      const FilePath &clangFallbackIncludeDir)
{
    FilePath dir = libexecPath("clang" + clangIncludePath(clangVersion));
    if (!dir.exists() || !dir.pathAppended("stdint.h").exists())
        dir = clangFallbackIncludeDir;
    return dir.canonicalPath();
}

/*!
    \internal
*/
static FilePath clangBinary(const QString &binaryBaseName, const FilePath &clangBinDirectory)
{
    FilePath executable =
        ICore::libexecPath("clang/bin").pathAppended(binaryBaseName).withExecutableSuffix();
    if (!executable.exists())
        executable = clangBinDirectory.pathAppended(binaryBaseName).withExecutableSuffix();
    return executable.canonicalPath();
}

/*!
    \internal
*/
FilePath ICore::clangExecutable(const FilePath &clangBinDirectory)
{
    return clangBinary("clang", clangBinDirectory);
}

/*!
    \internal
*/
FilePath ICore::clangdExecutable(const FilePath &clangBinDirectory)
{
    return clangBinary("clangd", clangBinDirectory);
}

/*!
    \internal
*/
FilePath ICore::clangTidyExecutable(const FilePath &clangBinDirectory)
{
    return clangBinary("clang-tidy", clangBinDirectory);
}

/*!
    \internal
*/
FilePath ICore::clazyStandaloneExecutable(const FilePath &clangBinDirectory)
{
    return clangBinary("clazy-standalone", clangBinDirectory);
}

/*!
    \internal
 */
FilePath ICore::lldbExecutable(const Utils::FilePath &lldbBinDirectory)
{
    return clangBinary("lldb", lldbBinDirectory);
}

static QString compilerString()
{
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
    QString platformSpecific;
#if defined(__apple_build_version__) // Apple clang has other version numbers
    platformSpecific = QLatin1String(" (Apple)");
#elif defined(Q_CC_MSVC)
    platformSpecific = QLatin1String(" (clang-cl)");
#endif
    return QLatin1String("Clang " ) + QString::number(__clang_major__) + QLatin1Char('.')
            + QString::number(__clang_minor__) + platformSpecific;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC " ) + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    if (_MSC_VER > 1999)
        return QLatin1String("MSVC <unknown>");
    if (_MSC_VER >= 1930)
        return QLatin1String("MSVC 2022");
    if (_MSC_VER >= 1920)
        return QLatin1String("MSVC 2019");
    if (_MSC_VER >= 1910)
        return QLatin1String("MSVC 2017");
    if (_MSC_VER >= 1900)
        return QLatin1String("MSVC 2015");
#endif
    return QLatin1String("<unknown compiler>");
}

/*!
    Returns a string with the IDE's name and version, in the form "\QC X.Y.Z".
    Use this for "Generated by" strings and similar tasks.
*/
QString ICore::versionString()
{
    QString ideVersionDescription;
    if (QCoreApplication::applicationVersion() != appInfo().displayVersion)
        ideVersionDescription = QString(" (%1)").arg(QCoreApplication::applicationVersion());
    return QString("%1 %2%3").arg(QGuiApplication::applicationDisplayName(),
                                  appInfo().displayVersion,
                                  ideVersionDescription);
}

/*!
    Returns a list IContexts for the current top level context widget, or an empty list if
    there is none.

    \sa updateAdditionalContexts()
    \sa addContextObject()
    \sa {The Action Manager and Commands}
*/
QList<IContext *> ICore::currentContextObjects()
{
    if (d->m_activeContext.isEmpty())
        return {};
    return d->m_contextWidgets[d->m_activeContext.first()->widget()];
}

/*!
    Returns the widget of the top level IContext of the current context, or \c
    nullptr if there is none.

    \sa currentContextObject()
*/
QWidget *ICore::currentContextWidget()
{
    return d->m_activeContext.isEmpty() ? nullptr : d->m_activeContext.first()->widget();
}

/*!
    Returns the main window of the application.

    For dialog parents use dialogParent().

    \sa dialogParent()
*/
QMainWindow *ICore::mainWindow()
{
    return d->m_mainwindow;
}

/*!
    Returns a widget pointer suitable to use as parent for QDialogs.
*/
QWidget *ICore::dialogParent()
{
    QWidget *active = QApplication::activeModalWidget();
    if (!active)
        active = QApplication::activeWindow();
    if (!active || active->windowFlags().testFlag(Qt::SplashScreen)
        || active->windowFlags().testFlag(Qt::Popup)) {
        active = d->m_mainwindow;
    }
    return active;
}

/*!
    \internal
*/
QStatusBar *ICore::statusBar()
{
    return d->m_modeStack->statusBar();
}

/*!
    Returns a central InfoBar that is shown in \QC's main window.
    Use for notifying the user of something without interrupting with
    dialog. Use sparingly.
*/
Utils::InfoBar *ICore::infoBar()
{
    return d->m_modeStack->infoBar();
}

/*!
    Shows a modal dialog that asks the user if they want to restart \QC.
    Uses \a text as the main text in the dialog, and triggers a restart
    of \QC if the user chooses that option.
*/
void ICore::askForRestart(const QString &text)
{
    QMessageBox mb(dialogParent());
    mb.setWindowTitle(Tr::tr("Restart Required"));
    mb.setText(text);
    mb.setIcon(QMessageBox::Information);
    mb.addButton(Tr::tr("Later"), QMessageBox::NoRole);
    mb.addButton(Tr::tr("Restart Now"), QMessageBox::YesRole);

    mb.connect(&mb, &QDialog::accepted, ICore::instance(), &ICore::restart, Qt::QueuedConnection);
    mb.exec();
}

/*!
    Raises and activates the window for \a widget. This contains workarounds
    for X11.
*/
void ICore::raiseWindow(QWidget *widget)
{
    if (!widget)
        return;
    QWidget *window = widget->window();
    if (!window)
        return;
    if (window == d->m_mainwindow) {
        d->m_mainwindow->raiseWindow();
    } else {
        window->raise();
        window->activateWindow();
    }
}

void ICore::raiseMainWindow()
{
    d->m_mainwindow->raiseWindow();
}

/*!
    Removes the contexts specified by \a remove from the list of active
    additional contexts, and adds the contexts specified by \a add with \a
    priority.

    The additional contexts are not associated with an IContext instance.

    High priority additional contexts have higher priority than the contexts
    added by IContext instances, low priority additional contexts have lower
    priority than the contexts added by IContext instances.

    \sa addContextObject()
    \sa {The Action Manager and Commands}
*/
void ICore::updateAdditionalContexts(const Context &remove, const Context &add,
                                     ContextPriority priority)
{
    for (const Id id : remove) {
        if (!id.isValid())
            continue;
        int index = d->m_lowPrioAdditionalContexts.indexOf(id);
        if (index != -1)
            d->m_lowPrioAdditionalContexts.removeAt(index);
        index = d->m_highPrioAdditionalContexts.indexOf(id);
        if (index != -1)
            d->m_highPrioAdditionalContexts.removeAt(index);
    }

    for (const Id id : add) {
        if (!id.isValid())
            continue;
        Context &cref = (priority == ICore::ContextPriority::High ? d->m_highPrioAdditionalContexts
                                                                  : d->m_lowPrioAdditionalContexts);
        if (!cref.contains(id))
            cref.prepend(id);
    }

    d->updateContext();
}

/*!
    Adds \a context with \a priority to the list of active additional contexts.

    \sa updateAdditionalContexts()
*/
void ICore::addAdditionalContext(const Context &context, ContextPriority priority)
{
    updateAdditionalContexts(Context(), context, priority);
}

/*!
    Removes \a context from the list of active additional contexts.

    \sa updateAdditionalContexts()
*/
void ICore::removeAdditionalContext(const Context &context)
{
    updateAdditionalContexts(context, Context(), ContextPriority::Low);
}

/*!
    Registers a \a window with the specified \a context. Registered windows are
    shown in the \uicontrol Window menu and get registered for the various
    window related actions, like the minimize, zoom, fullscreen and close
    actions. The context for the actions is \a context by default, but can be
    overridden with \a actionContext.

    Whenever the application focus is in \a window, its \a context is made
    active.
*/
void ICore::registerWindow(QWidget *window, const Context &context, const Context &actionContext)
{
    new WindowSupport(window, context, actionContext); // deletes itself when widget is destroyed
}

/*!
    Provides a hook for plugins to veto on closing the application.

    When the application window requests a close, all listeners are called. If
    one of the \a listener calls returns \c false, the process is aborted and
    the event is ignored. If all calls return \c true, coreAboutToClose()
    is emitted and the event is accepted or performed.
*/
void ICore::addPreCloseListener(const std::function<bool ()> &listener)
{
    d->m_preCloseListeners.append(listener);
}

QString uiConfigInformation()
{
    QString info("UI configuration:\n\n");
    info.append(QString("Color: %1\n").arg(StyleHelper::requestedBaseColor().name()));
    info.append(QString("Theme: %1 \"%2\"\n").arg(creatorTheme()->id())
                           .arg(creatorTheme()->displayName()));
    const QString toolbarStyle =
        StyleHelper::toolbarStyle() == StyleHelper::ToolbarStyleCompact ? "Compact" : "Relaxed";
    info.append(QString("Toolbar style: Utils::StyleHelper::ToolbarStyle%1\n").arg(toolbarStyle));
    const QString policy =
        QVariant::fromValue(QApplication::highDpiScaleFactorRoundingPolicy()).toString();
    QString userInterfaceLanguage = ICore::userInterfaceLanguage();
    if (userInterfaceLanguage.isEmpty())
        userInterfaceLanguage = QLocale::system().name() + " (System Language)";
    info.append(QString("Language: %1\n").arg(userInterfaceLanguage));
    info.append(QString("Device pixel ratio: %1, Qt::HighDpiScaleFactorRoundingPolicy::%2\n")
                    .arg(qApp->devicePixelRatio()).arg(policy));
    info.append(QString("Font DPI: %1\n").arg(qApp->fontMetrics().fontDpi()));

    info.append(QString("Utils::StyleHelper::UiElement:\n"));
#define QTC_ADD_UIELEMENT_FONT(uiElement) (                                              \
        info.append(QString("  %1: %2\n").arg(#uiElement)                                \
                .arg(StyleHelper::uiFont(StyleHelper::UiElement##uiElement).toString())) \
    );
    QTC_ADD_UIELEMENT_FONT(H1);
    QTC_ADD_UIELEMENT_FONT(H2);
    QTC_ADD_UIELEMENT_FONT(H3);
    QTC_ADD_UIELEMENT_FONT(H4);
    QTC_ADD_UIELEMENT_FONT(H5);
    QTC_ADD_UIELEMENT_FONT(H6);
    QTC_ADD_UIELEMENT_FONT(H6Capital);
    QTC_ADD_UIELEMENT_FONT(Body1);
    QTC_ADD_UIELEMENT_FONT(Body2);
    QTC_ADD_UIELEMENT_FONT(ButtonMedium);
    QTC_ADD_UIELEMENT_FONT(ButtonSmall);
    QTC_ADD_UIELEMENT_FONT(LabelMedium);
    QTC_ADD_UIELEMENT_FONT(LabelSmall);
    QTC_ADD_UIELEMENT_FONT(CaptionStrong);
    QTC_ADD_UIELEMENT_FONT(Caption);
    QTC_ADD_UIELEMENT_FONT(IconStandard);
    QTC_ADD_UIELEMENT_FONT(IconActive);
#undef QTC_ADD_UIELEMENT_FONT

    return info;
}

/*!
    \internal
*/
QString ICore::systemInformation()
{
    return PluginManager::systemInformation() + '\n' + uiConfigInformation() + '\n'
           + aboutInformationCompact() + '\n';
}

static const QString &screenShotsPath()
{
    static const QString path = qtcEnvironmentVariable("QTC_SCREENSHOTS_PATH");
    return path;
}

class ScreenShooter : public QObject
{
public:
    ScreenShooter(QWidget *widget, const QString &name, const QRect &rc)
        : m_widget(widget), m_name(name), m_rc(rc)
    {
        m_widget->installEventFilter(this);
    }

    bool eventFilter(QObject *watched, QEvent *event) override
    {
        QTC_ASSERT(watched == m_widget, return false);
        if (event->type() == QEvent::Show)
            QMetaObject::invokeMethod(this, &ScreenShooter::helper, Qt::QueuedConnection);
        return false;
    }

    void helper()
    {
        if (m_widget) {
            QRect rc = m_rc.isValid() ? m_rc : m_widget->rect();
            QPixmap pm = m_widget->grab(rc);
            for (int i = 0; ; ++i) {
                QString fileName = screenShotsPath() + '/' + m_name + QString("-%1.png").arg(i);
                if (!QFileInfo::exists(fileName)) {
                    pm.save(fileName);
                    break;
                }
            }
        }
        deleteLater();
    }

    QPointer<QWidget> m_widget;
    QString m_name;
    QRect m_rc;
};

/*!
    \internal
*/
void ICore::setupScreenShooter(const QString &name, QWidget *w, const QRect &rc)
{
    if (!screenShotsPath().isEmpty())
        new ScreenShooter(w, name, rc);
}

static void setRestart(bool restart)
{
    qApp->setProperty("restart", restart);
}

/*!
    Restarts \QC and restores the last session.
*/
void ICore::restart()
{
    setRestart(true);
    exit();
}

/*!
    \internal
*/
void ICore::setRelativePathToProjectFunction(const std::function<FilePath(const FilePath &)> &func)
{
    m_core->m_relativePathToProject = func;
}

/*!
    \internal
*/
FilePath ICore::pathRelativeToActiveProject(const FilePath &path)
{
    if (m_core->m_relativePathToProject)
        return m_core->m_relativePathToProject(path);

    return path;
}

/*!
    \internal
*/
void ICore::saveSettings(SaveSettingsReason reason)
{
    emit m_core->saveSettingsRequested(reason);

    QtcSettings *settings = PluginManager::settings();
    settings->beginGroup(settingsGroup);

    if (!(s_overrideColor.isValid() && StyleHelper::baseColor() == s_overrideColor))
        settings->setValueWithDefault(colorKey,
                                      StyleHelper::requestedBaseColor(),
                                      QColor(StyleHelper::DEFAULT_BASE_COLOR));

    if (Internal::globalMenuBar() && !Internal::globalMenuBar()->isNativeMenuBar())
        settings->setValue(menubarVisibleKey, Internal::globalMenuBar()->isVisible());

    settings->endGroup();

    DocumentManager::saveSettings();
    ActionManager::saveSettings();
    EditorManagerPrivate::saveSettings();
    d->m_leftNavigationWidget->saveSettings(settings);
    d->m_rightNavigationWidget->saveSettings(settings);

    // TODO Remove some time after Qt Creator 11
    // Work around Qt Creator <= 10 writing the default terminal to the settings.
    // TerminalCommand writes the terminal to the settings when changing it, which usually is
    // enough. But because of the bug in Qt Creator <= 10 we want to clean up the settings
    // even if the user never touched the terminal setting.
    if (HostOsInfo::isMacHost())
        TerminalCommand::setTerminalEmulator(TerminalCommand::terminalEmulator());

    ICore::settings(QSettings::SystemScope)->sync();
    ICore::settings(QSettings::UserScope)->sync();
}

/*!
    \internal
*/
QStringList ICore::additionalAboutInformation()
{
    return d->m_aboutInformation;
}

/*!
    \internal
*/
void ICore::clearAboutInformation()
{
    d->m_aboutInformation.clear();
}

/*!
    \internal
*/
void ICore::appendAboutInformation(const QString &line)
{
    d->m_aboutInformation.append(line);
}

/*!
    \internal
*/
QString ICore::aboutInformationCompact()
{
    QString information = QString("Product: %1\n").arg(versionString());
    information += QString("Based on: Qt %1 (%2, %3)\n")
                       .arg(QLatin1String(qVersion()), compilerString(),
                            QSysInfo::buildCpuArchitecture());
#ifdef QTC_SHOW_BUILD_DATE
    information += QString("Built on: %1 %2\n").arg(QLatin1String(__DATE__),
                                                    QLatin1String(__TIME__));
#endif
    const AppInfo &appInfo = Utils::appInfo();
    if (!appInfo.revision.isEmpty())
        information += QString("From revision: %1\n").arg(appInfo.revision.left(10));

    return information;
}

/*!
    \internal
*/
QString ICore::aboutInformationHtml()
{
    const QString buildCompatibilityString = Tr::tr("Based on Qt %1 (%2, %3)")
                                                 .arg(QLatin1String(qVersion()), compilerString(),
                                                      QSysInfo::buildCpuArchitecture());
    const AppInfo &appInfo = Utils::appInfo();
    QString ideRev;
    if (!appInfo.revision.isEmpty())
        ideRev = Tr::tr("<br/>From revision %1<br/>")
                     .arg(appInfo.revisionUrl.isEmpty()
                              ? appInfo.revision
                              : QString::fromLatin1("<a href=\"%1\">%2</a>")
                                    .arg(appInfo.revisionUrl, appInfo.revision));
    QString buildDateInfo;
#ifdef QTC_SHOW_BUILD_DATE
    buildDateInfo = Tr::tr("<br/>Built on %1 %2<br/>").arg(QLatin1String(__DATE__),
                                                           QLatin1String(__TIME__));
#endif

    const QString br = QLatin1String("<br/>");
    const QStringList additionalInfoLines = ICore::additionalAboutInformation();
    const QString additionalInfo =
        QStringList(Utils::transform(additionalInfoLines, &QString::toHtmlEscaped)).join(br);
    const QString information
        = Tr::tr("<h3>%1</h3>"
                 "%2<br/>"
                 "%3"
                 "%4"
                 "%5"
                 "<br/>"
                 "Copyright 2008-%6 %7. All rights reserved.<br/>"
                 "<br/>"
                 "The program is provided AS IS with NO WARRANTY OF ANY KIND, "
                 "INCLUDING THE WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A "
                 "PARTICULAR PURPOSE.<br/>")
              .arg(ICore::versionString(),
                   buildCompatibilityString,
                   buildDateInfo,
                   ideRev,
                   additionalInfo.isEmpty() ? QString() : br + additionalInfo + br,
                   appInfo.year,
                   appInfo.author)
          + "<br/>"
          + Tr::tr("The Qt logo as well as Qt®, Qt Quick®, Built with Qt®, Boot to Qt®, "
                   "Qt Quick Compiler®, Qt Enterprise®, Qt Mobile® and Qt Embedded® are "
                   "registered trademarks of The Qt Company Ltd.");

    return information;
}

void ICore::updateNewItemDialogState()
{
    static bool wasRunning = false;
    static QWidget *previousDialog = nullptr;
    if (wasRunning == isNewItemDialogRunning() && previousDialog == newItemDialog())
        return;
    wasRunning = isNewItemDialogRunning();
    previousDialog = newItemDialog();
    emit instance()->newItemDialogStateChanged();
}

/*!
    \internal
*/
void ICore::setNewDialogFactory(const std::function<NewDialog *(QWidget *)> &newFactory)
{
    m_newDialogFactory = newFactory;
}

static bool hideToolsMenu()
{
    return Core::ICore::settings()->value(Constants::SETTINGS_MENU_HIDE_TOOLS, false).toBool();
}

enum { debugMainWindow = 0 };

namespace Internal {

void ICorePrivate::init()
{
    m_mainwindow = new MainWindow;

    m_progressManager = new ProgressManagerPrivate;
    m_jsExpander = JsExpander::createGlobalJsExpander();
    m_vcsManager = new VcsManager;
    m_modeStack = new FancyTabWidget(m_mainwindow);
    m_shortcutSettings = new ShortcutSettings;
    m_toolSettings = new ToolSettings;
    m_mimeTypeSettings = new MimeTypeSettings;
    m_systemEditor = new SystemEditor;
    m_toggleLeftSideBarButton = new QToolButton;
    m_toggleRightSideBarButton = new QToolButton;

    (void) new DocumentManager(this);

    HistoryCompleter::setSettings(PluginManager::settings());

    if (HostOsInfo::isLinuxHost())
        QApplication::setWindowIcon(Icons::QTCREATORLOGO_BIG.icon());
    QString baseName = QApplication::style()->objectName();
    // Sometimes we get the standard windows 95 style as a fallback
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost()
            && baseName == QLatin1String("windows")) {
        baseName = QLatin1String("fusion");
    }

    // if the user has specified as base style in the theme settings,
    // prefer that
    const QStringList available = QStyleFactory::keys();
    const QStringList styles = Utils::creatorTheme()->preferredStyles();
    for (const QString &s : styles) {
        if (available.contains(s, Qt::CaseInsensitive)) {
            baseName = s;
            break;
        }
    }

    QApplication::setStyle(new ManhattanStyle(baseName));

    m_modeManager = new ModeManager(m_modeStack);
    connect(m_modeStack, &FancyTabWidget::topAreaClicked, this, [](Qt::MouseButton, Qt::KeyboardModifiers modifiers) {
        if (modifiers & Qt::ShiftModifier) {
            QColor color = QColorDialog::getColor(StyleHelper::requestedBaseColor(), ICore::dialogParent());
            if (color.isValid())
                StyleHelper::setBaseColor(color);
        }
    });

    m_mainwindow->setCentralWidget(d->m_modeStack);

    registerDefaultContainers();
    registerDefaultActions();

    m_leftNavigationWidget = new NavigationWidget(m_toggleLeftSideBarAction, Side::Left);
    m_rightNavigationWidget = new NavigationWidget(m_toggleRightSideBarAction, Side::Right);
    m_rightPaneWidget = new RightPaneWidget();

    m_editorManager = new EditorManager(this);
    m_externalToolManager = new ExternalToolManager();

    m_progressManager->progressView()->setParent(m_mainwindow);

    connect(qApp, &QApplication::focusChanged, this, &ICorePrivate::updateFocusWidget);

    // Add small Toolbuttons for toggling the navigation widgets
    StatusBarManager::addStatusBarWidget(m_toggleLeftSideBarButton, StatusBarManager::First);
    int childsCount = m_modeStack->statusBar()
                          ->findChildren<QWidget *>(QString(), Qt::FindDirectChildrenOnly)
                          .count();
    m_modeStack->statusBar()->insertPermanentWidget(childsCount - 1,
                                                    m_toggleRightSideBarButton); // before QSizeGrip

    //    setUnifiedTitleAndToolBarOnMac(true);
    //if (HostOsInfo::isAnyUnixHost())
    //signal(SIGINT, handleSigInt);

    m_modeStack->statusBar()->setProperty("p_styled", true);

    auto dropSupport = new DropSupport(m_mainwindow, [](QDropEvent *event, DropSupport *) {
        return event->source() == nullptr; // only accept drops from the "outside" (e.g. file manager)
    });
    connect(dropSupport, &DropSupport::filesDropped, this, &ICorePrivate::openDroppedFiles);

    if (HostOsInfo::isLinuxHost()) {
        m_trimTimer.setSingleShot(true);
        using namespace std::chrono_literals;
        m_trimTimer.setInterval(60s);
        // glibc may not actually free memory in free().
#ifdef Q_OS_LINUX
        connect(&m_trimTimer, &QTimer::timeout, this, [] { malloc_trim(0); });
#endif
    }
}



NavigationWidget *ICorePrivate::navigationWidget(Side side) const
{
    return side == Side::Left ? m_leftNavigationWidget : m_rightNavigationWidget;
}

void ICorePrivate::setSidebarVisible(bool visible, Side side)
{
    navigationWidget(side)->setShown(visible);
}

ICorePrivate::~ICorePrivate()
{
    // explicitly delete window support, because that calls methods from ICore that call methods
    // from mainwindow, so mainwindow still needs to be alive
    delete m_windowSupport;
    m_windowSupport = nullptr;

    delete m_externalToolManager;
    m_externalToolManager = nullptr;
    MessageManager::destroy();
    delete m_shortcutSettings;
    m_shortcutSettings = nullptr;
    delete m_toolSettings;
    m_toolSettings = nullptr;
    delete m_mimeTypeSettings;
    m_mimeTypeSettings = nullptr;
    delete m_systemEditor;
    m_systemEditor = nullptr;
    delete m_printer;
    m_printer = nullptr;
    delete m_vcsManager;
    m_vcsManager = nullptr;
    //we need to delete editormanager and statusbarmanager explicitly before the end of the destructor,
    //because they might trigger stuff that tries to access data from editorwindow, like removeContextWidget

    // All modes are now gone
    OutputPaneManager::destroy();

    delete m_leftNavigationWidget;
    delete m_rightNavigationWidget;
    m_leftNavigationWidget = nullptr;
    m_rightNavigationWidget = nullptr;

    delete m_editorManager;
    m_editorManager = nullptr;
    delete m_progressManager;
    m_progressManager = nullptr;

    delete m_rightPaneWidget;
    m_rightPaneWidget = nullptr;

    delete m_modeManager;
    m_modeManager = nullptr;

    delete m_jsExpander;
    m_jsExpander = nullptr;

    delete m_mainwindow;
    m_mainwindow = nullptr;
}

} // Internal

void ICore::extensionsInitialized()
{
    EditorManagerPrivate::extensionsInitialized();
    MimeTypeSettings::restoreSettings();
    d->m_windowSupport = new WindowSupport(d->m_mainwindow, Context("Core.MainWindow"));
    d->m_windowSupport->setCloseActionEnabled(false);
    OutputPaneManager::initialize();
    VcsManager::extensionsInitialized();
    d->m_leftNavigationWidget->setFactories(INavigationWidgetFactory::allNavigationFactories());
    d->m_rightNavigationWidget->setFactories(INavigationWidgetFactory::allNavigationFactories());

    ModeManager::extensionsInitialized();

    d->readSettings();
    d->updateContext();

    emit m_core->coreAboutToOpen();
    // Delay restoreWindowState, since it is overridden by LayoutRequest event
    QMetaObject::invokeMethod(d, &ICorePrivate::restoreWindowState, Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_core, &ICore::coreOpened, Qt::QueuedConnection);
}

void ICore::aboutToShutdown()
{
    disconnect(qApp, &QApplication::focusChanged, d, &ICorePrivate::updateFocusWidget);
    for (auto contextsPair : d->m_contextWidgets) {
        for (auto context : contextsPair.second)
            disconnect(context, &QObject::destroyed, d->m_mainwindow, nullptr);
    }
    d->m_activeContext.clear();
    d->m_mainwindow->hide();
}

void ICore::restartTrimmer()
{
    if (HostOsInfo::isLinuxHost() && !d->m_trimTimer.isActive())
        d->m_trimTimer.start();
}

namespace Internal {

void MainWindow::closeEvent(QCloseEvent *event)
{
    const auto cancelClose = [event] {
        event->ignore();
        setRestart(false);
    };

    // work around QTBUG-43344
    static bool alreadyClosed = false;
    if (alreadyClosed) {
        event->accept();
        return;
    }

    if (systemSettings().askBeforeExit()
        && (QMessageBox::question(this,
                                  Tr::tr("Exit %1?").arg(QGuiApplication::applicationDisplayName()),
                                  Tr::tr("Exit %1?").arg(QGuiApplication::applicationDisplayName()),
                                  QMessageBox::Yes | QMessageBox::No,
                                  QMessageBox::No)
            == QMessageBox::No)) {
        event->ignore();
        return;
    }

    ICore::saveSettings(ICore::MainWindowClosing);

    // Save opened files
    if (!DocumentManager::saveAllModifiedDocuments()) {
        cancelClose();
        return;
    }

    const QList<std::function<bool()>> listeners = d->m_preCloseListeners;
    for (const std::function<bool()> &listener : listeners) {
        if (!listener()) {
            cancelClose();
            return;
        }
    }

    emit m_core->coreAboutToClose();

    d->saveWindowSettings();

    d->m_leftNavigationWidget->closeSubWidgets();
    d->m_rightNavigationWidget->closeSubWidgets();

    event->accept();
    alreadyClosed = true;
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    ICore::restartTrimmer();
    AppMainWindow::keyPressEvent(event);
}

void MainWindow::mousePressEvent(QMouseEvent *event)
{
    ICore::restartTrimmer();
    AppMainWindow::mousePressEvent(event);
}

void ICorePrivate::openDroppedFiles(const QList<DropSupport::FileSpec> &files)
{
    m_mainwindow->raiseWindow();
    const FilePaths filePaths = Utils::transform(files, &DropSupport::FileSpec::filePath);
    ICore::openFiles(filePaths, ICore::SwitchMode);
}

void ICorePrivate::registerDefaultContainers()
{
    ActionContainer *menubar = ActionManager::createMenuBar(Constants::MENU_BAR);

    if (!HostOsInfo::isMacHost()) // System menu bar on Mac
        m_mainwindow->setMenuBar(menubar->menuBar());
    menubar->appendGroup(Constants::G_FILE);
    menubar->appendGroup(Constants::G_EDIT);
    menubar->appendGroup(Constants::G_VIEW);
    menubar->appendGroup(Constants::G_TOOLS);
    menubar->appendGroup(Constants::G_WINDOW);
    menubar->appendGroup(Constants::G_HELP);

    // File Menu
    ActionContainer *filemenu = ActionManager::createMenu(Constants::M_FILE);
    menubar->addMenu(filemenu, Constants::G_FILE);
    filemenu->menu()->setTitle(Tr::tr("&File"));
    filemenu->appendGroup(Constants::G_FILE_NEW);
    filemenu->appendGroup(Constants::G_FILE_OPEN);
    filemenu->appendGroup(Constants::G_FILE_SESSION);
    filemenu->appendGroup(Constants::G_FILE_PROJECT);
    filemenu->appendGroup(Constants::G_FILE_SAVE);
    filemenu->appendGroup(Constants::G_FILE_EXPORT);
    filemenu->appendGroup(Constants::G_FILE_CLOSE);
    filemenu->appendGroup(Constants::G_FILE_PRINT);
    filemenu->appendGroup(Constants::G_FILE_OTHER);
    connect(filemenu->menu(), &QMenu::aboutToShow, this, &ICorePrivate::aboutToShowRecentFiles);


    // Edit Menu
    ActionContainer *medit = ActionManager::createMenu(Constants::M_EDIT);
    menubar->addMenu(medit, Constants::G_EDIT);
    medit->menu()->setTitle(Tr::tr("&Edit"));
    medit->appendGroup(Constants::G_EDIT_UNDOREDO);
    medit->appendGroup(Constants::G_EDIT_COPYPASTE);
    medit->appendGroup(Constants::G_EDIT_SELECTALL);
    medit->appendGroup(Constants::G_EDIT_ADVANCED);
    medit->appendGroup(Constants::G_EDIT_FIND);
    medit->appendGroup(Constants::G_EDIT_OTHER);

    ActionContainer *mview = ActionManager::createMenu(Constants::M_VIEW);
    menubar->addMenu(mview, Constants::G_VIEW);
    mview->menu()->setTitle(Tr::tr("&View"));
    mview->appendGroup(Constants::G_VIEW_VIEWS);
    mview->appendGroup(Constants::G_VIEW_PANES);

    // Tools Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_TOOLS);
    ac->setParent(this);
    if (!hideToolsMenu())
        menubar->addMenu(ac, Constants::G_TOOLS);

    ac->menu()->setTitle(Tr::tr("&Tools"));

    // Window Menu
    ActionContainer *mwindow = ActionManager::createMenu(Constants::M_WINDOW);
    menubar->addMenu(mwindow, Constants::G_WINDOW);
    mwindow->menu()->setTitle(Tr::tr("&Window"));
    mwindow->appendGroup(Constants::G_WINDOW_SIZE);
    mwindow->appendGroup(Constants::G_WINDOW_SPLIT);
    mwindow->appendGroup(Constants::G_WINDOW_NAVIGATE);
    mwindow->appendGroup(Constants::G_WINDOW_LIST);
    mwindow->appendGroup(Constants::G_WINDOW_OTHER);

    // Help Menu
    ac = ActionManager::createMenu(Constants::M_HELP);
    menubar->addMenu(ac, Constants::G_HELP);
    ac->menu()->setTitle(Tr::tr("&Help"));
    Theme::setHelpMenu(ac->menu());
    ac->appendGroup(Constants::G_HELP_HELP);
    ac->appendGroup(Constants::G_HELP_SUPPORT);
    ac->appendGroup(Constants::G_HELP_ABOUT);
    ac->appendGroup(Constants::G_HELP_UPDATES);

    // macOS touch bar
    ac = ActionManager::createTouchBar(Constants::TOUCH_BAR,
                                       QIcon(),
                                       "Main TouchBar" /*never visible*/);
    ac->appendGroup(Constants::G_TOUCHBAR_HELP);
    ac->appendGroup(Constants::G_TOUCHBAR_NAVIGATION);
    ac->appendGroup(Constants::G_TOUCHBAR_EDITOR);
    ac->appendGroup(Constants::G_TOUCHBAR_OTHER);
    ac->touchBar()->setApplicationTouchBar();
}

void ICorePrivate::registerDefaultActions()
{
    ActionContainer *mfile = ActionManager::actionContainer(Constants::M_FILE);
    ActionContainer *medit = ActionManager::actionContainer(Constants::M_EDIT);
    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);
    ActionContainer *mtools = ActionManager::actionContainer(Constants::M_TOOLS);
    ActionContainer *mwindow = ActionManager::actionContainer(Constants::M_WINDOW);
    ActionContainer *mhelp = ActionManager::actionContainer(Constants::M_HELP);

    // File menu separators
    mfile->addSeparator(Constants::G_FILE_SAVE);
    mfile->addSeparator(Constants::G_FILE_EXPORT);
    mfile->addSeparator(Constants::G_FILE_PRINT);
    mfile->addSeparator(Constants::G_FILE_CLOSE);
    mfile->addSeparator(Constants::G_FILE_OTHER);
    // Edit menu separators
    medit->addSeparator(Constants::G_EDIT_COPYPASTE);
    medit->addSeparator(Constants::G_EDIT_SELECTALL);
    medit->addSeparator(Constants::G_EDIT_FIND);
    medit->addSeparator(Constants::G_EDIT_ADVANCED);

    // Return to editor shortcut: Note this requires Qt to fix up
    // handling of shortcut overrides in menus, item views, combos....
    ActionBuilder focusToEditor(this, Constants::S_RETURNTOEDITOR);
    focusToEditor.setText(Tr::tr("Return to Editor"));
    focusToEditor.setDefaultKeySequence(QKeySequence(Qt::Key_Escape));
    focusToEditor.addOnTriggered(this, [] { setFocusToEditor(); });

    // New Project Action
    ActionBuilder newProjectAction(this, Constants::NEW);
    newProjectAction.setText(Tr::tr("&New Project..."));
    newProjectAction.setIcon(Icon::fromTheme("document-new"));
    newProjectAction.setDefaultKeySequence(QKeySequence("Ctrl+Shift+N"));
    newProjectAction.addToContainer(Constants::M_FILE, Constants::G_FILE_NEW);
    newProjectAction.addOnTriggered(this, [] {
        if (!ICore::isNewItemDialogRunning()) {
            ICore::showNewItemDialog(
                Tr::tr("New Project", "Title of dialog"),
                Utils::filtered(Core::IWizardFactory::allWizardFactories(),
                                Utils::equal(&Core::IWizardFactory::kind,
                                             Core::IWizardFactory::ProjectWizard)),
                FilePath());
        } else {
            ICore::raiseWindow(ICore::newItemDialog());
        }
    });

    // New File Action
    ActionBuilder newFileAction(this, Constants::NEW_FILE);
    newFileAction.setText(Tr::tr("New File..."));
    newFileAction.setIcon(Icon::fromTheme("document-new"));
    newFileAction.setDefaultKeySequence(QKeySequence::New);
    newFileAction.addToContainer(Constants::M_FILE, Constants::G_FILE_NEW);
    newFileAction.addOnTriggered(this, [] {
        if (!ICore::isNewItemDialogRunning()) {
            ICore::showNewItemDialog(
                Tr::tr("New File", "Title of dialog"),
                Utils::filtered(Core::IWizardFactory::allWizardFactories(),
                                Utils::equal(&Core::IWizardFactory::kind,
                                             Core::IWizardFactory::FileWizard)),
                FilePath());
        } else {
            ICore::raiseWindow(ICore::newItemDialog());
        }
    });

    // Open Action
    ActionBuilder openAction(this, Constants::OPEN);
    openAction.setText(Tr::tr("&Open File or Project..."));
    openAction.setIcon(Icon::fromTheme("document-open"));
    openAction.setDefaultKeySequence(QKeySequence::Open);
    openAction.addToContainer(Constants::M_FILE, Constants::G_FILE_OPEN);
    openAction.addOnTriggered(this, [] { openFile(); });

    // Open With Action
    ActionBuilder openWithAction(this, Constants::OPEN_WITH);
    openWithAction.setText(Tr::tr("Open File &With..."));
    openWithAction.addToContainer(Constants::M_FILE, Constants::G_FILE_OPEN);
    openWithAction.addOnTriggered(this, &ICore::openFileWith);

    if (FSEngine::isAvailable()) {
        // Open From Device Action
        ActionBuilder openFromDeviceAction(this, Constants::OPEN_FROM_DEVICE);
        openFromDeviceAction.setText(Tr::tr("Open From Device..."));
        openFromDeviceAction.addToContainer(Constants::M_FILE, Constants::G_FILE_OPEN);
        openFromDeviceAction.addOnTriggered(this, [this] { openFileFromDevice(); });
    }

    // File->Recent Files Menu
    ActionContainer *ac = ActionManager::createMenu(Constants::M_FILE_RECENTFILES);
    mfile->addMenu(ac, Constants::G_FILE_OPEN);
    ac->menu()->setTitle(Tr::tr("Recent &Files"));
    ac->setOnAllDisabledBehavior(ActionContainer::Show);

    // Save Action
    ActionBuilder saveAction(this, Constants::SAVE);
    saveAction.setText(Tr::tr("&Save"));
    saveAction.setIcon(Icon::fromTheme("document-save"));
    saveAction.setEnabled(false);
    saveAction.setDefaultKeySequence(QKeySequence::Save);
    saveAction.setCommandAttribute(Command::CA_UpdateText);
    saveAction.setCommandDescription(Tr::tr("Save"));
    saveAction.addToContainer(Constants::M_FILE, Constants::G_FILE_SAVE);

    // Save As Action
    ActionBuilder saveAsAction(this, Constants::SAVEAS);
    saveAsAction.setText(Tr::tr("Save &As..."));
    saveAsAction.setIcon(Icon::fromTheme("document-save-as"));
    saveAsAction.setEnabled(false);
    saveAsAction.setDefaultKeySequence(Tr::tr("Ctrl+Shift+S"), QString());
    saveAsAction.setCommandAttribute(Command::CA_UpdateText);
    saveAsAction.setCommandDescription(Tr::tr("Save As..."));
    saveAsAction.addToContainer(Constants::M_FILE, Constants::G_FILE_SAVE);

    // SaveAll Action
    DocumentManager::registerSaveAllAction();

    // Print Action
    ActionBuilder printAction(this, Constants::PRINT);
    printAction.setText(Tr::tr("&Print..."));
    printAction.setIcon(Icon::fromTheme("document-print"));
    printAction.setEnabled(false);
    printAction.setDefaultKeySequence(QKeySequence::Print);
    printAction.addToContainer(Constants::M_FILE, Constants::G_FILE_PRINT);

    // Exit Action
    ActionBuilder exitAction(this, Constants::EXIT);
    exitAction.setText(Tr::tr("E&xit"));
    exitAction.setIcon(Icon::fromTheme("application-exit"));
    exitAction.setMenuRole(QAction::QuitRole);
    exitAction.setDefaultKeySequence(Tr::tr("Ctrl+Q"));
    exitAction.addToContainer(Constants::M_FILE, Constants::G_FILE_OTHER);
    exitAction.addOnTriggered(this, &ICore::exit);

    // Undo Action
    ActionBuilder undoAction(this, Constants::UNDO);
    undoAction.setText(Tr::tr("&Undo"));
    undoAction.setIcon(Icon::fromTheme("edit-undo"));
    undoAction.setDefaultKeySequence(QKeySequence::Undo);
    undoAction.setCommandAttribute(Command::CA_UpdateText);
    undoAction.setCommandDescription(Tr::tr("Undo"));
    undoAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_UNDOREDO);
    undoAction.setEnabled(false);

    // Redo Action
    ActionBuilder redoAction(this, Constants::REDO);
    redoAction.setIcon(Icon::fromTheme("edit-redo"));
    redoAction.setText(Tr::tr("&Redo"));
    redoAction.setDefaultKeySequence(QKeySequence::Redo);
    redoAction.setCommandAttribute(Command::CA_UpdateText);
    redoAction.setCommandDescription(Tr::tr("Redo"));
    redoAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_UNDOREDO);
    redoAction.setEnabled(false);

    // Cut Action
    ActionBuilder cutAction(this, Constants::CUT);
    cutAction.setText(Tr::tr("Cu&t"));
    cutAction.setIcon(Icon::fromTheme("edit-cut"));
    cutAction.setDefaultKeySequence(QKeySequence::Cut);
    cutAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_COPYPASTE);
    cutAction.setEnabled(false);

    // Copy Action
    ActionBuilder copyAction(this, Constants::COPY);
    copyAction.setText(Tr::tr("&Copy"));
    copyAction.setIcon(Icon::fromTheme("edit-copy"));
    copyAction.setDefaultKeySequence(QKeySequence::Copy);
    copyAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_COPYPASTE);
    copyAction.setEnabled(false);

    // Paste Action
    ActionBuilder pasteAction(this, Constants::PASTE);
    pasteAction.setText(Tr::tr("&Paste"));
    pasteAction.setIcon(Icon::fromTheme("edit-paste"));
    pasteAction.setDefaultKeySequence(QKeySequence::Paste);
    pasteAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_COPYPASTE);
    pasteAction.setEnabled(false);

    // Select All
    ActionBuilder selectAllAction(this, Constants::SELECTALL);
    selectAllAction.setText(Tr::tr("Select &All"));
    selectAllAction.setIcon(Icon::fromTheme("edit-select-all"));
    selectAllAction.setDefaultKeySequence(QKeySequence::SelectAll);
    selectAllAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_SELECTALL);
    selectAllAction.setEnabled(false);

    // Goto Action
    ActionBuilder gotoLineAction(this, Constants::GOTO);
    gotoLineAction.setText(Tr::tr("&Go to Line..."));
    gotoLineAction.setIcon(Icon::fromTheme("go-jump"));
    gotoLineAction.setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+L")));
    gotoLineAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_OTHER);
    gotoLineAction.setEnabled(false);

    // Zoom In Action
    ActionBuilder zoomInAction(this, Constants::ZOOM_IN);
    zoomInAction.setText(Tr::tr("Zoom In"));
    zoomInAction.setIcon(Icon::fromTheme("zoom-in"));
    zoomInAction.setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl++")));
    zoomInAction.setEnabled(false);

    // Zoom Out Action
    ActionBuilder zoomOutAction(this, Constants::ZOOM_OUT);
    zoomOutAction.setText(Tr::tr("Zoom Out"));
    zoomOutAction.setIcon(Icon::fromTheme("zoom-out"));
    if (useMacShortcuts)
        zoomOutAction.setDefaultKeySequences({QKeySequence(Tr::tr("Ctrl+-")), QKeySequence(Tr::tr("Ctrl+Shift+-"))});
    else
        zoomOutAction.setDefaultKeySequence(Tr::tr("Ctrl+-"));
    zoomOutAction.setEnabled(false);

    // Zoom Reset Action
    ActionBuilder zoomOriginalAction(this, Constants::ZOOM_RESET);
    zoomOriginalAction.setText(Tr::tr("Original Size"));
    zoomOriginalAction.setIcon(Icon::fromTheme("zoom-original"));
    zoomOriginalAction.setDefaultKeySequence(Tr::tr("Meta+0"), Tr::tr("Ctrl+0"));
    zoomOriginalAction.setEnabled(false);

    // Debug Qt Creator menu
    mtools->appendGroup(Constants::G_TOOLS_DEBUG);
    ActionContainer *mtoolsdebug = ActionManager::createMenu(Constants::M_TOOLS_DEBUG);
    mtoolsdebug->menu()->setTitle(Tr::tr("Debug %1").arg(QGuiApplication::applicationDisplayName()));
    mtools->addMenu(mtoolsdebug, Constants::G_TOOLS_DEBUG);

    ActionBuilder loggerAction(this, Constants::LOGGER);
    loggerAction.setText(Tr::tr("Show Logs..."));
    loggerAction.addToContainer(Constants::M_TOOLS_DEBUG);
    loggerAction.addOnTriggered(this, &LoggingViewer::showLoggingView);

    // Options Action
    medit->appendGroup(Constants::G_EDIT_PREFERENCES);
    medit->addSeparator(Constants::G_EDIT_PREFERENCES);

    ActionBuilder optionsAction(this, Constants::OPTIONS);
    optionsAction.setText(Tr::tr("Pr&eferences..."));
    optionsAction.setMenuRole(QAction::PreferencesRole);
    optionsAction.setDefaultKeySequence(QKeySequence::Preferences);
    optionsAction.addToContainer(Constants::M_EDIT, Constants::G_EDIT_PREFERENCES);
    optionsAction.addOnTriggered(this, [] { ICore::showOptionsDialog(Id()); });

    mwindow->addSeparator(Constants::G_WINDOW_LIST);

    if (useMacShortcuts) {
        // Minimize Action
        ActionBuilder minimizeAction(this, Constants::MINIMIZE_WINDOW);
        minimizeAction.setText(Tr::tr("Minimize"));
        minimizeAction.setEnabled(false); // actual implementation in WindowSupport
        minimizeAction.setDefaultKeySequence(QKeySequence(Tr::tr("Ctrl+M")));
        minimizeAction.addToContainer(Constants::M_WINDOW, Constants::G_WINDOW_SIZE);

        // Zoom Action
        ActionBuilder zoomAction(this, Constants::ZOOM_WINDOW);
        zoomAction.setText(Tr::tr("Zoom"));
        zoomAction.setEnabled(false); // actual implementation in WindowSupport
        zoomAction.addToContainer(Constants::M_WINDOW, Constants::G_WINDOW_SIZE);
    }

    // Full Screen Action
    ActionBuilder toggleFullScreenAction(this, Constants::TOGGLE_FULLSCREEN);
    toggleFullScreenAction.setText(Tr::tr("Full Screen"));
    toggleFullScreenAction.setDefaultKeySequence(Tr::tr("Ctrl+Meta+F"), Tr::tr("Ctrl+Shift+F11"));
    if (HostOsInfo::isMacHost())
        toggleFullScreenAction.setCommandAttribute(Command::CA_UpdateText);
    toggleFullScreenAction.addToContainer(Constants::M_WINDOW, Constants::G_WINDOW_SIZE);

    if (useMacShortcuts) {
        mwindow->addSeparator(Constants::G_WINDOW_SIZE);

        ActionBuilder closeAction(this, Constants::CLOSE_WINDOW);
        closeAction.setText(Tr::tr("Close Window"));
        closeAction.setEnabled(false);
        closeAction.setDefaultKeySequence(Tr::tr("Ctrl+Meta+W"));
        closeAction.addToContainer(Constants::M_WINDOW, Constants::G_WINDOW_SIZE);

        mwindow->addSeparator(Constants::G_WINDOW_SIZE);
    }

    // Show Left Sidebar Action
    ActionBuilder toggleLeftSideBarAction(this, Constants::TOGGLE_LEFT_SIDEBAR);
    toggleLeftSideBarAction.setIcon(Utils::Icons::TOGGLE_LEFT_SIDEBAR.icon());
    toggleLeftSideBarAction.setText(Tr::tr(Constants::TR_SHOW_LEFT_SIDEBAR));
    toggleLeftSideBarAction.setCheckable(true);
    toggleLeftSideBarAction.setCommandAttribute(Command::CA_UpdateText);
    toggleLeftSideBarAction.setDefaultKeySequence(Tr::tr("Ctrl+0"), Tr::tr("Alt+0"));
    toggleLeftSideBarAction.addToContainer(Constants::M_VIEW, Constants::G_VIEW_VIEWS);
    toggleLeftSideBarAction.addOnTriggered(this,
        [this](bool visible) { setSidebarVisible(visible, Side::Left); });

    m_toggleLeftSideBarAction = toggleLeftSideBarAction.contextAction();
    m_toggleLeftSideBarButton->setDefaultAction(ProxyAction::proxyActionWithIcon(
        toggleLeftSideBarAction.commandAction(), Utils::Icons::TOGGLE_LEFT_SIDEBAR_TOOLBAR.icon()));
    m_toggleLeftSideBarButton->setEnabled(false);

    // Show Right Sidebar Action
    ActionBuilder toggleRightSideBarAction(this, Constants::TOGGLE_RIGHT_SIDEBAR);
    toggleRightSideBarAction.setIcon(Utils::Icons::TOGGLE_RIGHT_SIDEBAR.icon());
    toggleRightSideBarAction.setText(Tr::tr(Constants::TR_SHOW_RIGHT_SIDEBAR));
    toggleRightSideBarAction.setCheckable(true);
    toggleRightSideBarAction.setCommandAttribute(Command::CA_UpdateText);
    toggleRightSideBarAction.setDefaultKeySequence(Tr::tr("Ctrl+Shift+0"), Tr::tr("Alt+Shift+0"));
    toggleRightSideBarAction.addToContainer(Constants::M_VIEW, Constants::G_VIEW_VIEWS);
    toggleRightSideBarAction.setEnabled(false);
    toggleRightSideBarAction.addOnTriggered(this,
        [this](bool visible) { setSidebarVisible(visible, Side::Right); });

    m_toggleRightSideBarAction = toggleRightSideBarAction.contextAction();
    m_toggleRightSideBarButton->setDefaultAction(ProxyAction::proxyActionWithIcon(
        toggleRightSideBarAction.commandAction(), Utils::Icons::TOGGLE_RIGHT_SIDEBAR_TOOLBAR.icon()));

    // Show Menubar Action
    if (globalMenuBar() && !globalMenuBar()->isNativeMenuBar()) {
        ActionBuilder toggleMenubarAction(this, Constants::TOGGLE_MENUBAR);
        toggleMenubarAction.setText(Tr::tr("Show Menu Bar"));
        toggleMenubarAction.bindContextAction(&m_toggleMenubarAction);
        toggleMenubarAction.setCheckable(true);
        toggleMenubarAction.setDefaultKeySequence(Tr::tr("Ctrl+Alt+M"));
        toggleMenubarAction.addToContainer(Constants::M_VIEW, Constants::G_VIEW_VIEWS);
        toggleMenubarAction.addOnToggled(this, [](bool visible) {
            if (!visible) {
                auto keySequenceAndText = [](const Utils::Id &actionName) {
                    const auto command = ActionManager::command(actionName);

                    const QString keySequence = command->keySequence().toString(
                        QKeySequence::NativeText);
                    const QString text = command->action()->text();

                    return QPair<QString, QString>(keySequence, text);
                };

                auto [menuBarKeys, menuBarText] = keySequenceAndText(Constants::TOGGLE_MENUBAR);
                auto [actionsFromMenuKeys, actionsFromMenuText] = keySequenceAndText(
                    "Locator.Actions from the menu");

                CheckableMessageBox::information(
                    Core::ICore::dialogParent(),
                    Tr::tr("Hide Menu Bar"),
                    Tr::tr("This will hide the menu bar completely. "
                           "You can show it again by typing %1."
                           "<br><br>"
                           "Or, trigger the \"%2\" action from the \"%3\" locator filter (%4).")
                        .arg(menuBarKeys)
                        .arg(menuBarText)
                        .arg(actionsFromMenuText)
                        .arg(actionsFromMenuKeys),
                    Key("ToogleMenuBarHint"));
            }
            globalMenuBar()->setVisible(visible);
        });
    }

    registerModeSelectorStyleActions();

    // Window->Views
    ActionContainer *mviews = ActionManager::createMenu(Constants::M_VIEW_VIEWS);
    mview->addMenu(mviews, Constants::G_VIEW_VIEWS);
    mviews->menu()->setTitle(Tr::tr("&Views"));

    // "Help" separators
    mhelp->addSeparator(Constants::G_HELP_SUPPORT);
    if (!HostOsInfo::isMacHost())
        mhelp->addSeparator(Constants::G_HELP_ABOUT);

    // About IDE Action
    ActionBuilder aboutIdeAction(this, Constants::ABOUT_QTCREATOR);
    aboutIdeAction.setIcon(Icon::fromTheme("help-about"));
    aboutIdeAction.setText(
      (HostOsInfo::isMacHost() ? Tr::tr("About &%1") : Tr::tr("About &%1..."))
          .arg(QGuiApplication::applicationDisplayName()));
    aboutIdeAction.setMenuRole(QAction::AboutRole);
    aboutIdeAction.addToContainer(Constants::M_HELP, Constants::G_HELP_ABOUT);
    aboutIdeAction.setEnabled(true);
    aboutIdeAction.addOnTriggered(this, [this] { aboutQtCreator(); });

    // About Plugins Action
    ActionBuilder aboutPluginsAction(this, Constants::ABOUT_PLUGINS);
    aboutPluginsAction.setText(Tr::tr("About &Plugins..."));
    aboutPluginsAction.setMenuRole(QAction::ApplicationSpecificRole);
    aboutPluginsAction.addToContainer(Constants::M_HELP, Constants::G_HELP_ABOUT);
    aboutPluginsAction.setEnabled(true);
    aboutPluginsAction.addOnTriggered(this, [this] { aboutPlugins(); });

    // Change Log Action
    ActionBuilder changeLogAction(this, Constants::CHANGE_LOG);
    changeLogAction.setText(Tr::tr("Change Log..."));
    changeLogAction.setMenuRole(QAction::ApplicationSpecificRole);
    changeLogAction.addToContainer(Constants::M_HELP, Constants::G_HELP_ABOUT);
    changeLogAction.setEnabled(true);
    changeLogAction.addOnTriggered(this, [this] { changeLog(); });

    // Contact
    ActionBuilder contactAction(this, "QtCreator.Contact");
    contactAction.setText(Tr::tr("Contact..."));
    contactAction.addToContainer(Constants::M_HELP, Constants::G_HELP_ABOUT);
    contactAction.setEnabled(true);
    contactAction.addOnTriggered(this, [this] { contact(); });

    // About sep
    if (!HostOsInfo::isMacHost()) { // doesn't have the "About" actions in the Help menu
        ActionBuilder tmpAction(this, "QtCreator.Help.Sep.About");
        tmpAction.setSeperator(true);
        tmpAction.addToContainer(Constants::M_HELP, Constants::G_HELP_ABOUT);
    }
}

void ICorePrivate::registerModeSelectorStyleActions()
{
    ActionContainer *mview = ActionManager::actionContainer(Constants::M_VIEW);

    // Cycle Mode Selector Styles
    ActionBuilder(this, Constants::CYCLE_MODE_SELECTOR_STYLE)
        .setText(Tr::tr("Cycle Mode Selector Styles"))
        .addOnTriggered(this, [this] {
            ModeManager::cycleModeStyle();
            updateModeSelectorStyleMenu();
        });

    // Mode Selector Styles
    ActionContainer *mmodeLayouts = ActionManager::createMenu(Constants::M_VIEW_MODESTYLES);
    mview->addMenu(mmodeLayouts, Constants::G_VIEW_VIEWS);
    QMenu *styleMenu = mmodeLayouts->menu();
    styleMenu->setTitle(Tr::tr("Mode Selector Style"));
    auto *stylesGroup = new QActionGroup(styleMenu);
    stylesGroup->setExclusive(true);

    m_setModeSelectorStyleIconsAndTextAction = stylesGroup->addAction(Tr::tr("Icons and Text"));
    connect(m_setModeSelectorStyleIconsAndTextAction, &QAction::triggered,
                                 [] { ModeManager::setModeStyle(ModeManager::Style::IconsAndText); });
    m_setModeSelectorStyleIconsAndTextAction->setCheckable(true);
    m_setModeSelectorStyleIconsOnlyAction = stylesGroup->addAction(Tr::tr("Icons Only"));
    connect(m_setModeSelectorStyleIconsOnlyAction, &QAction::triggered,
                                 [] { ModeManager::setModeStyle(ModeManager::Style::IconsOnly); });
    m_setModeSelectorStyleIconsOnlyAction->setCheckable(true);
    m_setModeSelectorStyleHiddenAction = stylesGroup->addAction(Tr::tr("Hidden"));
    connect(m_setModeSelectorStyleHiddenAction, &QAction::triggered,
                                 [] { ModeManager::setModeStyle(ModeManager::Style::Hidden); });
    m_setModeSelectorStyleHiddenAction->setCheckable(true);

    styleMenu->addActions(stylesGroup->actions());
}

void ICorePrivate::openFile()
{
    ICore::openFiles(EditorManager::getOpenFilePaths(), ICore::SwitchMode);
}

static IDocumentFactory *findDocumentFactory(const QList<IDocumentFactory*> &fileFactories,
                                             const FilePath &filePath)
{
    const QString typeName = Utils::mimeTypeForFile(filePath, MimeMatchMode::MatchDefaultAndRemote)
                                 .name();
    return Utils::findOrDefault(fileFactories, [typeName](IDocumentFactory *f) {
        return f->mimeTypes().contains(typeName);
    });
}

} // Internal

/*!
 * \internal
 * Either opens \a filePaths with editors or loads a project.
 *
 *  \a flags can be used to stop on first failure, indicate that a file name
 *  might include line numbers and/or switch mode to edit mode.
 *
 *  \a workingDirectory is used when files are opened by a remote client, since
 *  the file names are relative to the client working directory.
 *
 *  Returns the first opened document. Required to support the \c -block flag
 *  for client mode.
 *
 *  \sa IPlugin::remoteArguments()
 */

IDocument *ICore::openFiles(const FilePaths &filePaths,
                            ICore::OpenFilesFlags flags,
                            const FilePath &workingDirectory)
{
    const QList<IDocumentFactory*> documentFactories = IDocumentFactory::allDocumentFactories();
    IDocument *res = nullptr;

    const FilePath workingDirBase =
            workingDirectory.isEmpty() ? FilePath::currentWorkingPath() : workingDirectory;
    for (const FilePath &filePath : filePaths) {
        const FilePath absoluteFilePath = workingDirBase.resolvePath(filePath);
        if (IDocumentFactory *documentFactory = findDocumentFactory(documentFactories, filePath)) {
            IDocument *document = documentFactory->open(absoluteFilePath);
            if (!document) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else {
                if (!res)
                    res = document;
                if (flags & ICore::SwitchMode)
                    ModeManager::activateMode(Id(Constants::MODE_EDIT));
            }
        } else if (flags & (ICore::SwitchSplitIfAlreadyVisible | ICore::CanContainLineAndColumnNumbers)
                   || !res) {
            QFlags<EditorManager::OpenEditorFlag> emFlags;
            if (flags & ICore::SwitchSplitIfAlreadyVisible)
                emFlags |= EditorManager::SwitchSplitIfAlreadyVisible;
            IEditor *editor = nullptr;
            if (flags & ICore::CanContainLineAndColumnNumbers) {
                const Link &link = Link::fromString(absoluteFilePath.toString(), true);
                editor = EditorManager::openEditorAt(link, {}, emFlags);
            } else {
                editor = EditorManager::openEditor(absoluteFilePath, {}, emFlags);
            }
            if (!editor) {
                if (flags & ICore::StopOnLoadFail)
                    return res;
            } else if (!res) {
                res = editor->document();
            }
        } else {
            auto factory = IEditorFactory::preferredEditorFactories(absoluteFilePath).value(0);
            DocumentModelPrivate::addSuspendedDocument(absoluteFilePath, {},
                                                       factory ? factory->id() : Id());
        }
    }
    return res;
}

namespace Internal {

void ICorePrivate::setFocusToEditor()
{
    EditorManagerPrivate::doEscapeKeyFocusMoveMagic();
}

void ICorePrivate::openFileFromDevice()
{
    ICore::openFiles(EditorManager::getOpenFilePaths(QFileDialog::DontUseNativeDialog), ICore::SwitchMode);
}

static void acceptModalDialogs()
{
    const QWidgetList topLevels = QApplication::topLevelWidgets();
    QList<QPointer<QDialog>> dialogsToClose;
    for (QWidget *topLevel : topLevels) {
        if (auto dialog = qobject_cast<QDialog *>(topLevel)) {
            if (dialog->isModal())
                dialogsToClose.append(dialog);
        }
    }
    for (QDialog *dialog : dialogsToClose) {
        if (dialog)
            dialog->accept();
    }
}

} // Internal

void ICore::exit()
{
    // this function is most likely called from a user action
    // that is from an event handler of an object
    // since on close we are going to delete everything
    // so to prevent the deleting of that object we
    // just append it
    QMetaObject::invokeMethod(
        d->m_mainwindow,
        [] {
            // Modal dialogs block the close event. So close them, in case this was triggered from
            // a RestartDialog in the settings dialog.
            acceptModalDialogs();
            d->m_mainwindow->close();
        },
        Qt::QueuedConnection);
}

void ICore::openFileWith()
{
    const FilePaths filePaths = EditorManager::getOpenFilePaths();
    for (const FilePath &filePath : filePaths) {
        bool isExternal;
        const Id editorId = EditorManagerPrivate::getOpenWithEditorId(filePath, &isExternal);
        if (!editorId.isValid())
            continue;
        if (isExternal)
            EditorManager::openExternalEditor(filePath, editorId);
        else
            EditorManagerPrivate::openEditorWith(filePath, editorId);
    }
}

/*!
    Returns all registered IContext instance for the specified \a widget,
    if any.
*/
QList<IContext *> ICore::contextObjects(QWidget *widget)
{
    if (auto it = d->m_contextWidgets.find(widget); it != d->m_contextWidgets.end())
        return it->second;
    return {};
}

/*!
    Adds \a context to the list of registered IContext instances.
    Whenever the IContext's \l{IContext::widget()}{widget} is in the application
    focus widget's parent hierarchy, its \l{IContext::context()}{context} is
    added to the list of active contexts.

    \sa removeContextObject()
    \sa updateAdditionalContexts()
    \sa currentContextObject()
    \sa {The Action Manager and Commands}
*/

void ICore::addContextObject(IContext *context)
{
    if (!context)
        return;
    d->m_contextWidgets[context->widget()].append(context);
    connect(context, &QObject::destroyed, m_core, [context] { removeContextObject(context); });
}

/*!
    Unregisters a \a context object from the list of registered IContext
    instances. IContext instances are automatically removed when they are
    deleted.

    \sa addContextObject()
    \sa updateAdditionalContexts()
    \sa currentContextObject()
*/

void ICore::removeContextObject(IContext *context)
{
    if (!context)
        return;

    disconnect(context, &QObject::destroyed, m_core, nullptr);

    auto it = std::find_if(
        d->m_contextWidgets.begin(),
        d->m_contextWidgets.end(),
        [context](const std::pair<QWidget *, QList<IContext *>> &v) {
            return v.second.contains(context);
        });
    if (it == d->m_contextWidgets.end())
        return;

    it->second.removeAll(context);
    if (it->second.isEmpty())
        d->m_contextWidgets.erase(it);

    if (d->m_activeContext.removeAll(context) > 0)
        d->updateContextObject(d->m_activeContext);
}

namespace Internal {

void ICorePrivate::updateFocusWidget(QWidget *old, QWidget *now)
{
    Q_UNUSED(old)

    // Prevent changing the context object just because the menu or a menu item is activated
    if (qobject_cast<QMenuBar*>(now) || qobject_cast<QMenu*>(now))
        return;

    QList<IContext *> newContext;
    for (QWidget *p = QApplication::focusWidget(); p; p = p->parentWidget())
        newContext.append(ICore::contextObjects(p));

    // ignore toplevels that define no context, like popups without parent
    if (!newContext.isEmpty() || QApplication::focusWidget() == m_mainwindow->focusWidget())
        updateContextObject(newContext);
}

void ICorePrivate::updateContextObject(const QList<IContext *> &context)
{
    emit m_core->contextAboutToChange(context);
    m_activeContext = context;
    updateContext();
    if (debugMainWindow) {
        qDebug() << "new context objects =" << context;
        for (const IContext *c : context)
            qDebug() << (c ? c->widget() : nullptr) << (c ? c->widget()->metaObject()->className() : nullptr);
    }
}

void ICorePrivate::readSettings()
{
    QtcSettings *settings = PluginManager::settings();
    settings->beginGroup(settingsGroup);

    if (s_overrideColor.isValid()) {
        StyleHelper::setBaseColor(s_overrideColor);
        // Get adapted base color.
        s_overrideColor = StyleHelper::baseColor();
    } else {
        StyleHelper::setBaseColor(settings->value(colorKey,
                                  QColor(StyleHelper::DEFAULT_BASE_COLOR)).value<QColor>());
    }

    {
        ModeManager::Style modeStyle =
                ModeManager::Style(settings->value(modeSelectorLayoutKey, int(ModeManager::Style::IconsAndText)).toInt());

        // Migrate legacy setting from Qt Creator 4.6 and earlier
        static const char modeSelectorVisibleKey[] = "ModeSelectorVisible";
        if (!settings->contains(modeSelectorLayoutKey) && settings->contains(modeSelectorVisibleKey)) {
            bool visible = settings->value(modeSelectorVisibleKey, true).toBool();
            modeStyle = visible ? ModeManager::Style::IconsAndText : ModeManager::Style::Hidden;
        }

        ModeManager::setModeStyle(modeStyle);
        updateModeSelectorStyleMenu();
    }

    if (globalMenuBar() && !globalMenuBar()->isNativeMenuBar()) {
        const bool isVisible = settings->value(menubarVisibleKey, true).toBool();

        globalMenuBar()->setVisible(isVisible);
        if (m_toggleMenubarAction)
            m_toggleMenubarAction->setChecked(isVisible);
    }

    settings->endGroup();

    EditorManagerPrivate::readSettings();
    m_leftNavigationWidget->restoreSettings(settings);
    m_rightNavigationWidget->restoreSettings(settings);
    m_rightPaneWidget->readSettings(settings);
}

void ICorePrivate::saveWindowSettings()
{
    QtcSettings *settings = PluginManager::settings();
    settings->beginGroup(settingsGroup);

    // On OS X applications usually do not restore their full screen state.
    // To be able to restore the correct non-full screen geometry, we have to put
    // the window out of full screen before saving the geometry.
    // Works around QTBUG-45241
    if (Utils::HostOsInfo::isMacHost() && m_mainwindow->isFullScreen())
        m_mainwindow->setWindowState(m_mainwindow->windowState() & ~Qt::WindowFullScreen);
    settings->setValue(windowGeometryKey, m_mainwindow->saveGeometry());
    settings->setValue(windowStateKey, m_mainwindow->saveState());
    settings->setValue(modeSelectorLayoutKey, int(ModeManager::modeStyle()));

    settings->endGroup();
}

void ICorePrivate::updateModeSelectorStyleMenu()
{
    switch (ModeManager::modeStyle()) {
    case ModeManager::Style::IconsAndText:
        m_setModeSelectorStyleIconsAndTextAction->setChecked(true);
        break;
    case ModeManager::Style::IconsOnly:
        m_setModeSelectorStyleIconsOnlyAction->setChecked(true);
        break;
    case ModeManager::Style::Hidden:
        m_setModeSelectorStyleHiddenAction->setChecked(true);
        break;
    }
}

void ICorePrivate::updateContext()
{
    Context contexts = m_highPrioAdditionalContexts;

    for (IContext *context : std::as_const(m_activeContext))
        contexts.add(context->context());

    contexts.add(m_lowPrioAdditionalContexts);

    Context uniquecontexts;
    for (const Id &id : std::as_const(contexts)) {
        if (!uniquecontexts.contains(id))
            uniquecontexts.add(id);
    }

    ActionManager::setContext(uniquecontexts);
    emit m_core->contextChanged(uniquecontexts);
}

void ICorePrivate::aboutToShowRecentFiles()
{
    ActionContainer *aci = ActionManager::actionContainer(Constants::M_FILE_RECENTFILES);
    QMenu *menu = aci->menu();
    menu->clear();

    const QList<DocumentManager::RecentFile> recentFiles = DocumentManager::recentFiles();
    for (int i = 0; i < recentFiles.count(); ++i) {
        const DocumentManager::RecentFile file = recentFiles[i];

        const QString filePath = Utils::quoteAmpersands(file.first.shortNativePath());
        const QString actionText = ActionManager::withNumberAccelerator(filePath, i + 1);
        QAction *action = menu->addAction(actionText);
        connect(action, &QAction::triggered, this, [file] {
            EditorManager::openEditor(file.first, file.second);
        });
    }

    bool hasRecentFiles = !recentFiles.isEmpty();
    menu->setEnabled(hasRecentFiles);

    // add the Clear Menu item
    if (hasRecentFiles) {
        menu->addSeparator();
        QAction *action = menu->addAction(Tr::tr(Constants::TR_CLEAR_MENU));
        connect(action, &QAction::triggered,
                DocumentManager::instance(), &DocumentManager::clearRecentFiles);
    }
}

void ICorePrivate::aboutQtCreator()
{
    if (!m_versionDialog) {
        m_versionDialog = new VersionDialog(m_mainwindow);
        connect(m_versionDialog, &QDialog::finished,
                this, &ICorePrivate::destroyVersionDialog);
        ICore::registerWindow(m_versionDialog, Context("Core.VersionDialog"));
        m_versionDialog->show();
    } else {
        ICore::raiseWindow(m_versionDialog);
    }
}

void ICorePrivate::destroyVersionDialog()
{
    if (m_versionDialog) {
        m_versionDialog->deleteLater();
        m_versionDialog = nullptr;
    }
}

void ICorePrivate::aboutPlugins()
{
    PluginDialog dialog(m_mainwindow);
    dialog.exec();
}

class LogDialog : public QDialog
{
public:
    LogDialog(QWidget *parent)
        : QDialog(parent)
    {}
    bool event(QEvent *event) override
    {
        if (event->type() == QEvent::ShortcutOverride) {
            auto ke = static_cast<QKeyEvent *>(event);
            if (ke->key() == Qt::Key_Escape && !ke->modifiers()) {
                ke->accept();
                return true;
            }
        }
        return QDialog::event(event);
    }
};

void ICorePrivate::changeLog()
{
    static QPointer<LogDialog> dialog;
    if (dialog) {
        ICore::raiseWindow(dialog);
        return;
    }
    const FilePaths files =
            ICore::resourcePath("changelog").dirEntries({{"changes-*"}, QDir::Files});
    static const QRegularExpression versionRegex("\\d+[.]\\d+[.]\\d+");
    using VersionFilePair = std::pair<QVersionNumber, FilePath>;
    QList<VersionFilePair> versionedFiles = Utils::transform(files, [](const FilePath &fp) {
        const QRegularExpressionMatch match = versionRegex.match(fp.fileName());
        const QVersionNumber version = match.hasMatch()
                                           ? QVersionNumber::fromString(match.captured())
                                           : QVersionNumber();
        return std::make_pair(version, fp);
    });
    Utils::sort(versionedFiles, [](const VersionFilePair &a, const VersionFilePair &b) {
        return a.first > b.first;
    });

    auto versionCombo = new QComboBox;
    versionCombo->setMinimumWidth(80);
    for (const VersionFilePair &f : versionedFiles)
        versionCombo->addItem(f.first.toString());
    dialog = new LogDialog(ICore::dialogParent());
    auto versionLayout = new QHBoxLayout;
    versionLayout->addWidget(new QLabel(Tr::tr("Version:")));
    versionLayout->addWidget(versionCombo);
    versionLayout->addStretch(1);
    auto showInExplorer = new QPushButton(FileUtils::msgGraphicalShellAction());
    versionLayout->addWidget(showInExplorer);
    auto textEdit = new QTextBrowser;
    textEdit->setOpenExternalLinks(true);

    auto aggregate = new Aggregation::Aggregate;
    aggregate->add(textEdit);
    aggregate->add(new Core::BaseTextFind(textEdit));

    new MarkdownHighlighter(textEdit->document());

    auto textEditWidget = new QFrame;
    textEditWidget->setFrameStyle(QFrame::NoFrame);
    auto findToolBar = new FindToolBarPlaceHolder(dialog);
    findToolBar->setLightColored(true);
    auto textEditLayout = new QVBoxLayout;
    textEditLayout->setContentsMargins(0, 0, 0, 0);
    textEditLayout->setSpacing(0);
    textEditLayout->addWidget(textEdit);
    textEditLayout->addWidget(findToolBar);
    textEditWidget->setLayout(textEditLayout);
    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Close);
    auto dialogLayout = new QVBoxLayout;
    dialogLayout->addLayout(versionLayout);
    dialogLayout->addWidget(textEditWidget);
    dialogLayout->addWidget(buttonBox);
    dialog->setLayout(dialogLayout);
    dialog->resize(700, 600);
    dialog->setWindowTitle(Tr::tr("Change Log"));
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    ICore::registerWindow(dialog, Context("CorePlugin.VersionDialog"));

    connect(buttonBox, &QDialogButtonBox::rejected, dialog, &QDialog::close);
    QPushButton *closeButton = buttonBox->button(QDialogButtonBox::Close);
    if (QTC_GUARD(closeButton))
        closeButton->setDefault(true); // grab from "Open in Explorer" button

    const auto showLog = [textEdit, versionedFiles](int index) {
        if (index < 0 || index >= versionedFiles.size())
            return;
        const FilePath file = versionedFiles.at(index).second;
        QString contents = QString::fromUtf8(file.fileContents().value_or(QByteArray()));
        // (?<![[\/]) == don't replace if it is preceded by "[" or "/"
        // i.e. if it already is part of a link
        static const QRegularExpression bugexpr(R"((?<![[\/])((QT(CREATOR)?BUG|PYSIDE)-\d+))");
        contents.replace(bugexpr, R"([\1](https://bugreports.qt.io/browse/\1))");
        static const QRegularExpression docexpr("https://doc[.]qt[.]io/qtcreator/([.a-zA-Z/_-]*)");
        QList<QRegularExpressionMatch> matches;
        for (const QRegularExpressionMatch &m : docexpr.globalMatch(contents))
            matches.append(m);
        Utils::reverseForeach(matches, [&contents](const QRegularExpressionMatch &match) {
            const QString qthelpUrl = "qthelp://org.qt-project.qtcreator/doc/" + match.captured(1);
            if (!HelpManager::fileData(qthelpUrl).isEmpty())
                contents.replace(match.capturedStart(), match.capturedLength(), qthelpUrl);
        });
        textEdit->setMarkdown(contents);
    };
    connect(versionCombo, &QComboBox::currentIndexChanged, textEdit, showLog);
    showLog(versionCombo->currentIndex());

    connect(showInExplorer, &QPushButton::clicked, this, [versionCombo, versionedFiles] {
        const int index = versionCombo->currentIndex();
        if (index >= 0 && index < versionedFiles.size())
            FileUtils::showInGraphicalShell(ICore::dialogParent(), versionedFiles.at(index).second);
        else
            FileUtils::showInGraphicalShell(ICore::dialogParent(), ICore::resourcePath("changelog"));
    });

    dialog->show();
}

void ICorePrivate::contact()
{
    QMessageBox dlg(QMessageBox::Information, Tr::tr("Contact"),
           Tr::tr("<p>Qt Creator developers can be reached at the Qt Creator mailing list:</p>"
              "%1"
              "<p>or the #qt-creator channel on Libera.Chat IRC:</p>"
              "%2"
              "<p>Our bug tracker is located at %3.</p>"
              "<p>Please use %4 for bigger chunks of text.</p>")
                    .arg("<p>&nbsp;&nbsp;&nbsp;&nbsp;"
                            "<a href=\"https://lists.qt-project.org/listinfo/qt-creator\">"
                            "mailto:qt-creator@qt-project.org"
                         "</a></p>")
                    .arg("<p>&nbsp;&nbsp;&nbsp;&nbsp;"
                            "<a href=\"https://web.libera.chat/#qt-creator\">"
                            "https://web.libera.chat/#qt-creator"
                         "</a></p>")
                    .arg("<a href=\"https://bugreports.qt.io/projects/QTCREATORBUG\">"
                            "https://bugreports.qt.io"
                         "</a>")
                    .arg("<a href=\"https://pastebin.com\">"
                            "https://pastebin.com"
                         "</a>"),
           QMessageBox::Ok, m_mainwindow);
    dlg.exec();
}

void ICorePrivate::restoreWindowState()
{
    NANOTRACE_SCOPE("Core", "MainWindow::restoreWindowState");
    QtcSettings *settings = PluginManager::settings();
    settings->beginGroup(settingsGroup);
    if (!m_mainwindow->restoreGeometry(settings->value(windowGeometryKey).toByteArray()))
        m_mainwindow->resize(1260, 700); // size without window decoration
    m_mainwindow->restoreState(settings->value(windowStateKey).toByteArray());
    settings->endGroup();
    m_mainwindow->show();
    StatusBarManager::restoreSettings();
}

} // Internal

void ICore::setOverrideColor(const QColor &color)
{
    s_overrideColor = color;
}

} // namespace Core
