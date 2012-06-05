/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2012 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "codagdbadapter.h"

#include "debuggerstartparameters.h"
#include "codadevice.h"
#include "codautils.h"
#include "gdbmi.h"
#include "hostutils.h"

#include "symbiandevicemanager.h"

#include "registerhandler.h"
#include "threadshandler.h"
#include "debuggercore.h"
#include "debuggeractions.h"
#include "debuggerstringutils.h"
#include "watchutils.h"
#ifndef STANDALONE_RUNNER
#include "gdbengine.h"
#endif

#include <utils/qtcassert.h>
#include <utils/savedaction.h>
#include <utils/qtcprocess.h>

#include <QTimer>
#include <QDir>
#include <QTcpServer>
#include <QTcpSocket>

#ifndef Q_OS_WIN
#  include <sys/types.h>
#  include <unistd.h>
#endif

#define CB(callback) \
    static_cast<GdbEngine::AdapterCallback>(&CodaGdbAdapter::callback), \
    STRINGIFY(callback)

enum { debug = 0 };

/* Libraries we want to be notified about (pending introduction of a 'notify all'
 * setting in CODA, Bug #11842 */
static const char *librariesC[] = {
"pipelib.ldd", "rpipe.dll", "libc.dll",
"libdl.dll", "libm.dll", "libpthread.dll",
"libssl.dll", "libz.dll", "libzcore.dll", "libstdcpp.dll",
"sqlite3.dll", "phonon_mmf.dll", "QtCore.dll", "QtXml.dll", "QtGui.dll",
"QtNetwork.dll", "QtTest.dll", "QtSql.dll", "QtSvg.dll", "phonon.dll",
"QtScript.dll", "QtXmlPatterns.dll", "QtMultimedia.dll", "qjpeg.dll",
"qgif.dll", "qmng.dll", "qtiff.dll", "qico.dll", "qsvg.dll",
"qcncodecs.dll", "qjpcodecs.dll","qtwcodecs.dll", "qkrcodecs.dll", "qsvgicon.dll",
"qts60plugin_5_0.dll", "QtWebKit.dll"};

namespace Debugger {
namespace Internal {

using namespace Symbian;
using namespace Coda;

static inline QString startMsg(const Coda::Session &session)
{
    return CodaGdbAdapter::tr("Process started, PID: 0x%1, thread id: 0x%2, "
       "code segment: 0x%3, data segment: 0x%4.")
         .arg(session.pid, 0, 16).arg(session.tid, 0, 16)
         .arg(session.codeseg, 0, 16).arg(session.dataseg, 0, 16);
}

/* -------------- CodaGdbAdapter:
 * Startup-sequence:
 *  - startAdapter connects both sockets/devices
 *  - In the CODA Locator Event, gdb is started and the engine is notified
 *    that the adapter has started.
 *  - Engine calls setupInferior(), which starts the process.
 *  - Initial CODA module load suspended event is emitted (process is suspended).
 *    In the event handler, gdb is connected to the remote target. In the
 *    gdb answer to conntect remote, inferiorStartPrepared() is emitted.
 *  - Engine sets up breakpoints,etc and calls inferiorStartPhase2(), which
 *    resumes the suspended CODA process via gdb 'continue'.
 * Thread handling (30.06.2010):
 * CODA does not report thread creation/termination. So, if we receive
 * a stop in a different thread, we store an additional thread in snapshot.
 * When continuing in sendContinue(), we delete this thread, since we cannot
 * know whether it will exist at the next stop.
 * Also note that threads continue running in Symbian even if one crashes.
 * TODO: - Maybe thread reporting will be improved in CODA?
 *       - Stop all threads once one stops?
 *       - Breakpoints do not trigger in threads other than the main thread. */

CodaGdbAdapter::CodaGdbAdapter(GdbEngine *engine) :
    AbstractGdbAdapter(engine),
    m_running(false),
    m_stopReason(0),
    m_gdbAckMode(true),
    m_uid(0),
    m_verbose(0),
    m_firstResumableExeLoadedEvent(false),
    m_registerRequestPending(false),
    m_firstHelloEvent(true)
{
    m_bufferedMemoryRead = true;
    // Disable buffering if gdb's dcache is used.
    m_bufferedMemoryRead = false;

    m_gdbServer = 0;
    m_gdbConnection = 0;
    m_snapshot.reset();
#ifdef Q_OS_WIN
    const unsigned long portOffset = winGetCurrentProcessId() % 100;
#else
    const uid_t portOffset = getuid();
#endif
    m_gdbServerName = _("127.0.0.1:%1").arg(2222 + portOffset);

    setVerbose(debuggerCore()->boolSetting(VerboseLog) ? 1 : 0);

    connect(debuggerCore()->action(VerboseLog), SIGNAL(valueChanged(QVariant)),
        this, SLOT(setVerbose(QVariant)));
}

void CodaGdbAdapter::setupDeviceSignals()
{
    connect(m_codaDevice.data(), SIGNAL(error(QString)),
        this, SLOT(codaDeviceError(QString)));
    connect(m_codaDevice.data(), SIGNAL(logMessage(QString)),
        this, SLOT(codaLogMessage(QString)));
    connect(m_codaDevice.data(), SIGNAL(codaEvent(Coda::CodaEvent)),
        this, SLOT(codaEvent(Coda::CodaEvent)));
    connect(SymbianUtils::SymbianDeviceManager::instance(), SIGNAL(deviceRemoved(SymbianUtils::SymbianDevice)),
            this, SLOT(codaDeviceRemoved(SymbianUtils::SymbianDevice)));
}

CodaGdbAdapter::~CodaGdbAdapter()
{
    if (m_codaDevice)
        SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaDevice);

