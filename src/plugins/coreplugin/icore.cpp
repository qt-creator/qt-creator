/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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
    \fn void ICore::contextChanged(const Core::Context &context)
    Indicates that a new \a context just became the current context. This includes the context
    from the focus object as well as the additional context.
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
static ICore *m_instance = nullptr;
static MainWindow *m_mainwindow = nullptr;

ICore *ICore::instance()
{
    return m_instance;
}

bool ICore::isNewItemDialogRunning()
{
    return NewDialog::currentDialog() || IWizardFactory::isWizardRunning();
}

QWidget *ICore::newItemDialog()
{
    if (NewDialog::currentDialog())
        return NewDialog::currentDialog();
    return IWizardFactory::currentWizard();
}

ICore::ICore(MainWindow *mainwindow)
{
    m_instance = this;
    m_mainwindow = mainwindow;
    // Save settings once after all plugins are initialized:
    connect(PluginManager::instance(), &PluginManager::initializationDone,
            this, &ICore::saveSettings);
    connect(PluginManager::instance(), &PluginManager::testsFinished, [this] (int failedTests) {
        emit coreAboutToClose();
        QCoreApplication::exit(failedTests);
    });
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
    connect(newDialog, &QObject::destroyed, m_instance, &ICore::updateNewItemDialogState);
    newDialog->setWizardFactories(factories, defaultLocation, extraVariables);
    newDialog->setWindowTitle(title);
    newDialog->showDialog();

    updateNewItemDialogState();
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
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + '/' + RELATIVE_DATA_PATH);
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
    return QDir::cleanPath(QCoreApplication::applicationDirPath() + '/' + RELATIVE_DOC_PATH);
}

/*!
    Returns the path to the command line tools that are shipped with \QC (corresponding
    to the IDE_LIBEXEC_PATH qmake variable).
 */
QString ICore::libexecPath()
{
    return QDir::cleanPath(QApplication::applicationDirPath() + '/' + RELATIVE_LIBEXEC_PATH);
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
    if (_MSC_VER > 1999)
        return QLatin1String("MSVC <unknown>");
    if (_MSC_VER >= 1910)
        return QLatin1String("MSVC 2017");
    if (_MSC_VER >= 1900)
        return QLatin1String("MSVC 2015");
#endif
    return QLatin1String("<unknown compiler>");
}

QString ICore::versionString()
{
    QString ideVersionDescription;
    if (QLatin1String(Constants::IDE_VERSION_LONG) != QLatin1String(Constants::IDE_VERSION_DISPLAY))
        ideVersionDescription = tr(" (%1)").arg(QLatin1String(Constants::IDE_VERSION_LONG));
    return tr("%1 %2%3").arg(QLatin1String(Constants::IDE_DISPLAY_NAME),
                             QLatin1String(Constants::IDE_VERSION_DISPLAY),
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
    if (window && window == m_mainwindow) {
        m_mainwindow->raiseWindow();
    } else {
        window->raise();
        window->activateWindow();
    }
}

void ICore::updateAdditionalContexts(const Context &remove, const Context &add,
                                     ContextPriority priority)
{
    m_mainwindow->updateAdditionalContexts(remove, add, priority);
}

void ICore::addAdditionalContext(const Context &context, ContextPriority priority)
{
    m_mainwindow->updateAdditionalContexts(Context(), context, priority);
}

void ICore::removeAdditionalContext(const Context &context)
{
    m_mainwindow->updateAdditionalContexts(context, Context(), ContextPriority::Low);
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

/*!
    \fn ICore::addCloseCoreListener

    \brief The \c ICore::addCloseCoreListener function provides a hook for plugins
    to veto on closing the application.

    When the application window requests a close, all listeners are called.
    If one if these calls returns \c false, the process is aborted and the
    event is ignored. If all calls return \c true, \c ICore::coreAboutToClose()
    is emitted and the event is accepted or performed..
*/
void ICore::addPreCloseListener(const std::function<bool ()> &listener)
{
    m_mainwindow->addPreCloseListener(listener);
}

QString ICore::systemInformation()
{
    QString result = PluginManager::instance()->systemInformation() + '\n';
    result += versionString() + '\n';
    result += buildCompatibilityString() + '\n';
#ifdef IDE_REVISION
    result += QString("From revision %1\n").arg(QString::fromLatin1(Constants::IDE_REVISION_STR).left(10));
#endif
#ifdef QTC_SHOW_BUILD_DATE
     result += QString("Built on %1 %2\n").arg(QLatin1String(__DATE__), QLatin1String(__TIME__));
#endif
     return result;
}

void ICore::saveSettings()
{
    emit m_instance->saveSettingsRequested();
    m_mainwindow->saveSettings();

    ICore::settings(QSettings::SystemScope)->sync();
    ICore::settings(QSettings::UserScope)->sync();
}

QStringList ICore::additionalAboutInformation()
{
    return m_mainwindow->additionalAboutInformation();
}

void ICore::appendAboutInformation(const QString &line)
{
    m_mainwindow->appendAboutInformation(line);
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

} // namespace Core
