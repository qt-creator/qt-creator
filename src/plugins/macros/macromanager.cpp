/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nicolas Arnaud-Cormos.
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "macromanager.h"

#include "macrosconstants.h"
#include "macro.h"
#include "macrosettings.h"
#include "imacrohandler.h"
#include "savedialog.h"
#include "actionmacrohandler.h"
#include "texteditormacrohandler.h"
#include "findmacrohandler.h"

#include <texteditor/texteditorconstants.h>

#include <coreplugin/coreconstants.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/actionmanager/actioncontainer.h>
#include <coreplugin/actionmanager/command.h>
#include <coreplugin/icore.h>
#include <coreplugin/uniqueidmanager.h>
#include <coreplugin/icontext.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QShortcut>
#include <QKeySequence>
#include <QMainWindow>
#include <QSettings>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>
#include <QSignalMapper>
#include <QList>

using namespace Macros;
using namespace Macros::Internal;

/*!
    \namespace Macros
    \brief The Macros namespace contains support for macros in Qt Creator.
*/

/*!

    \class Macro::MacroManager
    \brief Manager for macros.

    The MacroManager manage all macros, it loads them on startup, keep track of the
    current macro and create new macros.

    There are two important methods in this class that can be used outside the Macros plugin:
    \list
    \o registerEventHandler: add a new event handler
    \o registerAction: add a macro event when this action is triggered
    \endlist

    This class is a singleton and can be accessed using the instance method.
*/

/*!
    \fn void registerAction(QAction *action, const QString &id)

    Append this action to the list of actions registered in a macro. The id is
    the action id passed to the ActionManager.
*/

class Macros::MacroManager::MacroManagerPrivate
{
public:
    MacroManagerPrivate(MacroManager *qq);

    MacroManager *q;
    MacroSettings settings;
    QMap<QString, Macro *> macros;
    Macro *currentMacro;
    bool isRecording;

    QList<IMacroHandler*> handlers;

    QSignalMapper *mapper;
    QMap<int, QShortcut *> shortcuts;
    int currentId;

    ActionMacroHandler *actionHandler;
    TextEditorMacroHandler *textEditorHandler;
    FindMacroHandler *findHandler;

    void init();
    void appendDirectory(const QString &directory);
    void removeDirectory(const QString &directory);
    void addMacro(Macro *macro);
    void removeMacro(const QString &name);
    void changeMacroDescription(Macro *macro, const QString &description);

    int addShortcut(Macro *macro, int id=-1);
    void removeShortcut(Macro *macro);

    bool executeMacro(Macro *macro);
    void showSaveDialog();
};

MacroManager::MacroManagerPrivate::MacroManagerPrivate(MacroManager *qq):
    q(qq),
    currentMacro(0),
    isRecording(false),
    mapper(new QSignalMapper(qq)),
    currentId(0)
{
    settings.fromSettings(Core::ICore::instance()->settings());

    connect(mapper, SIGNAL(mapped(QString)), q, SLOT(executeMacro(QString)));

    // Load/unload macros
    foreach (const QString &dir, settings.directories)
        appendDirectory(dir);

    actionHandler = new ActionMacroHandler;
    textEditorHandler = new TextEditorMacroHandler;
    findHandler = new FindMacroHandler;
}

void MacroManager::MacroManagerPrivate::appendDirectory(const QString &directory)
{
    macros.clear();
    QDir dir(directory);
    QStringList filter;
    filter << QString("*.")+Constants::M_EXTENSION;
    QStringList files = dir.entryList(filter, QDir::Files);

    foreach (const QString &name, files) {
        QString fileName = dir.absolutePath()+"/"+name;
        Macro *macro = new Macro;
        macro->loadHeader(fileName);
        addMacro(macro);

        // Create shortcut
        if (settings.shortcutIds.contains(macro->displayName())) {
            int id = settings.shortcutIds.value(macro->displayName()).toInt();
            addShortcut(macro, id);
        }
    }
}

void MacroManager::MacroManagerPrivate::removeDirectory(const QString &directory)
{
    QMapIterator<QString, Macro *> it(macros);
    QDir dir(directory);
    QStringList removeList;
    while (it.hasNext()) {
        it.next();
        QFileInfo fileInfo(it.value()->fileName());
        if (fileInfo.absoluteDir() == dir.absolutePath())
            removeList.append(it.key());
    }
    foreach (const QString &name, removeList)
        removeMacro(name);
}

void MacroManager::MacroManagerPrivate::addMacro(Macro *macro)
{
    macros[macro->displayName()] = macro;
}

void MacroManager::MacroManagerPrivate::removeMacro(const QString &name)
{
    if (!macros.contains(name))
        return;
    Macro *macro = macros.take(name);
    removeShortcut(macro);
    delete macro;
}

