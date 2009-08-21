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

#ifndef TRKDEVICE_H
#define TRKDEVICE_H

#include "trkfunctor.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QByteArray>

namespace trk {

struct TrkResult;
struct TrkMessage;
struct TrkDevicePrivate;
struct TrkWriteQueueDevicePrivate;

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
    void messageReceived(const TrkResult&);
    void error(const QString &s);

protected:
    void emitError(const QString &);
    virtual void timerEvent(QTimerEvent *ev);

private:
    void tryTrkRead();

    TrkDevicePrivate *d;
};

/* TrkWriteQueueDevice: Extends TrkDevice by write message queue allowing
 * for queueing messages with a notification callback. If the message receives
 * an ACK, the callback is invoked. */
class TrkWriteQueueDevice : public TrkDevice
{
    Q_OBJECT
public:
    explicit TrkWriteQueueDevice(QObject *parent = 0);
    virtual ~TrkWriteQueueDevice();

    // Construct as 'TrkWriteQueueDevice::Callback(instance, &Klass::method);'
    typedef TrkFunctor1<const TrkResult &> Callback;

    // Enqueue a message with a notification callback.
    void sendTrkMessage(unsigned char code,
                        Callback callBack = Callback(),
                        const QByteArray &data = QByteArray(),
                        const QVariant &cookie = QVariant(),
                        // Invoke callback on receiving NAK, too.
                        bool invokeOnNAK = false);

    // Enqeue an initial ping
    void sendTrkInitialPing();

    // Send an Ack synchronously, bypassing the queue
    bool sendTrkAck(unsigned char token);

private slots:
    void slotHandleResult(const TrkResult &);

protected:
    virtual void timerEvent(QTimerEvent *ev);

private:
    unsigned char nextTrkWriteToken();
    void queueTrkMessage(const TrkMessage &msg);
    void tryTrkWrite();
    bool trkWriteRawMessage(const TrkMessage &msg);
    bool trkWrite(const TrkMessage &msg);

    TrkWriteQueueDevicePrivate *qd;
};


} // namespace trk

#endif // TRKDEVICE_H
