/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#include "trkdevice.h"
#include "trkutils.h"

#include <QtCore/QString>
#include <QtCore/QDebug>

#ifdef Q_OS_WIN
#  include <windows.h>
#else
#  include <QtCore/QFile>

#  include <stdio.h>
#  include <sys/ioctl.h>
#  include <termios.h>
#  include <errno.h>
#  include <string.h>
#endif

enum { TimerInterval = 100 };

#ifdef Q_OS_WIN

// Format windows error from GetLastError() value: TODO: Use the one provided by the utisl lib.
QString winErrorMessage(unsigned long error)
{
    QString rc = QString::fromLatin1("#%1: ").arg(error);
    ushort *lpMsgBuf;

    const int len = FormatMessage(
            FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR)&lpMsgBuf, 0, NULL);
    if (len) {
        rc = QString::fromUtf16(lpMsgBuf, len);
        LocalFree(lpMsgBuf);
    } else {
        rc += QString::fromLatin1("<unknown error>");
    }
    return rc;
}

// Non-blocking replacement for win-api ReadFile function
BOOL WINAPI TryReadFile(HANDLE          hFile,
                        LPVOID          lpBuffer,
                        DWORD           nNumberOfBytesToRead,
                        LPDWORD         lpNumberOfBytesRead,
                        LPOVERLAPPED    lpOverlapped)
{
    COMSTAT comStat;
    if (!ClearCommError(hFile, NULL, &comStat)){
        qDebug() << "ClearCommError() failed";
        return FALSE;
    }
    if (comStat.cbInQue == 0) {
        *lpNumberOfBytesRead = 0;
        return FALSE;
    }
    return ReadFile(hFile,
                    lpBuffer,
                    qMin(comStat.cbInQue, nNumberOfBytesToRead),
                    lpNumberOfBytesRead,
                    lpOverlapped);
}
#endif

namespace trk {

struct TrkDevicePrivate {
    TrkDevicePrivate();
#ifdef Q_OS_WIN
    HANDLE hdevice;
#else
    QFile file;
#endif

