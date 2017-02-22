/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "synchronousprocess.h"
#include "qtcassert.h"
#include "hostosinfo.h"

#include <QDebug>
#include <QTimer>
#include <QTextCodec>
#include <QDir>
#include <QMessageBox>
#include <QThread>

#include <QApplication>

#include <limits.h>
#include <memory>

#ifdef Q_OS_UNIX
#    include <unistd.h>
#endif

/*!
    \class Utils::SynchronousProcess

    \brief The SynchronousProcess class runs a synchronous process in its own
    event loop that blocks only user input events. Thus, it allows for the GUI to
    repaint and append output to log windows.

    The stdOut(), stdErr() signals are emitted unbuffered as the process
    writes them.

    The stdOutBuffered(), stdErrBuffered() signals are emitted with complete
    lines based on the '\\n' marker if they are enabled using
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
void SynchronousProcessResponse::clear()
{
    result = StartFailed;
    exitCode = -1;
    rawStdOut.clear();
    rawStdErr.clear();
}

QString SynchronousProcessResponse::exitMessage(const QString &binary, int timeoutS) const
{
    switch (result) {
    case Finished:
        return SynchronousProcess::tr("The command \"%1\" finished successfully.").arg(QDir::toNativeSeparators(binary));
    case FinishedError:
        return SynchronousProcess::tr("The command \"%1\" terminated with exit code %2.").arg(QDir::toNativeSeparators(binary)).arg(exitCode);
    case TerminatedAbnormally:
        return SynchronousProcess::tr("The command \"%1\" terminated abnormally.").arg(QDir::toNativeSeparators(binary));
    case StartFailed:
        return SynchronousProcess::tr("The command \"%1\" could not be started.").arg(QDir::toNativeSeparators(binary));
    case Hang:
        return SynchronousProcess::tr("The command \"%1\" did not respond within the timeout limit (%2 s).")
                .arg(QDir::toNativeSeparators(binary)).arg(timeoutS);
    }
    return QString();
}

QByteArray SynchronousProcessResponse::allRawOutput() const
{
    if (!rawStdOut.isEmpty() && !rawStdErr.isEmpty()) {
        QByteArray result = rawStdOut;
        if (!result.endsWith('\n'))
            result += '\n';
        result += rawStdErr;
        return result;
    }
    return !rawStdOut.isEmpty() ? rawStdOut : rawStdErr;
}

QString SynchronousProcessResponse::allOutput() const
{
    const QString out = stdOut();
    const QString err = stdErr();

    if (!out.isEmpty() && !err.isEmpty()) {
        QString result = out;
        if (!result.endsWith('\n'))
            result += '\n';
        result += err;
        return result;
    }
    return !out.isEmpty() ? out : err;
}

QString SynchronousProcessResponse::stdOut() const
{
    return SynchronousProcess::normalizeNewlines(codec->toUnicode(rawStdOut));
}

QString SynchronousProcessResponse::stdErr() const
{
    return SynchronousProcess::normalizeNewlines(codec->toUnicode(rawStdErr));
}

QTCREATOR_UTILS_EXPORT QDebug operator<<(QDebug str, const SynchronousProcessResponse& r)
{
    QDebug nsp = str.nospace();
    nsp << "SynchronousProcessResponse: result=" << r.result << " ex=" << r.exitCode << '\n'
        << r.rawStdOut.size() << " bytes stdout, stderr=" << r.rawStdErr << '\n';
    return str;
}

SynchronousProcessResponse::Result defaultExitCodeInterpreter(int code)
{
    return code ? SynchronousProcessResponse::FinishedError
                : SynchronousProcessResponse::Finished;
}

// Data for one channel buffer (stderr/stdout)
class ChannelBuffer : public QObject
{
    Q_OBJECT

public:
    void clearForRun();

    QString linesRead();
    void append(const QByteArray &text, bool emitSignals);

    QByteArray rawData;
    QString incompleteLineBuffer; // lines not yet signaled
    QTextCodec *codec = nullptr; // Not owner
    std::unique_ptr<QTextCodec::ConverterState> codecState;
    int rawDataPos = 0;
    bool bufferedSignalsEnabled = false;
    bool firstBuffer = true;

signals:
    void outputBuffered(const QString &text, bool firstTime);
};

void ChannelBuffer::clearForRun()
{
    firstBuffer = true;
    rawDataPos = 0;
    rawData.clear();
    codecState.reset(new QTextCodec::ConverterState);
    incompleteLineBuffer.clear();
}

/* Check for complete lines read from the device and return them, moving the
 * buffer position. */
