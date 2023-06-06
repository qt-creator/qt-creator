// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "networkquery.h"

#include <QNetworkAccessManager>

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
        emit done(false);
        return;
    }
    m_reply.reset(m_manager->get(m_request));
    connect(m_reply.get(), &QNetworkReply::finished, this, [this] {
        disconnect(m_reply.get(), nullptr, this, nullptr);
        emit done(m_reply->error() == QNetworkReply::NoError);
        m_reply.release()->deleteLater();
    });
    if (m_reply->isRunning())
        emit started();
}

NetworkQuery::~NetworkQuery()
{
    if (m_reply)
        m_reply->abort();
}

} // namespace Tasking
