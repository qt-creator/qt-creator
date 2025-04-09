// Copyright (C) 2024 Jarek Kobus
// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "networkquery.h"

#include <QtNetwork/QNetworkAccessManager>

QT_BEGIN_NAMESPACE

namespace Tasking {

void NetworkQuery::start()
{
    if (m_reply) {
        qWarning("The NetworkQuery is already running. Ignoring the call to start().");
        return;
    }
    if (!m_manager) {
        qWarning("Can't start the NetworkQuery without the QNetworkAccessManager. "
                 "Stopping with an error.");
        emit done(DoneResult::Error);
        return;
    }
    switch (m_operation) {
    case NetworkOperation::Get:
        m_reply.reset(m_manager->get(m_request));
        break;
    case NetworkOperation::Put:
        m_reply.reset(m_manager->put(m_request, m_writeData));
        break;
    case NetworkOperation::Post:
        m_reply.reset(m_manager->post(m_request, m_writeData));
        break;
    case NetworkOperation::Delete:
        m_reply.reset(m_manager->deleteResource(m_request));
        break;
    }

    connect(m_reply.get(), &QNetworkReply::downloadProgress, this, &NetworkQuery::downloadProgress);
#if QT_CONFIG(ssl)
    connect(m_reply.get(), &QNetworkReply::sslErrors, this, &NetworkQuery::sslErrors);
#endif
    connect(m_reply.get(), &QNetworkReply::finished, this, [this] {
        disconnect(m_reply.get(), &QNetworkReply::finished, this, nullptr);
        emit done(toDoneResult(m_reply->error() == QNetworkReply::NoError));
        m_reply.release()->deleteLater();
    });
    if (m_reply->isRunning())
        emit started();
}

NetworkQuery::~NetworkQuery()
{
    if (m_reply) {
        disconnect(m_reply.get(), nullptr, this, nullptr);
        m_reply->abort();
    }
}

} // namespace Tasking

QT_END_NAMESPACE
