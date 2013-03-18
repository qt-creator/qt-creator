/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "synchronousprocess.h"
#include "qtcassert.h"
#include "hostosinfo.h"

#include <QDebug>
#include <QTimer>
#include <QTextCodec>
#include <QDir>
#include <QMessageBox>

#include <QApplication>

#include <limits.h>

#ifdef Q_OS_UNIX
#    include <unistd.h>
#endif

/*!
    \class Utils::SynchronousProcess

    \brief Runs a synchronous process in its own event loop
    that blocks only user input events. Thus, it allows for the gui to
    repaint and append output to log windows.

    The stdOut(), stdErr() signals are emitted unbuffered as the process
    writes them.

    The stdOutBuffered(), stdErrBuffered() signals are emitted with complete
    lines based on the '\n' marker if they are enabled using
    stdOutBufferedSignalsEnabled()/setStdErrBufferedSignalsEnabled().
    They would typically be used for log windows.

    There is a timeout handling that takes effect after the last data have been
    read from stdout/stdin (as opposed to waitForFinished(), which measures time
    since it was invoked). It is thus also suitable for slow processes that continously
    output data (like version system operations).

    The property timeOutMessageBoxEnabled influences whether a message box is
    shown asking the user if they want to kill the process on timeout (default: false).

    There are also static utility functions for dealing with fully synchronous
    processes, like reading the output with correct timeout handling.

    Caution: This class should NOT be used if there is a chance that the process
    triggers opening dialog boxes (for example, by file watchers triggering),
    as this will cause event loop problems.
*/

enum { debug = 0 };
enum { syncDebug = 0 };

enum { defaultMaxHangTimerCount = 10 };

namespace Utils {

// A special QProcess derivative allowing for terminal control.
class TerminalControllingProcess : public QProcess {
public:
    TerminalControllingProcess() : m_flags(0) {}

    unsigned flags() const { return m_flags; }
    void setFlags(unsigned tc) { m_flags = tc; }

protected:
    virtual void setupChildProcess();

private:
    unsigned m_flags;
};

void TerminalControllingProcess::setupChildProcess()
{
#ifdef Q_OS_UNIX
    // Disable terminal by becoming a session leader.
    if (m_flags & SynchronousProcess::UnixTerminalDisabled)
        setsid();
#endif
}

// ----------- SynchronousProcessResponse
SynchronousProcessResponse::SynchronousProcessResponse() :
   result(StartFailed),
   exitCode(-1)
{
}

void SynchronousProcessResponse::clear()
{
    result = StartFailed;
    exitCode = -1;
    stdOut.clear();
    stdErr.clear();
}

QString SynchronousProcessResponse::exitMessage(const QString &binary, int timeoutMS) const
{
    switch (result) {
    case Finished:
        return SynchronousProcess::tr("The command '%1' finished successfully.").arg(QDir::toNativeSeparators(binary));
    case FinishedError:
        return SynchronousProcess::tr("The command '%1' terminated with exit code %2.").arg(QDir::toNativeSeparators(binary)).arg(exitCode);
        break;
    case TerminatedAbnormally:
        return SynchronousProcess::tr("The command '%1' terminated abnormally.").arg(QDir::toNativeSeparators(binary));
    case StartFailed:
        return SynchronousProcess::tr("The command '%1' could not be started.").arg(QDir::toNativeSeparators(binary));
    case Hang:
        return SynchronousProcess::tr("The command '%1' did not respond within the timeout limit (%2 ms).").
                arg(QDir::toNativeSeparators(binary)).arg(timeoutMS);
    }
    return QString();
}

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse& r)
{
    QDebug nsp = str.nospace();
    nsp << "SynchronousProcessResponse: result=" << r.result << " ex=" << r.exitCode << '\n'
        << r.stdOut.size() << " bytes stdout, stderr=" << r.stdErr << '\n';
    return str;
}

// Data for one channel buffer (stderr/stdout)
struct ChannelBuffer {
    ChannelBuffer();
    void clearForRun();
    QByteArray linesRead();

    QByteArray data;
    bool firstData;
    bool bufferedSignalsEnabled;
    bool firstBuffer;
    int bufferPos;
};

ChannelBuffer::ChannelBuffer() :
    firstData(true),
    bufferedSignalsEnabled(false),
    firstBuffer(true),
    bufferPos(0)
{
}

void ChannelBuffer::clearForRun()
{
    firstData = true;
    firstBuffer = true;
    bufferPos = 0;
}

