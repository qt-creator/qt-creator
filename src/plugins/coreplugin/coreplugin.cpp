// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "coreplugin.h"
#include "coreplugintr.h"
#include "designmode.h"
#include "editmode.h"
#include "foldernavigationwidget.h"
#include "icore.h"
#include "idocument.h"
#include "iwizardfactory.h"
#include "loggingviewer.h"
#include "modemanager.h"
#include "session.h"
#include "settingsdatabase.h"
#include "themechooser.h"

#include "actionmanager/actionmanager.h"
#include "coreconstants.h"
#include "documentmanager.h"
#include "fileutils.h"
#include "find/findplugin.h"
#include "locator/locator.h"

#include <extensionsystem/pluginerroroverview.h>
#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>

#include <utils/algorithm.h>
#include <utils/checkablemessagebox.h>
#include <utils/commandline.h>
#include <utils/infobar.h>
#include <utils/macroexpander.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>
#include <utils/savefile.h>
#include <utils/store.h>
#include <utils/stringutils.h>
#include <utils/textutils.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QDateTime>
#include <QDebug>
#include <QDir>
#include <QGuiApplication>
#include <QJsonObject>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QUuid>

#include <cstdlib>

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

static CorePlugin *m_instance = nullptr;

const char kWarnCrashReportingSetting[] = "WarnCrashReporting";
const char kEnvironmentChanges[] = "Core/EnvironmentChanges";

void CorePlugin::setupSystemEnvironment()
{
    m_instance->m_startupSystemEnvironment = Environment::systemEnvironment();
    const EnvironmentItems changes = EnvironmentItem::fromStringList(
        ICore::settings()->value(kEnvironmentChanges).toStringList());
    setEnvironmentChanges(changes);
}

CorePlugin::CorePlugin()
{
    qRegisterMetaType<Id>();
    qRegisterMetaType<Utils::Text::Position>();
    qRegisterMetaType<Utils::CommandLine>();
    qRegisterMetaType<Utils::FilePath>();
    qRegisterMetaType<Utils::Environment>();
    qRegisterMetaType<Utils::Store>();
    qRegisterMetaType<Utils::Key>();
    qRegisterMetaType<Utils::KeyList>();
    qRegisterMetaType<Utils::OldStore>();
    m_instance = this;
    setupSystemEnvironment();
}

CorePlugin::~CorePlugin()
{
    IWizardFactory::destroyFeatureProvider();
    Find::destroy();

    delete m_locator;
    delete m_folderNavigationWidgetFactory;
    delete m_editMode;

    DesignMode::destroyModeIfRequired();

    delete m_core;
    SettingsDatabase::destroy();
    setCreatorTheme(nullptr);
}

CorePlugin *CorePlugin::instance()
{
    return m_instance;
}

struct CoreArguments {
    QColor overrideColor;
    Id themeId;
    bool presentationMode = false;
};

