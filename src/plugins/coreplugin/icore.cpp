/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
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

#include "icore.h"

/*!
    \namespace Core
    \brief The Core namespace contains all classes that make up the Core plugin
    which constitute the basic functionality of Qt Creator.
*/

/*!
    \namespace Core::Internal
    \internal
*/

/*!
    \class Core::ICore
    \brief The ICore class allows access to the different part that make up
    the basic functionality of Qt Creator.

    You should never create a subclass of this interface. The one and only
    instance is created by the Core plugin. You can access this instance
    from your plugin through \c{Core::instance()}.

    \mainclass
*/

/*!
    \fn QStringList ICore::showNewItemDialog(const QString &title,
                                      const QList<IWizard *> &wizards,
                                      const QString &defaultLocation = QString())
    \brief Opens a dialog where the user can choose from a set of \a wizards that
    create new files/classes/projects.

    The \a title argument is shown as the dialogs title. The path where the
    files will be created (if the user doesn't change it) is set
    in \a defaultLocation. It defaults to the path of the file manager's
    current file.

    \sa Core::FileManager
*/

/*!
    \fn void setNewItemDialogPreferredWizardKinds(IWizard::WizardKinds kinds)
    \internal

    When set to true, the general "New File or Project" dialog will
    collapse the project categories.
    This is set by the project explorer: When projects are open, the preferred
    thing is to create files/classes, if no projects are open, the preferred thing
    to create are projects.
*/

/*!
    \fn bool ICore::showOptionsDialog(const QString &group = QString(),
                               const QString &page = QString())
    \brief Opens the application options/preferences dialog with preselected
    \a page in a specified \a group.

    The arguments refer to the string IDs of the corresponding IOptionsPage.
*/

/*!
    \fn bool ICore::showWarningWithOptions(const QString &title, const QString &text,
                                   const QString &details = QString(),
                                   const QString &settingsCategory = QString(),
                                   const QString &settingsId = QString(),
                                   QWidget *parent = 0);

    \brief Show a warning message with a button that opens a settings page.

    Should be used to display configuration errors and point users to the setting.
    Returns true if the settings dialog was accepted.
*/


/*!
    \fn ActionManager *ICore::actionManager() const
    \brief Returns the application's action manager.

    The action manager is responsible for registration of menus and
    menu items and keyboard shortcuts.
*/

/*!
    \fn FileManager *ICore::fileManager() const
    \brief Returns the application's file manager.

    The file manager keeps track of files for changes outside the application.
*/

/*!
    \fn UniqueIDManager *ICore::uniqueIDManager() const
    \brief Returns the application's id manager.

    The unique ID manager transforms strings in unique integers and the other way round.
*/

/*!
    \fn MessageManager *ICore::messageManager() const
    \brief Returns the application's message manager.

    The message manager is the interface to the "General" output pane for
    general application debug messages.
*/

/*!
    \fn ExtensionSystem::PluginManager *ICore::pluginManager() const
    \brief Returns the application's plugin manager.

    The plugin manager handles the plugin life cycles and manages
    the common object pool.
*/

/*!
    \fn EditorManager *ICore::editorManager() const
    \brief Returns the application's editor manager.

    The editor manager handles all editor related tasks like opening
    documents, the stack of currently open documents and the currently
    active document.
*/

/*!
    \fn ProgressManager *ICore::progressManager() const
    \brief Returns the application's progress manager.

    Use the progress manager to register a concurrent task to
    show a progress bar the way Qt Creator does it.
*/

/*!
    \fn ScriptManager *ICore::scriptManager() const
    \internal
*/

/*!
    \fn VariableManager *ICore::variableManager() const
    \brief Returns the application's variable manager.

    The variable manager is used to register application wide string variables
    like \c MY_PROJECT_DIR such that strings like \c{somecommand ${MY_PROJECT_DIR}/sub}
    can be resolved/expanded from anywhere in the application.
*/

/*!
    \fn VCSManager *ICore::vcsManager() const
    \brief Returns the application's vcs manager.

    The vcs manager can be used to e.g. retrieve information about
    the version control system used for a directory on hard disk.
    The actual functionality for a specific version control system
    must be implemented in a IVersionControl object and registered in
    the plugin manager's object pool.
*/

/*!
    \fn ModeManager *ICore::modeManager() const
    \brief Returns the application's mode manager.

    The mode manager handles everything related to the instances of IMode
    that were added to the plugin manager's object pool as well as their
    buttons and the tool bar with the round buttons in the lower left
    corner of Qt Creator.
*/

