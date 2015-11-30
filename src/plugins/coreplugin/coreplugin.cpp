/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "coreplugin.h"
#include "designmode.h"
#include "editmode.h"
#include "idocument.h"
#include "helpmanager.h"
#include "mainwindow.h"
#include "modemanager.h"
#include "infobar.h"
#include "iwizardfactory.h"
#include "themesettings.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/locator/locator.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/fileutils.h>

#include <extensionsystem/pluginerroroverview.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/algorithm.h>
#include <utils/pathchooser.h>
#include <utils/macroexpander.h>
#include <utils/savefile.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>
#include <utils/theme/theme_p.h>

#include <QtPlugin>
#include <QDebug>
#include <QDateTime>
#include <QMenu>

using namespace Core;
using namespace Core::Internal;
using namespace Utils;

CorePlugin::CorePlugin()
  : m_mainWindow(0)
  , m_editMode(0)
  , m_designMode(0)
  , m_findPlugin(0)
  , m_locator(0)
{
    qRegisterMetaType<Id>();
}

CorePlugin::~CorePlugin()
{
    IWizardFactory::destroyFeatureProvider();

    delete m_findPlugin;
    delete m_locator;

    if (m_editMode) {
        removeObject(m_editMode);
        delete m_editMode;
    }

    if (m_designMode) {
        if (m_designMode->designModeIsRequired())
            removeObject(m_designMode);
        delete m_designMode;
    }

    delete m_mainWindow;
    setCreatorTheme(0);
}

void CorePlugin::parseArguments(const QStringList &arguments)
{
    const Id settingsThemeId = Id::fromSetting(ICore::settings()->value(
                QLatin1String(Constants::SETTINGS_THEME), QLatin1String("default")));
    Id themeId = settingsThemeId;
    QColor overrideColor;
    bool presentationMode = false;

    for (int i = 0; i < arguments.size(); ++i) {
        if (arguments.at(i) == QLatin1String("-color")) {
            const QString colorcode(arguments.at(i + 1));
            overrideColor = QColor(colorcode);
            i++; // skip the argument
        }
        if (arguments.at(i) == QLatin1String("-presentationMode"))
            presentationMode = true;
        if (arguments.at(i) == QLatin1String("-theme")) {
            themeId = Id::fromString(arguments.at(i + 1));
            i++;
        }
    }
    const QList<ThemeEntry> availableThemes = ThemeSettings::availableThemes();
    int themeIndex = Utils::indexOf(availableThemes, Utils::equal(&ThemeEntry::id, themeId));
    if (themeIndex < 0) {
        themeIndex = Utils::indexOf(availableThemes,
                                    Utils::equal(&ThemeEntry::id, settingsThemeId));
    }
    if (themeIndex < 0)
        themeIndex = 0;
    if (themeIndex < availableThemes.size()) {
        const ThemeEntry themeEntry = availableThemes.at(themeIndex);
        QSettings themeSettings(themeEntry.filePath(), QSettings::IniFormat);
        Theme *theme = new Theme(themeEntry.id().toString(), qApp);
        theme->readSettings(themeSettings);
        if (theme->flag(Theme::ApplyThemePaletteGlobally))
            QApplication::setPalette(theme->palette());
        setCreatorTheme(theme);
    }

    // defer creation of these widgets until here,
    // because they need a valid theme set
    m_mainWindow = new MainWindow;
    ActionManager::setPresentationModeEnabled(presentationMode);
    m_findPlugin = new FindPlugin;
    m_locator = new Locator;

    if (overrideColor.isValid())
        m_mainWindow->setOverrideColor(overrideColor);
}

