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

#include "urlopenprotocol.h"

#include <utils/qtcassert.h>

#include <QNetworkReply>

using namespace CodePaster;

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