    cleanup();
    logMessage(QLatin1String("Shutting down.\n"));
}

void CodaGdbAdapter::setVerbose(const QVariant &value)
{
    setVerbose(value.toInt());
}

void CodaGdbAdapter::setVerbose(int verbose)
{
    if (debug)
        qDebug("CodaGdbAdapter::setVerbose %d", verbose);
    m_verbose = verbose;
    if (m_codaDevice)
        m_codaDevice->setVerbose(m_verbose);
}

void CodaGdbAdapter::codaLogMessage(const QString &msg)
{
    logMessage(_("CODA ") + msg);
}

void CodaGdbAdapter::setGdbServerName(const QString &name)
{
    m_gdbServerName = name;
}

// Split a TCP address specification '<addr>[:<port>]'
static QPair<QString, unsigned short> splitIpAddressSpec(const QString &addressSpec, unsigned short defaultPort = 0)
{
    const int pos = addressSpec.indexOf(QLatin1Char(':'));
    if (pos == -1)
        return QPair<QString, unsigned short>(addressSpec, defaultPort);
    const QString address = addressSpec.left(pos);
    bool ok;
    const unsigned short port = addressSpec.mid(pos + 1).toUShort(&ok);
    if (!ok) {
        qWarning("Invalid IP address specification: '%s', defaulting to port %hu.", qPrintable(addressSpec), defaultPort);
        return QPair<QString, unsigned short>(addressSpec, defaultPort);
    }
    return QPair<QString, unsigned short>(address, port);
}

void CodaGdbAdapter::handleCodaRunControlModuleLoadContextSuspendedEvent(const CodaRunControlModuleLoadContextSuspendedEvent &se)
{
    m_snapshot.resetMemory();
    const ModuleLoadEventInfo &minfo = se.info();
    // Register in session, keep modules and libraries in sync.
    const QString moduleName = QString::fromUtf8(minfo.name);
    const bool isExe = moduleName.endsWith(_(".exe"), Qt::CaseInsensitive);
    // Add to shared library list
    if (!isExe) {
        if (minfo.loaded) {
            m_session.modules.push_back(moduleName);
            Coda::Library library;
            library.name = minfo.name;
            library.codeseg = minfo.codeAddress;
            library.dataseg = minfo.dataAddress;
            library.pid = RunControlContext::processIdFromTcdfId(se.id());
            m_session.libraries.push_back(library);
            // Load local symbol file into gdb provided there is one
            if (library.codeseg) {
                const QString localSymFileName =
                    localSymFileForLibrary(library.name, m_symbolFileFolder);
                if (!localSymFileName.isEmpty()) {
                    showMessage(msgLoadLocalSymFile(localSymFileName,
                        library.name, library.codeseg), LogMisc);
                    m_engine->postCommand(symFileLoadCommand(
                        localSymFileName, library.codeseg, library.dataseg));
                } // has local sym
            } // code seg

        } else {
            const int index = m_session.modules.indexOf(moduleName);
            if (index != -1) {
                m_session.modules.removeAt(index);
                m_session.libraries.removeAt(index);
            } else {
                // Might happen with preliminary version of CODA.
                qWarning("Received unload for module '%s' for which no load was received.",
                         qPrintable(moduleName));
            }

        }
    }
    // Handle resume.
    if (se.info().requireResume) {
        // If it is the first, resumable load event (.exe), make
        // gdb connect to remote target and resume in setupInferior2(),
        if (isExe && m_firstResumableExeLoadedEvent) {
            m_firstResumableExeLoadedEvent = false;
            m_session.codeseg = minfo.codeAddress;
            m_session.dataseg = minfo.dataAddress;
            logMessage(startMsg(m_session), LogMisc);

            const QByteArray symbolFile = m_symbolFile.toLocal8Bit();
            if (symbolFile.isEmpty()) {
                logMessage(_("WARNING: No symbol file available."), LogError);
            } else {
                // Does not seem to be necessary anymore.
                // FIXME: Startup sequence can be streamlined now as we do not
                // have to wait for the CODA startup to learn the load address.
                //m_engine->postCommand("add-symbol-file \"" + symbolFile + "\" "
                //    + QByteArray::number(m_session.codeseg));
                m_engine->postCommand("symbol-file \"" + symbolFile + "\"");
            }
            foreach (const QByteArray &s, Symbian::gdbStartupSequence())
                m_engine->postCommand(s);
            m_engine->postCommand("target remote " + gdbServerName().toLatin1(),
                                  CB(handleTargetRemote));
            if (debug)
                qDebug() << "Initial module load suspended: " << m_session.toString();
        } else {
            // Consecutive module load suspended: (not observed yet): Just continue
            m_codaDevice->sendRunControlResumeCommand(CodaCallback(), se.id());
        }
    }
}

void CodaGdbAdapter::handleTargetRemote(const GdbResponse &record)
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());
    if (record.resultClass == GdbResultDone) {
        m_engine->handleInferiorPrepared();
        if (debug)
            qDebug() << "handleTargetRemote" << m_session.toString();
    } else {
        QString msg = tr("Connecting to CODA server adapter failed:\n")
            + QString::fromLocal8Bit(record.data.findChild("msg").data());
        m_engine->notifyInferiorSetupFailed(msg);
    }
}

void CodaGdbAdapter::codaDeviceRemoved(const SymbianUtils::SymbianDevice &dev)
{
    const DebuggerStartParameters &parameters = startParameters();
    if (state() != DebuggerNotReady && !m_codaDevice.isNull() && parameters.remoteChannel == dev.portName()) {
        const QString message = QString::fromLatin1("Device '%1' has been disconnected.").arg(dev.friendlyName());
        logMessage(message);
        m_engine->handleAdapterCrashed(message);
        cleanup();
    }
}

void CodaGdbAdapter::codaEvent(const CodaEvent &e)
{
    if (debug)
        qDebug() << e.toString() << m_session.toString() << m_snapshot.toString();
    logMessage(e.toString());

    switch (e.type()) {
    case CodaEvent::LocatorHello:
        if (state() == EngineSetupRequested && m_firstHelloEvent) {
            m_firstHelloEvent = false;
            m_codaDevice->sendLoggingAddListenerCommand(CodaCallback());
            startGdb(); // Commands are only accepted after hello
        }
        break;
    case CodaEvent::RunControlModuleLoadSuspended: // A module was loaded
        handleCodaRunControlModuleLoadContextSuspendedEvent(
            static_cast<const CodaRunControlModuleLoadContextSuspendedEvent &>(e));
        break;
    case CodaEvent::RunControlContextAdded: // Thread/process added
        foreach (const RunControlContext &rc,
                static_cast<const CodaRunControlContextAddedEvent &>(e).contexts())
            if (rc.type() == RunControlContext::Thread)
                addThread(rc.threadId());
        break;
    case CodaEvent::RunControlContextRemoved: // Thread/process removed
        foreach (const QByteArray &id,
            static_cast<const CodaRunControlContextRemovedEvent &>(e).ids())
            switch (RunControlContext::typeFromCodaId(id)) {
            case RunControlContext::Thread:
                m_snapshot.removeThread(RunControlContext::threadIdFromTcdfId(id));
                break;
            case RunControlContext::Process:
                sendGdbServerMessage("W00", "Process exited");
                break;
        }
        break;
    case CodaEvent::RunControlSuspended: {
            // Thread suspended/stopped
            const CodaRunControlContextSuspendedEvent &se =
                static_cast<const CodaRunControlContextSuspendedEvent &>(e);
            const unsigned threadId = RunControlContext::threadIdFromTcdfId(se.id());
            const QString reason = QString::fromUtf8(se.reasonID());
            const QString message = QString::fromUtf8(se.message())
                .replace(_("\n"), _(", "));
            showMessage(_("Thread %1 stopped: '%2': %3").
                        arg(threadId).arg(reason, message), LogMisc);
            // Stopped in a new thread: Add.
            m_snapshot.reset();
            m_session.tid = threadId;
            if (m_snapshot.indexOfThread(threadId) == -1)
                m_snapshot.addThread(threadId);
            m_snapshot.setThreadState(threadId, reason);
            // Update registers first, then report stopped.
            m_running = false;
            m_stopReason = reason.contains(_("exception"), Qt::CaseInsensitive)
                           || reason.contains(_("panic"), Qt::CaseInsensitive) ?
                           gdbServerSignalSegfault : gdbServerSignalTrap;
            m_codaDevice->sendRegistersGetMRangeCommand(
                CodaCallback(this, &CodaGdbAdapter::handleAndReportReadRegistersAfterStop),
                currentThreadContextId(), 0,
                Symbian::RegisterCount);
    }
        break;
    case CodaEvent::LoggingWriteEvent: // TODO: Not tested yet.
        showMessage(e.toString() + QLatin1Char('\n'), AppOutput);
        break;
    default:
        break;
    }
}

void CodaGdbAdapter::startGdb()
{
    QStringList gdbArgs;
    gdbArgs.append(_("--nx")); // Do not read .gdbinit file
    m_engine->startGdb(gdbArgs);
}

void CodaGdbAdapter::handleGdbStartDone()
{
    m_engine->handleAdapterStarted();
}

void CodaGdbAdapter::handleGdbStartFailed()
{
    cleanup();
}

void CodaGdbAdapter::codaDeviceError(const QString  &errorString)
{
    logMessage(errorString);
    if (state() == EngineSetupRequested) {
        m_engine->handleAdapterStartFailed(errorString);
    } else {
        m_engine->handleAdapterCrashed(errorString);
    }
}

void CodaGdbAdapter::logMessage(const QString &msg, int channel)
{
    if (m_verbose || channel != LogDebug)
        showMessage(msg, channel);
    if (debug)
        qDebug("%s", qPrintable(msg));
}

//
// Gdb
//
void CodaGdbAdapter::handleGdbConnection()
{
    logMessage(QLatin1String("HANDLING GDB CONNECTION"));
    QTC_CHECK(m_gdbConnection == 0);
    m_gdbConnection = m_gdbServer->nextPendingConnection();
    QTC_ASSERT(m_gdbConnection, return);
    connect(m_gdbConnection, SIGNAL(disconnected()),
            m_gdbConnection, SLOT(deleteLater()));
    connect(m_gdbConnection, SIGNAL(readyRead()),
            SLOT(readGdbServerCommand()));
}

static inline QString msgGdbPacket(const QString &p)
{
    return _("gdb:                              ") + p;
}

