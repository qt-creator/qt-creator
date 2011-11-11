/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "kdepasteprotocol.h"

#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QTextStream>
#include <QtCore/QXmlStreamReader>
#include <QtCore/QByteArray>
#include <QtCore/QStringList>

#include <QtNetwork/QNetworkReply>

enum { debug =  0 };

static const char hostNameC[]= "paste.kde.org";
static const char hostUrlC[]= "http://paste.kde.org/";
static const char formatC[]= "xml";
static const char showPhpScriptpC[] = "show.php";

enum { expirySeconds = 86400 };

namespace CodePaster {
KdePasteProtocol::KdePasteProtocol(const NetworkAccessManagerProxyPtr &nw) :
    NetworkProtocol(nw),
    m_fetchReply(0),
    m_pasteReply(0),
    m_listReply(0),
    m_fetchId(-1),
    m_postId(-1),
    m_hostChecked(false)
{
}

QString KdePasteProtocol::protocolName()
{
    return QLatin1String("Paste.KDE.Org");
}

unsigned KdePasteProtocol::capabilities() const
{
    return ListCapability;
}

bool KdePasteProtocol::checkConfiguration(QString *errorMessage)
{
    if (m_hostChecked)  // Check the host once.
        return true;
    const bool ok = httpStatus(QLatin1String(hostNameC), errorMessage);
    if (ok)
        m_hostChecked = true;
    return ok;
}

static inline QByteArray pasteLanguage(Protocol::ContentType ct)
{
    switch (ct) {
    case Protocol::Text:
        break;
    case Protocol::C:
        return "paste_lang=c++";
        break;
    case Protocol::JavaScript:
        return "paste_lang=Javascript";
        break;
    case Protocol::Diff:
        return "paste_lang=Diff";
    case Protocol::Xml:
        return "paste_lang=XML";
    }
    return QByteArray("paste_lang=text");
}

void KdePasteProtocol::paste(const QString &text,
                                   ContentType ct,
                                   const QString &username,
                                   const QString & /* comment */,
                                   const QString & /* description */)
{
    QTC_ASSERT(!m_pasteReply, return;)

    // Format body
    QByteArray pasteData = "api_submit=true&mode=";
    pasteData += formatC;
    if (!username.isEmpty()) {
        pasteData += "&paste_user=";
        pasteData += QUrl::toPercentEncoding(username);
    }
    pasteData += "&paste_data=";
    pasteData += QUrl::toPercentEncoding(fixNewLines(text));
    pasteData += "&paste_expire=";
    pasteData += QByteArray::number(expirySeconds);
    pasteData += '&';
    pasteData += pasteLanguage(ct);

    m_pasteReply = httpPost(QLatin1String(hostUrlC), pasteData);
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

void KdePasteProtocol::pasteFinished()
{
    if (m_pasteReply->error()) {
        qWarning("%s protocol error: %s", qPrintable(protocolName()),qPrintable(m_pasteReply->errorString()));
    } else {
        // Parse id from '<result><id>143204</id><hash></hash></result>'
        // No useful error reports have been observed.
        const QString id = parseElement(m_pasteReply, QLatin1String("id"));
        if (id.isEmpty()) {
            qWarning("%s protocol error: Could not send entry.", qPrintable(protocolName()));
        } else {
            emit pasteDone(QLatin1String(hostUrlC) + id);
        }
    }

    m_pasteReply->deleteLater();
    m_pasteReply = 0;
}

void KdePasteProtocol::fetch(const QString &id)
{
    QTC_ASSERT(!m_fetchReply, return;)

    // Did we get a complete URL or just an id?
    m_fetchId = id;
    const int lastSlashPos = m_fetchId.lastIndexOf(QLatin1Char('/'));
    if (lastSlashPos != -1)
        m_fetchId.remove(0, lastSlashPos + 1);
    QString url;
    QTextStream(&url) << hostUrlC << showPhpScriptpC
                << "?format=" << formatC << "&id=" << m_fetchId;
    if (debug)
        qDebug() << "fetch: sending " << url;

    m_fetchReply = httpGet(url);
    connect(m_fetchReply, SIGNAL(finished()), this, SLOT(fetchFinished()));
}

// Parse: '<result><id>143228</id><author>foo</author><timestamp>1320661026</timestamp><language>text</language>
//  <data>bar</data></result>'

void KdePasteProtocol::fetchFinished()
{
    const QString title = protocolName() + QLatin1String(": ") + m_fetchId;
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

void KdePasteProtocol::list()
{
    QTC_ASSERT(!m_listReply, return;)

    QString url;
    QTextStream(&url) << hostUrlC << "api/" << formatC << "/all?format=" << formatC;
    m_listReply = httpGet(url);
    connect(m_listReply, SIGNAL(finished()), this, SLOT(listFinished()));
    if (debug)
        qDebug() << "list: sending " << url << m_listReply;
}

// Parse 'result><pastes><paste_1>id1</paste_1><paste_2>id2</paste_2>...'
static inline QStringList parseList(QIODevice *device)
{
    QStringList result;
    QXmlStreamReader reader(device);
    const QString pasteElementPrefix = QLatin1String("paste_");
    while (!reader.atEnd()) {
#if QT_VERSION >= 0x040800
        if (reader.readNext() == QXmlStreamReader::StartElement && reader.name().startsWith(pasteElementPrefix))
#else
        if (reader.readNext() == QXmlStreamReader::StartElement && reader.name().toString().startsWith(pasteElementPrefix))
#endif
            result.append(reader.readElementText());
    }
    return result;
}

void KdePasteProtocol::listFinished()
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

} // namespace CodePaster