CoreArguments parseArguments(const QStringList &arguments)
{
    CoreArguments args;
    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments.at(i) == QLatin1String("-color")) {
            const QString colorcode(arguments.at(i + 1));
            args.overrideColor = QColor(colorcode);
            i++; // skip the argument
        }
        if (arguments.at(i) == QLatin1String("-presentationMode"))
            args.presentationMode = true;
        if (arguments.at(i) == QLatin1String("-theme")) {
            args.themeId = Id::fromString(arguments.at(i + 1));
            i++; // skip the argument
        }
    }
    return args;
}

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    // register all mime types from all plugins
    for (ExtensionSystem::PluginSpec *plugin : ExtensionSystem::PluginManager::plugins()) {
        if (!plugin->isEffectivelyEnabled())
            continue;
        const QJsonObject metaData = plugin->metaData();
        const QJsonValue mimetypes = metaData.value("Mimetypes");
        QString mimetypeString;
        if (Utils::readMultiLineString(mimetypes, &mimetypeString))
            Utils::addMimeTypes(plugin->name() + ".mimetypes", mimetypeString.trimmed().toUtf8());
    }

    if (ThemeEntry::availableThemes().isEmpty()) {
        *errorMessage = Tr::tr("No themes found in installation.");
        return false;
    }
    const CoreArguments args = parseArguments(arguments);
    Theme *themeFromArg = ThemeEntry::createTheme(args.themeId);
    Theme *theme = themeFromArg ? themeFromArg
                                : ThemeEntry::createTheme(ThemeEntry::themeSetting());
    Theme::setInitialPalette(theme); // Initialize palette before setting it
    setCreatorTheme(theme);
    InfoBar::initialize(ICore::settings());
    CheckableMessageBox::initialize(ICore::settings());
    new ActionManager(this);
    ActionManager::setPresentationModeEnabled(args.presentationMode);
    m_core = new ICore;
    if (args.overrideColor.isValid())
        ICore::setOverrideColor(args.overrideColor);
    m_locator = new Locator;
    std::srand(unsigned(QDateTime::currentDateTime().toSecsSinceEpoch()));
    ICore::init();
    m_editMode = new EditMode;
    ModeManager::activateMode(m_editMode->id());
    m_folderNavigationWidgetFactory = new FolderNavigationWidgetFactory;
    m_sessionManager.reset(new SessionManager);

    IWizardFactory::initialize();

    // Make sure we respect the process's umask when creating new files
    SaveFile::initializeUmask();

    Find::initialize();
    m_locator->initialize();

    MacroExpander *expander = Utils::globalMacroExpander();
    expander->registerVariable("CurrentDate:ISO", Tr::tr("The current date (ISO)."),
                               [] { return QDate::currentDate().toString(Qt::ISODate); });
    expander->registerVariable("CurrentTime:ISO", Tr::tr("The current time (ISO)."),
                               [] { return QTime::currentTime().toString(Qt::ISODate); });
    expander->registerVariable("CurrentDate:RFC", Tr::tr("The current date (RFC2822)."),
                               [] { return QDate::currentDate().toString(Qt::RFC2822Date); });
    expander->registerVariable("CurrentTime:RFC", Tr::tr("The current time (RFC2822)."),
                               [] { return QTime::currentTime().toString(Qt::RFC2822Date); });
    expander->registerVariable("CurrentDate:Locale", Tr::tr("The current date (Locale)."),
                               [] { return QLocale::system()
                                        .toString(QDate::currentDate(), QLocale::ShortFormat); });
    expander->registerVariable("CurrentTime:Locale", Tr::tr("The current time (Locale)."),
                               [] { return QLocale::system()
                                        .toString(QTime::currentTime(), QLocale::ShortFormat); });
    expander->registerVariable("Config:DefaultProjectDirectory", Tr::tr("The configured default directory for projects."),
                               [] { return DocumentManager::projectsDirectory().toString(); });
    expander->registerVariable("Config:LastFileDialogDirectory", Tr::tr("The directory last visited in a file dialog."),
                               [] { return DocumentManager::fileDialogLastVisitedDirectory().toString(); });
    expander->registerVariable("HostOs:isWindows",
                               Tr::tr("Is %1 running on Windows?")
                                   .arg(QGuiApplication::applicationDisplayName()),
                               [] {
                                   return QVariant(Utils::HostOsInfo::isWindowsHost()).toString();
                               });
    expander->registerVariable("HostOs:isOSX",
                               Tr::tr("Is %1 running on OS X?")
                                   .arg(QGuiApplication::applicationDisplayName()),
                               [] { return QVariant(Utils::HostOsInfo::isMacHost()).toString(); });
    expander->registerVariable("HostOs:isLinux",
                               Tr::tr("Is %1 running on Linux?")
                                   .arg(QGuiApplication::applicationDisplayName()),
                               [] { return QVariant(Utils::HostOsInfo::isLinuxHost()).toString(); });
    expander->registerVariable("HostOs:isUnix",
                               Tr::tr("Is %1 running on any unix-based platform?")
                                   .arg(QGuiApplication::applicationDisplayName()),
                               [] {
                                   return QVariant(Utils::HostOsInfo::isAnyUnixHost()).toString();
                               });
    expander->registerVariable("HostOs:PathListSeparator",
                               Tr::tr("The path list separator for the platform."),
                               [] { return QString(Utils::HostOsInfo::pathListSeparator()); });
    expander->registerVariable("HostOs:ExecutableSuffix",
                               Tr::tr("The platform executable suffix."),
                               [] { return QString(Utils::HostOsInfo::withExecutableSuffix("")); });
    expander->registerVariable("IDE:ResourcePath",
                               Tr::tr("The directory where %1 finds its pre-installed resources.")
                                   .arg(QGuiApplication::applicationDisplayName()),
                               [] { return ICore::resourcePath().toString(); });
    expander->registerPrefix("CurrentDate:", Tr::tr("The current date (QDate formatstring)."),
                             [](const QString &fmt) { return QDate::currentDate().toString(fmt); });
    expander->registerPrefix("CurrentTime:", Tr::tr("The current time (QTime formatstring)."),
                             [](const QString &fmt) { return QTime::currentTime().toString(fmt); });
    expander->registerVariable("UUID", Tr::tr("Generate a new UUID."),
                               [] { return QUuid::createUuid().toString(); });

    expander->registerPrefix("#:", Tr::tr("A comment."), [](const QString &) { return QString(); });

    Utils::PathChooser::setAboutToShowContextMenuHandler(&CorePlugin::addToPathChooserContextMenu);