bool CorePlugin::initialize(const QStringList &arguments, QString *errorMessage)
{
    if (ThemeSettings::availableThemes().isEmpty()) {
        *errorMessage = tr("No themes found in installation.");
        return false;
    }
    new ActionManager(this);
    Theme::initialPalette(); // Initialize palette before setting it
    qsrand(QDateTime::currentDateTime().toTime_t());
    parseArguments(arguments);
    const bool success = m_mainWindow->init(errorMessage);
    if (success) {
        m_editMode = new EditMode;
        addObject(m_editMode);
        ModeManager::activateMode(m_editMode->id());
        m_designMode = new DesignMode;
        InfoBar::initializeGloballySuppressed();
    }

    IWizardFactory::initialize();

    // Make sure we respect the process's umask when creating new files
    SaveFile::initializeUmask();

    m_findPlugin->initialize(arguments, errorMessage);
    m_locator->initialize(this, arguments, errorMessage);

    MacroExpander *expander = Utils::globalMacroExpander();
    expander->registerVariable("CurrentDate:ISO", tr("The current date (ISO)."),
                               []() { return QDate::currentDate().toString(Qt::ISODate); });
    expander->registerVariable("CurrentTime:ISO", tr("The current time (ISO)."),
                               []() { return QTime::currentTime().toString(Qt::ISODate); });
    expander->registerVariable("CurrentDate:RFC", tr("The current date (RFC2822)."),
                               []() { return QDate::currentDate().toString(Qt::RFC2822Date); });
    expander->registerVariable("CurrentTime:RFC", tr("The current time (RFC2822)."),
                               []() { return QTime::currentTime().toString(Qt::RFC2822Date); });
    expander->registerVariable("CurrentDate:Locale", tr("The current date (Locale)."),
                               []() { return QDate::currentDate().toString(Qt::DefaultLocaleShortDate); });
    expander->registerVariable("CurrentTime:Locale", tr("The current time (Locale)."),
                               []() { return QTime::currentTime().toString(Qt::DefaultLocaleShortDate); });
    expander->registerVariable("Config:DefaultProjectDirectory", tr("The configured default directory for projects."),
                               []() { return DocumentManager::projectsDirectory(); });
    expander->registerVariable("Config:LastFileDialogDirectory", tr("The directory last visited in a file dialog."),
                               []() { return DocumentManager::fileDialogLastVisitedDirectory(); });
    expander->registerVariable("HostOs:isWindows", tr("Is Qt Creator running on Windows?"),
                               []() { return QVariant(Utils::HostOsInfo::isWindowsHost()).toString(); });
    expander->registerVariable("HostOs:isOSX", tr("Is Qt Creator running on OS X?"),
                               []() { return QVariant(Utils::HostOsInfo::isMacHost()).toString(); });
    expander->registerVariable("HostOs:isLinux", tr("Is Qt Creator running on Linux?"),
                               []() { return QVariant(Utils::HostOsInfo::isLinuxHost()).toString(); });
    expander->registerVariable("HostOs:isUnix", tr("Is Qt Creator running on any unix-based platform?"),
                               []() { return QVariant(Utils::HostOsInfo::isAnyUnixHost()).toString(); });
    expander->registerPrefix("CurrentDate:", tr("The current date (QDate formatstring)."),
                             [](const QString &fmt) { return QDate::currentDate().toString(fmt); });
    expander->registerPrefix("CurrentTime:", tr("The current time (QTime formatstring)."),
                             [](const QString &fmt) { return QTime::currentTime().toString(fmt); });

    expander->registerPrefix("#:", tr("A comment."), [](const QString &) { return QStringLiteral(""); });

    // Make sure all wizards are there when the user might access the keyboard shortcuts:
    connect(ICore::instance(), &ICore::optionsDialogRequested, []() { IWizardFactory::allWizardFactories(); });

    Utils::PathChooser::setAboutToShowContextMenuHandler(&CorePlugin::addToPathChooserContextMenu);

    return success;
}

void CorePlugin::extensionsInitialized()
{
    if (m_designMode->designModeIsRequired())
        addObject(m_designMode);
    m_findPlugin->extensionsInitialized();
    m_locator->extensionsInitialized();
    m_mainWindow->extensionsInitialized();
    if (ExtensionSystem::PluginManager::hasError()) {
        auto errorOverview = new ExtensionSystem::PluginErrorOverview(m_mainWindow);
        errorOverview->setAttribute(Qt::WA_DeleteOnClose);
        errorOverview->setModal(true);
        errorOverview->show();
    }
}

bool CorePlugin::delayedInitialize()
{
    HelpManager::setupHelpManager();
    m_locator->delayedInitialize();
    IWizardFactory::allWizardFactories(); // scan for all wizard factories
    return true;
}

QObject *CorePlugin::remoteCommand(const QStringList & /* options */,
                                   const QString &workingDirectory,
                                   const QStringList &args)
{
    IDocument *res = m_mainWindow->openFiles(
                args, ICore::OpenFilesFlags(ICore::SwitchMode | ICore::CanContainLineAndColumnNumbers),
                workingDirectory);
    m_mainWindow->raiseWindow();
    return res;
}

void CorePlugin::fileOpenRequest(const QString &f)
{
    remoteCommand(QStringList(), QString(), QStringList(f));
}

void CorePlugin::addToPathChooserContextMenu(Utils::PathChooser *pathChooser, QMenu *menu)
{
    QList<QAction*> actions = menu->actions();
    QAction *firstAction = actions.isEmpty() ? nullptr : actions.first();

    auto *showInGraphicalShell = new QAction(Core::FileUtils::msgGraphicalShellAction(), menu);
    connect(showInGraphicalShell, &QAction::triggered, pathChooser, [pathChooser]() {
        Core::FileUtils::showInGraphicalShell(pathChooser, pathChooser->path());
    });
    menu->insertAction(firstAction, showInGraphicalShell);

    auto *showInTerminal = new QAction(Core::FileUtils::msgTerminalAction(), menu);
    connect(showInTerminal, &QAction::triggered, pathChooser, [pathChooser]() {
        Core::FileUtils::openTerminal(pathChooser->path());
    });
    menu->insertAction(firstAction, showInTerminal);

    if (firstAction)
        menu->insertSeparator(firstAction);
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    m_findPlugin->aboutToShutdown();
    m_mainWindow->aboutToShutdown();
    return SynchronousShutdown;
}
