// Copyright (C) 2025 Jarek Kobus
// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtTaskTree/qnetworkreplywrappertask.h>

#include <QtCore/private/qobject_p.h>

#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>

#include <memory>

QT_BEGIN_NAMESPACE

using namespace QtTaskTree;

class QNetworkReplyWrapperPrivate : public QObjectPrivate
{
public:
    QNetworkRequest m_request;
    QNetworkAccessManager::Operation m_operation = QNetworkAccessManager::GetOperation;
    QByteArray m_verb; // Used by CustomOperation
    QByteArray m_writeData; // Used by PutOperation, PostOperation and CustomOperation
    QNetworkAccessManager *m_manager = nullptr;
    std::unique_ptr<QNetworkReply> m_reply;
};

/*!
    \class QNetworkReplyWrapper
    \inheaderfile qnetworkwrappertask.h
    \inmodule TaskTree
    \brief A wrapper around QNetworkReply and QNetworkAccessManager.
    \reentrant

    QNetworkReplyWrapper is a convenient class combining QNetworkAccessManager
    and QNetworkReply.

    The mandatory configuration is to call setNetworkAccessManager()
    and setRequest(). By default the QNetworkReplyWrapper is configured
    with QNetworkAccessManager::GetOperation. Use setOperation() to perform
    other operations. Use setData() when the configured operation is
    Put, Post or Custom. Use setVerb() when the configured operation is Custom.

    The associated QNetworkReply may be accessed via reply() method.
    The QNetworkReply is created dynamically by the start() method and
    managed by QNetworkReplyWrapper.
    It is deleted just after emitting done() signal.
*/

/*!
    Creates QNetworkReplyWrapper with a given \a parent.
*/
QNetworkReplyWrapper::QNetworkReplyWrapper(QObject *parent)
    : QObject(*new QNetworkReplyWrapperPrivate, parent)
{}

/*!
    Destroys the QNetworkReplyWrapper.
    If the accociated reply() is still running, it's aborted.
*/
QNetworkReplyWrapper::~QNetworkReplyWrapper()
{
    Q_D(QNetworkReplyWrapper);
    if (d->m_reply) {
        disconnect(d->m_reply.get(), nullptr, this, nullptr);
        d->m_reply->abort();
    }
}

/*!
    Set the \a request to be used on start().
*/
void QNetworkReplyWrapper::setRequest(const QNetworkRequest &request)
{
    d_func()->m_request = request;
}

/*!
    Set the \a operation to be used on start().
*/
void QNetworkReplyWrapper::setOperation(QNetworkAccessManager::Operation operation)
{
    d_func()->m_operation = operation;
}

/*!
    Set the \a verb to be used on start(). It's used only in case of
    QNetworkAccessManager::CustomOperation.
*/
void QNetworkReplyWrapper::setVerb(const QByteArray &verb)
{
    d_func()->m_verb = verb;
}

/*!
    Set the \a data to be used on start(). It's used only in case of
    QNetworkAccessManager::PutOperation, QNetworkAccessManager::PostOperation,
    or QNetworkAccessManager::CustomOperation.
*/
void QNetworkReplyWrapper::setData(const QByteArray &data)
{
    d_func()->m_writeData = data;
}

/*!
    Set the \a manager to be used on start().
*/
void QNetworkReplyWrapper::setNetworkAccessManager(QNetworkAccessManager *manager)
{
    d_func()->m_manager = manager;
}

/*!
    Returns the pointer to the accociated QNetworkReply.
    Before the QNetworkReplyWrapper is started and after it's finished,
    this function returns nullptr.
    It's safe to access the QNetworkReply after the started() signal is emitted
    and until done() signal is emitted.
*/
QNetworkReply *QNetworkReplyWrapper::reply() const
{
    return d_func()->m_reply.get();
}

/*!
    Starts the QNetworkReplyWrapper.
*/
void QNetworkReplyWrapper::start()
{
    Q_D(QNetworkReplyWrapper);
    if (d->m_reply) {
        qWarning("The QNetworkReplyWrapper is already running. Ignoring the call to start().");
        return;
    }
    if (!d->m_manager) {
        qWarning("Can't start the QNetworkReplyWrapper without the QNetworkAccessManager. "
                 "Stopping with an error.");
        Q_EMIT done(DoneResult::Error);
        return;
    }
    switch (d->m_operation) {
    case QNetworkAccessManager::HeadOperation:
        d->m_reply.reset(d->m_manager->head(d->m_request));
        break;
    case QNetworkAccessManager::GetOperation:
        d->m_reply.reset(d->m_manager->get(d->m_request));
        break;
    case QNetworkAccessManager::PutOperation:
        d->m_reply.reset(d->m_manager->put(d->m_request, d->m_writeData));
        break;
    case QNetworkAccessManager::PostOperation:
        d->m_reply.reset(d->m_manager->post(d->m_request, d->m_writeData));
        break;
    case QNetworkAccessManager::DeleteOperation:
        d->m_reply.reset(d->m_manager->deleteResource(d->m_request));
        break;
    case QNetworkAccessManager::CustomOperation:
        d->m_reply.reset(d->m_manager->sendCustomRequest(d->m_request, d->m_verb, d->m_writeData));
        break;
    case QNetworkAccessManager::UnknownOperation:
        qWarning("Can't start the QNetworkReplyWrapper with UnknownOperation. "
                 "Stopping with an error.");
        Q_EMIT done(DoneResult::Error);
        return;
    }

    connect(d->m_reply.get(), &QNetworkReply::downloadProgress, this, &QNetworkReplyWrapper::downloadProgress);
#if QT_CONFIG(ssl)
    connect(d->m_reply.get(), &QNetworkReply::sslErrors, this, &QNetworkReplyWrapper::sslErrors);
#endif
    connect(d->m_reply.get(), &QNetworkReply::finished, this, [this] {
        Q_D(QNetworkReplyWrapper);
        Q_EMIT done(toDoneResult(d->m_reply->error() == QNetworkReply::NoError));
        d->m_reply.release()->deleteLater();
    }, Qt::SingleShotConnection);

    if (d->m_reply->isRunning())
        Q_EMIT started();
}

/*!
    \fn void QNetworkReplyWrapper::started()

    This signal is emitted after successful start of the managed QNetworkReply.

    \sa start()
*/

/*!
    \fn void QNetworkReplyWrapper::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)

    This signal re-emitted from the running QNetworkReply,
    passing \a bytesReceived and \a bytesTotal.

    \sa reply()
*/

#if QT_CONFIG(ssl)
/*!
    \fn void QNetworkReplyWrapper::sslErrors(const QList<QSslError> &errors)

    This signal re-emitted from the running QNetworkReply, passing a list of ssl \a errors.

    \sa reply()
*/
#endif

/*!
    \fn void QNetworkReplyWrapper::done(QtTaskTree::DoneResult result)

    This signal is emitted after the associated QNetworkReply finished.
    The passed \a result indicates whether it has finished with success
    or an error.

    \sa reply()
*/

/*!
    \typedef QNetworkReplyWrapperTask
    \relates QCustomTask

    Type alias for the QCustomTask<QNetworkReplyWrapper>,
    to be used inside recipes.
*/

QT_END_NAMESPACE