/* Check for complete lines read from the device and return them, moving the
 * buffer position. This is based on the assumption that '\n' is the new line
 * marker in any sane codec. */
QByteArray ChannelBuffer::linesRead()
{
    // Any new lines?
    const int lastLineIndex = data.lastIndexOf('\n');
    if (lastLineIndex == -1 || lastLineIndex <= bufferPos)
        return QByteArray();
    const int nextBufferPos = lastLineIndex + 1;
    const QByteArray lines = data.mid(bufferPos, nextBufferPos - bufferPos);
    bufferPos = nextBufferPos;
    return lines;
}

// ----------- SynchronousProcessPrivate
struct SynchronousProcessPrivate {
    SynchronousProcessPrivate();
    void clearForRun();

    QTextCodec *m_stdOutCodec;
    TerminalControllingProcess m_process;
    QTimer m_timer;
    QEventLoop m_eventLoop;
    SynchronousProcessResponse m_result;
    int m_hangTimerCount;
    int m_maxHangTimerCount;
    bool m_startFailure;
    bool m_timeOutMessageBoxEnabled;
    bool m_waitingForUser;
    QString m_binary;

    ChannelBuffer m_stdOut;
    ChannelBuffer m_stdErr;
};

SynchronousProcessPrivate::SynchronousProcessPrivate() :
    m_stdOutCodec(0),
    m_hangTimerCount(0),
    m_maxHangTimerCount(defaultMaxHangTimerCount),
    m_startFailure(false),
    m_timeOutMessageBoxEnabled(false),
    m_waitingForUser(false)
{
}

void SynchronousProcessPrivate::clearForRun()
{
    m_hangTimerCount = 0;
    m_stdOut.clearForRun();
    m_stdErr.clearForRun();
    m_result.clear();
    m_startFailure = false;
    m_binary.clear();
}

// ----------- SynchronousProcess
SynchronousProcess::SynchronousProcess() :
    d(new SynchronousProcessPrivate)
{
    d->m_timer.setInterval(1000);
    connect(&d->m_timer, SIGNAL(timeout()), this, SLOT(slotTimeout()));
    connect(&d->m_process, SIGNAL(finished(int,QProcess::ExitStatus)), this, SLOT(finished(int,QProcess::ExitStatus)));
    connect(&d->m_process, SIGNAL(error(QProcess::ProcessError)), this, SLOT(error(QProcess::ProcessError)));
    connect(&d->m_process, SIGNAL(readyReadStandardOutput()),
            this, SLOT(stdOutReady()));
    connect(&d->m_process, SIGNAL(readyReadStandardError()),
            this, SLOT(stdErrReady()));
}

SynchronousProcess::~SynchronousProcess()
{
    disconnect(&d->m_timer, 0, this, 0);
    disconnect(&d->m_process, 0, this, 0);
    delete d;
}

void SynchronousProcess::setTimeout(int timeoutMS)
{
    if (timeoutMS >= 0)
        d->m_maxHangTimerCount = qMax(2, timeoutMS / 1000);
    else
        d->m_maxHangTimerCount = INT_MAX;
}

int SynchronousProcess::timeout() const
{
    return d->m_maxHangTimerCount == INT_MAX ? -1 : 1000 * d->m_maxHangTimerCount;
}

void SynchronousProcess::setStdOutCodec(QTextCodec *c)
{
    d->m_stdOutCodec = c;
}

QTextCodec *SynchronousProcess::stdOutCodec() const
{
    return d->m_stdOutCodec;
}

bool SynchronousProcess::stdOutBufferedSignalsEnabled() const
{
    return d->m_stdOut.bufferedSignalsEnabled;
}

void SynchronousProcess::setStdOutBufferedSignalsEnabled(bool v)
{
    d->m_stdOut.bufferedSignalsEnabled = v;
}

bool SynchronousProcess::stdErrBufferedSignalsEnabled() const
{
    return d->m_stdErr.bufferedSignalsEnabled;
}

void SynchronousProcess::setStdErrBufferedSignalsEnabled(bool v)
{
    d->m_stdErr.bufferedSignalsEnabled = v;
}

QStringList SynchronousProcess::environment() const
{
    return d->m_process.environment();
}

bool SynchronousProcess::timeOutMessageBoxEnabled() const
{
    return d->m_timeOutMessageBoxEnabled;
}

