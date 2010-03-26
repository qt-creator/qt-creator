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

#include "command_p.h"

#include "icore.h"
#include "uniqueidmanager.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QMainWindow>

/*!
    \class Core::Command
    \mainclass

    \brief The class Command represents an action like a menu item, tool button, or shortcut.
    You don't create Command objects directly, instead use \l{ActionManager::registerAction()}
    to register an action and retrieve a Command. The Command object represents the user visible
    action and its properties. If multiple actions are registered with the same ID (but
    different contexts) the returned Command is the shared one between these actions.

    A Command has two basic properties: A default shortcut and a default text. The default
    shortcut is a key sequence that the user can use to trigger the active action that
    the Command represents. The default text is e.g. used for representing the Command
    in the keyboard shortcut preference pane. If the default text is empty, the text
    of the visible action is used.

    The user visible action is updated to represent the state of the active action (if any).
    For performance reasons only the enabled and visible state are considered by default though.
    You can tell a Command to also update the actions icon and text by setting the
    corresponding \l{Command::CommandAttribute}{attribute}.

    If there is no active action, the default behavior of the visible action is to be disabled.
    You can change that behavior to make the visible action hide instead via the Command's
    \l{Command::CommandAttribute}{attributes}.
*/

/*!
    \enum Command::CommandAttribute
    Defines how the user visible action is updated when the active action changes.
    The default is to update the enabled and visible state, and to disable the
    user visible action when there is no active action.
    \omitvalue CA_Mask
    \value CA_UpdateText
        Also update the actions text.
    \value CA_UpdateIcon
        Also update the actions icon.
    \value CA_Hide
        When there is no active action, hide the user "visible" action, instead of just
        disabling it.
    \value CA_NonConfigureable
        Flag to indicate that the keyboard shortcut of this Command should not be
        configurable by the user.
*/

/*!
    \fn void Command::setDefaultKeySequence(const QKeySequence &key)
    Set the default keyboard shortcut that can be used to activate this command to \a key.
    This is used if the user didn't customize the shortcut, or resets the shortcut
    to the default one.
*/

/*!
    \fn void Command::defaultKeySequence() const
    Returns the default keyboard shortcut that can be used to activate this command.
    \sa setDefaultKeySequence()
*/

/*!
    \fn void Command::keySequenceChanged()
    Sent when the keyboard shortcut assigned to this Command changes, e.g.
    when the user sets it in the keyboard shortcut settings dialog.
*/

/*!
    \fn QKeySequence Command::keySequence() const
    Returns the current keyboard shortcut assigned to this Command.
    \sa defaultKeySequence()
*/

/*!
    \fn void Command::setKeySequence(const QKeySequence &key)
    \internal
*/

/*!
    \fn void Command::setDefaultText(const QString &text)
    Set the \a text that is used to represent the Command in the
    keyboard shortcut settings dialog. If you don't set this,
    the current text from the user visible action is taken (which
    is ok in many cases).
*/

/*!
    \fn QString Command::defaultText() const
    Returns the text that is used to present this Command to the user.
    \sa setDefaultText()
*/

/*!
    \fn int Command::id() const
    \internal
*/

/*!
    \fn QString Command::stringWithAppendedShortcut(const QString &string) const
    Returns the \a string with an appended representation of the keyboard shortcut
    that is currently assigned to this Command.
*/

/*!
    \fn QAction *Command::action() const
    Returns the user visible action for this Command.
    If the Command represents a shortcut, it returns null.
    Use this action to put it on e.g. tool buttons. The action
    automatically forwards trigger and toggle signals to the
    action that is currently active for this Command.
    It also shows the current keyboard shortcut in its
    tool tip (in addition to the tool tip of the active action)
    and gets disabled/hidden when there is
    no active action for the current context.
*/

/*!
    \fn QShortcut *Command::shortcut() const
    Returns the shortcut for this Command.
    If the Command represents an action, it returns null.
*/

/*!
    \fn void Command::setAttribute(CommandAttribute attribute)
    Add the \a attribute to the attributes of this Command.
    \sa CommandAttribute
    \sa removeAttribute()
    \sa hasAttribute()
*/

/*!
    \fn void Command::removeAttribute(CommandAttribute attribute)
    Remove the \a attribute from the attributes of this Command.
    \sa CommandAttribute
    \sa setAttribute()
*/

