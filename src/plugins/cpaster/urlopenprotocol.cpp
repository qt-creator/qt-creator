// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "urlopenprotocol.h"

#include <utils/qtcassert.h>

#include <QNetworkReply>

namespace CodePaster {

UrlOpenProtocol::UrlOpenProtocol() : Protocol({"Open URL"})
{}

void UrlOpenProtocol::fetch(const QString &url)
{
    QTC_ASSERT(!m_fetchReply, return);
    m_fetchReply = httpGet(url);
    connect(m_fetchReply, &QNetworkReply::finished,
            this, &UrlOpenProtocol::fetchFinished);
}

void UrlOpenProtocol::fetchFinished()
{
    if (m_fetchReply->error())
        reportError(m_fetchReply->errorString());
    else
        emit fetchDone(m_fetchReply->url().toString(), QString::fromUtf8(m_fetchReply->readAll()));
    m_fetchReply->deleteLater();
    m_fetchReply = nullptr;
}

void UrlOpenProtocol::paste(const QString &, ContentType, int, const QString &, const QString &)
{
}

} // CodePaster