#ifdef ENABLE_CRASHPAD
    connect(ICore::instance(), &ICore::coreOpened, this, &CorePlugin::warnAboutCrashReporing,
            Qt::QueuedConnection);
#endif

    return true;
}

static Id generateOpenPageCommandId(IOptionsPage *page)
{
    // The page and category are prioritized by their alphabetical order so usually the ids are
    // prepended by some prioritizing characters like D.ProjectExplorer.KitsOptions separated
    // by dots. Create a new actions id by joining the last parts of the page and category id
    // with an additional ".SettingsPage."
    const QStringList pageIdParts = page->id().toString().split('.');
    const QStringList categoryIdParts = page->category().toString().split('.');
    if (pageIdParts.isEmpty() || categoryIdParts.isEmpty())
        return {};

    const Id candidate = Id::fromString(
        QStringList{"Preferences", categoryIdParts.last(), pageIdParts.last()}.join('.'));
    QString suffix;
    int i = 0;
    while (ActionManager::command(candidate.withSuffix(suffix)))
        suffix = QString::number(++i);
    return candidate.withSuffix(suffix);
}

static void registerActionsForOptions()
{
    QMap<Utils::Id, QString> categoryDisplay;
    for (IOptionsPage *page : IOptionsPage::allOptionsPages()) {
        if (!categoryDisplay.contains(page->category()) && !page->displayCategory().isEmpty())
            categoryDisplay[page->category()] = page->displayCategory();
    }
    for (IOptionsPage *page : IOptionsPage::allOptionsPages()) {
        const Id commandId = generateOpenPageCommandId(page);
        if (!commandId.isValid())
            continue;
        const QString actionTitle = Tr::tr("%1 > %2 Preferences...")
            .arg(categoryDisplay.value(page->category()), page->displayName());
        auto action = new QAction(actionTitle, m_instance);
        QObject::connect(action, &QAction::triggered, m_instance, [id = page->id()] {
            ICore::showOptionsDialog(id);
        });
        ActionManager::registerAction(action, commandId);
    }
}

void CorePlugin::extensionsInitialized()
{
    DesignMode::createModeIfRequired();
    Find::extensionsInitialized();
    m_locator->extensionsInitialized();
    ICore::extensionsInitialized();
    if (ExtensionSystem::PluginManager::hasError()) {
        auto errorOverview = new ExtensionSystem::PluginErrorOverview(ICore::mainWindow());
        errorOverview->setAttribute(Qt::WA_DeleteOnClose);
        errorOverview->setModal(true);
        errorOverview->show();
    }
    checkSettings();
    registerActionsForOptions();
}