/*!
    \fn bool Command::hasAttribute(CommandAttribute attribute) const
    Returns if the Command has the \a attribute set.
    \sa CommandAttribute
    \sa removeAttribute()
    \sa setAttribute()
*/

/*!
    \fn bool Command::isActive() const
    Returns if the Command has an active action/shortcut for the current
    context.
*/

/*!
    \fn Command::~Command()
    \internal
*/

using namespace Core::Internal;

/*!
    \class CommandPrivate
    \internal
*/

CommandPrivate::CommandPrivate(int id)
    : m_attributes(0), m_id(id)
{
}

void CommandPrivate::setDefaultKeySequence(const QKeySequence &key)
{
    m_defaultKey = key;
}

QKeySequence CommandPrivate::defaultKeySequence() const
{
    return m_defaultKey;
}

void CommandPrivate::setDefaultText(const QString &text)
{
    m_defaultText = text;
}

QString CommandPrivate::defaultText() const
{
    return m_defaultText;
}

int CommandPrivate::id() const
{
    return m_id;
}

QAction *CommandPrivate::action() const
{
    return 0;
}

QShortcut *CommandPrivate::shortcut() const
{
    return 0;
}

void CommandPrivate::setAttribute(CommandAttribute attr)
{
    m_attributes |= attr;
}

void CommandPrivate::removeAttribute(CommandAttribute attr)
{
    m_attributes &= ~attr;
}

bool CommandPrivate::hasAttribute(CommandAttribute attr) const
{
    return (m_attributes & attr);
}

QString CommandPrivate::stringWithAppendedShortcut(const QString &str) const
{
    return QString("%1 <span style=\"color: gray; font-size: small\">%2</span>").arg(str).arg(
            keySequence().toString(QKeySequence::NativeText));
}

// ---------- Shortcut ------------

/*!
    \class Shortcut
    \internal
*/

Shortcut::Shortcut(int id)
    : CommandPrivate(id), m_shortcut(0)
{

}

QString Shortcut::name() const
{
    if (!m_shortcut)
        return QString();

    return m_shortcut->whatsThis();
}

void Shortcut::setShortcut(QShortcut *shortcut)
{
    m_shortcut = shortcut;
}

QShortcut *Shortcut::shortcut() const
{
    return m_shortcut;
}

void Shortcut::setContext(const QList<int> &context)
{
    m_context = context;
}

QList<int> Shortcut::context() const
{
    return m_context;
}

void Shortcut::setDefaultKeySequence(const QKeySequence &key)
{
    if (m_shortcut->key().isEmpty())
        setKeySequence(key);
    CommandPrivate::setDefaultKeySequence(key);
}

void Shortcut::setKeySequence(const QKeySequence &key)
{
    m_shortcut->setKey(key);
    emit keySequenceChanged();
}

QKeySequence Shortcut::keySequence() const
{
    return m_shortcut->key();
}

void Shortcut::setDefaultText(const QString &text)
{
    m_defaultText = text;
}

QString Shortcut::defaultText() const
{
    return m_defaultText;
}

bool Shortcut::setCurrentContext(const QList<int> &context)
{
    foreach (int ctxt, m_context) {
        if (context.contains(ctxt)) {
            m_shortcut->setEnabled(true);
            return true;
        }
    }
    m_shortcut->setEnabled(false);
    return false;
}

bool Shortcut::isActive() const
{
    return m_shortcut->isEnabled();
}

// ---------- Action ------------

/*!
  \class Action
  \internal
*/
Action::Action(int id)
    : CommandPrivate(id),
    m_action(0),
    m_currentAction(0),
    m_active(false),
    m_contextInitialized(false)
{

}

QString Action::name() const
{
    if (!m_action)
        return QString();

    return m_action->text();
}

void Action::setAction(QAction *action)
{
    m_action = action;
    if (m_action) {
        m_action->setParent(this);
        m_toolTip = m_action->toolTip();
    }
}

QAction *Action::action() const
{
    return m_action;
}

void Action::setLocations(const QList<CommandLocation> &locations)
{
    m_locations = locations;
}

QList<CommandLocation> Action::locations() const
{
    return m_locations;
}