void MacroManager::MacroManagerPrivate::changeMacroDescription(Macro *macro, const QString &description)
{
    macro->load();
    macro->setDescription(description);
    macro->save(macro->fileName());
}

int MacroManager::MacroManagerPrivate::addShortcut(Macro *macro, int id)
{
    if (id==-1)
        id=currentId;
    QShortcut *shortcut=0;
    if (shortcuts.contains(id)) {
        shortcut = shortcuts.value(id);
    } else {
        Core::Context context(TextEditor::Constants::C_TEXTEDITOR);
        Core::ICore *core = Core::ICore::instance();
        Core::ActionManager *am = core->actionManager();
        shortcut = new QShortcut(core->mainWindow());
        connect(shortcut, SIGNAL(activated()), mapper, SLOT(map()));
        const QString macroId = QLatin1String(Constants::SHORTCUT_MACRO) + QString("%1").arg(id, 2, 10, QLatin1Char('0'));
        Core::Command *command = am->registerShortcut(shortcut, macroId, context);

        // Add a default shortcut for the 10 first shortcuts
        if (id < 10)
            command->setDefaultKeySequence(QKeySequence(QString(tr("Alt+R,Alt+%1").arg(id))));
        shortcuts[id] = shortcut;
    }
    // Update the current id (first id without shortcut)
    while (shortcuts.contains(currentId))
        currentId++;

    // Assign the shortcut
    macro->setShortcutId(id);
    shortcut->setWhatsThis(macro->displayName());
    mapper->setMapping(shortcut, macro->displayName());
    return id;
}

void MacroManager::MacroManagerPrivate::removeShortcut(Macro *macro)
{
    // Remove the old shortcut
    if (macro->shortcutId() >= 0) {
        QShortcut* shortcut = qobject_cast<QShortcut *>(mapper->mapping(macro->displayName()));
        shortcut->setWhatsThis("");
        if (shortcut)
            mapper->removeMappings(shortcut);
        if (currentId > macro->shortcutId())
            currentId = macro->shortcutId();
        macro->setShortcutId(-1);
    }
}

bool MacroManager::MacroManagerPrivate::executeMacro(Macro *macro)
{
    macro->load();
    bool error = false;
    foreach (const MacroEvent &macroEvent, macro->events()) {
        foreach (IMacroHandler *handler, handlers) {
            if (handler->canExecuteEvent(macroEvent)) {
                if (!handler->executeEvent(macroEvent))
                    error = true;
                break;
            }
        }
        if (error)
            break;
    }

    if (error) {
        QMessageBox::warning(Core::ICore::instance()->mainWindow(),
                             tr("Executing Macro"),
                             tr("An error occured while replaying the macro, execution stopped."));
    }

    // Set the focus back to the editor
    // TODO: is it really needed??
    const Core::EditorManager *editorManager = Core::EditorManager::instance();
    if (editorManager->currentEditor())
        editorManager->currentEditor()->widget()->setFocus(Qt::OtherFocusReason);

    return !error;
}

void MacroManager::MacroManagerPrivate::showSaveDialog()
{
    QMainWindow *mainWindow = Core::ICore::instance()->mainWindow();
    SaveDialog dialog(mainWindow);
    if (dialog.exec()) {
        bool changed = false;
        if (settings.showSaveDialog == dialog.hideSaveDialog()) {
            settings.showSaveDialog = !dialog.hideSaveDialog();
            changed = true;
        }

        if (dialog.name().isEmpty())
            return;

        // Check if there's a default directory
        // If not, ask a directory to the user
        QString directory = settings.defaultDirectory;
        QDir dir(directory);
        if (directory.isEmpty() || !dir.exists()) {
            directory = QFileDialog::getExistingDirectory(
                    mainWindow,
                    tr("Choose a default macro directory"),
                    QDir::homePath());
            if (directory.isNull())
                return;
            settings.directories.append(directory);
            settings.defaultDirectory= directory;
            changed = true;
        }
        QString fileName = directory + '/' + dialog.name()
                           + '.' + Constants::M_EXTENSION;
        currentMacro->setDescription(dialog.description());
        currentMacro->save(fileName);
        addMacro(currentMacro);

        if (dialog.createShortcut())
            settings.shortcutIds[dialog.name()] = addShortcut(currentMacro);

        if (changed)
            q->saveSettings();
    }
}


// ---------- MacroManager ------------
MacroManager *MacroManager::m_instance = 0;

MacroManager::MacroManager(QObject *parent) :
    QObject(parent),
    d(new MacroManagerPrivate(this))
{
    registerMacroHandler(d->actionHandler);
    registerMacroHandler(d->findHandler);
    registerMacroHandler(d->textEditorHandler);
    m_instance = this;
}

