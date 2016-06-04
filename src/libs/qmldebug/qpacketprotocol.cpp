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

#include "qpacketprotocol.h"

#include <qelapsedtimer.h>

namespace QmlDebug {

static const unsigned int MAX_PACKET_SIZE = 0x7FFFFFFF;

/*!
  \class QPacketProtocol
  \internal

  \brief The QPacketProtocol class encapsulates communicating discrete packets
  across fragmented IO channels, such as TCP sockets.

  QPacketProtocol makes it simple to send arbitrary sized data "packets" across
  fragmented transports such as TCP and UDP.

  As transmission boundaries are not respected, sending packets over protocols
  like TCP frequently involves "stitching" them back together at the receiver.
  QPacketProtocol makes this easier by performing this task for you.  Packet
  data sent using QPacketProtocol is prepended with a 4-byte size header
  allowing the receiving QPacketProtocol to buffer the packet internally until
  it has all been received.  QPacketProtocol does not perform any sanity
  checking on the size or on the data, so this class should only be used in
  prototyping or trusted situations where DOS attacks are unlikely.

  QPacketProtocol does not perform any communications itself.  Instead it can
  operate on any QIODevice that supports the QIODevice::readyRead() signal.  A
  logical "packet" is encapsulated by the companion QPacket class.  The
  following example shows two ways to send data using QPacketProtocol.  The
  transmitted data is equivalent in both.

  \code
  QTcpSocket socket;
  // ... connect socket ...

  QPacketProtocol protocol(&socket);

  // Send packet the quick way
  protocol.send() << "Hello world" << 123;

  // Send packet the longer way
  QPacket packet;
  packet << "Hello world" << 123;
  protocol.send(packet);
  \endcode

  Likewise, the following shows how to read data from QPacketProtocol, assuming
  that the QPacketProtocol::readyRead() signal has been emitted.

  \code
  // ... QPacketProtocol::readyRead() is emitted ...

  int a;
  QByteArray b;

  // Receive packet the quick way
  protocol.read() >> a >> b;

  // Receive packet the longer way
  QPacket packet = protocol.read();
  p >> a >> b;
  \endcode

  \ingroup io
  \sa QPacket
*/

class QPacketProtocolPrivate : public QObject
{
    Q_OBJECT

public:
    QPacketProtocolPrivate(QPacketProtocol *parent, QIODevice *_dev)
        : QObject(parent), inProgressSize(-1), maxPacketSize(MAX_PACKET_SIZE),
          waitingForPacket(false), dev(_dev)
    {
        Q_ASSERT(4 == sizeof(qint32));

        QObject::connect(this, &QPacketProtocolPrivate::readyRead,
                         parent, &QPacketProtocol::readyRead);
        QObject::connect(this, &QPacketProtocolPrivate::invalidPacket,
                         parent, &QPacketProtocol::invalidPacket);
        QObject::connect(dev, &QIODevice::readyRead,
                         this, &QPacketProtocolPrivate::readyToRead);
        QObject::connect(dev, &QIODevice::aboutToClose,
                         this, &QPacketProtocolPrivate::aboutToClose);
        QObject::connect(dev, &QIODevice::bytesWritten,
                         this, &QPacketProtocolPrivate::bytesWritten);
    }

signals:
    void readyRead();
    void invalidPacket();

public:
    void aboutToClose()
    {
        inProgress.clear();
        sendingPackets.clear();
        inProgressSize = -1;
    }

    void bytesWritten(qint64 bytes)
    {
        Q_ASSERT(!sendingPackets.isEmpty());

        while (bytes) {
            if (sendingPackets.at(0) > bytes) {
                sendingPackets[0] -= bytes;
                bytes = 0;
            } else {
                bytes -= sendingPackets.at(0);
                sendingPackets.removeFirst();
            }
        }
    }

