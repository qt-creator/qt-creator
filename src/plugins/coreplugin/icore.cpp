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

#include "icore.h"
#include "windowsupport.h"

#include <app/app_version.h>
#include <extensionsystem/pluginmanager.h>

#include <utils/qtcassert.h>

#include <QSysInfo>
#include <QApplication>

/*!
    \namespace Core
    \brief The Core namespace contains all classes that make up the Core plugin
    which constitute the basic functionality of \QC.
*/

/*!
    \namespace Core::Internal
    \internal
*/

/*!
    \class Core::ICore
    \brief The ICore class allows access to the different parts that make up
    the basic functionality of \QC.

    You should never create a subclass of this interface. The one and only
    instance is created by the Core plugin. You can access this instance
    from your plugin through \c{Core::instance()}.

    \mainclass
*/

/*!
    \fn void ICore::showNewItemDialog(const QString &title,
                                      const QList<IWizard *> &wizards,
                                      const QString &defaultLocation = QString(),
                                      const QVariantMap &extraVariables = QVariantMap())

    Opens a dialog where the user can choose from a set of \a wizards that
    create new files, classes, or projects.

    The \a title argument is shown as the dialog title. The path where the
    files will be created (if the user does not change it) is set
    in \a defaultLocation. It defaults to the path of the file manager's
    current file.

    \sa Core::DocumentManager
*/

/*!
    \fn bool ICore::showOptionsDialog(Id group, Id page, QWidget *parent = 0);

    Opens the application \gui Options (or \gui Preferences) dialog with preselected
    \a page in the specified \a group.

    The arguments refer to the string IDs of the corresponding IOptionsPage.
*/

/*!
    \fn bool ICore::showWarningWithOptions(const QString &title, const QString &text,
                                   const QString &details = QString(),
                                   Id settingsCategory = Id(),
                                   Id settingsId = Id(),
                                   QWidget *parent = 0)

    Shows a warning message with a button that opens a settings page.

    Should be used to display configuration errors and point users to the setting.
    Returns \c true if the settings dialog was accepted.
*/


/*!
    \fn QSettings *ICore::settings(QSettings::Scope scope = QSettings::UserScope)

    Returns the application's main settings object.

    You can use it to retrieve or set application wide settings
    (in contrast to session or project specific settings).

    If \a scope is \c QSettings::UserScope (the default), the
    users settings will be read from the users settings, with
    a fallback to global settings provided with \QC.

    If \a scope is \c QSettings::SystemScope, only the system settings
    shipped with the current version of \QC will be read. This
    functionality exists for internal purposes only.

    \see settingsDatabase()
*/

/*!
    \fn SettingsDatabase *ICore::settingsDatabase()

    Returns the application's settings database.

    The settings database is meant as an alternative to the regular settings
    object. It is more suitable for storing large amounts of data. The settings
    are application wide.

    \see SettingsDatabase
*/

/*!
    \fn QPrinter *ICore::printer()

    Returns the application's printer object.

    Always use this printer object for printing, so the different parts of the
    application re-use its settings.
*/

/*!
    \fn QString ICore::resourcePath()

    Returns the absolute path that is used for resources like
    project templates and the debugger macros.

    This abstraction is needed to avoid platform-specific code all over
    the place, since on Mac OS X, for example, the resources are part of the
    application bundle.
*/


/*!
    \fn QString ICore::userResourcePath()

    Returns the absolute path in the users directory that is used for
    resources like project templates.

    Use this function for finding the place for resources that the user may
    write to, for example, to allow for custom palettes or templates.
*/

/*!
    \fn QWidget *ICore::mainWindow()

    Returns the main application window.

    For dialog parents use \c dialogParent().
*/

/*!
    \fn QWidget *ICore::dialogParent()

    Returns a widget pointer suitable to use as parent for QDialogs.
 */

/*!
    \fn IContext *ICore::currentContextObject()

    Returns the context object of the current main context.

    \sa ICore::updateAdditionalContexts()
    \sa ICore::addContextObject()
*/

