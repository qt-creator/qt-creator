#include "unixptyprocess.h"
#include <QStandardPaths>

#include <errno.h>
#include <termios.h>
#if !defined(Q_OS_ANDROID) && !defined(Q_OS_FREEBSD)
#include <utmpx.h>
#endif
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <QCoreApplication>
#include <QFileInfo>

UnixPtyProcess::UnixPtyProcess()
    : IPtyProcess()
    , m_readMasterNotify(0)
{
    m_shellProcess.setWorkingDirectory(
        QStandardPaths::writableLocation(QStandardPaths::HomeLocation));
}

UnixPtyProcess::~UnixPtyProcess()
{
    kill();
}

bool UnixPtyProcess::startProcess(const QString &shellPath,
                                  const QStringList &arguments,
                                  const QString &workingDir,
                                  QStringList environment,
                                  qint16 cols,
                                  qint16 rows)
{
    if (!isAvailable()) {
        m_lastError = QString("UnixPty Error: unavailable");
        return false;
    }

    if (m_shellProcess.state() == QProcess::Running)
        return false;

    QFileInfo fi(shellPath);
    if (fi.isRelative() || !QFile::exists(shellPath)) {
        //todo add auto-find executable in PATH env var
        m_lastError = QString("UnixPty Error: shell file path must be absolute");
        return false;
    }

    m_shellPath = shellPath;
    m_size = QPair<qint16, qint16>(cols, rows);

    int rc = 0;

    m_shellProcess.m_handleMaster = ::posix_openpt(O_RDWR | O_NOCTTY);

    int flags = fcntl(m_shellProcess.m_handleMaster, F_GETFL, 0);
    fcntl(m_shellProcess.m_handleMaster, F_SETFL, flags | O_NONBLOCK);

    if (m_shellProcess.m_handleMaster <= 0) {
        m_lastError = QString("UnixPty Error: unable to open master -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    m_shellProcess.m_handleSlaveName = QLatin1String(ptsname(m_shellProcess.m_handleMaster));
    if (m_shellProcess.m_handleSlaveName.isEmpty()) {
        m_lastError = QString("UnixPty Error: unable to get slave name -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    rc = grantpt(m_shellProcess.m_handleMaster);
    if (rc != 0) {
        m_lastError
            = QString("UnixPty Error: unable to change perms for slave -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    rc = unlockpt(m_shellProcess.m_handleMaster);
    if (rc != 0) {
        m_lastError = QString("UnixPty Error: unable to unlock slave -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    m_shellProcess.m_handleSlave = ::open(m_shellProcess.m_handleSlaveName.toLatin1().data(),
                                          O_RDWR | O_NOCTTY);
    if (m_shellProcess.m_handleSlave < 0) {
        m_lastError = QString("UnixPty Error: unable to open slave -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    rc = fcntl(m_shellProcess.m_handleMaster, F_SETFD, FD_CLOEXEC);
    if (rc == -1) {
        m_lastError
            = QString("UnixPty Error: unable to set flags for master -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    rc = fcntl(m_shellProcess.m_handleSlave, F_SETFD, FD_CLOEXEC);
    if (rc == -1) {
        m_lastError
            = QString("UnixPty Error: unable to set flags for slave -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    struct ::termios ttmode;
    rc = tcgetattr(m_shellProcess.m_handleMaster, &ttmode);
    if (rc != 0) {
        m_lastError = QString("UnixPty Error: termios fail -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    ttmode.c_iflag = ICRNL | IXON | IXANY | IMAXBEL | BRKINT;
#if defined(IUTF8)
    ttmode.c_iflag |= IUTF8;
#endif

    ttmode.c_oflag = OPOST | ONLCR;
    ttmode.c_cflag = CREAD | CS8 | HUPCL;
    ttmode.c_lflag = ICANON | ISIG | IEXTEN | ECHO | ECHOE | ECHOK | ECHOKE | ECHOCTL;

    ttmode.c_cc[VEOF] = 4;
    ttmode.c_cc[VEOL] = -1;
    ttmode.c_cc[VEOL2] = -1;
    ttmode.c_cc[VERASE] = 0x7f;
    ttmode.c_cc[VWERASE] = 23;
    ttmode.c_cc[VKILL] = 21;
    ttmode.c_cc[VREPRINT] = 18;
    ttmode.c_cc[VINTR] = 3;
    ttmode.c_cc[VQUIT] = 0x1c;
    ttmode.c_cc[VSUSP] = 26;
    ttmode.c_cc[VSTART] = 17;
    ttmode.c_cc[VSTOP] = 19;
    ttmode.c_cc[VLNEXT] = 22;
    ttmode.c_cc[VDISCARD] = 15;
    ttmode.c_cc[VMIN] = 1;
    ttmode.c_cc[VTIME] = 0;

#if (__APPLE__)
    ttmode.c_cc[VDSUSP] = 25;
    ttmode.c_cc[VSTATUS] = 20;
#endif

    cfsetispeed(&ttmode, B38400);
    cfsetospeed(&ttmode, B38400);

    rc = tcsetattr(m_shellProcess.m_handleMaster, TCSANOW, &ttmode);
    if (rc != 0) {
        m_lastError
            = QString("UnixPty Error: unabble to set associated params -> %1").arg(QLatin1String(strerror(errno)));
        kill();
        return false;
    }

    m_readMasterNotify = new QSocketNotifier(m_shellProcess.m_handleMaster,
                                             QSocketNotifier::Read,
                                             &m_shellProcess);
    m_readMasterNotify->setEnabled(true);
    m_readMasterNotify->moveToThread(m_shellProcess.thread());
    QObject::connect(m_readMasterNotify, &QSocketNotifier::activated, [this](int socket) {
        Q_UNUSED(socket)

        const size_t maxRead = 16 * 1024;
        static std::array<char, maxRead> buffer;

        int len = ::read(m_shellProcess.m_handleMaster, buffer.data(), buffer.size());

        struct termios termAttributes;
        tcgetattr(m_shellProcess.m_handleMaster, &termAttributes);
        const bool isPasswordEntry = !(termAttributes.c_lflag & ECHO) && (termAttributes.c_lflag & ICANON);
        m_inputFlags.setFlag(PtyInputFlag::InputModeHidden, isPasswordEntry);

        if (len > 0) {
            m_shellReadBuffer.append(buffer.data(), len);
            m_shellProcess.emitReadyRead();
        }
    });

    QObject::connect(&m_shellProcess, &QProcess::finished, &m_shellProcess, [this](int exitCode) {
        m_exitCode = exitCode;
        emit m_shellProcess.aboutToClose();
        m_readMasterNotify->disconnect();
    });

    QStringList varNames;
    for (const QString &line : std::as_const(environment))
        varNames.append(line.split("=").first());

    QProcessEnvironment envFormat;
    for (const QString &line : std::as_const(environment))
        envFormat.insert(line.split("=").first(), line.split("=").last());

    m_shellProcess.setWorkingDirectory(workingDir);
    m_shellProcess.setProcessEnvironment(envFormat);
    m_shellProcess.setReadChannel(QProcess::StandardOutput);
    m_shellProcess.start(m_shellPath, arguments);
    if (!m_shellProcess.waitForStarted())
        return false;

    m_pid = m_shellProcess.processId();

    resize(cols, rows);

    return true;
}

bool UnixPtyProcess::resize(qint16 cols, qint16 rows)
{
    struct winsize winp;
    winp.ws_col = cols;
    winp.ws_row = rows;
    winp.ws_xpixel = 0;
    winp.ws_ypixel = 0;

    bool res = ((ioctl(m_shellProcess.m_handleMaster, TIOCSWINSZ, &winp) != -1)
                && (ioctl(m_shellProcess.m_handleSlave, TIOCSWINSZ, &winp) != -1));

    if (res) {
        m_size = QPair<qint16, qint16>(cols, rows);
    }

    return res;
}

bool UnixPtyProcess::kill()
{
    m_shellProcess.m_handleSlaveName = QString();
    if (m_shellProcess.m_handleSlave >= 0) {
        ::close(m_shellProcess.m_handleSlave);
        m_shellProcess.m_handleSlave = -1;
    }
    if (m_shellProcess.m_handleMaster >= 0) {
        ::close(m_shellProcess.m_handleMaster);
        m_shellProcess.m_handleMaster = -1;
    }

    if (m_shellProcess.state() == QProcess::Running) {
        m_readMasterNotify->disconnect();
        m_readMasterNotify->deleteLater();

        m_shellProcess.terminate();
        m_shellProcess.waitForFinished(1000);

        if (m_shellProcess.state() == QProcess::Running) {
            QProcess::startDetached(QString("kill -9 %1").arg(pid()));
            m_shellProcess.kill();
            m_shellProcess.waitForFinished(1000);
        }

        return (m_shellProcess.state() == QProcess::NotRunning);
    }
    return false;
}

IPtyProcess::PtyType UnixPtyProcess::type()
{
    return IPtyProcess::UnixPty;
}

QString UnixPtyProcess::dumpDebugInfo()
{
#ifdef PTYQT_DEBUG
    return QString("PID: %1, In: %2, Out: %3, Type: %4, Cols: %5, Rows: %6, IsRunning: %7, Shell: "
                   "%8, SlaveName: %9")
        .arg(m_pid)
        .arg(m_shellProcess.m_handleMaster)
        .arg(m_shellProcess.m_handleSlave)
        .arg(type())
        .arg(m_size.first)
        .arg(m_size.second)
        .arg(m_shellProcess.state() == QProcess::Running)
        .arg(m_shellPath)
        .arg(m_shellProcess.m_handleSlaveName);
#else
    return QString("Nothing...");
#endif
}

QIODevice *UnixPtyProcess::notifier()
{
    return &m_shellProcess;
}

QByteArray UnixPtyProcess::readAll()
{
    QByteArray tmpBuffer = m_shellReadBuffer;
    m_shellReadBuffer.clear();
    return tmpBuffer;
}

qint64 UnixPtyProcess::write(const QByteArray &byteArray)
{
    return ::write(m_shellProcess.m_handleMaster, byteArray.constData(), byteArray.size());
}

bool UnixPtyProcess::isAvailable()
{
    //todo check something more if required
    return true;
}

void UnixPtyProcess::moveToThread(QThread *targetThread)
{
    m_shellProcess.moveToThread(targetThread);
}

void ShellProcess::configChildProcess()
{
    dup2(m_handleSlave, STDIN_FILENO);
    dup2(m_handleSlave, STDOUT_FILENO);
    dup2(m_handleSlave, STDERR_FILENO);

    pid_t sid = setsid();
    ioctl(m_handleSlave, TIOCSCTTY, 0);
    tcsetpgrp(m_handleSlave, sid);

#if !defined(Q_OS_ANDROID) && !defined(Q_OS_FREEBSD)
    // on Android imposible to put record to the 'utmp' file
    struct utmpx utmpxInfo;
    memset(&utmpxInfo, 0, sizeof(utmpxInfo));

    strncpy(utmpxInfo.ut_user, qgetenv("USER"), sizeof(utmpxInfo.ut_user) - 1);

    QString device(m_handleSlaveName);
    if (device.startsWith("/dev/"))
        device = device.mid(5);

    const auto deviceAsLatin1 = device.toLatin1();
    const char *d = deviceAsLatin1.constData();

    strncpy(utmpxInfo.ut_line, d, sizeof(utmpxInfo.ut_line) - 1);

    strncpy(utmpxInfo.ut_id, d + strlen(d) - sizeof(utmpxInfo.ut_id), sizeof(utmpxInfo.ut_id));

    struct timeval tv;
    gettimeofday(&tv, 0);
    utmpxInfo.ut_tv.tv_sec = tv.tv_sec;
    utmpxInfo.ut_tv.tv_usec = tv.tv_usec;

    utmpxInfo.ut_type = USER_PROCESS;
    utmpxInfo.ut_pid = getpid();

    utmpxname(_PATH_UTMPX);
    setutxent();
    pututxline(&utmpxInfo);
    endutxent();

#if !defined(Q_OS_UNIX)
    updwtmpx(_PATH_UTMPX, &loginInfo);
#endif

#endif
}