void CodaGdbAdapter::readGdbServerCommand()
{
    QTC_ASSERT(m_gdbConnection, return);
    QByteArray packet = m_gdbConnection->readAll();
    m_gdbReadBuffer.append(packet);

    logMessage(QLatin1String("gdb: -> ") + currentTime()
               + QLatin1Char(' ') + QString::fromLatin1(packet));
    if (packet != m_gdbReadBuffer)
        logMessage(_("buffer: ") + QString::fromLatin1(m_gdbReadBuffer));

    QByteArray &ba = m_gdbReadBuffer;
    while (ba.size()) {
        char code = ba.at(0);
        ba = ba.mid(1);

        if (code == '+') {
            //logMessage("ACK");
            continue;
        }

        if (code == '-') {
            logMessage(QLatin1String("NAK: Retransmission requested"), LogError);
            // This seems too harsh.
            //emit adapterCrashed("Communication problem encountered.");
            continue;
        }

        if (code == char(0x03)) {
            logMessage(QLatin1String("INTERRUPT RECEIVED"));
            interruptInferior();
            continue;
        }

        if (code != '$') {
            logMessage(QLatin1String("Broken package (2) ") + quoteUnprintableLatin1(ba)
                + QLatin1String(Coda::hexNumber(code)), LogError);
            continue;
        }

        int pos = ba.indexOf('#');
        if (pos == -1) {
            logMessage(QLatin1String("Invalid checksum format in ")
                + quoteUnprintableLatin1(ba), LogError);
            continue;
        }

        bool ok = false;
        uint checkSum = ba.mid(pos + 1, 2).toUInt(&ok, 16);
        if (!ok) {
            logMessage(QLatin1String("Invalid checksum format 2 in ")
                + quoteUnprintableLatin1(ba), LogError);
            return;
        }

        //logMessage(QString("Packet checksum: %1").arg(checkSum));
        Coda::byte sum = 0;
        for (int i = 0; i < pos; ++i)
            sum += ba.at(i);

        if (sum != checkSum) {
            logMessage(QString::fromLatin1("ERROR: Packet checksum wrong: %1 %2 in %3").
                       arg(checkSum).arg(sum).arg(quoteUnprintableLatin1(ba)), LogError);
        }

        QByteArray cmd = ba.left(pos);
        ba.remove(0, pos + 3);
        handleGdbServerCommand(cmd);
    }
}

bool CodaGdbAdapter::sendGdbServerPacket(const QByteArray &packet, bool doFlush)
{
    if (!m_gdbConnection) {
        logMessage(_("Cannot write to gdb: No connection (%1)")
            .arg(_(packet)), LogError);
        return false;
    }
    if (m_gdbConnection->state() != QAbstractSocket::ConnectedState) {
        logMessage(_("Cannot write to gdb: Not connected (%1)")
            .arg(_(packet)), LogError);
        return false;
    }
    if (m_gdbConnection->write(packet) == -1) {
        logMessage(_("Cannot write to gdb: %1 (%2)")
            .arg(m_gdbConnection->errorString()).arg(_(packet)), LogError);
        return false;
    }
    if (doFlush)
        m_gdbConnection->flush();
    return true;
}

void CodaGdbAdapter::sendGdbServerAck()
{
    if (!m_gdbAckMode)
        return;
    logMessage(QLatin1String("gdb: <- +"));
    sendGdbServerPacket(QByteArray(1, '+'), false);
}

void CodaGdbAdapter::sendGdbServerMessage(const QByteArray &msg, const QByteArray &logNote)
{
    Coda::byte sum = 0;
    for (int i = 0; i != msg.size(); ++i)
        sum += msg.at(i);

    char checkSum[30];
    qsnprintf(checkSum, sizeof(checkSum) - 1, "%02x ", sum);

    //logMessage(QString("Packet checksum: %1").arg(sum));

    QByteArray packet;
    packet.append('$');
    packet.append(msg);
    packet.append('#');
    packet.append(checkSum);
    int pad = qMax(0, 24 - packet.size());
    logMessage(QLatin1String("gdb: <- ") + currentTime() + QLatin1Char(' ')
               + QString::fromLatin1(packet) + QString(pad, QLatin1Char(' '))
               + QLatin1String(logNote));
    sendGdbServerPacket(packet, true);
}

static QByteArray msgStepRangeReceived(unsigned from, unsigned to, bool over)
{
    QByteArray rc = "Stepping range received for step ";
    rc += over ? "over" : "into";
    rc += " (0x";
    rc += QByteArray::number(from, 16);
    rc += " to 0x";
    rc += QByteArray::number(to, 16);
    rc += ')';
    return rc;
}