void Action::setDefaultKeySequence(const QKeySequence &key)
{
    if (m_action->shortcut().isEmpty())
        setKeySequence(key);
    CommandPrivate::setDefaultKeySequence(key);
}

void Action::setKeySequence(const QKeySequence &key)
{
    m_action->setShortcut(key);
    updateToolTipWithKeySequence();
    emit keySequenceChanged();
}

void Action::updateToolTipWithKeySequence()
{
    if (m_action->shortcut().isEmpty())
        m_action->setToolTip(m_toolTip);
    else
        m_action->setToolTip(stringWithAppendedShortcut(m_toolTip));
}

QKeySequence Action::keySequence() const
{
    return m_action->shortcut();
}

bool Action::setCurrentContext(const QList<int> &context)
{
    m_context = context;

    QAction *oldAction = m_currentAction;
    m_currentAction = 0;
    for (int i = 0; i < m_context.size(); ++i) {
        if (QAction *a = m_contextActionMap.value(m_context.at(i), 0)) {
            m_currentAction = a;
            break;
        }
    }

    if (m_currentAction == oldAction && m_contextInitialized)
        return true;
    m_contextInitialized = true;

    if (oldAction) {
        disconnect(oldAction, SIGNAL(changed()), this, SLOT(actionChanged()));
        disconnect(m_action, SIGNAL(triggered(bool)), oldAction, SIGNAL(triggered(bool)));
        disconnect(m_action, SIGNAL(toggled(bool)), oldAction, SLOT(setChecked(bool)));
    }
    if (m_currentAction) {
        connect(m_currentAction, SIGNAL(changed()), this, SLOT(actionChanged()));
        // we want to avoid the toggling semantic on slot trigger(), so we just connect the signals
        connect(m_action, SIGNAL(triggered(bool)), m_currentAction, SIGNAL(triggered(bool)));
        // we need to update the checked state, so we connect to setChecked slot, which also fires a toggled signal
        connect(m_action, SIGNAL(toggled(bool)), m_currentAction, SLOT(setChecked(bool)));
        actionChanged();
        m_active = true;
        return true;
    }
    // no active/delegate action, "visible" action is not enabled/visible
    if (hasAttribute(CA_Hide))
        m_action->setVisible(false);
    m_action->setEnabled(false);
    m_active = false;
    return false;
}

static inline QString msgActionWarning(QAction *newAction, int k, QAction *oldAction)
{
    QString msg;
    QTextStream str(&msg);
    str << "addOverrideAction " << newAction->objectName() << '/' << newAction->text()
         << ": Action ";
    if (oldAction)
        str << oldAction->objectName() << '/' << oldAction->text();
    str << " is already registered for context " << k << ' '
        << Core::ICore::instance()->uniqueIDManager()->stringForUniqueIdentifier(k)
        << '.';
    return msg;
}

void Action::addOverrideAction(QAction *action, const QList<int> &context)
{
    if (context.isEmpty()) {
        m_contextActionMap.insert(0, action);
    } else {
        for (int i=0; i<context.size(); ++i) {
            int k = context.at(i);
            if (m_contextActionMap.contains(k))
                qWarning("%s", qPrintable(msgActionWarning(action, k, m_contextActionMap.value(k, 0))));
            m_contextActionMap.insert(k, action);
        }
    }
}

void Action::actionChanged()
{
    if (hasAttribute(CA_UpdateIcon)) {
        m_action->setIcon(m_currentAction->icon());
        m_action->setIconText(m_currentAction->iconText());
#ifndef Q_WS_MAC
        m_action->setIconVisibleInMenu(m_currentAction->isIconVisibleInMenu());
#endif
    }
    if (hasAttribute(CA_UpdateText)) {
        m_action->setText(m_currentAction->text());
        m_toolTip = m_currentAction->toolTip();
        updateToolTipWithKeySequence();
        m_action->setStatusTip(m_currentAction->statusTip());
        m_action->setWhatsThis(m_currentAction->whatsThis());
    }

    m_action->setCheckable(m_currentAction->isCheckable());
    bool block = m_action->blockSignals(true);
    m_action->setChecked(m_currentAction->isChecked());
    m_action->blockSignals(block);

    m_action->setEnabled(m_currentAction->isEnabled());
    m_action->setVisible(m_currentAction->isVisible());
}

bool Action::isActive() const
{
    return m_active;
}
