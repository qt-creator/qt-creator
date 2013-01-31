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

#include "qpacketprotocol.h"

#include <qbuffer.h>
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

        QObject::connect(this, SIGNAL(readyRead()),
                         parent, SIGNAL(readyRead()));
        QObject::connect(this, SIGNAL(packetWritten()),
                         parent, SIGNAL(packetWritten()));
        QObject::connect(this, SIGNAL(invalidPacket()),
                         parent, SIGNAL(invalidPacket()));
        QObject::connect(dev, SIGNAL(readyRead()),
                         this, SLOT(readyToRead()));
        QObject::connect(dev, SIGNAL(aboutToClose()),
                         this, SLOT(aboutToClose()));
        QObject::connect(dev, SIGNAL(bytesWritten(qint64)),
                         this, SLOT(bytesWritten(qint64)));
    }

Q_SIGNALS:
    void readyRead();
    void packetWritten();
    void invalidPacket();

public Q_SLOTS:
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
                emit packetWritten();
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
                    QObject::disconnect(dev, SIGNAL(readyRead()),
                                        this, SLOT(readyToRead()));
                    QObject::disconnect(dev, SIGNAL(aboutToClose()),
                                        this, SLOT(aboutToClose()));
                    QObject::disconnect(dev, SIGNAL(bytesWritten(qint64)),
                                        this, SLOT(bytesWritten(qint64)));
                    dev = 0;
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
  Construct a QPacketProtocol instance that works on \a dev with the
  specified \a parent.
 */
QPacketProtocol::QPacketProtocol(QIODevice *dev, QObject *parent)
    : QObject(parent), d(new QPacketProtocolPrivate(this, dev))
{
    Q_ASSERT(dev);
}

/*!
  Destroys the QPacketProtocol instance.
 */
QPacketProtocol::~QPacketProtocol()
{
}

/*!
  Returns the maximum packet size allowed.  By default this is
  2,147,483,647 bytes.

  If a packet claiming to be larger than the maximum packet size is received,
  the QPacketProtocol::invalidPacket() signal is emitted.

  \sa QPacketProtocol::setMaximumPacketSize()
 */
qint32 QPacketProtocol::maximumPacketSize() const
{
    return d->maxPacketSize;
}

/*!
  Sets the maximum allowable packet size to \a max.

  \sa QPacketProtocol::maximumPacketSize()
 */
qint32 QPacketProtocol::setMaximumPacketSize(qint32 max)
{
    if (max > (signed)sizeof(qint32))
        d->maxPacketSize = max;
    return d->maxPacketSize;
}

/*!
  Returns a streamable object that is transmitted on destruction.  For example

  \code
  protocol.send() << "Hello world" << 123;
  \endcode

  will send a packet containing "Hello world" and 123.  To construct more
  complex packets, explicitly construct a QPacket instance.
 */
QPacketAutoSend QPacketProtocol::send()
{
    return QPacketAutoSend(this);
}

/*!
  \fn void QPacketProtocol::send(const QPacket & packet)

  Transmit the \a packet.
 */
void QPacketProtocol::send(const QPacket & p)
{
    if (p.b.isEmpty())
        return; // We don't send empty packets

    qint64 sendSize = p.b.size() + sizeof(qint32);

    d->sendingPackets.append(sendSize);
    qint32 sendSize32 = sendSize;
    qint64 writeBytes = d->dev->write((char *)&sendSize32, sizeof(qint32));
    Q_ASSERT(writeBytes == sizeof(qint32));
    writeBytes = d->dev->write(p.b);
    Q_ASSERT(writeBytes == p.b.size());
}

/*!
  Returns the number of received packets yet to be read.
  */
qint64 QPacketProtocol::packetsAvailable() const
{
    return d->packets.count();
}

/*!
  Discard any unread packets.
  */
void QPacketProtocol::clear()
{
    d->packets.clear();
}

/*!
  Return the next unread packet, or an invalid QPacket instance if no packets
  are available.  This method does NOT block.
  */
QPacket QPacketProtocol::read()
{
    if (0 == d->packets.count())
        return QPacket();

    QPacket rv(d->packets.at(0));
    d->packets.removeFirst();
    return rv;
}


/*
   Returns the difference between msecs and elapsed. If msecs is -1,
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
  This function locks until a new packet is available for reading and the
  \l{QIODevice::}{readyRead()} signal has been emitted. The function
  will timeout after \a msecs milliseconds; the default timeout is
  30000 milliseconds.

  The function returns true if the readyRead() signal is emitted and
  there is new data available for reading; otherwise it returns false
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
  Return the QIODevice passed to the QPacketProtocol constructor.
*/
QIODevice *QPacketProtocol::device()
{
    return d->dev;
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
  \fn void QPacketProtocol::packetWritten()

  Emitted each time a packet is completing written to the device.  This signal
  may be used for communications flow control.
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
QPacket::QPacket()
    : QDataStream(), buf(0)
{
    buf = new QBuffer(&b);
    buf->open(QIODevice::WriteOnly);
    setDevice(buf);
    setVersion(QDataStream::Qt_4_7);
}

/*!
  Destroys the QPacket instance.
  */
QPacket::~QPacket()
{
    if (buf) {
        delete buf;
        buf = 0;
    }
}

/*!
  Creates a copy of \a other.  The initial stream positions are shared, but the
  two packets are otherwise independent.
 */
QPacket::QPacket(const QPacket & other)
    : QDataStream(), b(other.b), buf(0)
{
    buf = new QBuffer(&b);
    buf->open(other.buf->openMode());
    setDevice(buf);
}

/*!
  \internal
  */
QPacket::QPacket(const QByteArray & ba)
    : QDataStream(), b(ba), buf(0)
{
    buf = new QBuffer(&b);
    buf->open(QIODevice::ReadOnly);
    setDevice(buf);
}

/*!
  Returns true if this packet is empty - that is, contains no data.
  */
bool QPacket::isEmpty() const
{
    return b.isEmpty();
}

/*!
  Returns raw packet data.
  */
QByteArray QPacket::data() const
{
    return b;
}

/*!
  Clears data in the packet.  This is useful for reusing one writable packet.
  For example
  \code
  QPacketProtocol protocol(...);

  QPacket packet;

  packet << "Hello world!" << 123;
  protocol.send(packet);

  packet.clear();
  packet << "Goodbyte world!" << 789;
  protocol.send(packet);
  \endcode
 */
void QPacket::clear()
{
    QBuffer::OpenMode oldMode = buf->openMode();
    buf->close();
    b.clear();
    buf->setBuffer(&b); // reset QBuffer internals with new size of b.
    buf->open(oldMode);
}

/*!
  \class QPacketAutoSend
  \internal

  \internal
  */
QPacketAutoSend::QPacketAutoSend(QPacketProtocol *_p)
    : QPacket(), p(_p)
{
}

QPacketAutoSend::~QPacketAutoSend()
{
    if (!b.isEmpty())
        p->send(*this);
}

} // namespace QmlDebug

#include <qpacketprotocol.moc>