void CodaGdbAdapter::handleGdbServerCommand(const QByteArray &cmd)
{
    if (debug)
        qDebug("handleGdbServerCommand: %s", cmd.constData());
    // http://sourceware.org/gdb/current/onlinedocs/gdb_34.html
    if (0) {}
    else if (cmd == "!") {
        sendGdbServerAck();
        //sendGdbServerMessage("", "extended mode not enabled");
        sendGdbServerMessage("OK", "extended mode enabled");
    }

    else if (cmd.startsWith('?')) {
        logMessage(msgGdbPacket(_("Query halted")));
        // Indicate the reason the target halted.
        // The reply is the same as for step and continue.
        sendGdbServerAck();
        // The command below will trigger fetching a stack trace while
        // the process does not seem to be fully functional. Most notably
        // the PC points to a 0x9..., which is not in "our" range
        //sendGdbServerMessage("T05library:r;", "target halted (library load)");
        //sendGdbServerMessage("S05", "target halted (trap)");
        sendGdbServerMessage("S00", "target halted (trap)");
        //sendGdbServerMessage("O" + QByteArray("Starting...").toHex());
    }

    else if (cmd == "c") {
        logMessage(msgGdbPacket(_("Continue")));
        sendGdbServerAck();
        sendContinue();
    }

    else if (cmd.startsWith('C')) {
        logMessage(msgGdbPacket(_("Continue with signal")));
        // C sig[;addr] Continue with signal sig (hex signal number)
        //Reply: See section D.3 Stop Reply Packets, for the reply specifications.
        bool ok = false;
        const uint signalNumber = cmd.mid(1).toUInt(&ok, 16);
        //TODO: Meaning of the message is not clear.
        sendGdbServerAck();
        logMessage(_("Not implemented 'Continue with signal' %1: ").arg(signalNumber),
            LogWarning);
        sendGdbServerMessage('O' + QByteArray("Console output").toHex());
        sendGdbServerMessage("W81"); // "Process exited with result 1
        sendContinue();
    }

    else if (cmd.startsWith('D')) {
        sendGdbServerAck();
        sendGdbServerMessage("OK", "shutting down");
    }

    else if (cmd == "g") {
        // Read general registers.
        logMessage(msgGdbPacket(_("Read registers")));
        if (m_snapshot.registersValid(m_session.tid)) {
            sendGdbServerAck();
            reportRegisters();
        } else {
            sendGdbServerAck();
            if (m_codaDevice->registerNames().isEmpty()) {
                m_registerRequestPending = true;
            } else {
                sendRegistersGetMCommand();
            }
        }
    }

    else if (cmd == "gg") {
        // Force re-reading general registers for debugging purpose.
        sendGdbServerAck();
        m_snapshot.setRegistersValid(m_session.tid, false);
        sendRegistersGetMCommand();
    }

    else if (cmd.startsWith("salstep,")) {
        // Receive address range for current line for future use when stepping.
        sendGdbServerAck();
        m_snapshot.parseGdbStepRange(cmd, false);
        sendGdbServerMessage("", msgStepRangeReceived(m_snapshot.lineFromAddress,
            m_snapshot.lineToAddress, m_snapshot.stepOver));
    }

    else if (cmd.startsWith("salnext,")) {
        // Receive address range for current line for future use when stepping.
        sendGdbServerAck();
        m_snapshot.parseGdbStepRange(cmd, true);
        sendGdbServerMessage("", msgStepRangeReceived(m_snapshot.lineFromAddress,
            m_snapshot.lineToAddress, m_snapshot.stepOver));
    }

    else if (cmd.startsWith("Hc")) {
        sendGdbServerAck();
        gdbSetCurrentThread(cmd, "Set current thread for step & continue ");
    }

    else if (cmd.startsWith("Hg")) {
        sendGdbServerAck();
        gdbSetCurrentThread(cmd, "Set current thread ");
    }

    else if (cmd == "k" || cmd.startsWith("vKill")) {
        // Kill inferior process
        logMessage(msgGdbPacket(_("kill")));
        sendRunControlTerminateCommand();
    }

    else if (cmd.startsWith('m')) {
        logMessage(msgGdbPacket(_("Read memory")));
        // m addr,length
        sendGdbServerAck();
        const QPair<quint64, unsigned> addrLength = parseGdbReadMemoryRequest(cmd);
        if (addrLength.first && addrLength.second) {
            readMemory(addrLength.first, addrLength.second, m_bufferedMemoryRead);
        } else {
            sendGdbServerMessage("E20", "Error " + cmd);
        }
    }

    else if (cmd.startsWith('X'))  { // Write memory
        const int dataPos = cmd.indexOf(':');
        if (dataPos == -1) {
            sendGdbServerMessage("E20", "Error (colon expected) " + cmd);
            return;
        }
        const QPair<quint64, unsigned> addrLength =
            parseGdbReadMemoryRequest(cmd.left(dataPos));
        if (addrLength.first == 0) {
            sendGdbServerMessage("E20", "Error (address = 0) " + cmd);
            return;
        }
        // Requests with len=0 are apparently used to probe writing.
        if (addrLength.second == 0) {
            sendGdbServerMessage("OK", "Probe memory write at 0x"
                + QByteArray::number(addrLength.first, 16));
            return;
        }
        // Data appear to be plain binary.
        const QByteArray data = cmd.mid(dataPos + 1);
        if (addrLength.second != unsigned(data.size())) {
            sendGdbServerMessage("E20", "Data length mismatch " + cmd);
            return;
        }
        logMessage(_("Writing %1 bytes from 0x%2: %3").
                   arg(addrLength.second).arg(addrLength.first, 0, 16).
                   arg(QString::fromLatin1(data.toHex())));
        m_codaDevice->sendMemorySetCommand(
            CodaCallback(this, &CodaGdbAdapter::handleWriteMemory),
            m_codaProcessId, addrLength.first, data);
    }

    else if (cmd.startsWith('p')) {
        logMessage(msgGdbPacket(_("read register")));
        // 0xf == current instruction pointer?
        //sendGdbServerMessage("0000", "current IP");
        sendGdbServerAck();
        bool ok = false;
        const uint registerNumber = cmd.mid(1).toUInt(&ok, 16);
        const int threadIndex = m_snapshot.indexOfThread(m_session.tid);
        QTC_ASSERT(threadIndex != -1, return);
        const Symbian::Thread &thread =  m_snapshot.threadInfo.at(threadIndex);
        if (thread.registerValid) {
            sendGdbServerMessage(thread.gdbReportSingleRegister(registerNumber),
                thread.gdbSingleRegisterLogMessage(registerNumber));
        } else {
            //qDebug() << "Fetching single register";
            m_codaDevice->sendRegistersGetMRangeCommand(
                CodaCallback(this, &CodaGdbAdapter::handleAndReportReadRegister),
                currentThreadContextId(), registerNumber, 1);
        }
    }

    else if (cmd.startsWith('P')) {
        logMessage(msgGdbPacket(_("write register")));
        // $Pe=70f96678#d3
        sendGdbServerAck();
        const QPair<uint, uint> regnumValue = parseGdbWriteRegisterWriteRequest(cmd);
        // FIXME: Assume all goes well.
        m_snapshot.setRegisterValue(m_session.tid, regnumValue.first, regnumValue.second);
        logMessage(_("Setting register #%1 to 0x%2").arg(regnumValue.first)
            .arg(regnumValue.second, 0, 16));
        QByteArray registerValue;
        Coda::appendInt(&registerValue, Coda::BigEndian); // Registers are big endian
        m_codaDevice->sendRegistersSetCommand(
            CodaCallback(this, &CodaGdbAdapter::handleWriteRegister),
            currentThreadContextId(), regnumValue.first, registerValue,
            QVariant(regnumValue.first));
        // Note that App CODA refuses to write registers 13 and 14
    }

    else if (cmd == "qAttached") {
        //$qAttached#8f
        // 1: attached to an existing process
        // 0: created a new process
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray(1, '0'), "new process created");
    }

    else if (cmd.startsWith("qC")) {
        logMessage(msgGdbPacket(_("query thread id")));
        // Return the current thread ID
        //$qC#b4
        sendGdbServerAck();
        sendGdbServerMessage("QC" + QByteArray::number(m_session.tid, 16));
    }

    else if (cmd.startsWith("qSupported")) {
        //$qSupported#37
        //$qSupported:multiprocess+#c6
        //logMessage("Handling 'qSupported'");
        sendGdbServerAck();
        sendGdbServerMessage(Symbian::gdbQSupported);
    }

    // Tracepoint handling as of gdb 7.2 onwards.
    else if (cmd == "qTStatus") { // Tracepoints
        sendGdbServerAck();
        sendGdbServerMessage("T0;tnotrun:0", "No trace experiment running");
    }
    // Trace variables as of gdb 7.2 onwards.
    else if (cmd == "qTfV" || cmd == "qTsP" || cmd == "qTfP") {
        sendGdbServerAck();
        sendGdbServerMessage("l", "No trace points");
    }

    else if (cmd.startsWith("qThreadExtraInfo")) {
        // $qThreadExtraInfo,1f9#55
        sendGdbServerAck();
        sendGdbServerMessage(m_snapshot.gdbQThreadExtraInfo(cmd));
    }

    else if (cmd == "qfDllInfo") {
        // That's the _first_ query package.
        // Happens with  gdb 6.4.50.20060226-cvs / CodeSourcery.
        // Never made it into FSF gdb that got qXfer:libraries:read instead.
        // http://sourceware.org/ml/gdb/2007-05/msg00038.html
        // Name=hexname,TextSeg=textaddr[,DataSeg=dataaddr]
        sendGdbServerAck();
        sendGdbServerMessage(m_session.gdbQsDllInfo(),
            "library information transfer finished");
    }

    else if (cmd == "qsDllInfo") {
        // That's a following query package.
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray(1, 'l'),
            "library information transfer finished");
    }

    else if (cmd == "qPacketInfo") {
        // Happens with  gdb 6.4.50.20060226-cvs / CodeSourcery.
        // Deprecated by qSupported?
        sendGdbServerAck();
        sendGdbServerMessage("", "FIXME: nothing?");
    }

    else if (cmd == "qOffsets") {
        sendGdbServerAck();
        QByteArray answer = "TextSeg=";
        answer.append(QByteArray::number(m_session.codeseg, 16));
        answer.append(";DataSeg=");
        answer.append(QByteArray::number(m_session.dataseg, 16));
        sendGdbServerMessage(answer);
    }

    else if (cmd == "qSymbol::") {
        if (m_verbose)
            logMessage(msgGdbPacket(_("notify can handle symbol lookup")));
        // Notify the target that GDB is prepared to serve symbol lookup requests.
        sendGdbServerAck();
        if (1)
            sendGdbServerMessage("OK", "no further symbols needed");
        else
            sendGdbServerMessage("qSymbol:" + QByteArray("_Z7E32Mainv").toHex(),
                "ask for more");
    }

    else if (cmd.startsWith("qXfer:features:read:target.xml:")) {
        //  $qXfer:features:read:target.xml:0,7ca#46...Ack
        sendGdbServerAck();
        sendGdbServerMessage(Symbian::gdbArchitectureXml);
    }

    else if (cmd == "qfThreadInfo") {
        // That's the _first_ query package.
        sendGdbServerAck();
        sendGdbServerMessage(m_snapshot.gdbQsThreadInfo(),
            "thread information transferred");
    }

    else if (cmd == "qsThreadInfo") {
        // That's a following query package
        sendGdbServerAck();
        sendGdbServerMessage(QByteArray(1, 'l'),
            "thread information transfer finished");
    }

    else if (cmd.startsWith("qXfer:libraries:read")) {
        sendGdbServerAck();
        sendGdbServerMessage(m_session.gdbLibraryList(),
            "library information transferred");
    }

    else if (cmd == "QStartNoAckMode") {
        //$qSupported#37
        logMessage(QLatin1String("Handling 'QStartNoAckMode'"));
        sendGdbServerAck();
        sendGdbServerMessage("OK", "ack no-ack mode");
        m_gdbAckMode = false;
    }

    else if (cmd.startsWith("QPassSignals")) {
        // list of signals to pass directly to inferior
        // $QPassSignals:e;10;14;17;1a;1b;1c;21;24;25;4c;#8f
        // happens only if "QPassSignals+;" is qSupported
        sendGdbServerAck();
        // FIXME: use the parameters
        sendGdbServerMessage("OK", "passing signals accepted");
    }

    else if (cmd == "s" || cmd.startsWith("vCont;s")) {
        const uint pc = m_snapshot.registerValue(m_session.tid, RegisterPC);
        logMessage(msgGdbPacket(_("Step range from 0x%1").
                                arg(pc, 0, 16)));
        sendGdbServerAck();
        m_running = true;
        sendStepRange();
    }

    else if (cmd.startsWith('T')) {
        // FIXME: Check whether thread is alive.
        sendGdbServerAck();
        sendGdbServerMessage("OK"); // pretend all is well
    }

    else if (cmd == "vCont?") {
        // Actions supported by the vCont packet.
        sendGdbServerAck();
        sendGdbServerMessage("vCont;c;C;s;S"); // we don't support vCont.
    }

    else if (cmd == "vCont;c") {
        // vCont[;action[:thread-id]]...'
        sendGdbServerAck();
        m_running = true;
        sendContinue();
    }

    else if (cmd.startsWith("Z0,") || cmd.startsWith("Z1,")) {
        // Insert breakpoint.
        sendGdbServerAck();
        logMessage(msgGdbPacket(_("Insert breakpoint")));
        // $Z0,786a4ccc,4#99
        const QPair<quint64, unsigned> addrLen = parseGdbSetBreakpointRequest(cmd);
        if (addrLen.first) {
            //qDebug() << "ADDR: " << hexNumber(addr) << " LEN: " << len;
            logMessage(_("Inserting breakpoint at 0x%1, %2")
                .arg(addrLen.first, 0, 16).arg(addrLen.second));
            // const QByteArray ba = trkBreakpointMessage(addr, len, len == 4);
            Coda::Breakpoint bp(addrLen.first);
            bp.size = addrLen.second;
            bp.setContextId(m_session.pid);
            // We use the automatic ids calculated from the location
            // address instead of the map in snapshot.
            m_codaDevice->sendBreakpointsAddCommand(
                CodaCallback(this, &CodaGdbAdapter::handleAndReportSetBreakpoint),
                bp);
        } else {
            logMessage(_("MISPARSED BREAKPOINT '") + QLatin1String(cmd)
                       + QLatin1String("'')") , LogError);
        }
    }

    else if (cmd.startsWith("z0,") || cmd.startsWith("z1,")) {
        // Remove breakpoint.
        sendGdbServerAck();
        logMessage(msgGdbPacket(_("Remove breakpoint")));
        // $z0,786a4ccc,4#99
        const int pos = cmd.lastIndexOf(',');
        const uint addr = cmd.mid(3, pos - 3).toUInt(0, 16);
        m_codaDevice->sendBreakpointsRemoveCommand(
            CodaCallback(this, &CodaGdbAdapter::handleClearBreakpoint),
            Coda::Breakpoint::idFromLocation(addr));
    }

    else if (cmd.startsWith("qPart:") || cmd.startsWith("qXfer:"))  {
        QByteArray data = cmd.mid(1 + cmd.indexOf(':'));
        // "qPart:auxv:read::0,147": Read OS auxiliary data, see info aux.
        bool handled = false;
        if (data.startsWith("auxv:read::")) {
            const int offsetPos = data.lastIndexOf(':') + 1;
            const int commaPos = data.lastIndexOf(',');
            if (commaPos != -1) {
                bool ok1 = false, ok2 = false;
                const int offset = data.mid(offsetPos,  commaPos - offsetPos)
                    .toUInt(&ok1, 16);
                const int length = data.mid(commaPos + 1).toUInt(&ok2, 16);
                if (ok1 && ok2) {
                    const QString msg = _("Read of OS auxiliary "
                        "vector (%1, %2) not implemented.").arg(offset).arg(length);
                    logMessage(msgGdbPacket(msg), LogWarning);
                    sendGdbServerMessage("E20", msg.toLatin1());
                    handled = true;
                }
            }
        } // auxv read

        if (!handled) {
            const QString msg = _("FIXME unknown 'XFER'-request: ")
                + QString::fromLatin1(cmd);
            logMessage(msgGdbPacket(msg), LogWarning);
            sendGdbServerMessage("E20", msg.toLatin1());
        }

    } // qPart/qXfer
       else {
        logMessage(msgGdbPacket(_("FIXME unknown: ")
            + QString::fromLatin1(cmd)), LogWarning);
    }
}

