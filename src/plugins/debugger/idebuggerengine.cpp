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

#include "idebuggerengine.h"
#include "debuggermanager.h"


namespace Debugger {
namespace Internal {

void IDebuggerEngine::fetchMemory(MemoryViewAgent *, QObject *,
        quint64 addr, quint64 length)
{
    Q_UNUSED(addr);
    Q_UNUSED(length);
}

void IDebuggerEngine::setRegisterValue(int regnr, const QString &value)
{
    Q_UNUSED(regnr);
    Q_UNUSED(value);
}

bool IDebuggerEngine::checkConfiguration(int toolChain,
    QString *errorMessage, QString *settingsPage) const
{
    Q_UNUSED(toolChain);
    Q_UNUSED(errorMessage);
    Q_UNUSED(settingsPage);
    return true;
}

void IDebuggerEngine::showDebuggerInput(int channel, const QString &msg)
{
    m_manager->showDebuggerInput(channel, msg);
}

void IDebuggerEngine::showDebuggerOutput(int channel, const QString &msg)
{
    m_manager->showDebuggerOutput(channel, msg);
}

} // namespace Internal
} // namespace Debugger