/*!
    \fn void ICore::updateAdditionalContexts(const Context &remove, const Context &add)
    Changes the currently active additional contexts.

    Removes the list of additional contexts specified by \a remove and adds the
    list of additional contexts specified by \a add.

    \sa ICore::hasContext()
*/

/*!
    \fn bool ICore::hasContext(int context) const
    Returns whether the given \a context is currently one of the active contexts.

    \sa ICore::updateAdditionalContexts()
    \sa ICore::addContextObject()
*/

/*!
    \fn void ICore::addContextObject(IContext *context)
    Registers an additional \a context object.

    After registration this context object gets automatically the
    current context object whenever its widget gets focus.

    \sa ICore::removeContextObject()
    \sa ICore::updateAdditionalContexts()
    \sa ICore::currentContextObject()
*/

/*!
    \fn void ICore::removeContextObject(IContext *context)
    Unregisters a \a context object from the list of know contexts.

    \sa ICore::addContextObject()
    \sa ICore::updateAdditionalContexts()
    \sa ICore::currentContextObject()
}
*/

/*!
    \fn void ICore::openFiles(const QStringList &fileNames, OpenFilesFlags flags = None)
    Opens all files from a list of \a fileNames like it would be
    done if they were given to \QC on the command line, or
    they were opened via \gui File > \gui Open.
*/

/*!
    \fn ICore::ICore(Internal::MainWindow *mw)
    \internal
*/

/*!
    \fn ICore::~ICore()
    \internal
*/

/*!
    \fn void ICore::coreOpened()
    Indicates that all plugins have been loaded and the main window is shown.
*/

/*!
    \fn void ICore::saveSettingsRequested()
    Signals that the user has requested that the global settings
    should be saved to disk.

    At the moment that happens when the application is closed, and on \gui{Save All}.
*/

/*!
    \fn void ICore::optionsDialogRequested()
    Enables plugins to perform actions just before the \gui Tools > \gui Options
    dialog is shown.
*/

/*!
    \fn void ICore::coreAboutToClose()
    Enables plugins to perform some pre-end-of-life actions.

    The application is guaranteed to shut down after this signal is emitted.
    It is there as an addition to the usual plugin lifecycle functions, namely
    \c IPlugin::aboutToShutdown(), just for convenience.
*/

/*!
    \fn void ICore::contextAboutToChange(const QList<Core::IContext *> &context)
    Indicates that a new \a context will shortly become the current context
    (meaning that its widget got focus).
*/

/*!
    \fn void ICore::contextChanged(Core::IContext *context, const Core::Context &additionalContexts)
    Indicates that a new \a context just became the current context
    (meaning that its widget got focus), or that the additional context ids
    specified by \a additionalContexts changed.
*/

#include "dialogs/newdialog.h"
#include "iwizardfactory.h"
#include "mainwindow.h"
#include "documentmanager.h"

#include <utils/hostosinfo.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QStatusBar>

using namespace Core::Internal;
using namespace ExtensionSystem;

