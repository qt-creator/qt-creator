// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "urlopenprotocol.h"

#include <utils/qtcassert.h>

#include <QNetworkReply>

namespace CodePaster {

QString UrlOpenProtocol::name() const
{
    return QLatin1String("Open URL"); // unused
}

unsigned UrlOpenProtocol::capabilities() const
{
    return 0;
}

void UrlOpenProtocol::fetch(const QString &url)
{
    QTC_ASSERT(!m_fetchReply, return);
    m_fetchReply = httpGet(url);
    connect(m_fetchReply, &QNetworkReply::finished,
            this, &UrlOpenProtocol::fetchFinished);
}

void UrlOpenProtocol::fetchFinished()
{
    const QString title = m_fetchReply->url().toString();
    QString content;
    const bool error = m_fetchReply->error();
    if (error)
        content = m_fetchReply->errorString();
    else
        content = QString::fromUtf8(m_fetchReply->readAll());
    m_fetchReply->deleteLater();
    m_fetchReply = nullptr;
    emit fetchDone(title, content, error);
}

void UrlOpenProtocol::paste(const QString &, ContentType, int, const QString &,
                            const QString &, const QString &)
{
}

} // CodePaster
