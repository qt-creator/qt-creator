/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
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
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "qtsingleapplication.h"

#include <extensionsystem/pluginmanager.h>
#include <extensionsystem/pluginspec.h>
#include <extensionsystem/iplugin.h>

#include <QtCore/QDir>
#include <QtCore/QTextStream>
#include <QtCore/QFileInfo>
#include <QtCore/QDebug>
#include <QtCore/QTimer>
#include <QtCore/QLibraryInfo>
#include <QtCore/QTranslator>
#include <QtCore/QSettings>
#include <QtCore/QVariant>

#include <QtNetwork/QNetworkProxyFactory>

#include <QtGui/QMessageBox>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>

enum { OptionIndent = 4, DescriptionIndent = 24 };

static const char *appNameC = "Qt Creator";
static const char *corePluginNameC = "Core";
static const char *fixedOptionsC =
" [OPTION]... [FILE]...\n"
"Options:\n"
"    -help               Display this help\n"
"    -version            Display program version\n"
"    -client             Attempt to connect to already running instance\n";

static const char *HELP_OPTION1 = "-h";
static const char *HELP_OPTION2 = "-help";
static const char *HELP_OPTION3 = "/h";
static const char *HELP_OPTION4 = "--help";
static const char *VERSION_OPTION = "-version";
static const char *CLIENT_OPTION = "-client";

typedef QList<ExtensionSystem::PluginSpec *> PluginSpecSet;

// Helpers for displaying messages. Note that there is no console on Windows.
#ifdef Q_OS_WIN
// Format as <pre> HTML
static inline void toHtml(QString &t)
{
    t.replace(QLatin1Char('&'), QLatin1String("&amp;"));
    t.replace(QLatin1Char('<'), QLatin1String("&lt;"));
    t.replace(QLatin1Char('>'), QLatin1String("&gt;"));
    t.insert(0, QLatin1String("<html><pre>"));
    t.append(QLatin1String("</pre></html>"));
}

static void displayHelpText(QString t) // No console on Windows.
{
    toHtml(t);
    QMessageBox::information(0, QLatin1String(appNameC), t);
}

static void displayError(const QString &t) // No console on Windows.
{
    QMessageBox::critical(0, QLatin1String(appNameC), t);
}

#else

static void displayHelpText(const QString &t)
{
    qWarning("%s", qPrintable(t));
}

static void displayError(const QString &t)
{
    qCritical("%s", qPrintable(t));
}

#endif

static void printVersion(const ExtensionSystem::PluginSpec *coreplugin,
                         const ExtensionSystem::PluginManager &pm)
{
    QString version;
    QTextStream str(&version);
    str << '\n' << appNameC << ' ' << coreplugin->version()<< " based on Qt " << qVersion() << "\n\n";
    pm.formatPluginVersions(str);
    str << '\n' << coreplugin->copyright() << '\n';
    displayHelpText(version);
}

static void printHelp(const QString &a0, const ExtensionSystem::PluginManager &pm)
{
    QString help;
    QTextStream str(&help);
    str << "Usage: " << a0  << fixedOptionsC;
    ExtensionSystem::PluginManager::formatOptions(str, OptionIndent, DescriptionIndent);
    pm.formatPluginOptions(str,  OptionIndent, DescriptionIndent);
    displayHelpText(help);
}

static inline QString msgCoreLoadFailure(const QString &why)
{
    return QCoreApplication::translate("Application", "Failed to load core: %1").arg(why);
}

static inline QString msgSendArgumentFailed()
{
    return QCoreApplication::translate("Application", "Unable to send command line arguments to the already running instance. It appears to be not responding.");
}

static inline QStringList getPluginPaths()
{
    QStringList rc;
    // Figure out root:  Up one from 'bin'
    QDir rootDir = QApplication::applicationDirPath();
    rootDir.cdUp();
    const QString rootDirPath = rootDir.canonicalPath();
    // 1) "plugins" (Win/Linux)
    QString pluginPath = rootDirPath;
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String(IDE_LIBRARY_BASENAME);
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("qtcreator");
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("plugins");
    rc.push_back(pluginPath);
    // 2) "PlugIns" (OS X)
    pluginPath = rootDirPath;
    pluginPath += QLatin1Char('/');
    pluginPath += QLatin1String("PlugIns");
    rc.push_back(pluginPath);
    return rc;
}

#ifdef Q_OS_MAC
#  define SHARE_PATH "/../Resources"
#else
#  define SHARE_PATH "/../share/qtcreator"
#endif

int main(int argc, char **argv)
{
#ifdef Q_OS_MAC
    // increase the number of file that can be opened in Qt Creator.
    struct rlimit rl;
    getrlimit(RLIMIT_NOFILE, &rl);
    rl.rlim_cur = rl.rlim_max;
    setrlimit(RLIMIT_NOFILE, &rl);
#endif

    SharedTools::QtSingleApplication app((QLatin1String(appNameC)), argc, argv);

    QTranslator translator;
    QTranslator qtTranslator;
    QString locale = QLocale::system().name();

    // Must be done before any QSettings class is created
    QSettings::setPath(QSettings::IniFormat, QSettings::SystemScope,
            QCoreApplication::applicationDirPath()+QLatin1String(SHARE_PATH));

    // Work around bug in QSettings which gets triggered on Windows & Mac only
#ifdef Q_OS_MAC
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
            QDir::homePath()+"/.config");