bool CorePlugin::delayedInitialize()
{
    m_locator->delayedInitialize();
    IWizardFactory::allWizardFactories(); // scan for all wizard factories
    return true;
}

QObject *CorePlugin::remoteCommand(const QStringList & /* options */,
                                   const QString &workingDirectory,
                                   const QStringList &args)
{
    if (!ExtensionSystem::PluginManager::isInitializationDone()) {
        connect(ExtensionSystem::PluginManager::instance(),
                &ExtensionSystem::PluginManager::initializationDone,
                this, [=] { remoteCommand(QStringList(), workingDirectory, args); });
        return nullptr;
    }
    const FilePaths filePaths = Utils::transform(args, FilePath::fromUserInput);
    IDocument *res = ICore::openFiles(
                filePaths,
                ICore::OpenFilesFlags(ICore::SwitchMode | ICore::CanContainLineAndColumnNumbers | ICore::SwitchSplitIfAlreadyVisible),
                FilePath::fromString(workingDirectory));
    ICore::raiseMainWindow();
    return res;
}

Environment CorePlugin::startupSystemEnvironment()
{
    return m_instance->m_startupSystemEnvironment;
}

EnvironmentItems CorePlugin::environmentChanges()
{
    return m_instance->m_environmentChanges;
}

void CorePlugin::setEnvironmentChanges(const EnvironmentItems &changes)
{
    if (m_instance->m_environmentChanges == changes)
        return;
    m_instance->m_environmentChanges = changes;
    Environment systemEnv = m_instance->m_startupSystemEnvironment;
    systemEnv.modify(changes);
    Environment::setSystemEnvironment(systemEnv);
    ICore::settings()->setValueWithDefault(kEnvironmentChanges,
                                           EnvironmentItem::toStringList(changes));
    if (ICore::instance())
        emit ICore::instance()->systemEnvironmentChanged();
}

void CorePlugin::fileOpenRequest(const QString &f)
{
    remoteCommand(QStringList(), QString(), QStringList(f));
}

void CorePlugin::addToPathChooserContextMenu(Utils::PathChooser *pathChooser, QMenu *menu)
{
    QList<QAction*> actions = menu->actions();
    QAction *firstAction = actions.isEmpty() ? nullptr : actions.first();

    if (pathChooser->filePath().exists()) {
        auto showInGraphicalShell = new QAction(FileUtils::msgGraphicalShellAction(), menu);
        connect(showInGraphicalShell, &QAction::triggered, pathChooser, [pathChooser] {
            Core::FileUtils::showInGraphicalShell(pathChooser, pathChooser->filePath());
        });
        menu->insertAction(firstAction, showInGraphicalShell);

        auto showInTerminal = new QAction(FileUtils::msgTerminalHereAction(), menu);
        connect(showInTerminal, &QAction::triggered, pathChooser, [pathChooser] {
            if (pathChooser->openTerminalHandler())
                pathChooser->openTerminalHandler()();
            else
                FileUtils::openTerminal(pathChooser->filePath(), {});
        });
        menu->insertAction(firstAction, showInTerminal);

    } else {
        auto *mkPathAct = new QAction(Tr::tr("Create Folder"), menu);
        connect(mkPathAct, &QAction::triggered, pathChooser, [pathChooser] {
            QDir().mkpath(pathChooser->filePath().toString());
            pathChooser->triggerChanged();
        });
        menu->insertAction(firstAction, mkPathAct);
    }

    if (firstAction)
        menu->insertSeparator(firstAction);
}