QString ChannelBuffer::linesRead()
{
    // Convert and append the new input to the buffer of incomplete lines
    const char *start = rawData.constData() + rawDataPos;
    const int len = rawData.size() - rawDataPos;
    incompleteLineBuffer.append(codec->toUnicode(start, len, codecState.get()));
    rawDataPos = rawData.size();

    // Any completed lines in the incompleteLineBuffer?
    const int lastLineIndex = qMax(incompleteLineBuffer.lastIndexOf('\n'),
                                   incompleteLineBuffer.lastIndexOf('\r'));
    if (lastLineIndex == -1)
        return QString();

    // Get completed lines and remove them from the incompleteLinesBuffer:
    const QString lines = SynchronousProcess::normalizeNewlines(incompleteLineBuffer.left(lastLineIndex + 1));
    incompleteLineBuffer = incompleteLineBuffer.mid(lastLineIndex + 1);

    return lines;
}

void ChannelBuffer::append(const QByteArray &text, bool emitSignals)
{
    if (text.isEmpty())
        return;
    rawData += text;
    if (!emitSignals)
        return;

    // Buffered. Emit complete lines?
    if (bufferedSignalsEnabled) {
        const QString lines = linesRead();
        if (!lines.isEmpty()) {
            emit outputBuffered(lines, firstBuffer);
            firstBuffer = false;
        }
    }
}

// ----------- SynchronousProcessPrivate
class SynchronousProcessPrivate {
public:
    void clearForRun();

    QTextCodec *m_codec = QTextCodec::codecForLocale();
    TerminalControllingProcess m_process;
    QTimer m_timer;
    QEventLoop m_eventLoop;
    SynchronousProcessResponse m_result;
    QString m_binary;
    ChannelBuffer m_stdOut;
    ChannelBuffer m_stdErr;
    ExitCodeInterpreter m_exitCodeInterpreter = defaultExitCodeInterpreter;

    int m_hangTimerCount = 0;
    int m_maxHangTimerCount = defaultMaxHangTimerCount;
    bool m_startFailure = false;
    bool m_timeOutMessageBoxEnabled = false;
    bool m_waitingForUser = false;
};

void SynchronousProcessPrivate::clearForRun()
{
    m_hangTimerCount = 0;
    m_stdOut.clearForRun();
    m_stdOut.codec = m_codec;
    m_stdErr.clearForRun();
    m_stdErr.codec = m_codec;
    m_result.clear();
    m_result.codec = m_codec;
    m_startFailure = false;
    m_binary.clear();
}

// ----------- SynchronousProcess
SynchronousProcess::SynchronousProcess() :
    d(new SynchronousProcessPrivate)
{
    d->m_timer.setInterval(1000);
    connect(&d->m_timer, &QTimer::timeout, this, &SynchronousProcess::slotTimeout);
    connect(&d->m_process,
            static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
            this, &SynchronousProcess::finished);
    connect(&d->m_process, &QProcess::errorOccurred, this, &SynchronousProcess::error);
    connect(&d->m_process, &QProcess::readyReadStandardOutput,
            this, [this]() {
                d->m_hangTimerCount = 0;
                processStdOut(true);
            });
    connect(&d->m_process, &QProcess::readyReadStandardError,
            this, [this]() {
                d->m_hangTimerCount = 0;
                processStdErr(true);
            });
    connect(&d->m_stdOut, &ChannelBuffer::outputBuffered, this, &SynchronousProcess::stdOutBuffered);
    connect(&d->m_stdErr, &ChannelBuffer::outputBuffered, this, &SynchronousProcess::stdErrBuffered);
}

