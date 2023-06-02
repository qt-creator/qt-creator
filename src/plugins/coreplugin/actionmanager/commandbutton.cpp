// Copyright (C) 2016 Konstantin Tokarev.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "commandbutton.h"
#include "actionmanager.h"
#include "command.h"

#include <utils/proxyaction.h>
#include <utils/qtcassert.h>

using namespace Core;
using namespace Utils;

/*!
    \class Core::CommandAction
    \inheaderfile coreplugin/actionmanager/commandbutton.h
    \inmodule QtCreator

    \brief The CommandAction class is an action associated with one of
    the registered Command objects.

    It shares the icon and text of the command.
    The tooltip of the action consists of toolTipBase property value and Command's
    key sequence which is automatically updated when user changes it.
 */

/*!
    \class Core::CommandButton
    \inheaderfile coreplugin/actionmanager/commandbutton.h
    \inmodule QtCreator

    \brief The CommandButton class is an action associated with one of
    the registered Command objects.

    The tooltip of the button consists of toolTipBase property value and Command's
    key sequence which is automatically updated when user changes it.
 */

/*!
    \internal
*/
CommandAction::CommandAction(QWidget *parent)
    : QAction(parent)
    , m_command(nullptr)
{
}

/*!
    \internal
*/
CommandAction::CommandAction(Id id, QWidget *parent)
    : QAction(parent)
    , m_command(nullptr)
{
    setCommandId(id);
}

/*!
    Sets the ID of the command associated with this tool button to \a id.
*/
void CommandAction::setCommandId(Id id)
{
    if (m_command)
        disconnect(m_command.data(),
                   &Command::keySequenceChanged,
                   this,
                   &CommandAction::updateToolTip);

    m_command = ActionManager::command(id);
    QTC_ASSERT(m_command, return);

    if (m_toolTipBase.isEmpty())
        m_toolTipBase = m_command->description();

    setIcon(m_command->action()->icon());
    setIconText(m_command->action()->iconText());
    setText(m_command->action()->text());

    updateToolTip();
    connect(m_command.data(), &Command::keySequenceChanged, this, &CommandAction::updateToolTip);
}

/*!
    The base tool tip that is extended with the command's shortcut.
    Defaults to the command's description.

    \sa Command::description()
*/
QString CommandAction::toolTipBase() const
{
    return m_toolTipBase;
}

/*!
    Sets the base tool tip that is extended with the command's shortcut to
    \a toolTipBase.

    \sa toolTipBase()
*/
void CommandAction::setToolTipBase(const QString &toolTipBase)
{
    m_toolTipBase = toolTipBase;
    updateToolTip();
}

void CommandAction::updateToolTip()
{
    if (m_command)
        setToolTip(Utils::ProxyAction::stringWithAppendedShortcut(m_toolTipBase,
                                                                  m_command->keySequence()));
}

/*!
    \internal
*/
CommandButton::CommandButton(QWidget *parent)
    : QToolButton(parent)
{}

/*!
    \internal
*/
CommandButton::CommandButton(Utils::Id id, QWidget *parent)
    : QToolButton(parent)
{
    setCommandId(id);
}

void CommandButton::setCommandId(Utils::Id id)
{
    if (m_command)
        disconnect(m_command.data(),
                   &Command::keySequenceChanged,
                   this,
                   &CommandButton::updateToolTip);

    m_command = ActionManager::command(id);
    QTC_ASSERT(m_command, return);

    if (m_toolTipBase.isEmpty())
        m_toolTipBase = m_command->description();

    updateToolTip();
    connect(m_command.data(), &Command::keySequenceChanged, this, &CommandButton::updateToolTip);
}

/*!
    The base tool tip that is extended with the command's shortcut.
    Defaults to the command's description.

    \sa Command::description()
*/
QString CommandButton::toolTipBase() const
{
    return m_toolTipBase;
}

/*!
    Sets the base tool tip that is extended with the command's shortcut to
    \a toolTipBase.

    \sa toolTipBase()
*/
void CommandButton::setToolTipBase(const QString &toolTipBase)
{
    m_toolTipBase = toolTipBase;
    updateToolTip();
}

void CommandButton::updateToolTip()
{
    if (m_command)
        setToolTip(Utils::ProxyAction::stringWithAppendedShortcut(m_toolTipBase,
                                                                  m_command->keySequence()));
}
