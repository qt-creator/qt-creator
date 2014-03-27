/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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

#include "kdepasteprotocol.h"

#include <utils/qtcassert.h>

#include <QDebug>
#include <QXmlStreamReader>
#include <QByteArray>
#include <QStringList>

#include <QNetworkReply>

#include <algorithm>

enum { debug = 0 };

static inline QByteArray expiryParameter(int daysRequested)
{
    // Obtained by 'pastebin.kde.org/api/xml/parameter/expire' on 26.03.2014
    static const int expiryTimesSec[] = {1800, 21600, 86400, 604800, 2592000, 31536000};
    const int *end = expiryTimesSec + sizeof(expiryTimesSec) / sizeof(expiryTimesSec[0]);
    // Find the first element >= requested span, search up to n - 1 such that 'end' defaults to last value.
    const int *match = std::lower_bound(expiryTimesSec, end - 1, 24 * 60 * 60 * daysRequested);
    return QByteArray("expire=") + QByteArray::number(*match);
}

namespace CodePaster {
StickyNotesPasteProtocol::StickyNotesPasteProtocol() :
    m_fetchReply(0),
    m_pasteReply(0),
    m_listReply(0),
    m_fetchId(-1),
    m_postId(-1),
    m_hostChecked(false)
{
}

void StickyNotesPasteProtocol::setHostUrl(const QString &hostUrl)
{
    m_hostUrl = hostUrl;
    if (!m_hostUrl.endsWith(QLatin1Char('/')))
        m_hostUrl.append(QLatin1Char('/'));
}

unsigned StickyNotesPasteProtocol::capabilities() const
{
    return ListCapability | PostDescriptionCapability;
}

bool StickyNotesPasteProtocol::checkConfiguration(QString *errorMessage)
{
    if (m_hostChecked)  // Check the host once.
        return true;
    const bool ok = httpStatus(m_hostUrl, errorMessage);
    if (ok)
        m_hostChecked = true;
    return ok;
}

// Query 'http://pastebin.kde.org/api/xml/parameter/language' to obtain the valid values.
static inline QByteArray pasteLanguage(Protocol::ContentType ct)
{
    switch (ct) {
    case Protocol::Text:
        break;
    case Protocol::C:
        return "language=c";
    case Protocol::Cpp:
        return "language=cpp-qt";
    case Protocol::JavaScript:
        return "language=javascript";
    case Protocol::Diff:
        return "language=diff";
    case Protocol::Xml:
        return "language=xml";
    }
    return QByteArray("language=text");
}

void StickyNotesPasteProtocol::paste(const QString &text,
                                   ContentType ct, int expiryDays,
                                   const QString &username,
                                   const QString &comment,
                                   const QString &description)
{
    enum { maxDescriptionLength = 30 }; // Length of description is limited.

    Q_UNUSED(username)
    Q_UNUSED(comment);
    QTC_ASSERT(!m_pasteReply, return);

    // Format body
    QByteArray pasteData = "&data=";
    pasteData += QUrl::toPercentEncoding(fixNewLines(text));
    pasteData += '&';
    pasteData += pasteLanguage(ct);
    pasteData += '&';
    pasteData += expiryParameter(expiryDays);
    if (!description.isEmpty()) {
        pasteData += "&title=";
        pasteData += QUrl::toPercentEncoding(description.left(maxDescriptionLength));
    }

    m_pasteReply = httpPost(m_hostUrl + QLatin1String("api/xml/create"), pasteData);
    connect(m_pasteReply, SIGNAL(finished()), this, SLOT(pasteFinished()));
    if (debug)
        qDebug() << "paste: sending " << m_pasteReply << pasteData;
}

// Parse for an element and return its contents
static QString parseElement(QIODevice *device, const QString &elementName)
{
    QXmlStreamReader reader(device);
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement
            && reader.name() == elementName)
            return reader.readElementText();
    }
    return QString();
}

void StickyNotesPasteProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("%s protocol error: %s", qPrintable(name()),qPrintable(m_pasteReply->errorString()));
    } else {
        // Parse id from '<result><id>143204</id><hash></hash></result>'
        // No useful error reports have been observed.
        const QString id = parseElement(m_pasteReply, QLatin1String("id"));
        if (id.isEmpty())
            qWarning("%s protocol error: Could not send entry.", qPrintable(name()));
        else
            emit pasteDone(m_hostUrl + id);
    }

    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

void StickyNotesPasteProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return);

    // Did we get a complete URL or just an id?
    m_fetchId = id;
    const int lastSlashPos = m_fetchId.lastIndexOf(QLatin1Char('/'));
    if (lastSlashPos != -1)
        m_fetchId.remove(0, lastSlashPos + 1);
    QString url = m_hostUrl + QLatin1String("api/xml/show/") + m_fetchId;
    if (debug)
        qDebug() << "fetch: sending " << url;

    m_fetchReply = httpGet(url);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
}

// Parse: '<result><id>143228</id><author>foo</author><timestamp>1320661026</timestamp><language>text</language>
//  <data>bar</data></result>'

void StickyNotesPasteProtocol::fetchFinished()
{
    const QString title = name() + QLatin1String(": ") + m_fetchId;
    QString content;
    const bool error = m_fetchReply->error();
    if (error) {
        content = m_fetchReply->errorString();
        if (debug)
            qDebug() << "fetchFinished: error" << m_fetchId << content;
    } else {
        content = parseElement(m_fetchReply, QLatin1String("data"));
        content.remove(QLatin1Char('\r'));
    }
    m_fetchReply->deleteLater();
    m_fetchReply = 0;
    emit fetchDone(title, content, error);
}

void StickyNotesPasteProtocol::list()
{
    QTC_ASSERT(!m_listReply, return);

    // Trailing slash is important to prevent redirection.
    QString url = m_hostUrl + QLatin1String("api/xml/list");
    m_listReply = httpGet(url);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
    if (debug)
        qDebug() << "list: sending " << url << m_listReply;
}

// Parse 'result><pastes><paste>id1</paste><paste>id2</paste>...'
static inline QStringList parseList(QIODevice *device)
{
    QStringList result;
    QXmlStreamReader reader(device);
    const QString pasteElement = QLatin1String("paste");
    while (!reader.atEnd()) {
        if (reader.readNext() == QXmlStreamReader::StartElement && reader.name() == pasteElement)
            result.append(reader.readElementText());
    }
    return result;
}

void StickyNotesPasteProtocol::listFinished()
{
    const bool error = m_listReply->error();
    if (error) {
        if (debug)
            qDebug() << "listFinished: error" << m_listReply->errorString();
    } else {
        emit listDone(name(), parseList(m_listReply));
    }
    m_listReply->deleteLater();
    m_listReply = 0;
}

QString KdePasteProtocol::protocolName()
{
    return QLatin1String("Paste.KDE.Org");
}

} // namespace CodePaster
