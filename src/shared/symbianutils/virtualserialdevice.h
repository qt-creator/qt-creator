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

#ifndef VIRTUALSERIALPORT_H
#define VIRTUALSERIALPORT_H

#include <QIODevice>
#include <QString>
#include <QMutex>

QT_BEGIN_NAMESPACE
class QWaitCondition;
QT_END_NAMESPACE

#include "symbianutils_global.h"

namespace SymbianUtils {

class VirtualSerialDevicePrivate;

class SYMBIANUTILS_EXPORT VirtualSerialDevice : public QIODevice
{
    Q_OBJECT
public:
    explicit VirtualSerialDevice(const QString &name, QObject *parent = 0);
    ~VirtualSerialDevice();

    bool open(OpenMode mode);
    void close();
    const QString &getPortName() const;
    void flush();

    qint64 bytesAvailable() const;
    bool isSequential() const;
    bool waitForBytesWritten(int msecs);
    bool waitForReadyRead(int msecs);

protected:
    qint64 readData(char *data, qint64 maxSize);
    qint64 writeData(const char *data, qint64 maxSize);

private:
    void platInit();
    void platClose();
    void emitBytesWrittenIfNeeded(QMutexLocker &locker, qint64 len);

private:
    QString portName;
    mutable QMutex lock;
    QList<QByteArray> pendingWrites;
    bool emittingBytesWritten;
    QWaitCondition* waiterForBytesWritten;
    VirtualSerialDevicePrivate *d;

// Platform-specific stuff
#ifdef Q_OS_WIN
private:
    qint64 writeNextBuffer(QMutexLocker &locker);
    void doWriteCompleted(QMutexLocker &locker);
private slots:
    void writeCompleted();
    void commEventOccurred();
#endif

#ifdef Q_OS_UNIX
private:
    bool tryWrite(const char *data, qint64 maxSize, qint64 &bytesWritten);
    enum FlushPendingOption {
        NothingSpecial = 0,
        StopAfterWritingOneBuffer = 1,
        EmitBytesWrittenAsync = 2 // Needed so we don't emit bytesWritten signal directly from writeBytes
    };
    Q_DECLARE_FLAGS(FlushPendingOptions, FlushPendingOption)
    bool tryFlushPendingBuffers(QMutexLocker& locker, FlushPendingOptions flags = NothingSpecial);

private slots:
    void writeHasUnblocked(int fileHandle);

signals:
    void AsyncCall_emitBytesWrittenIfNeeded(qint64 len);

#endif

};

} // namespace SymbianUtils

#endif // VIRTUALSERIALPORT_H
