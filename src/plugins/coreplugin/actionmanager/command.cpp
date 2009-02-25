/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include <QtCore/QDebug>
#include <QtGui/QAction>
#include <QtGui/QShortcut>

#include "command_p.h"

/*!
    \class Core::Command
    \mainclass
    \ingroup qwb

    \brief The class...

    The Command interface...
*/

/*!
    \enum Command::CommandType
*/

/*!
    \enum Command::CommandAttribute
*/

/*!
    \fn void Command::setCategory(const QString &name)

    Sets the category to \a name.
*/

/*!
    \fn virtual void Command::setDefaultKeySequence(const QKeySequence &key)
*/

/*!
    \fn virtual int Command::id() const
*/

/*!
    \fn virtual CommandType Command::type() const
*/

/*!
    \fn virtual QAction *Command::action() const
*/

/*!
    \fn virtual QShortcut *Command::shortcut() const
*/

/*!
    \fn virtual void Command::setAttribute(CommandAttribute attr)
*/

/*!
    \fn virtual void Command::removeAttribute(CommandAttribute attr)
*/

/*!
    \fn virtual bool Command::hasAttribute(CommandAttribute attr) const
*/

/*!
    \fn virtual bool Command::isActive() const
*/

/*!
    \fn virtual Command::~Command()
*/

using namespace Core::Internal;

/*!
    \class CommandPrivate
    \inheaderfile command_p.h
    \internal
*/

CommandPrivate::CommandPrivate(CommandType type, int id)
    : m_type(type), m_id(id)
{
}

void CommandPrivate::setStateFlags(int state)
{
    m_type |= (state & CS_Mask);
}

int CommandPrivate::stateFlags() const
{
    return (m_type & CS_Mask);
}

void CommandPrivate::setCategory(const QString &name)
{
    m_category = name;
}

QString CommandPrivate::category() const
{
    if (m_category.isEmpty())
        return QObject::tr("Other");
    return m_category;
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

CommandPrivate::CommandType CommandPrivate::type() const
{
    return (CommandType)(m_type & CT_Mask);
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
    m_type |= attr;
}

void CommandPrivate::removeAttribute(CommandAttribute attr)
{
    m_type &= ~attr;
}

bool CommandPrivate::hasAttribute(CommandAttribute attr) const
{
    return (m_type & attr);
}

QString CommandPrivate::stringWithAppendedShortcut(const QString &str) const
{
    return QString("%1 <span style=\"color: gray; font-size: small\">%2</span>").arg(str).arg(
            keySequence().toString(QKeySequence::NativeText));
}

// ---------- Shortcut ------------

/*!
    \class Shortcut
    \ingroup qwb
*/

/*!
    ...
*/
Shortcut::Shortcut(int id)
    : CommandPrivate(CT_Shortcut, id), m_shortcut(0)
{

}

/*!
    ...
*/
QString Shortcut::name() const
{
    if (!m_shortcut)
        return QString();

    return m_shortcut->whatsThis();
}

/*!
    ...
*/
void Shortcut::setShortcut(QShortcut *shortcut)
{
    m_shortcut = shortcut;
}

/*!
    ...
*/
QShortcut *Shortcut::shortcut() const
{
    return m_shortcut;
}

/*!
    ...
*/
void Shortcut::setContext(const QList<int> &context)
{
    m_context = context;
}

/*!
    ...
*/
QList<int> Shortcut::context() const
{
    return m_context;
}

/*!
    ...
*/
void Shortcut::setDefaultKeySequence(const QKeySequence &key)
{
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

/*!
    ...
*/
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

/*!
    ...
*/
bool Shortcut::isActive() const
{
    return m_shortcut->isEnabled();
}

// ---------- Action ------------

/*!
    \class Action
    \ingroup qwb
*/

/*!
    ...
*/
Action::Action(CommandType type, int id)
    : CommandPrivate(type, id), m_action(0)
{

}

/*!
    ...
*/
QString Action::name() const
{
    if (!m_action)
        return QString();

    return m_action->text();
}

/*!
    ...
*/
void Action::setAction(QAction *action)
{
    m_action = action;
    if (m_action) {
        m_action->setParent(this);
        m_toolTip = m_action->toolTip();
    }
}

/*!
    ...
*/
QAction *Action::action() const
{
    return m_action;
}

/*!
    ...
*/
void Action::setLocations(const QList<CommandLocation> &locations)
{
    m_locations = locations;
}

/*!
    ...
*/
QList<CommandLocation> Action::locations() const
{
    return m_locations;
}

/*!
    ...
*/
void Action::setDefaultKeySequence(const QKeySequence &key)
{
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

// ---------- OverrideableAction ------------

/*!
    \class OverrideableAction
    \ingroup qwb
*/

/*!
    ...
*/
OverrideableAction::OverrideableAction(int id)
    : Action(CT_OverridableAction, id), m_currentAction(0), m_active(false),
    m_contextInitialized(false)
{
}

/*!
    ...
*/
void OverrideableAction::setAction(QAction *action)
{
    Action::setAction(action);
}

/*!
    ...
*/
bool OverrideableAction::setCurrentContext(const QList<int> &context)
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
    if (hasAttribute(CA_Hide))
        m_action->setVisible(false);
    m_action->setEnabled(false);
    m_active = false;
    return false;
}

/*!
    ...
*/
void OverrideableAction::addOverrideAction(QAction *action, const QList<int> &context)
{
    if (context.isEmpty()) {
        m_contextActionMap.insert(0, action);
    } else {
        for (int i=0; i<context.size(); ++i) {
            int k = context.at(i);
            if (m_contextActionMap.contains(k))
                qWarning() << QString("addOverrideAction: action already registered for context when registering '%1'").arg(action->text());
            m_contextActionMap.insert(k, action);
        }
    }
}

/*!
    ...
*/
void OverrideableAction::actionChanged()
{
    if (hasAttribute(CA_UpdateIcon)) {
        m_action->setIcon(m_currentAction->icon());
        m_action->setIconText(m_currentAction->iconText());
    }
    if (hasAttribute(CA_UpdateText)) {
        m_action->setText(m_currentAction->text());
        m_toolTip = m_currentAction->toolTip();
        updateToolTipWithKeySequence();
        m_action->setStatusTip(m_currentAction->statusTip());
        m_action->setWhatsThis(m_currentAction->whatsThis());
    }

    bool block = m_action->blockSignals(true);
    m_action->setChecked(m_currentAction->isChecked());
    m_action->blockSignals(block);

    m_action->setEnabled(m_currentAction->isEnabled());
    m_action->setVisible(m_currentAction->isVisible());
}

/*!
    ...
*/
bool OverrideableAction::isActive() const
{
    return m_active;
}