    void readyToRead()
    {
        while (true) {
            // Need to get trailing data
            if (-1 == inProgressSize) {
                // We need a size header of sizeof(qint32)
                if (sizeof(qint32) > (uint)dev->bytesAvailable())
                    return;

                // Read size header
                int read = dev->read((char *)&inProgressSize, sizeof(qint32));
                Q_ASSERT(read == sizeof(qint32));
                Q_UNUSED(read);

                // Check sizing constraints
                if (inProgressSize > maxPacketSize) {
                    QObject::disconnect(dev, &QIODevice::readyRead,
                                        this, &QPacketProtocolPrivate::readyToRead);
                    QObject::disconnect(dev, &QIODevice::aboutToClose,
                                        this, &QPacketProtocolPrivate::aboutToClose);
                    QObject::disconnect(dev, &QIODevice::bytesWritten,
                                        this, &QPacketProtocolPrivate::bytesWritten);
                    emit invalidPacket();
                    return;
                }

                inProgressSize -= sizeof(qint32);
            } else {
                inProgress.append(dev->read(inProgressSize - inProgress.size()));

                if (inProgressSize == inProgress.size()) {
                    // Packet has arrived!
                    packets.append(inProgress);
                    inProgressSize = -1;
                    inProgress.clear();

                    waitingForPacket = false;
                    emit readyRead();
                } else
                    return;
            }
        }
    }

public:
    QList<qint64> sendingPackets;
    QList<QByteArray> packets;
    QByteArray inProgress;
    qint32 inProgressSize;
    qint32 maxPacketSize;
    bool waitingForPacket;
    QIODevice *dev;
};

/*!
  Constructs a QPacketProtocol instance that works on \a dev with the
  specified \a parent.
 */
QPacketProtocol::QPacketProtocol(QIODevice *dev, QObject *parent)
    : QObject(parent), d(new QPacketProtocolPrivate(this, dev))
{
    Q_ASSERT(dev);
}

/*!
  Transmits the packet \a p.
 */
void QPacketProtocol::send(const QByteArray &p)
{
    if (p.isEmpty())
        return; // We don't send empty packets

    qint64 sendSize = p.size() + sizeof(qint32);

    d->sendingPackets.append(sendSize);
    qint32 sendSize32 = sendSize;
    qint64 writeBytes = d->dev->write((char *)&sendSize32, sizeof(qint32));
    Q_ASSERT(writeBytes == sizeof(qint32));
    writeBytes = d->dev->write(p);
    Q_ASSERT(writeBytes == p.size());
    Q_UNUSED(writeBytes); // For building in release mode.
}

/*!
  Returns the number of received packets yet to be read.
  */
qint64 QPacketProtocol::packetsAvailable() const
{
    return d->packets.count();
}

/*!
  Returns the next unread packet, or an invalid QPacket instance if no packets
  are available.  This function does NOT block.
  */
QByteArray QPacketProtocol::read()
{
    if (0 == d->packets.count())
        return QByteArray();

    return d->packets.takeFirst();
}


/*!
   Returns the difference between \a msecs and \a elapsed. If msecs is -1,
   however, -1 is returned.
*/
static int qt_timeout_value(int msecs, int elapsed)
{
    if (msecs == -1)
        return -1;

    int timeout = msecs - elapsed;
    return timeout < 0 ? 0 : timeout;
}

/*!
  Locks until a new packet is available for reading and the
  \l{QIODevice::}{readyRead()} signal has been emitted. The function
  will timeout after \a msecs milliseconds; the default timeout is
  30000 milliseconds.

  Returns true if the readyRead() signal is emitted and
  there is new data available for reading; otherwise returns false
  (if an error occurred or the operation timed out).
  */

bool QPacketProtocol::waitForReadyRead(int msecs)
{
    if (!d->packets.isEmpty())
        return true;

    QElapsedTimer stopWatch;
    stopWatch.start();

    d->waitingForPacket = true;
    do {
        if (!d->dev->waitForReadyRead(msecs))
            return false;
        if (!d->waitingForPacket)
            return true;
        msecs = qt_timeout_value(msecs, stopWatch.elapsed());
    } while (true);
}

/*!
  \fn void QPacketProtocol::readyRead()

  Emitted whenever a new packet is received.  Applications may use
  QPacketProtocol::read() to retrieve this packet.
 */

/*!
  \fn void QPacketProtocol::invalidPacket()

  A packet larger than the maximum allowable packet size was received.  The
  packet will be discarded and, as it indicates corruption in the protocol, no
  further packets will be received.
 */

/*!
  \class QPacket
  \internal

  \brief The QPacket class encapsulates an unfragmentable packet of data to be
  transmitted by QPacketProtocol.

  The QPacket class works together with QPacketProtocol to make it simple to
  send arbitrary sized data "packets" across fragmented transports such as TCP
  and UDP.

  QPacket provides a QDataStream interface to an unfragmentable packet.
  Applications should construct a QPacket, propagate it with data and then
  transmit it over a QPacketProtocol instance.  For example:
  \code
  QPacketProtocol protocol(...);

  QPacket myPacket;
  myPacket << "Hello world!" << 123;
  protocol.send(myPacket);
  \endcode

  As long as both ends of the connection are using the QPacketProtocol class,
  the data within this packet will be delivered unfragmented at the other end,
  ready for extraction.

  \code
  QByteArray greeting;
  int count;

  QPacket myPacket = protocol.read();

  myPacket >> greeting >> count;
  \endcode

  Only packets returned from QPacketProtocol::read() may be read from.  QPacket
  instances constructed by directly by applications are for transmission only
  and are considered "write only".  Attempting to read data from them will
  result in undefined behavior.

  \ingroup io
  \sa QPacketProtocol
 */

/*!
  Constructs an empty write-only packet.
  */
QPacket::QPacket(int version)
{
    buf.open(QIODevice::WriteOnly);
    setDevice(&buf);
    setVersion(version);
}

/*!
  Constructs a read-only packet.
 */
QPacket::QPacket(int version, const QByteArray &data)
{
    buf.setData(data);
    buf.open(QIODevice::ReadOnly);
    setDevice(&buf);
    setVersion(version);
}

/*!
  Returns raw packet data.
  */
QByteArray QPacket::data() const
{
    return buf.data();
}

} // namespace QmlDebug

#include <qpacketprotocol.moc>
