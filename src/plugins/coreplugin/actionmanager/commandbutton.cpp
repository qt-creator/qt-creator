// Copyright (C) 2016 Konstantin Tokarev.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "commandbutton.h"
#include "actionmanager.h"
#include "command.h"


#include <utils/proxyaction.h>

using namespace Core;
using namespace Utils;

/*!
    \class Core::CommandButton
    \inheaderfile coreplugin/actionmanager/commandbutton.h
    \inmodule QtCreator

    \brief The CommandButton class is a tool button associated with one of
    the registered Command objects.

    Tooltip of this button consists of toolTipBase property value and Command's
    key sequence which is automatically updated when user changes it.
 */

/*!
    \property CommandButton::toolTipBase
    \brief The tool tip base for the command button.
*/

/*!
    \internal
*/
CommandButton::CommandButton(QWidget *parent)
    : QToolButton(parent)
    , m_command(nullptr)
{
}

/*!
    \internal
*/
CommandButton::CommandButton(Id id, QWidget *parent)
    : QToolButton(parent)
    , m_command(nullptr)
{
    setCommandId(id);
}

/*!
    Sets the ID of the command associated with this tool button to \a id.
*/
void CommandButton::setCommandId(Id id)
{
    if (m_command)
        disconnect(m_command.data(), &Command::keySequenceChanged, this, &CommandButton::updateToolTip);

    m_command = ActionManager::command(id);

    if (m_toolTipBase.isEmpty())
        m_toolTipBase = m_command->description();

    updateToolTip();
    connect(m_command.data(), &Command::keySequenceChanged, this, &CommandButton::updateToolTip);
}

QString CommandButton::toolTipBase() const
{
    return m_toolTipBase;
}

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
