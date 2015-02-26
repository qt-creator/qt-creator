/****************************************************************************
**
** Copyright (C) 2015 Konstantin Tokarev.
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
        disconnect(m_command, SIGNAL(keySequenceChanged()), this, SLOT(updateToolTip()));

    m_command = ActionManager::command(id);

    if (m_toolTipBase.isEmpty())
        m_toolTipBase = m_command->description();

    updateToolTip();
    connect(m_command, SIGNAL(keySequenceChanged()), this, SLOT(updateToolTip()));
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