void CodaGdbAdapter::sendRunControlTerminateCommand()
{
    // Requires id of main thread to terminate.
    // Note that calling 'Settings|set|removeExecutable' crashes CODA,
    // so, it is apparently not required.
    m_codaDevice->sendRunControlTerminateCommand(
        CodaCallback(this, &CodaGdbAdapter::handleRunControlTerminate),
                                                mainThreadContextId());
}

void CodaGdbAdapter::handleRunControlTerminate(const CodaCommandResult &)
{
    QString msg = QString::fromLatin1("CODA disconnected");
    const bool emergencyShutdown = m_gdbProc.state() != QProcess::Running
                                   && state() != EngineShutdownOk;
    if (emergencyShutdown)
        msg += QString::fromLatin1(" (emergency shutdown)");
    logMessage(msg, LogMisc);
    if (emergencyShutdown) {
        cleanup();
        m_engine->notifyAdapterShutdownOk();
    }
}

void CodaGdbAdapter::gdbSetCurrentThread(const QByteArray &cmd, const char *why)
{
    // Thread ID from Hg/Hc commands: '-1': All, '0': arbitrary, else hex thread id.
    const QByteArray id = cmd.mid(2);
    const int threadId = id == "-1" ? -1 : id.toInt(0, 16);
    const QByteArray message = QByteArray(why) + QByteArray::number(threadId);
    logMessage(msgGdbPacket(_(message)));
    // Set thread for subsequent operations (`m', `M', `g', `G', et.al.).
    // for 'other operations.  0 - any thread.
    //$Hg0#df
    m_session.tid = threadId <= 0 ? m_session.mainTid : uint(threadId);
    sendGdbServerMessage("OK", message);
}

void CodaGdbAdapter::interruptInferior()
{
    m_codaDevice->sendRunControlSuspendCommand(CodaCallback(), m_codaProcessId);
}