void SynchronousProcess::setTimeOutMessageBoxEnabled(bool v)
{
    d->m_timeOutMessageBoxEnabled = v;
}

void SynchronousProcess::setEnvironment(const QStringList &e)
{
    d->m_process.setEnvironment(e);
}

void SynchronousProcess::setProcessEnvironment(const QProcessEnvironment &environment)
{
    d->m_process.setProcessEnvironment(environment);
}

QProcessEnvironment SynchronousProcess::processEnvironment() const
{
    return d->m_process.processEnvironment();
}

unsigned SynchronousProcess::flags() const
{
    return d->m_process.flags();
}

void SynchronousProcess::setFlags(unsigned tc)
{
    d->m_process.setFlags(tc);
}

void SynchronousProcess::setWorkingDirectory(const QString &workingDirectory)
{
    d->m_process.setWorkingDirectory(workingDirectory);
}

QString SynchronousProcess::workingDirectory() const
{
    return d->m_process.workingDirectory();
}

QProcess::ProcessChannelMode SynchronousProcess::processChannelMode () const
{
    return d->m_process.processChannelMode();
}

void SynchronousProcess::setProcessChannelMode(QProcess::ProcessChannelMode m)
{
    d->m_process.setProcessChannelMode(m);
}

SynchronousProcessResponse SynchronousProcess::run(const QString &binary,
                                                 const QStringList &args)
{
    if (debug)
        qDebug() << '>' << Q_FUNC_INFO << binary << args;

    d->clearForRun();

    // On Windows, start failure is triggered immediately if the
    // executable cannot be found in the path. Do not start the
    // event loop in that case.
    d->m_binary = binary;
    d->m_process.start(binary, args, QIODevice::ReadOnly);
    d->m_process.closeWriteChannel();
    if (!d->m_startFailure) {
        d->m_timer.start();
        QApplication::setOverrideCursor(Qt::WaitCursor);
        d->m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
        if (d->m_result.result == SynchronousProcessResponse::Finished || d->m_result.result == SynchronousProcessResponse::FinishedError) {
            processStdOut(false);
            processStdErr(false);
        }

        d->m_result.stdOut = convertStdOut(d->m_stdOut.data);
        d->m_result.stdErr = convertStdErr(d->m_stdErr.data);

        d->m_timer.stop();
        QApplication::restoreOverrideCursor();
    }

    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << binary << d->m_result;
    return  d->m_result;
}

static inline bool askToKill(const QString &binary = QString())
{
    const QString title = SynchronousProcess::tr("Process not Responding");
    QString msg = binary.isEmpty() ?
                  SynchronousProcess::tr("The process is not responding.") :
                  SynchronousProcess::tr("The process '%1' is not responding.").arg(QDir::toNativeSeparators(binary));
    msg += QLatin1Char(' ');
    msg += SynchronousProcess::tr("Would you like to terminate it?");
    // Restore the cursor that is set to wait while running.
    const bool hasOverrideCursor = QApplication::overrideCursor() != 0;
    if (hasOverrideCursor)
        QApplication::restoreOverrideCursor();
    QMessageBox::StandardButton answer = QMessageBox::question(0, title, msg, QMessageBox::Yes|QMessageBox::No);
    if (hasOverrideCursor)
        QApplication::setOverrideCursor(Qt::WaitCursor);
    return answer == QMessageBox::Yes;
}

void SynchronousProcess::slotTimeout()
{
    if (!d->m_waitingForUser && (++d->m_hangTimerCount > d->m_maxHangTimerCount)) {
        if (debug)
            qDebug() << Q_FUNC_INFO << "HANG detected, killing";
        d->m_waitingForUser = true;
        const bool terminate = !d->m_timeOutMessageBoxEnabled || askToKill(d->m_binary);
        d->m_waitingForUser = false;
        if (terminate) {
            SynchronousProcess::stopProcess(d->m_process);
            d->m_result.result = SynchronousProcessResponse::Hang;
        } else {
            d->m_hangTimerCount = 0;
        }
    } else {
        if (debug)
            qDebug() << Q_FUNC_INFO << d->m_hangTimerCount;
    }
}

