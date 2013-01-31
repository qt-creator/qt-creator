/**************************************************************************
**
** Copyright (c) 2013 Nicolas Arnaud-Cormos
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "macromanager.h"

#include "macrosconstants.h"
#include "macroevent.h"
#include "macro.h"
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
#include <coreplugin/id.h>
#include <coreplugin/icontext.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSettings>
#include <QSignalMapper>
#include <QList>

#include <QShortcut>
#include <QAction>
#include <QFileDialog>
#include <QMessageBox>

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
    QMap<QString, Macro *> macros;
    Macro *currentMacro;
    bool isRecording;

    QList<IMacroHandler*> handlers;

    QSignalMapper *mapper;

    ActionMacroHandler *actionHandler;
    TextEditorMacroHandler *textEditorHandler;
    FindMacroHandler *findHandler;

    void initialize();
    void addMacro(Macro *macro);
    void removeMacro(const QString &name);
    void changeMacroDescription(Macro *macro, const QString &description);

    bool executeMacro(Macro *macro);
    void showSaveDialog();
};

MacroManager::MacroManagerPrivate::MacroManagerPrivate(MacroManager *qq):
    q(qq),
    currentMacro(0),
    isRecording(false),
    mapper(new QSignalMapper(qq))
{
    connect(mapper, SIGNAL(mapped(QString)), q, SLOT(executeMacro(QString)));

    // Load existing macros
    initialize();

    actionHandler = new ActionMacroHandler;
    textEditorHandler = new TextEditorMacroHandler;
    findHandler = new FindMacroHandler;
}

void MacroManager::MacroManagerPrivate::initialize()
{
    macros.clear();
    QDir dir(q->macrosDirectory());
    QStringList filter;
    filter << QLatin1String("*.") + QLatin1String(Constants::M_EXTENSION);
    QStringList files = dir.entryList(filter, QDir::Files);

    foreach (const QString &name, files) {
        QString fileName = dir.absolutePath() + QLatin1Char('/') + name;
        Macro *macro = new Macro;
        if (macro->loadHeader(fileName))
            addMacro(macro);
        else
            delete macro;
    }
}

static Core::Id makeId(const QString &name)
{
    return Core::Id::fromString(QLatin1String(Constants::PREFIX_MACRO) + name);
}

void MacroManager::MacroManagerPrivate::addMacro(Macro *macro)
{
    // Add sortcut
    Core::Context context(TextEditor::Constants::C_TEXTEDITOR);
    QShortcut *shortcut = new QShortcut(Core::ICore::mainWindow());
    shortcut->setWhatsThis(macro->description());
    Core::ActionManager::registerShortcut(shortcut, makeId(macro->displayName()), context);
    connect(shortcut, SIGNAL(activated()), mapper, SLOT(map()));
    mapper->setMapping(shortcut, macro->displayName());

    // Add macro to the map
    macros[macro->displayName()] = macro;
}

void MacroManager::MacroManagerPrivate::removeMacro(const QString &name)
{
    if (!macros.contains(name))
        return;
    // Remove shortcut
    Core::ActionManager::unregisterShortcut(makeId(name));

    // Remove macro from the map
    Macro *macro = macros.take(name);
    delete macro;
}

void MacroManager::MacroManagerPrivate::changeMacroDescription(Macro *macro, const QString &description)
{
    if (!macro->load())
        return;
    macro->setDescription(description);
    macro->save(macro->fileName(), Core::ICore::mainWindow());

    // Change shortcut what's this
    Core::Command *command = Core::ActionManager::command(makeId(macro->displayName()));
    if (command && command->shortcut())
        command->shortcut()->setWhatsThis(description);
}

bool MacroManager::MacroManagerPrivate::executeMacro(Macro *macro)
{
    bool error = !macro->load();
    foreach (const MacroEvent &macroEvent, macro->events()) {
        if (error)
            break;
        foreach (IMacroHandler *handler, handlers) {
            if (handler->canExecuteEvent(macroEvent)) {
                if (!handler->executeEvent(macroEvent))
                    error = true;
                break;
            }
        }
    }

    if (error) {
        QMessageBox::warning(Core::ICore::mainWindow(),
                             tr("Playing Macro"),
                             tr("An error occurred while replaying the macro, execution stopped."));
    }

    // Set the focus back to the editor
    // TODO: is it really needed??
    if (Core::IEditor *current = Core::EditorManager::currentEditor())
        current->widget()->setFocus(Qt::OtherFocusReason);

    return !error;
}

void MacroManager::MacroManagerPrivate::showSaveDialog()
{
    QWidget *mainWindow = Core::ICore::mainWindow();
    SaveDialog dialog(mainWindow);
    if (dialog.exec()) {
        if (dialog.name().isEmpty())
            return;

        // Save in the resource path
        QString fileName = q->macrosDirectory() + QLatin1Char('/') + dialog.name()
                           + QLatin1Char('.') + QLatin1String(Constants::M_EXTENSION);
        currentMacro->setDescription(dialog.description());
        currentMacro->save(fileName, mainWindow);
        addMacro(currentMacro);
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

void MacroManager::startMacro()
{
    d->isRecording = true;
    // Delete anonymous macro
    if (d->currentMacro && d->currentMacro->displayName().isEmpty())
        delete d->currentMacro;
    d->currentMacro = new Macro;

    Core::ActionManager::command(Constants::START_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::END_MACRO)->action()->setEnabled(true);
    Core::ActionManager::command(Constants::EXECUTE_LAST_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::SAVE_LAST_MACRO)->action()->setEnabled(false);
    foreach (IMacroHandler *handler, d->handlers)
        handler->startRecording(d->currentMacro);

    QString endShortcut = Core::ActionManager::command(Constants::END_MACRO)->defaultKeySequence().toString();
    QString executeShortcut = Core::ActionManager::command(Constants::EXECUTE_LAST_MACRO)->defaultKeySequence().toString();
    QString help = tr("Macro mode. Type \"%1\" to stop recording and \"%2\" to play it")
        .arg(endShortcut).arg(executeShortcut);
    Core::EditorManager::instance()->showEditorStatusBar(
                QLatin1String(Constants::M_STATUS_BUFFER),
                help,
                tr("Stop Recording Macro"), this, SLOT(endMacro()));
}

void MacroManager::endMacro()
{
    Core::EditorManager::instance()->hideEditorStatusBar(QLatin1String(Constants::M_STATUS_BUFFER));

    Core::ActionManager::command(Constants::START_MACRO)->action()->setEnabled(true);
    Core::ActionManager::command(Constants::END_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::EXECUTE_LAST_MACRO)->action()->setEnabled(true);
    Core::ActionManager::command(Constants::SAVE_LAST_MACRO)->action()->setEnabled(true);
    foreach (IMacroHandler *handler, d->handlers)
        handler->endRecordingMacro(d->currentMacro);

    d->isRecording = false;
}

void MacroManager::executeLastMacro()
{
    if (!d->currentMacro)
        return;

    // make sure the macro doesn't accidentally invoke a macro action
    Core::ActionManager::command(Constants::START_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::END_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::EXECUTE_LAST_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::SAVE_LAST_MACRO)->action()->setEnabled(false);

    d->executeMacro(d->currentMacro);

    Core::ActionManager::command(Constants::START_MACRO)->action()->setEnabled(true);
    Core::ActionManager::command(Constants::END_MACRO)->action()->setEnabled(false);
    Core::ActionManager::command(Constants::EXECUTE_LAST_MACRO)->action()->setEnabled(true);
    Core::ActionManager::command(Constants::SAVE_LAST_MACRO)->action()->setEnabled(true);
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

    Core::ActionManager::command(Constants::SAVE_LAST_MACRO)->action()->setEnabled(true);

    return true;
}

void MacroManager::deleteMacro(const QString &name)
{
    Macro *macro = d->macros.value(name);
    if (macro) {
        QString fileName = macro->fileName();
        d->removeMacro(name);
        QFile::remove(fileName);
    }
}

const QMap<QString,Macro*> &MacroManager::macros() const
{
    return d->macros;
}

void MacroManager::registerMacroHandler(IMacroHandler *handler)
{
    d->handlers.prepend(handler);
}

MacroManager *MacroManager::instance()
{
    return m_instance;
}

void MacroManager::changeMacro(const QString &name, const QString &description)
{
    if (!d->macros.contains(name))
        return;
    Macro *macro = d->macros.value(name);

    // Change description
    if (macro->description() != description)
        d->changeMacroDescription(macro, description);
}

void Macros::MacroManager::saveLastMacro()
{
    if (d->currentMacro->events().count())
        d->showSaveDialog();
}

QString Macros::MacroManager::macrosDirectory() const
{
    const QString &path =
        Core::ICore::userResourcePath() + QLatin1String("/macros");
    if (QFile::exists(path) || QDir().mkpath(path))
        return path;
    return QString();
}