void CodaGdbAdapter::startAdapter()
{
    m_snapshot.fullReset();
    m_session.reset();
    m_firstResumableExeLoadedEvent = true;
    m_codaProcessId.clear();
    m_firstHelloEvent = true;

    // Retrieve parameters.
    const DebuggerStartParameters &parameters = startParameters();
    m_remoteExecutable = parameters.executable;
    m_remoteArguments = Utils::QtcProcess::splitArgs(parameters.processArgs);
    m_symbolFile = parameters.symbolFileName;
    if (!m_symbolFile.isEmpty())
        m_symbolFileFolder = QFileInfo(m_symbolFile).absolutePath();

    QSharedPointer<QTcpSocket> codaSocket;
    if (parameters.communicationChannel ==
            DebuggerStartParameters::CommunicationChannelTcpIp) {
        m_codaDevice = QSharedPointer<CodaDevice>(new CodaDevice, &CodaDevice::deleteLater);
        setupDeviceSignals();
        codaSocket = QSharedPointer<QTcpSocket>(new QTcpSocket);
        m_codaDevice->setDevice(codaSocket);
    } else {
        m_codaDevice = SymbianUtils::SymbianDeviceManager::instance()
            ->getCodaDevice(parameters.remoteChannel);

        const bool ok = !m_codaDevice.isNull() && m_codaDevice->device()->isOpen();
        if (!ok) {
            const QString reason = m_codaDevice.isNull() ?
                        tr("Could not obtain device.") :
                        m_codaDevice->device()->errorString();
            const QString msg = QString::fromLatin1("Could not open serial device '%1': %2")
                                .arg(parameters.remoteChannel, reason);
            logMessage(msg, LogError);
            m_engine->handleAdapterStartFailed(msg);
            return;
        }
        setupDeviceSignals();
        m_codaDevice->setVerbose(m_verbose);
    }

    if (debug)
        qDebug() << parameters.processArgs;

    m_uid = parameters.executableUid;
    QString codaServerAddress = parameters.serverAddress;
    unsigned short codaServerPort = parameters.serverPort;

//    m_remoteArguments.clear(); FIXME: Should this be here?

    // Unixish gdbs accept only forward slashes
    m_symbolFile.replace(QLatin1Char('\\'), QLatin1Char('/'));
    // Start
    QTC_ASSERT(state() == EngineSetupRequested, qDebug() << state());
    showMessage(_("TRYING TO START ADAPTER"));
    logMessage(_("### Starting CodaGdbAdapter"));

    QTC_ASSERT(m_gdbServer == 0, delete m_gdbServer);
    QTC_ASSERT(m_gdbConnection == 0, m_gdbConnection = 0);
    m_gdbServer = new QTcpServer(this);

    const QPair<QString, unsigned short> address = splitIpAddressSpec(m_gdbServerName);
    if (!m_gdbServer->listen(QHostAddress(address.first), address.second)) {
        QString msg = QString::fromLatin1("Unable to start the gdb server at %1: %2.")
            .arg(m_gdbServerName).arg(m_gdbServer->errorString());
        logMessage(msg, LogError);
        m_engine->handleAdapterStartFailed(msg);
        return;
    }

    logMessage(QString::fromLatin1("Gdb server running on %1.\nLittle endian assumed.")
        .arg(m_gdbServerName));

    connect(m_gdbServer, SIGNAL(newConnection()),
        this, SLOT(handleGdbConnection()));

    if (parameters.communicationChannel ==
            DebuggerStartParameters::CommunicationChannelTcpIp) {
        logMessage(_("Connecting to CODA on %1:%2")
                   .arg(codaServerAddress).arg(codaServerPort));
        codaSocket->connectToHost(codaServerAddress, codaServerPort);
    } else {
        m_codaDevice->sendSerialPing(false);
    }
}

void CodaGdbAdapter::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    // Compile additional libraries.
    QStringList libraries;
    const unsigned libraryCount = sizeof(librariesC)/sizeof(char *);
    for (unsigned i = 0; i < libraryCount; ++i)
        libraries.push_back(QString::fromLatin1(librariesC[i]));

    m_codaDevice->sendProcessStartCommand(
        CodaCallback(this, &CodaGdbAdapter::handleCreateProcess),
        m_remoteExecutable, m_uid, m_remoteArguments,
        QString(), true, libraries);
}

void CodaGdbAdapter::addThread(unsigned id)
{
    showMessage(QString::fromLatin1("Thread %1 reported").arg(id), LogMisc);
    // Make thread known, register as main if it is the first one.
    if (m_snapshot.indexOfThread(id) == -1) {
        m_snapshot.addThread(id);
        if (m_session.tid == 0) {
            m_session.tid = id;
            if (m_session.mainTid == 0)
                m_session.mainTid = id;
        }
        // We cannot retrieve register values unless the registers of that
        // thread have been retrieved (CODA oddity).
        const QByteArray contextId = RunControlContext::codaId(m_session.pid, id);
        m_codaDevice->sendRegistersGetChildrenCommand(
            CodaCallback(this, &CodaGdbAdapter::handleRegisterChildren),
                         contextId, QVariant(contextId));
    }
}

void CodaGdbAdapter::handleCreateProcess(const CodaCommandResult &result)
{
    if (debug)
        qDebug() << "ProcessCreated: " << result.toString();
    if (!result) {
        const QString errorMessage = result.errorString();
        logMessage(_("Failed to start process: %1").arg(errorMessage), LogError);
        m_engine->notifyInferiorSetupFailed(result.errorString());
        return;
    }
    QTC_ASSERT(!result.values.isEmpty(), return);

    RunControlContext ctx;
    ctx.parse(result.values.front());
    logMessage(ctx.toString());

    m_session.pid = ctx.processId();
    m_codaProcessId = RunControlContext::codaId(m_session.pid);
    if (const unsigned threadId = ctx.threadId())
        addThread(threadId);
    // See ModuleLoadSuspendedEvent for the rest.
    m_session.codeseg = 0;
    m_session.dataseg = 0;
}

void CodaGdbAdapter::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());
    m_engine->notifyEngineRunAndInferiorStopOk();
    // Trigger the initial "continue" manually.
    m_engine->continueInferiorInternal();
}

//
// AbstractGdbAdapter interface implementation
//

void CodaGdbAdapter::write(const QByteArray &data)
{
    // Write magic packets directly to TRK.
    if (data.startsWith("@#")) {
        QByteArray data1 = data.mid(2);
        if (data1.endsWith(char(10)))
            data1.chop(1);
        if (data1.endsWith(char(13)))
            data1.chop(1);
        if (data1.endsWith(' '))
            data1.chop(1);
        bool ok;
        const uint addr = data1.toUInt(&ok, 0);
        logMessage(_("Direct step (@#) 0x%1: not implemented").arg(addr, 0, 16),
            LogError);
        // directStep(addr);
        return;
    }
    if (data.startsWith("@$")) {
        QByteArray ba = QByteArray::fromHex(data.mid(2));
        qDebug() << "Writing: " << quoteUnprintableLatin1(ba);
        // if (ba.size() >= 1)
        // sendMessage(ba.at(0), TrkCB(handleDirectTrk), ba.mid(1));
        return;
    }
    if (data.startsWith("@@")) {
        logMessage(_("Direct write (@@): not implemented"), LogError);
        return;
    }
    m_gdbProc.write(data);
}

void CodaGdbAdapter::cleanup()
{
    delete m_gdbServer;
    m_gdbServer = 0;
    if (m_codaDevice) {
        // Ensure process is stopped after being suspended.
        // This cannot be used when the object is deleted
        // as the responce will return to a not existing object
        sendRunControlTerminateCommand();
        disconnect(m_codaDevice.data(), 0, this, 0);
        SymbianUtils::SymbianDeviceManager::instance()->releaseCodaDevice(m_codaDevice);
    }
}

void CodaGdbAdapter::shutdownAdapter()
{
    if (m_gdbProc.state() == QProcess::Running) {
        cleanup();
        m_engine->notifyAdapterShutdownOk();
    } else {
        // Something is wrong, gdb crashed. Kill debuggee (see handleDeleteProcess2)
        if (m_codaDevice && m_codaDevice->device()->isOpen()) {
            logMessage(QLatin1String("Emergency shutdown of CODA"), LogError);
            sendRunControlTerminateCommand();
        }
    }
}

void CodaGdbAdapter::codaReloadRegisters()
{
    // Take advantage of direct access to cached register values.
    m_snapshot.syncRegisters(m_session.tid, m_engine->registerHandler());
}

void CodaGdbAdapter::codaReloadThreads()
{
    m_snapshot.syncThreads(m_engine->threadsHandler());
}

void CodaGdbAdapter::handleWriteRegister(const CodaCommandResult &result)
{
    const int registerNumber = result.cookie.toInt();
    if (result) {
        sendGdbServerMessage("OK");
    } else {
        logMessage(_("ERROR writing register #%1: %2")
            .arg(registerNumber).arg(result.errorString()), LogError);
        sendGdbServerMessage("E01");
    }
}

