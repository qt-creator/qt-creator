/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Author: Nicolas Arnaud-Cormos, KDAB (nicolas.arnaud-cormos@kdab.com)
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

#include "memcheckengine.h"
#include "memchecktool.h"
#include "valgrindsettings.h"
#include "xmlprotocol/error.h"
#include "xmlprotocol/status.h"

#include <debugger/debuggerkitinformation.h>
#include <debugger/debuggerstartparameters.h>
#include <debugger/debuggerruncontrol.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/qtcassert.h>

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Valgrind::XmlProtocol;

namespace Valgrind {
namespace Internal {

class LocalAddressFinder : public RunWorker
{
public:
    LocalAddressFinder(RunControl *runControl, QHostAddress *localServerAddress)
        : RunWorker(runControl), connection(device()->sshParameters())
    {
        connect(&connection, &QSsh::SshConnection::connected, this, [this, localServerAddress] {
            *localServerAddress = connection.connectionInfo().localAddress;
            reportStarted();
        });
        connect(&connection, &QSsh::SshConnection::error, this, [this] {
            reportFailure();
        });
    }

    void start() override
    {
        connection.connectToHost();
    }

    QSsh::SshConnection connection;
};

MemcheckToolRunner::MemcheckToolRunner(RunControl *runControl, bool withGdb)
    : ValgrindToolRunner(runControl),
      m_withGdb(withGdb),
      m_localServerAddress(QHostAddress::LocalHost)
{
    setDisplayName("MemcheckToolRunner");
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::error,
            this, &MemcheckToolRunner::parserError);
    connect(m_runner.parser(), &XmlProtocol::ThreadedParser::suppressionCount,
            this, &MemcheckToolRunner::suppressionCount);

    if (withGdb) {
        connect(&m_runner, &ValgrindRunner::valgrindStarted,
                this, &MemcheckToolRunner::startDebugger);
        connect(&m_runner, &ValgrindRunner::logMessageReceived,
                this, &MemcheckToolRunner::appendLog);
//        m_runner.disableXml();
    } else {
        connect(m_runner.parser(), &XmlProtocol::ThreadedParser::internalError,
                this, &MemcheckToolRunner::internalParserError);
    }

    // We need a real address to connect to from the outside.
    if (device()->type() != ProjectExplorer::Constants::DESKTOP_DEVICE_TYPE)
        addDependency(new LocalAddressFinder(runControl, &m_localServerAddress));
}

QString MemcheckToolRunner::progressTitle() const
{
    return tr("Analyzing Memory");
}

void MemcheckToolRunner::start()
{
    m_runner.setLocalServerAddress(m_localServerAddress);
    ValgrindToolRunner::start();
}

void MemcheckToolRunner::stop()
{
    disconnect(m_runner.parser(), &ThreadedParser::internalError,
               this, &MemcheckToolRunner::internalParserError);
    ValgrindToolRunner::stop();
}

QStringList MemcheckToolRunner::toolArguments() const
{
    QStringList arguments = {"--tool=memcheck", "--gen-suppressions=all"};

    QTC_ASSERT(m_settings, return arguments);

    if (m_settings->trackOrigins())
        arguments << "--track-origins=yes";

    if (m_settings->showReachable())
        arguments << "--show-reachable=yes";

    QString leakCheckValue;
    switch (m_settings->leakCheckOnFinish()) {
    case ValgrindBaseSettings::LeakCheckOnFinishNo:
        leakCheckValue = "no";
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishYes:
        leakCheckValue = "full";
        break;
    case ValgrindBaseSettings::LeakCheckOnFinishSummaryOnly:
    default:
        leakCheckValue = "summary";
        break;
    }
    arguments << "--leak-check=" + leakCheckValue;

    foreach (const QString &file, m_settings->suppressionFiles())
        arguments << QString("--suppressions=%1").arg(file);

    arguments << QString("--num-callers=%1").arg(m_settings->numCallers());

    if (m_withGdb)
        arguments << "--vgdb=yes" << "--vgdb-error=0";

    return arguments;
}

QStringList MemcheckToolRunner::suppressionFiles() const
{
    return m_settings->suppressionFiles();
}

void MemcheckToolRunner::startDebugger(qint64 valgrindPid)
{
    Debugger::DebuggerStartParameters sp;
    sp.inferior = runnable().as<StandardRunnable>();
    sp.startMode = Debugger::AttachToRemoteServer;
    sp.displayName = QString("VGdb %1").arg(valgrindPid);
    sp.remoteChannel = QString("| vgdb --pid=%1").arg(valgrindPid);
    sp.useContinueInsteadOfRun = true;
    sp.expectedSignals.append("SIGTRAP");

    auto gdbWorker = new Debugger::DebuggerRunTool(runControl());
    gdbWorker->setStartParameters(sp);
    gdbWorker->initiateStart();
    connect(runControl(), &RunControl::stopped, gdbWorker, &RunControl::deleteLater);
}

void MemcheckToolRunner::appendLog(const QByteArray &data)
{
    appendMessage(QString::fromUtf8(data), Utils::StdOutFormat);
}

} // namespace Internal
} // namespace Valgrind