SynchronousProcess::~SynchronousProcess()
{
    disconnect(&d->m_timer, 0, this, 0);
    disconnect(&d->m_process, 0, this, 0);
    delete d;
}

void SynchronousProcess::setTimeoutS(int timeoutS)
{
    if (timeoutS > 0)
        d->m_maxHangTimerCount = qMax(2, timeoutS);
    else
        d->m_maxHangTimerCount = INT_MAX / 1000;
}

int SynchronousProcess::timeoutS() const
{
    return d->m_maxHangTimerCount == (INT_MAX / 1000) ? -1 : d->m_maxHangTimerCount;
}

void SynchronousProcess::setCodec(QTextCodec *c)
{
    QTC_ASSERT(c, return);
    d->m_codec = c;
}

QTextCodec *SynchronousProcess::codec() const
{
    return d->m_codec;
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

void SynchronousProcess::setExitCodeInterpreter(const ExitCodeInterpreter &interpreter)
{
    QTC_ASSERT(interpreter, return);
    d->m_exitCodeInterpreter = interpreter;
}

ExitCodeInterpreter SynchronousProcess::exitCodeInterpreter() const
{
    return d->m_exitCodeInterpreter;
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

static bool isGuiThread()
{
    return QThread::currentThread() == QCoreApplication::instance()->thread();
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
        if (isGuiThread())
            QApplication::setOverrideCursor(Qt::WaitCursor);
        d->m_eventLoop.exec(QEventLoop::ExcludeUserInputEvents);
        if (d->m_result.result == SynchronousProcessResponse::Finished || d->m_result.result == SynchronousProcessResponse::FinishedError) {
            processStdOut(false);
            processStdErr(false);
        }

        d->m_result.rawStdOut = d->m_stdOut.rawData;
        d->m_result.rawStdErr = d->m_stdErr.rawData;

        d->m_timer.stop();
        if (isGuiThread())
            QApplication::restoreOverrideCursor();
    }

    if (debug)
        qDebug() << '<' << Q_FUNC_INFO << binary << d->m_result;
    return  d->m_result;
}

SynchronousProcessResponse SynchronousProcess::runBlocking(const QString &binary, const QStringList &args)
{
    d->clearForRun();

    // On Windows, start failure is triggered immediately if the
    // executable cannot be found in the path. Do not start the
    // event loop in that case.
    d->m_binary = binary;
    d->m_process.start(binary, args, QIODevice::ReadOnly);
    if (!d->m_process.waitForStarted(d->m_maxHangTimerCount * 1000)) {
        d->m_result.result = SynchronousProcessResponse::StartFailed;
        return d->m_result;
    }
    d->m_process.closeWriteChannel();
    if (d->m_process.waitForFinished(d->m_maxHangTimerCount * 1000)) {
        if (d->m_process.state() == QProcess::Running) {
            d->m_result.result = SynchronousProcessResponse::Hang;
            d->m_process.terminate();
            if (d->m_process.waitForFinished(1000) && d->m_process.state() == QProcess::Running) {
                d->m_process.kill();
                d->m_process.waitForFinished(1000);
            }
        }
    }

    if (d->m_process.state() != QProcess::NotRunning)
        return d->m_result;

    d->m_result.exitCode = d->m_process.exitCode();
    if (d->m_result.result == SynchronousProcessResponse::StartFailed) {
        if (d->m_process.exitStatus() != QProcess::NormalExit)
            d->m_result.result = SynchronousProcessResponse::TerminatedAbnormally;
        else
            d->m_result.result = (exitCodeInterpreter())(d->m_result.exitCode);
    }
    processStdOut(false);
    processStdErr(false);

    d->m_result.rawStdOut = d->m_stdOut.rawData;
    d->m_result.rawStdErr = d->m_stdErr.rawData;

    return d->m_result;
}

bool SynchronousProcess::terminate()
{
    return stopProcess(d->m_process);
}

static inline bool askToKill(const QString &binary = QString())
{
    if (!isGuiThread())
        return true;
    const QString title = SynchronousProcess::tr("Process not Responding");
    QString msg = binary.isEmpty() ?
                  SynchronousProcess::tr("The process is not responding.") :
                  SynchronousProcess::tr("The process \"%1\" is not responding.").arg(QDir::toNativeSeparators(binary));
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
        d->m_result.result = d->m_exitCodeInterpreter(exitCode);
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

void SynchronousProcess::processStdOut(bool emitSignals)
{
    // Handle binary data
    d->m_stdOut.append(d->m_process.readAllStandardOutput(), emitSignals);
}

void SynchronousProcess::processStdErr(bool emitSignals)
{
    // Handle binary data
    d->m_stdErr.append(d->m_process.readAllStandardError(), emitSignals);
}

QSharedPointer<QProcess> SynchronousProcess::createProcess(unsigned flags)
{
    TerminalControllingProcess *process = new TerminalControllingProcess;
    process->setFlags(flags);
    return QSharedPointer<QProcess>(process);
}

// Static utilities: Keep running as long as it gets data.
bool SynchronousProcess::readDataFromProcess(QProcess &p, int timeoutS,
                                             QByteArray *stdOut, QByteArray *stdErr,
                                             bool showTimeOutMessageBox)
{
    if (syncDebug)
        qDebug() << ">readDataFromProcess" << timeoutS;
    if (p.state() != QProcess::Running) {
        qWarning("readDataFromProcess: Process in non-running state passed in.");
        return false;
    }

    QTC_ASSERT(p.readChannel() == QProcess::StandardOutput, return false);

    // Keep the process running until it has no longer has data
    bool finished = false;
    bool hasData = false;
    do {
        finished = p.waitForFinished(timeoutS > 0 ? timeoutS * 1000 : -1)
                || p.state() == QProcess::NotRunning;
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
        hasData = hang && showTimeOutMessageBox && !askToKill(p.program());
    } while (hasData && !finished);
    if (syncDebug)
        qDebug() << "<readDataFromProcess" << finished;
    return finished;
}

bool SynchronousProcess::stopProcess(QProcess &p)
{
    if (p.state() == QProcess::NotRunning)
        return true;
    p.terminate();
    if (p.waitForFinished(300) && p.state() == QProcess::Running)
        return true;
    p.kill();
    return p.waitForFinished(300) || p.state() == QProcess::NotRunning;
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
    case OsTypeLinux:
    case OsTypeOtherUnix:
    case OsTypeOther:
        break;
    case OsTypeWindows: {
            static const char *windowsExtensions[] = {".cmd", ".bat", ".exe", ".com"};
            // Check the Windows extensions using the order
            const int windowsExtensionCount = sizeof(windowsExtensions)/sizeof(const char*);
            for (int e = 0; e < windowsExtensionCount; e ++) {
                const QFileInfo windowsBinary(dir.filePath(binary + QLatin1String(windowsExtensions[e])));
                if (windowsBinary.isFile() && windowsBinary.isExecutable())
                    return windowsBinary.absoluteFilePath();
            }
        }
        break;
    case OsTypeMac: {
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

QString SynchronousProcess::normalizeNewlines(const QString &text)
{
    QString res = text;
    res.replace(QLatin1String("\r\n"), QLatin1String("\n"));
    return res;
}

QString SynchronousProcess::locateBinary(const QString &binary)
{
    const QByteArray path = qgetenv("PATH");
    return locateBinary(QString::fromLocal8Bit(path), binary);
}

} // namespace Utils

#include "synchronousprocess.moc"
