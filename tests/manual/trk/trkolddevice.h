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

#ifndef TRKOLDDEVICE_H
#define TRKOLDDEVICE_H

#include "callback.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QByteArray>
#include <QtCore/QSharedPointer>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE


namespace trk {
    struct TrkResult;
}

namespace trkold {

struct TrkMessage;
struct TrkDevicePrivate;
class TrkWriteQueue;
struct TrkWriteQueueIODevicePrivate;

/* TrkDevice: Implements a Windows COM or Linux device for
 * Trk communications. Provides synchronous write and asynchronous
 * read operation.
 * The serialFrames property specifies whether packets are encapsulated in
 * "0x90 <length>" frames, which is currently the case for serial ports. */

class TrkDevice : public QObject
{
    Q_DISABLE_COPY(TrkDevice)
    Q_OBJECT
    Q_PROPERTY(bool serialFrame READ serialFrame WRITE setSerialFrame)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose)
public:
    explicit TrkDevice(QObject *parent = 0);
    virtual ~TrkDevice();

    bool open(const QString &port, QString *errorMessage);
    bool isOpen() const;
    void close();

    QString errorString() const;

    bool serialFrame() const;
    void setSerialFrame(bool f);

    bool verbose() const;
    void setVerbose(bool b);

    bool write(const QByteArray &data, QString *errorMessage);

signals:
    void messageReceived(const trk::TrkResult &result);
    // Emitted with the contents of messages enclosed in 07e, not for log output
    void rawDataReceived(const QByteArray &data);
    void error(const QString &msg);
    void logMessage(const QString &msg);

protected:
    void emitError(const QString &msg);
    virtual void timerEvent(QTimerEvent *ev);

private:
    void tryTrkRead();

    TrkDevicePrivate *d;
};

/* TrkWriteQueueDevice: Extends TrkDevice by write message queue allowing
 * for queueing messages with a notification callback. If the message receives
 * an ACK, the callback is invoked.
 * The special message TRK_WRITE_QUEUE_NOOP_CODE code can be used for synchronisation.
 * The respective  message will not be sent, the callback is just invoked. */

enum { TRK_WRITE_QUEUE_NOOP_CODE = 0x7f };

class TrkWriteQueueDevice : public TrkDevice
{
    Q_OBJECT
public:
    explicit TrkWriteQueueDevice(QObject *parent = 0);
    virtual ~TrkWriteQueueDevice();

    // Construct as 'TrkWriteQueueDevice::TrkCallback(instance, &Class::method);'
    typedef trk::Callback<const trk::TrkResult &> TrkCallback;

    // Enqueue a message with a notification callback.
    void sendTrkMessage(unsigned char code,
                        TrkCallback callBack = TrkCallback(),
                        const QByteArray &data = QByteArray(),
                        const QVariant &cookie = QVariant(),
                        // Invoke callback on receiving NAK, too.
                        bool invokeOnNAK = false);

    // Enqeue an initial ping
    void sendTrkInitialPing();

    // Send an Ack synchronously, bypassing the queue
    bool sendTrkAck(unsigned char token);

signals:
    void logMessage(const QString &msg);

private slots:
    void slotHandleResult(const trk::TrkResult &);

protected:
    virtual void timerEvent(QTimerEvent *ev);

private:
    void tryTrkWrite();
    bool trkWriteRawMessage(const TrkMessage &msg);

    TrkWriteQueue *qd;
};

/* A Trk queueing device wrapping around a QIODevice (otherwise
 * mimicking TrkWriteQueueDevice).
 * Can be used to forward Trk over a network or to simulate things. */

class TrkWriteQueueIODevice : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(TrkWriteQueueIODevice)
    Q_PROPERTY(bool serialFrame READ serialFrame WRITE setSerialFrame)
    Q_PROPERTY(bool verbose READ verbose WRITE setVerbose)
public:
    typedef trk::Callback<const trk::TrkResult &> TrkCallback;

    explicit TrkWriteQueueIODevice(const QSharedPointer<QIODevice> &device,
                                   QObject *parent = 0);
    virtual ~TrkWriteQueueIODevice();

    bool serialFrame() const;
    void setSerialFrame(bool f);

    bool verbose() const;
    void setVerbose(bool b);

    void sendTrkMessage(unsigned char code,
                        TrkCallback callback = TrkCallback(),
                        const QByteArray &data = QByteArray(),
                        const QVariant &cookie = QVariant(),
                        bool invokeOnNAK = false);
    void sendTrkInitialPing();
    bool sendTrkAck(unsigned char token);

signals:
    void messageReceived(const trk::TrkResult&);
    // Emitted with the contents of messages enclosed in 07e, not for log output
    void rawDataReceived(const QByteArray &data);
    void logMessage(const QString &msg);


protected:
    virtual void timerEvent(QTimerEvent *ev);

private:
    void tryTrkRead();
    void tryTrkWrite();
    bool trkWriteRawMessage(const TrkMessage &msg);

    TrkWriteQueueIODevicePrivate *d;
};

} // namespace trkold

#endif // TRKOLDDEVICE_H