#endif
#ifdef Q_OS_WIN
    QSettings::setPath(QSettings::IniFormat, QSettings::UserScope,
            qgetenv("appdata"));
#endif

    // keep this in sync with the MainWindow ctor in coreplugin/mainwindow.cpp
    const QSettings settings(QSettings::IniFormat, QSettings::UserScope,
                                 QLatin1String("Nokia"), QLatin1String("QtCreator"));
    locale = settings.value("General/OverrideLanguage", locale).toString();


    const QString &creatorTrPath = QCoreApplication::applicationDirPath()
                        + QLatin1String(SHARE_PATH "/translations");
    if (translator.load(QLatin1String("qtcreator_") + locale, creatorTrPath)) {
        const QString &qtTrPath = QLibraryInfo::location(QLibraryInfo::TranslationsPath);
        const QString &qtTrFile = QLatin1String("qt_") + locale;
        // Binary installer puts Qt tr files into creatorTrPath
        if (qtTranslator.load(qtTrFile, qtTrPath) || qtTranslator.load(qtTrFile, creatorTrPath)) {
            app.installTranslator(&translator);
            app.installTranslator(&qtTranslator);
            app.setProperty("qtc_locale", locale);
        } else {
            translator.load(QString()); // unload()
        }
    }

    // Make sure we honor the system's proxy settings
    QNetworkProxyFactory::setUseSystemConfiguration(true);

    // Load
    ExtensionSystem::PluginManager pluginManager;
    pluginManager.setFileExtension(QLatin1String("pluginspec"));

    const QStringList pluginPaths = getPluginPaths();
    pluginManager.setPluginPaths(pluginPaths);

    const QStringList arguments = app.arguments();
    QMap<QString, QString> foundAppOptions;
    if (arguments.size() > 1) {
        QMap<QString, bool> appOptions;
        appOptions.insert(QLatin1String(HELP_OPTION1), false);
        appOptions.insert(QLatin1String(HELP_OPTION2), false);
        appOptions.insert(QLatin1String(HELP_OPTION3), false);
        appOptions.insert(QLatin1String(HELP_OPTION4), false);
        appOptions.insert(QLatin1String(VERSION_OPTION), false);
        appOptions.insert(QLatin1String(CLIENT_OPTION), false);
        QString errorMessage;
        if (!pluginManager.parseOptions(arguments,
                                        appOptions,
                                        &foundAppOptions,
                                        &errorMessage)) {
            displayError(errorMessage);
            printHelp(QFileInfo(app.applicationFilePath()).baseName(), pluginManager);
            return -1;
        }
    }

    const PluginSpecSet plugins = pluginManager.plugins();
    ExtensionSystem::PluginSpec *coreplugin = 0;
    foreach (ExtensionSystem::PluginSpec *spec, plugins) {
        if (spec->name() == QLatin1String(corePluginNameC)) {
            coreplugin = spec;
            break;
        }
    }
    if (!coreplugin) {
        QString nativePaths = QDir::toNativeSeparators(pluginPaths.join(QLatin1String(",")));
        const QString reason = QCoreApplication::translate("Application", "Could not find 'Core.pluginspec' in %1").arg(nativePaths);
        displayError(msgCoreLoadFailure(reason));
        return 1;
    }
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }
    if (foundAppOptions.contains(QLatin1String(VERSION_OPTION))) {
        printVersion(coreplugin, pluginManager);
        return 0;
    }
    if (foundAppOptions.contains(QLatin1String(HELP_OPTION1))
            || foundAppOptions.contains(QLatin1String(HELP_OPTION2))
            || foundAppOptions.contains(QLatin1String(HELP_OPTION3))
            || foundAppOptions.contains(QLatin1String(HELP_OPTION4))) {
        printHelp(QFileInfo(app.applicationFilePath()).baseName(), pluginManager);
        return 0;
    }

    const bool isFirstInstance = !app.isRunning();
    if (!isFirstInstance && foundAppOptions.contains(QLatin1String(CLIENT_OPTION))) {
        if (!app.sendMessage(pluginManager.serializedArguments())) {
            displayError(msgSendArgumentFailed());
            return -1;
        }
        return 0;
    }

    pluginManager.loadPlugins();
    if (coreplugin->hasError()) {
        displayError(msgCoreLoadFailure(coreplugin->errorString()));
        return 1;
    }
    {
        QStringList errors;
        foreach (ExtensionSystem::PluginSpec *p, pluginManager.plugins())
            if (p->hasError())
                errors.append(p->errorString());
        if (!errors.isEmpty())
            QMessageBox::warning(0,
                QCoreApplication::translate("Application", "Qt Creator - Plugin loader messages"),
                errors.join(QString::fromLatin1("\n\n")));
    }

    if (isFirstInstance) {
        // Set up lock and remote arguments for the first instance only.
        // Silently fallback to unconnected instances for any subsequent
        // instances.
        app.initialize();
        QObject::connect(&app, SIGNAL(messageReceived(QString)),
                         &pluginManager, SLOT(remoteArguments(QString)));
    }
    QObject::connect(&app, SIGNAL(fileOpenRequest(QString)), coreplugin->plugin(), SLOT(fileOpenRequest(QString)));

    // Do this after the event loop has started
    QTimer::singleShot(100, &pluginManager, SLOT(startTests()));
    return app.exec();
}

