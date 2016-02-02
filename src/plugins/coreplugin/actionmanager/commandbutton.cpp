/****************************************************************************
**
** Copyright (C) 2016 Konstantin Tokarev.
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

#include "commandbutton.h"
#include "actionmanager.h"
#include "command.h"
#include "../id.h"

#include <utils/proxyaction.h>

using namespace Core;

/*!
    \class Core::CommandButton

    \brief The CommandButton class is a tool button associated with one of
    the registered Command objects.

    Tooltip of this button consists of toolTipBase property value and Command's
    key sequence which is automatically updated when user changes it.
 */

CommandButton::CommandButton(QWidget *parent)
    : QToolButton(parent)
    , m_command(0)
{
}

CommandButton::CommandButton(Id id, QWidget *parent)
    : QToolButton(parent)
    , m_command(0)
{
    setCommandId(id);
}

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
