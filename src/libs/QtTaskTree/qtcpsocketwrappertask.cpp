// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qtcpsocketwrappertask.h>

#include <QtCore/private/qobject_p.h>

#include <QtNetwork/QTcpSocket>

#include <memory>

QT_BEGIN_NAMESPACE

using namespace QtTaskTree;

class QTcpSocketWrapperPrivate : public QObjectPrivate
{
public:
    QHostAddress m_address;
    quint16 m_port = 0;
    QByteArray m_writeData;
    std::unique_ptr<QTcpSocket> m_socket;
    QAbstractSocket::SocketError m_error = QAbstractSocket::UnknownSocketError;
};

/*!
    \class QTcpSocketWrapper
    \inheaderfile qtcpsocketwrappertask.h
    \inmodule TaskTree
    \brief A wrapper around QTcpSocket.
    \reentrant

    QTcpSocketWrapper is a convenient wrapper around QTcpSocket.

    Configure the QTcpSocketWrapper with setAddress(), setPort(),
    and setData() before calling start().

    The wrapped QTcpSocket may be accessed via socket() method.
    The QTcpSocket is created dynamically by the start() method and
    managed by QTcpSocketWrapper. It is deleted just after emitting
    done() signal.
*/

/*!
    Creates QTcpSocketWrapper with a given \a parent.
*/
QTcpSocketWrapper::QTcpSocketWrapper(QObject *parent)
    : QObject(*new QTcpSocketWrapperPrivate, parent)
{}

/*!
    Destroys the QTcpSocketWrapper. If the accociated socket()
    is still running, it's aborted.
*/
QTcpSocketWrapper::~QTcpSocketWrapper()
{
    Q_D(QTcpSocketWrapper);
    if (d->m_socket) {
        d->m_socket->disconnect();
        d->m_socket->abort();
    }
}

/*!
    Set the \a address to be used on start().
*/
void QTcpSocketWrapper::setAddress(const QHostAddress &address)
{
    d_func()->m_address = address;
}

/*!
    Set the \a port to be used on start().
*/
void QTcpSocketWrapper::setPort(quint16 port)
{
    d_func()->m_port = port;
}

/*!
    Set the \a data to be used on start(). If not empty, the \a data
    is written automatically to the socket after the connection is established.
*/
void QTcpSocketWrapper::setData(const QByteArray &data)
{
    d_func()->m_writeData = data;
}

/*!
    Returns the pointer to the accociated QTcpSocket.
    Before the QTcpSocketWrapper is started and after it's finished,
    this function returns \c nullptr.
    It's safe to access the QTcpSocket after the started() signal is emitted
    and until done() signal is emitted.
*/
QTcpSocket *QTcpSocketWrapper::socket() const
{
    return d_func()->m_socket.get();
}

/*!
    Starts the QTcpSocketWrapper.
*/
void QTcpSocketWrapper::start()
{
    Q_D(QTcpSocketWrapper);
    if (d->m_socket) {
        qWarning("The TcpSocket is already running. Ignoring the call to start().");
        return;
    }
    if (d->m_address.isNull()) {
        qWarning("Can't start the TcpSocket with invalid address. "
                 "Stopping with an error.");
        d->m_error = QAbstractSocket::HostNotFoundError;
        Q_EMIT done(DoneResult::Error);
        return;
    }

    d->m_socket.reset(new QTcpSocket);
    connect(d->m_socket.get(), &QAbstractSocket::errorOccurred, this,
            [this](QAbstractSocket::SocketError error) {
        Q_D(QTcpSocketWrapper);
        d->m_error = error;
        d->m_socket->disconnect();
        Q_EMIT done(DoneResult::Error);
        d->m_socket.release()->deleteLater();
    });
    connect(d->m_socket.get(), &QAbstractSocket::connected, this, [this] {
        Q_D(QTcpSocketWrapper);
        if (!d->m_writeData.isEmpty())
            d->m_socket->write(d->m_writeData);
        Q_EMIT started();
    });
    connect(d->m_socket.get(), &QAbstractSocket::disconnected, this, [this] {
        Q_D(QTcpSocketWrapper);
        d->m_socket->disconnect();
        Q_EMIT done(DoneResult::Success);
        d->m_socket.release()->deleteLater();
    });

    d->m_socket->connectToHost(d->m_address, d->m_port);
}

/*!
    \fn void QTcpSocketWrapper::started()

    This signal is emitted after the managed QTcpSocket is connected.

    \sa start()
*/

/*!
    \fn void QTcpSocketWrapper::done(QtTaskTree::DoneResult result)

    This signal is emitted after the associated QTcpSocket finished.
    The passed \a result indicates whether it has finished with success
    or an error.

    \sa socket()
*/

/*!
    \typedef QTcpSocketWrapperTask
    \relates QCustomTask

    Type alias for the QCustomTask<QTcpSocketWrapper>,
    to be used inside recipes.
*/

QT_END_NAMESPACE
