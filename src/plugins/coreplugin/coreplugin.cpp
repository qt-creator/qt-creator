/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "coreplugin.h"
#include "designmode.h"
#include "editmode.h"
#include "helpmanager.h"
#include "mainwindow.h"
#include "mimedatabase.h"
#include "modemanager.h"
#include "infobar.h"
#include "iwizardfactory.h"

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/find/findplugin.h>
#include <coreplugin/locator/locator.h>
#include <coreplugin/coreconstants.h>

#include <utils/macroexpander.h>
#include <utils/savefile.h>
#include <utils/stringutils.h>
#include <utils/theme/theme.h>

#include <QtPlugin>
#include <QDebug>
#include <QDateTime>
#include <QDir>

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
    qRegisterMetaType<Core::Id>();
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
    QString themeName = QLatin1String("default");
    QColor overrideColor;
    bool overrideTheme = false;
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
            overrideTheme = true;
            themeName = arguments.at(i + 1);
            i++;
        }
    }

    QSettings *settings = Core::ICore::settings();
    QString themeURI = settings->value(QLatin1String(Core::Constants::SETTINGS_THEME)).toString();

    if (!QFileInfo::exists(themeURI) || overrideTheme) {
        const QString builtInTheme = QStringLiteral("%1/themes/%2.creatortheme")
            .arg(ICore::resourcePath()).arg(themeName);
        if (QFile::exists(builtInTheme)) {
            themeURI = builtInTheme;
        } else if (themeName.endsWith(QLatin1String(".creatortheme"))) {
            themeURI = themeName;
        } else { // TODO: Fallback to default theme
            qCritical("%s", qPrintable(QCoreApplication::translate("Application", "No valid theme '%1'")
                                       .arg(themeName)));
        }
    }

    QSettings themeSettings(themeURI, QSettings::IniFormat);
    Theme *theme = new Theme(qApp);
    theme->readSettings(themeSettings);
    setCreatorTheme(theme);
    if (theme->flag(Theme::ApplyThemePaletteGlobally))
        QApplication::setPalette(creatorTheme()->palette(QApplication::palette()));

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

    // Make sure we respect the process's umask when creating new files
    Utils::SaveFile::initializeUmask();

    m_findPlugin->initialize(arguments, errorMessage);
    m_locator->initialize(this, arguments, errorMessage);

    Utils::MacroExpander *expander = Utils::globalMacroExpander();
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
    expander->registerPrefix("CurrentDate:", tr("The current date (QDate formatstring)"),
                             [](const QString &fmt) { return QDate::currentDate().toString(fmt); });
    expander->registerPrefix("CurrentTime:", tr("The current time (QTime formatstring)"),
                             [](const QString &fmt) { return QTime::currentTime().toString(fmt); });

    return success;
}

void CorePlugin::extensionsInitialized()
{
    MimeDatabase::syncUserModifiedMimeTypes();
    if (m_designMode->designModeIsRequired())
        addObject(m_designMode);
    m_findPlugin->extensionsInitialized();
    m_locator->extensionsInitialized();
    m_mainWindow->extensionsInitialized();
}

bool CorePlugin::delayedInitialize()
{
    HelpManager::setupHelpManager();
    m_locator->delayedInitialize();
    return true;
}

QObject *CorePlugin::remoteCommand(const QStringList & /* options */, const QStringList &args)
{
    IDocument *res = m_mainWindow->openFiles(
                args, ICore::OpenFilesFlags(ICore::SwitchMode | ICore::CanContainLineNumbers));
    m_mainWindow->raiseWindow();
    return res;
}

void CorePlugin::fileOpenRequest(const QString &f)
{
    remoteCommand(QStringList(), QStringList(f));
}

ExtensionSystem::IPlugin::ShutdownFlag CorePlugin::aboutToShutdown()
{
    m_findPlugin->aboutToShutdown();
    m_mainWindow->aboutToShutdown();
    return SynchronousShutdown;
}
