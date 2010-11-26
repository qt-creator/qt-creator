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

#define QT_NO_CAST_FROM_ASCII

#include "lldbenginehost.h"

#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggerdialogs.h"
#include "debuggerplugin.h"
#include "debuggerstringutils.h"
#include "coreplugin/icore.h"

#include "breakhandler.h"
#include "breakpoint.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"
#include "threadshandler.h"
#include "debuggeragents.h"

#include <utils/qtcassert.h>
#include <QtCore/QDebug>
#include <QtCore/QProcess>
#include <QtCore/QFileInfo>
#include <QtCore/QThread>
#include <QtCore/QCoreApplication>
#include <QtNetwork/QLocalSocket>
#include <QtNetwork/QLocalServer>

namespace Debugger {
namespace Internal {

LLDBEngineHost::LLDBEngineHost(const DebuggerStartParameters &startParameters)
    :IPCEngineHost(startParameters)
{
    QLocalServer *s = new QLocalServer(this);
    s->removeServer (QLatin1String("/tmp/qtcreator-debuggeripc"));
    s->listen (QLatin1String("/tmp/qtcreator-debuggeripc"));

    m_guestp = new QProcess(this);
    m_guestp->setProcessChannelMode(QProcess::ForwardedChannels);

    connect(m_guestp, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished (int, QProcess::ExitStatus)));

    QString a(Core::ICore::instance()->resourcePath() + QLatin1String("/qtcreator-lldb"));
    m_guestp->start(a,QStringList());

    if (!m_guestp->waitForStarted()) {
        showStatusMessage(tr("lldb failed to start"));
        notifyEngineIll();
        return;
    }

    s->waitForNewConnection(-1);
    QLocalSocket *f = s->nextPendingConnection();
    s->close(); // wtf race in accept
    setGuestDevice(f);
}

LLDBEngineHost::~LLDBEngineHost()
{
    disconnect(m_guestp, SIGNAL(finished(int, QProcess::ExitStatus)),
            this, SLOT(finished (int, QProcess::ExitStatus)));
    m_guestp->terminate();
    m_guestp->kill();
}

void LLDBEngineHost::finished(int, QProcess::ExitStatus)
{
    showStatusMessage(QLatin1String("lldb crashed"));
    notifyEngineIll();
}

DebuggerEngine *createLLDBEngine(const DebuggerStartParameters &startParameters)
{
    return new LLDBEngineHost(startParameters);
}

} // namespace Internal
} // namespace Debugger