void CodaGdbAdapter::sendRegistersGetMCommand()
{
    // Send off a register command, which requires the names to be present.
    QTC_ASSERT(!m_codaDevice->registerNames().isEmpty(), return);

    m_codaDevice->sendRegistersGetMRangeCommand(
                CodaCallback(this, &CodaGdbAdapter::handleAndReportReadRegisters),
                currentThreadContextId(), 0,
                Symbian::RegisterCount);
}

void CodaGdbAdapter::reportRegisters()
{
    const int threadIndex = m_snapshot.indexOfThread(m_session.tid);
    QTC_ASSERT(threadIndex != -1, return);
    const Symbian::Thread &thread = m_snapshot.threadInfo.at(threadIndex);
    sendGdbServerMessage(thread.gdbReportRegisters(),
        thread.gdbRegisterLogMessage(m_verbose));
}

void CodaGdbAdapter::handleRegisterChildren(const CodaCommandResult &result)
{
    QTC_ASSERT(m_codaDevice, return);
    const QByteArray contextId = result.cookie.toByteArray();
    if (!result) {
        logMessage(QLatin1String("Error retrieving register children of ")
                   + result.cookie.toString() + QLatin1String(": ")
                   + result.errorString(), LogError);
        return;
    }
    // Parse out registers.
    // If this is a single 'pid.tid.rGPR' parent entry, recurse to get the actual
    // registers, ('pid.tid.rGPR.R0'..). At least 'pid.tid.rGPR' must have been
    // retrieved to be able to access the register contents.
    QVector<QByteArray> registerNames = CodaDevice::parseRegisterGetChildren(result);
    if (registerNames.size() == 1) {
        m_codaDevice->sendRegistersGetChildrenCommand(
            CodaCallback(this, &CodaGdbAdapter::handleRegisterChildren),
                         registerNames.front(), result.cookie);
        return;
    }
    // First thread: Set base names in device.
    if (!m_codaDevice->registerNames().isEmpty())
        return;
    // Make sure we get all registers
    const int registerCount = registerNames.size();
    if (registerCount != Symbian::RegisterCount) {
        logMessage(_("Invalid number of registers received, expected %1, got %2").
                   arg(Symbian::RegisterCount).arg(registerCount), LogError);
        return;
    }
    // Set up register names (strip thread context "pid.tid"+'.')
    QString msg = _("Retrieved %1 register names: ").arg(registerCount);
    const int contextLength = contextId.size() + 1;
    for (int i = 0; i < registerCount; i++) {
        registerNames[i].remove(0, contextLength);
        if (i)
            msg += QLatin1Char(',');
        msg += QString::fromLatin1(registerNames[i]);
    }
    logMessage(msg);
    m_codaDevice->setRegisterNames(registerNames);
    if (m_registerRequestPending) { // Request already pending?
        logMessage(_("Resuming registers request after receiving register names..."));
        sendRegistersGetMCommand();
    }
}

void CodaGdbAdapter::handleReadRegisters(const CodaCommandResult &result)
{
    // Check for errors.
    if (!result) {
        logMessage(QLatin1String("ERROR: ") + result.errorString(), LogError);
        return;
    }
    if (result.values.isEmpty() || result.values.front().type() != Json::JsonValue::String) {
        logMessage(_("Format error in register message: ") + result.toString(),
            LogError);
        return;
    }

    unsigned i = result.cookie.toUInt();
    // TODO: When reading 8-byte floating-point registers is supported, thread
    // registers won't be an array of uints.
    uint *registers = m_snapshot.registers(m_session.tid);
    QTC_ASSERT(registers, return);

    QByteArray bigEndianRaw = QByteArray::fromBase64(result.values.front().data());
    // TODO: When reading 8-byte floating-point registers is supported, will
    // need to know the list of registers and lengths of each register.
    for (int j = 0; j < bigEndianRaw.size(); j += 4) {
        registers[i++] = ((bigEndianRaw.at(j    ) & 0xff) << 24) +
                         ((bigEndianRaw.at(j + 1) & 0xff) << 16) +
                         ((bigEndianRaw.at(j + 2) & 0xff) <<  8) +
                          (bigEndianRaw.at(j + 3) & 0xff);
    }

    m_snapshot.setRegistersValid(m_session.tid, true);
    if (debug)
        qDebug() << "handleReadRegisters: " << m_snapshot.toString();
}

void CodaGdbAdapter::handleAndReportReadRegisters(const CodaCommandResult &result)
{
    handleReadRegisters(result);
    reportRegisters();
}

void CodaGdbAdapter::handleAndReportReadRegister(const CodaCommandResult &result)
{
    handleReadRegisters(result);
    const uint registerNumber = result.cookie.toUInt();
    const int threadIndex = m_snapshot.indexOfThread(m_session.tid);
    QTC_ASSERT(threadIndex != -1, return);
    const Symbian::Thread &thread = m_snapshot.threadInfo.at(threadIndex);
    sendGdbServerMessage(thread.gdbReportSingleRegister(registerNumber),
        thread.gdbSingleRegisterLogMessage(registerNumber));
}

QByteArray CodaGdbAdapter::stopMessage() const
{
    QByteArray logMsg = "Stopped with registers in thread 0x";
    logMsg += QByteArray::number(m_session.tid, 16);
    if (m_session.tid == m_session.mainTid)
        logMsg += " [main]";
    const int idx = m_snapshot.indexOfThread(m_session.tid);
    if (idx == -1)
        return logMsg;
    const Symbian::Thread &thread = m_snapshot.threadInfo.at(idx);
    logMsg += ", at 0x";
    logMsg += QByteArray::number(thread.registers[Symbian::RegisterPC], 16);
    logMsg += ", (loaded at 0x";
    logMsg += QByteArray::number(m_session.codeseg, 16);
    logMsg += ", offset 0x";
    logMsg += QByteArray::number(thread.registers[Symbian::RegisterPC] - m_session.codeseg, 16);
    logMsg += "), Register contents: ";
    logMsg += thread.registerContentsLogMessage();
    return logMsg;
}

void CodaGdbAdapter::handleAndReportReadRegistersAfterStop(const CodaCommandResult &result)
{
    handleReadRegisters(result);
    handleReadRegisters(result);
    const bool reportThread = m_session.tid != m_session.mainTid;
    sendGdbServerMessage(m_snapshot.gdbStopMessage(m_session.tid,
        m_stopReason, reportThread), stopMessage());
}

void CodaGdbAdapter::handleAndReportSetBreakpoint(const CodaCommandResult &result)
{
    if (result) {
        sendGdbServerMessage("OK");
    } else {
        logMessage(_("Error setting breakpoint: ") + result.errorString(), LogError);
        sendGdbServerMessage("E21");
    }
}

void CodaGdbAdapter::handleClearBreakpoint(const CodaCommandResult &result)
{
    logMessage(QLatin1String("CLEAR BREAKPOINT "));
    if (!result)
        logMessage(QLatin1String("Error clearing breakpoint: ") +
                   result.errorString(), LogError);
    sendGdbServerMessage("OK");
}

void CodaGdbAdapter::readMemory(uint addr, uint len, bool buffered)
{
    Q_ASSERT(len < (2 << 16));

    // We try to get medium-sized chunks of data from the device
    if (m_verbose > 2)
        logMessage(_("readMemory %1 bytes from 0x%2 blocksize=%3")
            .arg(len).arg(addr, 0, 16).arg(MemoryChunkSize));

    m_snapshot.wantedMemory = MemoryRange(addr, addr + len);
    tryAnswerGdbMemoryRequest(buffered);
}

static QString msgMemoryReadError(uint addr, uint len = 0)
{
    const QString lenS = len ? QString::number(len) : _("<unknown>");
    return _("Memory read error at: 0x%1 %2").arg(addr, 0, 16).arg(lenS);
}

void CodaGdbAdapter::sendMemoryGetCommand(const MemoryRange &range, bool buffered)
{
    const QVariant cookie = QVariant::fromValue(range);
    const CodaCallback cb = buffered ?
      CodaCallback(this, &CodaGdbAdapter::handleReadMemoryBuffered) :
      CodaCallback(this, &CodaGdbAdapter::handleReadMemoryUnbuffered);
    m_codaDevice->sendMemoryGetCommand(cb, currentThreadContextId(), range.from, range.size(), cookie);
}