namespace Core {

// The Core Singleton
static ICore *m_instance = 0;
static MainWindow *m_mainwindow;

ICore *ICore::instance()
{
    return m_instance;
}

bool ICore::isNewItemDialogRunning()
{
    return NewDialog::isRunning() || IWizardFactory::isWizardRunning();
}

ICore::ICore(MainWindow *mainwindow)
{
    m_instance = this;
    m_mainwindow = mainwindow;
    // Save settings once after all plugins are initialized:
    connect(PluginManager::instance(), SIGNAL(initializationDone()),
            this, SLOT(saveSettings()));
    connect(PluginManager::instance(), &PluginManager::testsFinished, [this] (int failedTests) {
        emit coreAboutToClose();
        QCoreApplication::exit(failedTests);
    });
    connect(m_mainwindow, SIGNAL(newItemDialogRunningChanged()),
            this, SIGNAL(newItemDialogRunningChanged()));
}

ICore::~ICore()
{
    m_instance = 0;
    m_mainwindow = 0;
}

void ICore::showNewItemDialog(const QString &title,
                              const QList<IWizardFactory *> &factories,
                              const QString &defaultLocation,
                              const QVariantMap &extraVariables)
{
    QTC_ASSERT(!isNewItemDialogRunning(), return);
    auto newDialog = new NewDialog(dialogParent());
    connect(newDialog, &QObject::destroyed, m_instance, &ICore::validateNewDialogIsRunning);
    newDialog->setWizardFactories(factories, defaultLocation, extraVariables);
    newDialog->setWindowTitle(title);
    newDialog->showDialog();

    validateNewDialogIsRunning();
}

bool ICore::showOptionsDialog(const Id page, QWidget *parent)
{
    return m_mainwindow->showOptionsDialog(page, parent);
}

QString ICore::msgShowOptionsDialog()
{
    return QCoreApplication::translate("Core", "Configure...", "msgShowOptionsDialog");
}

QString ICore::msgShowOptionsDialogToolTip()
{
    if (Utils::HostOsInfo::isMacHost())
        return QCoreApplication::translate("Core", "Open Preferences dialog.",
                                           "msgShowOptionsDialogToolTip (mac version)");
    else
        return QCoreApplication::translate("Core", "Open Options dialog.",
                                           "msgShowOptionsDialogToolTip (non-mac version)");
}

bool ICore::showWarningWithOptions(const QString &title, const QString &text,
                                   const QString &details, Id settingsId, QWidget *parent)
{
    return m_mainwindow->showWarningWithOptions(title, text, details, settingsId, parent);
}

QSettings *ICore::settings(QSettings::Scope scope)
{
    if (scope == QSettings::UserScope)
        return PluginManager::settings();
    else
        return PluginManager::globalSettings();
}

SettingsDatabase *ICore::settingsDatabase()
{
    return m_mainwindow->settingsDatabase();
}

QPrinter *ICore::printer()
{
    return m_mainwindow->printer();
}

QString ICore::userInterfaceLanguage()
{
    return qApp->property("qtc_locale").toString();
}

QString ICore::resourcePath()
{
    const QString sharePath = QLatin1String(Utils::HostOsInfo::isMacHost()
                                            ? "/../Resources" : "/../share/qtcreator");
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + sharePath);
}

QString ICore::userResourcePath()
{
    // Create qtcreator dir if it doesn't yet exist
    const QString configDir = QFileInfo(settings(QSettings::UserScope)->fileName()).path();
    const QString urp = configDir + QLatin1String("/qtcreator");

    if (!QFileInfo::exists(urp + QLatin1Char('/'))) {
        QDir dir;
        if (!dir.mkpath(urp))
            qWarning() << "could not create" << urp;
    }

    return urp;
}

QString ICore::documentationPath()
{
    const QString docPath = QLatin1String(Utils::HostOsInfo::isMacHost()
                                            ? "/../Resources/doc" : "/../share/doc/qtcreator");
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + docPath);
}

/*!
    Returns the path to the command line tools that are shipped with \QC (corresponding
    to the IDE_LIBEXEC_PATH qmake variable).
 */
QString ICore::libexecPath()
{
    QString path;
    switch (Utils::HostOsInfo::hostOs()) {
    case Utils::OsTypeWindows:
        path = QCoreApplication::applicationDirPath();
        break;
    case Utils::OsTypeMac:
        path = QCoreApplication::applicationDirPath() + QLatin1String("/../Resources");
        break;
    case Utils::OsTypeLinux:
    case Utils::OsTypeOtherUnix:
    case Utils::OsTypeOther:
        path = QCoreApplication::applicationDirPath() + QLatin1String("/../libexec/qtcreator");
        break;
    }
    return QDir::cleanPath(path);
}