void SynchronousProcess::finished(int exitCode, QProcess::ExitStatus e)
{
    if (debug)
        qDebug() << Q_FUNC_INFO << exitCode << e;
    d->m_hangTimerCount = 0;
    switch (e) {
    case QProcess::NormalExit:
        d->m_result.result = exitCode ? SynchronousProcessResponse::FinishedError : SynchronousProcessResponse::Finished;
        d->m_result.exitCode = exitCode;
        break;
    case QProcess::CrashExit:
        // Was hang detected before and killed?
        if (d->m_result.result != SynchronousProcessResponse::Hang)
            d->m_result.result = SynchronousProcessResponse::TerminatedAbnormally;
        d->m_result.exitCode = -1;
        break;
    }
    d->m_eventLoop.quit();
}

void SynchronousProcess::error(QProcess::ProcessError e)
{
    d->m_hangTimerCount = 0;
    if (debug)
        qDebug() << Q_FUNC_INFO << e;
    // Was hang detected before and killed?
    if (d->m_result.result != SynchronousProcessResponse::Hang)
        d->m_result.result = SynchronousProcessResponse::StartFailed;
    d->m_startFailure = true;
    d->m_eventLoop.quit();
}

void SynchronousProcess::stdOutReady()
{
    d->m_hangTimerCount = 0;
    processStdOut(true);
}

void SynchronousProcess::stdErrReady()
{
    d->m_hangTimerCount = 0;
    processStdErr(true);
}

QString SynchronousProcess::convertStdErr(const QByteArray &ba)
{
    return QString::fromLocal8Bit(ba.constData(), ba.size()).remove(QLatin1Char('\r'));
}

QString SynchronousProcess::convertStdOut(const QByteArray &ba) const
{
    QString stdOut = d->m_stdOutCodec ? d->m_stdOutCodec->toUnicode(ba) : QString::fromLocal8Bit(ba.constData(), ba.size());
    return stdOut.remove(QLatin1Char('\r'));
}

void SynchronousProcess::processStdOut(bool emitSignals)
{
    // Handle binary data
    const QByteArray ba = d->m_process.readAllStandardOutput();
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << emitSignals << ba;
    if (!ba.isEmpty()) {
        d->m_stdOut.data += ba;
        if (emitSignals) {
            // Emit binary signals
            emit stdOut(ba, d->m_stdOut.firstData);
            d->m_stdOut.firstData = false;
            // Buffered. Emit complete lines?
            if (d->m_stdOut.bufferedSignalsEnabled) {
                const QByteArray lines = d->m_stdOut.linesRead();
                if (!lines.isEmpty()) {
                    emit stdOutBuffered(convertStdOut(lines), d->m_stdOut.firstBuffer);
                    d->m_stdOut.firstBuffer = false;
                }
            }
        }
    }
}

void SynchronousProcess::processStdErr(bool emitSignals)
{
    // Handle binary data
    const QByteArray ba = d->m_process.readAllStandardError();
    if (debug > 1)
        qDebug() << Q_FUNC_INFO << emitSignals << ba;
    if (!ba.isEmpty()) {
        d->m_stdErr.data += ba;
        if (emitSignals) {
            // Emit binary signals
            emit stdErr(ba, d->m_stdErr.firstData);
            d->m_stdErr.firstData = false;
            if (d->m_stdErr.bufferedSignalsEnabled) {
                // Buffered. Emit complete lines?
                const QByteArray lines = d->m_stdErr.linesRead();
                if (!lines.isEmpty()) {
                    emit stdErrBuffered(convertStdErr(lines), d->m_stdErr.firstBuffer);
                    d->m_stdErr.firstBuffer = false;
                }
            }
        }
    }
}

QSharedPointer<QProcess> SynchronousProcess::createProcess(unsigned flags)
{
    TerminalControllingProcess *process = new TerminalControllingProcess;
    process->setFlags(flags);
    return QSharedPointer<QProcess>(process);
}