void CodaGdbAdapter::handleReadMemoryBuffered(const CodaCommandResult &result)
{
    QTC_ASSERT(qVariantCanConvert<MemoryRange>(result.cookie), return);

    const QByteArray memory = CodaDevice::parseMemoryGet(result);
    const MemoryRange range = result.cookie.value<MemoryRange>();

    const bool error = !result;
    const bool insufficient = unsigned(memory.size()) != range.size();
    if (error || insufficient) {
        QString msg = error ?
                    _("Error reading memory: %1").arg(result.errorString()) :
                    _("Error reading memory (got %1 of %2): %3")
                        .arg(memory.size()).arg(range.size())
                        .arg(msgMemoryReadError(range.from, range.size()));
        msg += QString::fromLatin1("\n(Retrying unbuffered...)");
        logMessage(msg, LogError);
        // FIXME: This does not handle large requests properly.
        sendMemoryGetCommand(range, false);
        return;
    }

    m_snapshot.insertMemory(range, memory);
    tryAnswerGdbMemoryRequest(true);
}

void CodaGdbAdapter::handleReadMemoryUnbuffered(const CodaCommandResult &result)
{
    QTC_ASSERT(qVariantCanConvert<MemoryRange>(result.cookie), return);

    const QByteArray memory = CodaDevice::parseMemoryGet(result);
    const MemoryRange range = result.cookie.value<MemoryRange>();

    const bool error = !result;
    const bool insufficient = unsigned(memory.size()) != range.size();
    if (error || insufficient) {
        QString msg = error ?
                    _("Error reading memory: %1").arg(result.errorString()) :
                    _("Error reading memory (got %1 of %2): %3")
                        .arg(memory.size()).arg(range.size())
                        .arg(msgMemoryReadError(range.from, range.size()));
        logMessage(msg, LogError);
        sendGdbServerMessage(QByteArray("E20"),
            msgMemoryReadError(32, range.from).toLatin1());
        return;
    }
    m_snapshot.insertMemory(range, memory);
    tryAnswerGdbMemoryRequest(false);
}

void CodaGdbAdapter::tryAnswerGdbMemoryRequest(bool buffered)
{
    //logMessage("TRYING TO ANSWER MEMORY REQUEST ");
    MemoryRange wanted = m_snapshot.wantedMemory;
    MemoryRange needed = m_snapshot.wantedMemory;
    MEMORY_DEBUG("WANTED: " << wanted);
    Snapshot::Memory::const_iterator it = m_snapshot.memory.begin();
    Snapshot::Memory::const_iterator et = m_snapshot.memory.end();
    for ( ; it != et; ++it) {
        MEMORY_DEBUG("   NEEDED STEP: " << needed);
        needed -= it.key();
    }
    MEMORY_DEBUG("NEEDED FINAL: " << needed);

    if (needed.to == 0) {
        // FIXME: need to combine chunks first.

        // All fine. Send package to gdb.
        it = m_snapshot.memory.begin();
        et = m_snapshot.memory.end();
        for ( ; it != et; ++it) {
            if (it.key().from <= wanted.from && wanted.to <= it.key().to) {
                int offset = wanted.from - it.key().from;
                int len = wanted.to - wanted.from;
                QByteArray ba = it.value().mid(offset, len);
                sendGdbServerMessage(ba.toHex(), m_snapshot.memoryReadLogMessage
                    (wanted.from, m_session.tid, m_verbose, ba));
                return;
            }
        }
        // Happens when chunks are not combined
        QTC_CHECK(false);
        showMessage(QLatin1String("CHUNKS NOT COMBINED"));
#        ifdef MEMORY_DEBUG
        qDebug() << "CHUNKS NOT COMBINED";
        it = m_snapshot.memory.begin();
        et = m_snapshot.memory.end();
        for ( ; it != et; ++it)
            qDebug() << Coda::hexNumber(it.key().from) << Coda::hexNumber(it.key().to);
        qDebug() << "WANTED" << wanted.from << wanted.to;
#        endif
        sendGdbServerMessage("E22", "");
        return;
    }

    MEMORY_DEBUG("NEEDED AND UNSATISFIED: " << needed);
    if (buffered) {
        uint blockaddr = (needed.from / MemoryChunkSize) * MemoryChunkSize;
        logMessage(_("Requesting buffered memory %1 bytes from 0x%2")
            .arg(MemoryChunkSize).arg(blockaddr, 0, 16));
        MemoryRange range(blockaddr, blockaddr + MemoryChunkSize);
        MEMORY_DEBUG("   FETCH BUFFERED MEMORY : " << range);
        sendMemoryGetCommand(range, true);
    } else { // Unbuffered, direct requests
        int len = needed.to - needed.from;
        logMessage(_("Requesting unbuffered memory %1 bytes from 0x%2")
            .arg(len).arg(needed.from, 0, 16));
        sendMemoryGetCommand(needed, false);
        MEMORY_DEBUG("   FETCH UNBUFFERED MEMORY : " << needed);
    }
}

void CodaGdbAdapter::handleWriteMemory(const CodaCommandResult &result)
{
    if (result) {
        sendGdbServerMessage("OK", "Write memory");
    } else {
        logMessage(_("Error writing memory: ") + result.errorString(), LogError);
        sendGdbServerMessage("E21");
    }
}

QByteArray CodaGdbAdapter::mainThreadContextId() const
{
    return RunControlContext::codaId(m_session.pid, m_session.mainTid);
}

QByteArray CodaGdbAdapter::currentThreadContextId() const
{
    return RunControlContext::codaId(m_session.pid, m_session.tid);
}

void CodaGdbAdapter::sendContinue()
{
    // Remove all but main thread as we do not know whether they will exist
    // at the next stop.
    if (m_snapshot.threadInfo.size() > 1)
        m_snapshot.threadInfo.remove(1, m_snapshot.threadInfo.size() - 1);
    m_codaDevice->sendRunControlResumeCommand(CodaCallback(), m_codaProcessId);
}

void CodaGdbAdapter::sendStepRange()
{
    uint from = m_snapshot.lineFromAddress;
    uint to = m_snapshot.lineToAddress;
    const uint pc = m_snapshot.registerValue(m_session.tid, RegisterPC);
    if (from <= pc && pc <= to) {
        const QString msg = _("Step in 0x%1 .. 0x%2 instead of 0x%3...").
                            arg(from, 0, 16).arg(to, 0, 16).arg(pc, 0, 16);
        showMessage(msg);
    } else {
        from = pc;
        to = pc;
    }
    // TODO: Step range does not seem to work yet?
    const RunControlResumeMode mode = (from == to && to == pc) ?
          (m_snapshot.stepOver ? RM_STEP_OVER       : RM_STEP_INTO) :
          (m_snapshot.stepOver ? RM_STEP_OVER_RANGE : RM_STEP_INTO_RANGE);

    logMessage(_("Stepping from 0x%1 to 0x%2 (current PC=0x%3), mode %4").
               arg(from, 0, 16).arg(to, 0, 16).arg(pc).arg(int(mode)));
    m_codaDevice->sendRunControlResumeCommand(
        CodaCallback(this, &CodaGdbAdapter::handleStep),
        currentThreadContextId(),
        mode, 1, from, to);
}

void CodaGdbAdapter::handleStep(const CodaCommandResult &result)
{
    if (!result) { // Try fallback with Continue.
        logMessage(QString::fromLatin1("Error while stepping: %1 (fallback to 'continue')").
                   arg(result.errorString()), LogWarning);
        sendContinue();
        // Doing nothing as below does not work as gdb seems to insist on
        // making some progress through a 'step'.
        //sendMessage(0x12,
        //    TrkCB(handleAndReportReadRegistersAfterStop),
        //    trkReadRegistersMessage());
        return;
    }
    // The gdb server response is triggered later by the Stop Reply packet.
    logMessage(QLatin1String("STEP FINISHED ") + currentTime());
}

} // namespace Internal
} // namespace Debugger