    QByteArray trkReadBuffer;
    bool trkWriteBusy;
    int timerId;
    bool serialFrame;
    bool verbose;
    QString errorString;
};

TrkDevicePrivate::TrkDevicePrivate() :
#ifdef Q_OS_WIN
    hdevice(INVALID_HANDLE_VALUE),
#endif
    trkWriteBusy(false),
    timerId(-1),
    serialFrame(true),
    verbose(false)
{

}

TrkDevice::TrkDevice(QObject *parent) :
    QObject(parent),
    d(new TrkDevicePrivate)
{
}

bool TrkDevice::open(const QString &port, QString *errorMessage)
{
    close();
#ifdef Q_OS_WIN
    d->hdevice = CreateFile(port.toStdWString().c_str(),
                           GENERIC_READ | GENERIC_WRITE,
                           0,
                           NULL,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

    if (INVALID_HANDLE_VALUE == d->hdevice) {
        *errorMessage = QString::fromLatin1("Could not open device '%1': %2").arg(port, winErrorMessage(GetLastError()));
        return false;
    }
    d->timerId = startTimer(TimerInterval);
    return true;
#else
    d->file.setFileName(port);
    if (!d->file.open(QIODevice::ReadWrite|QIODevice::Unbuffered)) {
        *errorMessage = QString::fromLatin1("Cannot open %1: %2").arg(port, d->file.errorString());
        return false;
    }

    struct termios termInfo;
    if (tcgetattr(d->file.handle(), &termInfo) < 0) {
        *errorMessage = QString::fromLatin1("Unable to retrieve terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno)));
        return false;
    }
    // Turn off terminal echo as not get messages back, among other things
    termInfo.c_cflag|=CREAD|CLOCAL;
    termInfo.c_lflag&=(~(ICANON|ECHO|ECHOE|ECHOK|ECHONL|ISIG));
    termInfo.c_iflag&=(~(INPCK|IGNPAR|PARMRK|ISTRIP|ICRNL|IXANY));
    termInfo.c_oflag&=(~OPOST);
    termInfo.c_cc[VMIN]=0;
    termInfo.c_cc[VINTR] = _POSIX_VDISABLE;
    termInfo.c_cc[VQUIT] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTART] = _POSIX_VDISABLE;
    termInfo.c_cc[VSTOP] = _POSIX_VDISABLE;
    termInfo.c_cc[VSUSP] = _POSIX_VDISABLE;
    if (tcsetattr(d->file.handle(), TCSAFLUSH, &termInfo) < 0) {
        *errorMessage = QString::fromLatin1("Unable to apply terminal settings: %1 %2").arg(errno).arg(QString::fromAscii(strerror(errno)));
        return false;
    }
    d->timerId = startTimer(TimerInterval);
    return true;
#endif
}


TrkDevice::~TrkDevice()
{
    close();
    delete d;
}

void TrkDevice::close()
{
    if (!isOpen())
        return;
    if (d->timerId != -1) {
        killTimer(d->timerId);
        d->timerId = -1;
    }
#ifdef Q_OS_WIN
    CloseHandle(d->hdevice);
#else
    d->file.close();
#endif
    if (d->verbose)
        qDebug() << "Close";
}

bool TrkDevice::isOpen() const
{
#ifdef Q_OS_WIN
    return d->hdevice != INVALID_HANDLE_VALUE;
#else
    return d->file.isOpen();
#endif
}

QString TrkDevice::errorString() const
{
    return d->errorString;
}

bool TrkDevice::serialFrame() const
{
    return d->serialFrame;
}

void TrkDevice::setSerialFrame(bool f)
{
    d->serialFrame = f;
}

bool TrkDevice::verbose() const
{
    return d->verbose;
}

void TrkDevice::setVerbose(bool b)
{
    d->verbose = b;
}

bool TrkDevice::write(const QByteArray &data, QString *errorMessage)
{
    if (d->verbose)
        qDebug() << ">WRITE" << data.toHex();
#ifdef Q_OS_WIN
    DWORD charsWritten;
    if (!WriteFile(d->hdevice, data.data(), data.size(), &charsWritten, NULL)) {
        *errorMessage = QString::fromLatin1("Error writing data: %1").arg(winErrorMessage(GetLastError()));
        return false;
    }
    FlushFileBuffers(d->hdevice);
    return true;
#else
    if (d->file.write(data) == -1 || !d->file.flush()) {
        *errorMessage = QString::fromLatin1("Cannot write: %1").arg(d->file.errorString());
        return false;
    }
    return  true;
#endif
}

#ifndef Q_OS_WIN
static inline int bytesAvailable(int fileNo)
{
    int numBytes;
    const int rc = ioctl(fileNo, FIONREAD, &numBytes);
    if (rc < 0)
        numBytes=0;
    return numBytes;
}
#endif

void TrkDevice::tryTrkRead()
{
#ifdef Q_OS_WIN
    const DWORD BUFFERSIZE = 1024;
    char buffer[BUFFERSIZE];
    DWORD charsRead;
    DWORD totalCharsRead = 0;

    while (TryReadFile(d->hdevice, buffer, BUFFERSIZE, &charsRead, NULL)) {
        totalCharsRead += charsRead;
        d->trkReadBuffer.append(buffer, charsRead);
        if (isValidTrkResult(d->trkReadBuffer, d->serialFrame))
            break;
    }
    if (d->verbose && totalCharsRead)
        qDebug() << "Read" << d->trkReadBuffer.toHex();
    if (!totalCharsRead)
        return;
    const ushort len = isValidTrkResult(d->trkReadBuffer, d->serialFrame);
    if (!len) {
        d->errorString = QString::fromLatin1("Partial message: %1").arg(stringFromArray(d->trkReadBuffer));
        qWarning("%s\n", qPrintable(d->errorString));
        return;
    }
#else
    const int size = bytesAvailable(d->file.handle());
    if (!size)
        return;
    const QByteArray data = d->file.read(size);
    d->trkReadBuffer.append(data);
    const ushort len = isValidTrkResult(d->trkReadBuffer, d->serialFrame);
    if (!len) {
        if (d->trkReadBuffer.size() > 10) {
            d->errorString = QString::fromLatin1("Unable to extract message from '%1' '%2'").
                             arg(QLatin1String(d->trkReadBuffer.toHex())).arg(QString::fromAscii(d->trkReadBuffer));
            qWarning("%s\n", qPrintable(d->errorString));
        }
        return;
    }
#endif // Q_OS_WIN
    TrkResult r;
    while (extractResult(&d->trkReadBuffer, d->serialFrame, &r)) {
        if (d->verbose)
            qDebug() << "Read TrkResult " << r.data.toHex();
        emit messageReceived(r);
    }
}

void TrkDevice::timerEvent(QTimerEvent *)
{
    tryTrkRead();
}

} // namespace trk