MacroManager::~MacroManager()
{
    // Cleanup macro
    QStringList macroList = d->macros.keys();
    foreach (const QString &name, macroList)
        d->removeMacro(name);

    // Cleanup handlers
    qDeleteAll(d->handlers);

    delete d;
}

const MacroSettings &MacroManager::settings() const
{
    return d->settings;
}

void MacroManager::startMacro()
{
    d->isRecording = true;
    // Delete anonymous macro
    if (d->currentMacro && d->currentMacro->displayName().isEmpty())
        delete d->currentMacro;
    d->currentMacro = new Macro;

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    am->command(Constants::START_MACRO)->action()->setEnabled(false);
    am->command(Constants::END_MACRO)->action()->setEnabled(true);
    am->command(Constants::EXECUTE_LAST_MACRO)->action()->setEnabled(false);
    foreach (IMacroHandler *handler, d->handlers)
        handler->startRecording(d->currentMacro);

    QString endShortcut = am->command(Constants::END_MACRO)->defaultKeySequence().toString();
    QString executeShortcut = am->command(Constants::EXECUTE_LAST_MACRO)->defaultKeySequence().toString();
    QString help = tr("Macro mode. Type \"%1\" to stop recording and \"%2\" to execute it")
        .arg(endShortcut).arg(executeShortcut);
    Core::EditorManager::instance()->showEditorStatusBar(
                QLatin1String(Constants::M_STATUS_BUFFER),
                help,
                tr("End Macro"), this, SLOT(endMacro()));
}

void MacroManager::endMacro()
{
    Core::EditorManager::instance()->hideEditorStatusBar(QLatin1String(Constants::M_STATUS_BUFFER));

    Core::ActionManager *am = Core::ICore::instance()->actionManager();
    am->command(Constants::START_MACRO)->action()->setEnabled(true);
    am->command(Constants::END_MACRO)->action()->setEnabled(false);
    am->command(Constants::EXECUTE_LAST_MACRO)->action()->setEnabled(true);
    foreach (IMacroHandler *handler, d->handlers)
        handler->endRecordingMacro(d->currentMacro);

    d->isRecording = false;

    if (d->currentMacro->events().count() && d->settings.showSaveDialog)
        d->showSaveDialog();
}

void MacroManager::executeLastMacro()
{
    if (d->currentMacro)
        d->executeMacro(d->currentMacro);
}

bool MacroManager::executeMacro(const QString &name)
{
    // Don't execute macro while recording
    if (d->isRecording || !d->macros.contains(name))
        return false;

    Macro *macro = d->macros.value(name);
    if (!d->executeMacro(macro))
        return false;

    // Delete anonymous macro
    if (d->currentMacro && d->currentMacro->displayName().isEmpty())
        delete d->currentMacro;
    d->currentMacro = macro;
    return true;
}

void MacroManager::appendDirectory(const QString &directory)
{
    d->appendDirectory(directory);
    d->settings.directories.append(directory);
}

void MacroManager::removeDirectory(const QString &directory)
{
    d->removeDirectory(directory);
    d->settings.directories.removeAll(directory);
}

void MacroManager::setDefaultDirectory(const QString &directory)
{
    d->settings.defaultDirectory = directory;
}

void MacroManager::showSaveDialog(bool value)
{
    d->settings.showSaveDialog = value;
}

void MacroManager::deleteMacro(const QString &name)
{
    Macro *macro = d->macros.value(name);
    if (macro) {
        QString fileName = macro->fileName();
        d->removeMacro(name);
        d->settings.shortcutIds.remove(name);
        QFile::remove(fileName);
    }
}

const QMap<QString,Macro*> &MacroManager::macros() const
{
    return d->macros;
}

void MacroManager::saveSettings()
{
    d->settings.toSettings(Core::ICore::instance()->settings());
}

void MacroManager::registerMacroHandler(IMacroHandler *handler)
{
    d->handlers.prepend(handler);
}

MacroManager *MacroManager::instance()
{
    return m_instance;
}

void MacroManager::changeMacro(const QString &name, const QString &description, bool shortcut)
{
    if (!d->macros.contains(name))
        return;
    Macro *macro = d->macros.value(name);

    // Change description
    if (macro->description() != description)
        d->changeMacroDescription(macro, description);

    // Change shortcut
    if ((macro->shortcutId()==-1 && !shortcut)
            || (macro->shortcutId()>=0 && shortcut))
        return;

    if (shortcut) {
        d->settings.shortcutIds[name] = d->addShortcut(macro);
    } else {
        d->removeShortcut(macro);
        d->settings.shortcutIds.remove(name);
    }
}