static QString compilerString()
{
#if defined(Q_CC_CLANG) // must be before GNU, because clang claims to be GNU too
    QString isAppleString;
#if defined(__apple_build_version__) // Apple clang has other version numbers
    isAppleString = QLatin1String(" (Apple)");
#endif
    return QLatin1String("Clang " ) + QString::number(__clang_major__) + QLatin1Char('.')
            + QString::number(__clang_minor__) + isAppleString;
#elif defined(Q_CC_GNU)
    return QLatin1String("GCC " ) + QLatin1String(__VERSION__);
#elif defined(Q_CC_MSVC)
    if (_MSC_VER >= 1800) // 1800: MSVC 2013 (yearly release cycle)
        return QLatin1String("MSVC ") + QString::number(2008 + ((_MSC_VER / 100) - 13));
    if (_MSC_VER >= 1500) // 1500: MSVC 2008, 1600: MSVC 2010, ... (2-year release cycle)
        return QLatin1String("MSVC ") + QString::number(2008 + 2 * ((_MSC_VER / 100) - 15));
#endif
    return QLatin1String("<unknown compiler>");
}

QString ICore::versionString()
{
    QString ideVersionDescription;
#ifdef IDE_VERSION_DESCRIPTION
    ideVersionDescription = tr(" (%1)").arg(QLatin1String(Constants::IDE_VERSION_DESCRIPTION_STR));
#endif
    return tr("Qt Creator %1%2").arg(QLatin1String(Constants::IDE_VERSION_LONG),
                                     ideVersionDescription);
}

QString ICore::buildCompatibilityString()
{
    return tr("Based on Qt %1 (%2, %3 bit)").arg(QLatin1String(qVersion()),
                                                 compilerString(),
                                                 QString::number(QSysInfo::WordSize));
}

IContext *ICore::currentContextObject()
{
    return m_mainwindow->currentContextObject();
}


QWidget *ICore::mainWindow()
{
    return m_mainwindow;
}

QWidget *ICore::dialogParent()
{
    QWidget *active = QApplication::activeModalWidget();
    if (!active)
        active = QApplication::activeWindow();
    if (!active)
        active = m_mainwindow;
    return active;
}

QStatusBar *ICore::statusBar()
{
    return m_mainwindow->statusBar();
}

void ICore::raiseWindow(QWidget *widget)
{
    if (!widget)
        return;
    QWidget *window = widget->window();
    if (window == m_mainwindow) {
        m_mainwindow->raiseWindow();
    } else {
        window->raise();
        window->activateWindow();
    }
}

void ICore::updateAdditionalContexts(const Context &remove, const Context &add)
{
    m_mainwindow->updateAdditionalContexts(remove, add);
}

void ICore::addAdditionalContext(const Context &context)
{
    m_mainwindow->updateAdditionalContexts(Context(), context);
}

void ICore::removeAdditionalContext(const Context &context)
{
    m_mainwindow->updateAdditionalContexts(context, Context());
}

void ICore::addContextObject(IContext *context)
{
    m_mainwindow->addContextObject(context);
}

void ICore::removeContextObject(IContext *context)
{
    m_mainwindow->removeContextObject(context);
}

void ICore::registerWindow(QWidget *window, const Context &context)
{
    new WindowSupport(window, context); // deletes itself when widget is destroyed
}

void ICore::openFiles(const QStringList &arguments, ICore::OpenFilesFlags flags)
{
    m_mainwindow->openFiles(arguments, flags);
}

void ICore::saveSettings()
{
    emit m_instance->saveSettingsRequested();

    ICore::settings(QSettings::SystemScope)->sync();
    ICore::settings(QSettings::UserScope)->sync();
}

void ICore::validateNewDialogIsRunning()
{
    static bool wasRunning = false;
    if (wasRunning == isNewItemDialogRunning())
        return;
    wasRunning = isNewItemDialogRunning();
    emit instance()->newItemDialogRunningChanged();
}

} // namespace Core