// Static utilities: Keep running as long as it gets data.
bool SynchronousProcess::readDataFromProcess(QProcess &p, int timeOutMS,
                                             QByteArray *stdOut, QByteArray *stdErr,
                                             bool showTimeOutMessageBox)
{
    if (syncDebug)
        qDebug() << ">readDataFromProcess" << timeOutMS;
    if (p.state() != QProcess::Running) {
        qWarning("readDataFromProcess: Process in non-running state passed in.");
        return false;
    }

    QTC_ASSERT(p.readChannel() == QProcess::StandardOutput, return false);

    // Keep the process running until it has no longer has data
    bool finished = false;
    bool hasData = false;
    do {
        finished = p.state() == QProcess::NotRunning || p.waitForFinished(timeOutMS);
        hasData = false;
        // First check 'stdout'
        if (p.bytesAvailable()) { // applies to readChannel() only
            hasData = true;
            const QByteArray newStdOut = p.readAllStandardOutput();
            if (stdOut)
                stdOut->append(newStdOut);
        }
        // Check 'stderr' separately. This is a special handling
        // for 'git pull' and the like which prints its progress on stderr.
        const QByteArray newStdErr = p.readAllStandardError();
        if (!newStdErr.isEmpty()) {
            hasData = true;
            if (stdErr)
                stdErr->append(newStdErr);
        }
        // Prompt user, pretend we have data if says 'No'.
        const bool hang = !hasData && !finished;
        if (hang && showTimeOutMessageBox) {
            QString binary;
#if QT_VERSION >= 0x050000
            binary = p.program();
#endif
            if (!askToKill(binary))
                hasData = true;
        }
    } while (hasData && !finished);
    if (syncDebug)
        qDebug() << "<readDataFromProcess" << finished;
    return finished;
}

bool SynchronousProcess::stopProcess(QProcess &p)
{
    if (p.state() != QProcess::Running)
        return true;
    p.terminate();
    if (p.waitForFinished(300))
        return true;
    p.kill();
    return p.waitForFinished(300);
}

// Path utilities

// Locate a binary in a directory, applying all kinds of
// extensions the operating system supports.
static QString checkBinary(const QDir &dir, const QString &binary)
{
    // naive UNIX approach
    const QFileInfo info(dir.filePath(binary));
    if (info.isFile() && info.isExecutable())
        return info.absoluteFilePath();

    // Does the OS have some weird extension concept or does the
    // binary have a 3 letter extension?
    if (HostOsInfo::isAnyUnixHost() && !HostOsInfo::isMacHost())
        return QString();
    const int dotIndex = binary.lastIndexOf(QLatin1Char('.'));
    if (dotIndex != -1 && dotIndex == binary.size() - 4)
        return  QString();

    switch (HostOsInfo::hostOs()) {
    case HostOsInfo::HostOsLinux:
    case HostOsInfo::HostOsOtherUnix:
    case HostOsInfo::HostOsOther:
        break;
    case HostOsInfo::HostOsWindows: {
            static const char *windowsExtensions[] = {".cmd", ".bat", ".exe", ".com" };
            // Check the Windows extensions using the order
            const int windowsExtensionCount = sizeof(windowsExtensions)/sizeof(const char*);
            for (int e = 0; e < windowsExtensionCount; e ++) {
                const QFileInfo windowsBinary(dir.filePath(binary + QLatin1String(windowsExtensions[e])));
                if (windowsBinary.isFile() && windowsBinary.isExecutable())
                    return windowsBinary.absoluteFilePath();
            }
        }
        break;
    case HostOsInfo::HostOsMac: {
            // Check for Mac app folders
            const QFileInfo appFolder(dir.filePath(binary + QLatin1String(".app")));
            if (appFolder.isDir()) {
                QString macBinaryPath = appFolder.absoluteFilePath();
                macBinaryPath += QLatin1String("/Contents/MacOS/");
                macBinaryPath += binary;
                const QFileInfo macBinary(macBinaryPath);
                if (macBinary.isFile() && macBinary.isExecutable())
                    return macBinary.absoluteFilePath();
            }
        }
        break;
    }
    return QString();
}

QString SynchronousProcess::locateBinary(const QString &path, const QString &binary)
{
    // Absolute file?
    const QFileInfo absInfo(binary);
    if (absInfo.isAbsolute())
        return checkBinary(absInfo.dir(), absInfo.fileName());

    // Windows finds binaries  in the current directory
    if (HostOsInfo::isWindowsHost()) {
        const QString currentDirBinary = checkBinary(QDir::current(), binary);
        if (!currentDirBinary.isEmpty())
            return currentDirBinary;
    }

    const QStringList paths = path.split(HostOsInfo::pathListSeparator());
    if (paths.empty())
        return QString();
    const QStringList::const_iterator cend = paths.constEnd();
    for (QStringList::const_iterator it = paths.constBegin(); it != cend; ++it) {
        const QDir dir(*it);
        const QString rc = checkBinary(dir, binary);
        if (!rc.isEmpty())
            return rc;
    }
    return QString();
}

QString SynchronousProcess::locateBinary(const QString &binary)
{
    const QByteArray path = qgetenv("PATH");
    return locateBinary(QString::fromLocal8Bit(path), binary);
}

} // namespace Utils