/*!
    \fn MimeDatabase *ICore::mimeDatabase() const
    \brief Returns the application's mime database.

    Use the mime database to manage mime types.
*/

/*!
    \fn QSettings *ICore::settings(QSettings::UserScope scope) const
    \brief Returns the application's main settings object.

    You can use it to retrieve or set application wide settings
    (in contrast to session or project specific settings).

    If \a scope is QSettings::UserScope (the default), the
    users settings will be read from the users settings, with
    a fallback to global settings provided with Qt Creator.

    If \a scope is QSettings::SystemScope, only the system settings
    shipped with the current version of Qt Creator will be read. This
    functionality exists for internal purposes only.

    \see settingsDatabase()
*/

/*!
    \fn SettingsDatabase *ICore::settingsDatabase() const
    \brief Returns the application's settings database.

    The settings database is meant as an alternative to the regular settings
    object. It is more suitable for storing large amounts of data. The settings
    are application wide.

    \see SettingsDatabase
*/

/*!
    \fn QPrinter *ICore::printer() const
    \brief Returns the application's printer object.

    Always use this printer object for printing, so the different parts of the
    application re-use its settings.
*/

/*!
    \fn QString ICore::resourcePath() const
    \brief Returns the absolute path that is used for resources like
    project templates and the debugger macros.

    This abstraction is needed to avoid platform-specific code all over
    the place, since e.g. on Mac the resources are part of the application bundle.
*/

/*!
    \fn QMainWindow *ICore::mainWindow() const
    \brief Returns the main application window.

    For use as dialog parent etc.
*/

/*!
    \fn IContext *ICore::currentContextObject() const
    \brief Returns the context object of the current main context.

    \sa ICore::updateAdditionalContexts()
    \sa ICore::addContextObject()
*/

/*!
    \fn void ICore::updateAdditionalContexts(const QList<int> &remove, const QList<int> &add)
    \brief Change the currently active additional contexts.

    Removes the list of additional contexts specified by \a remove and adds the
    list of additional contexts specified by \a add.

    \sa ICore::hasContext()
*/

/*!
    \fn bool ICore::hasContext(int context) const
    \brief Returns if the given \a context is currently one of the active contexts.

    \sa ICore::updateAdditionalContexts()
    \sa ICore::addContextObject()
*/

/*!
    \fn void ICore::addContextObject(IContext *context)
    \brief Registers an additional \a context object.

    After registration this context object gets automatically the
    current context object whenever its widget gets focus.

    \sa ICore::removeContextObject()
    \sa ICore::updateAdditionalContexts()
    \sa ICore::currentContextObject()
*/

/*!
    \fn void ICore::removeContextObject(IContext *context)
    \brief Unregisters a \a context object from the list of know contexts.

    \sa ICore::addContextObject()
    \sa ICore::updateAdditionalContexts()
    \sa ICore::currentContextObject()
}
*/

/*!
    \fn void ICore::openFiles(const QStringList &fileNames)
    \brief Open all files from a list of \a fileNames like it would be
    done if they were given to Qt Creator on the command line, or
    they were opened via \gui{File|Open}.
*/

/*!
    \fn ICore::ICore()
    \internal
*/

/*!
    \fn ICore::~ICore()
    \internal
*/

/*!
    \fn void ICore::coreOpened()
    \brief Emitted after all plugins have been loaded and the main window shown.
*/

/*!
    \fn void ICore::saveSettingsRequested()
    \brief Emitted to signal that the user has requested that the global settings
    should be saved to disk.

    At the moment that happens when the application is closed, and on \gui{Save All}.
*/

/*!
    \fn void ICore::optionsDialogRequested()
    \brief Signal that allows plugins to perform actions just before the \gui{Tools|Options}
    dialog is shown.
*/

/*!
    \fn void ICore::coreAboutToClose()
    \brief Plugins can do some pre-end-of-life actions when they get this signal.

    The application is guaranteed to shut down after this signal is emitted.
    It's there as an addition to the usual plugin lifecycle methods, namely
    IPlugin::shutdown(), just for convenience.
*/

/*!
    \fn void ICore::contextAboutToChange(Core::IContext *context)
    \brief Sent just before a new \a context becomes the current context
    (meaning that its widget got focus).
*/

/*!
    \fn void ICore::contextChanged(Core::IContext *context, const QList<int> &additionalContexts)
    \brief Sent just after a new \a context became the current context
    (meaning that its widget got focus), or if the additional context ids changed.
*/