void CorePlugin::checkSettings()
{
    const auto showMsgBox = [this](const QString &msg, QMessageBox::Icon icon) {
        connect(ICore::instance(), &ICore::coreOpened, this, [msg, icon] {
            QMessageBox msgBox(ICore::dialogParent());
            msgBox.setWindowTitle(Tr::tr("Settings File Error"));
            msgBox.setText(msg);
            msgBox.setIcon(icon);
            msgBox.exec();
        }, Qt::QueuedConnection);
    };
    const QtcSettings * const userSettings = ICore::settings();
    QString errorDetails;
    switch (userSettings->status()) {
    case QSettings::NoError: {
        const QFileInfo fi(userSettings->fileName());
        if (fi.exists() && !fi.isWritable()) {
            const QString errorMsg = Tr::tr("The settings file \"%1\" is not writable.\n"
                                            "You will not be able to store any %2 settings.")
                                         .arg(QDir::toNativeSeparators(userSettings->fileName()),
                                              QGuiApplication::applicationDisplayName());
            showMsgBox(errorMsg, QMessageBox::Warning);
        }
        return;
    }
    case QSettings::AccessError:
        errorDetails = Tr::tr("The file is not readable.");
        break;
    case QSettings::FormatError:
        errorDetails = Tr::tr("The file is invalid.");
        break;
    }
    const QString errorMsg
        = Tr::tr("Error reading settings file \"%1\": %2\n"
                 "You will likely experience further problems using this instance of %3.")
              .arg(QDir::toNativeSeparators(userSettings->fileName()),
                   errorDetails,
                   QGuiApplication::applicationDisplayName());
    showMsgBox(errorMsg, QMessageBox::Critical);
}

void CorePlugin::warnAboutCrashReporing()
{
    if (!ICore::infoBar()->canInfoBeAdded(kWarnCrashReportingSetting))
        return;

    QString warnStr = ICore::settings()->value("CrashReportingEnabled", false).toBool()
            ? Tr::tr("%1 collects crash reports for the sole purpose of fixing bugs. "
                 "To disable this feature go to %2.")
            : Tr::tr("%1 can collect crash reports for the sole purpose of fixing bugs. "
                 "To enable this feature go to %2.");

    if (Utils::HostOsInfo::isMacHost()) {
        warnStr = warnStr.arg(QGuiApplication::applicationDisplayName(),
                              QGuiApplication::applicationDisplayName()
                                  + Tr::tr(" > Preferences > Environment > System"));
    } else {
        warnStr = warnStr.arg(QGuiApplication::applicationDisplayName(),
                              Tr::tr("Edit > Preferences > Environment > System"));
    }

    Utils::InfoBarEntry info(kWarnCrashReportingSetting, warnStr,
                             Utils::InfoBarEntry::GlobalSuppression::Enabled);
    info.addCustomButton(ICore::msgShowOptionsDialog(), [] {
        ICore::infoBar()->removeInfo(kWarnCrashReportingSetting);
        ICore::infoBar()->globallySuppressInfo(kWarnCrashReportingSetting);
        ICore::showOptionsDialog(Core::Constants::SETTINGS_ID_SYSTEM);
    });

    info.setDetailsWidgetCreator([]() -> QWidget * {
        auto label = new QLabel;
        label->setWordWrap(true);
        label->setOpenExternalLinks(true);
        label->setText(msgCrashpadInformation());
        label->setContentsMargins(0, 0, 0, 8);
        return label;
    });
    ICore::infoBar()->addInfo(info);
}

// static
QString CorePlugin::msgCrashpadInformation()
{
    return Tr::tr("%1 uses Google Crashpad for collecting crashes and sending them to our backend "
                  "for processing. Crashpad may capture arbitrary contents from crashed process’ "
                  "memory, including user sensitive information, URLs, and whatever other content "
                  "users have trusted %1 with. The collected crash reports are however only used "
                  "for the sole purpose of fixing bugs.")
               .arg(QGuiApplication::applicationDisplayName())
           + "<br><br>" + Tr::tr("More information:")
           + "<br><a href='https://chromium.googlesource.com/crashpad/crashpad/+/master/doc/"
             "overview_design.md'>"
           + Tr::tr("Crashpad Overview")
           + "</a>"
             "<br><a href='https://sentry.io/security/'>"
           + Tr::tr("%1 security policy").arg("Sentry.io") + "</a>";
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    LoggingViewer::hideLoggingView();
    Find::aboutToShutdown();
    m_locator->aboutToShutdown();
    ICore::aboutToShutdown();
    return SynchronousShutdown;
}
