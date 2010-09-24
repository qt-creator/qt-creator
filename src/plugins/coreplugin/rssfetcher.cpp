/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#include "rssfetcher.h"
#include "coreconstants.h"

#include <QtCore/QDebug>
#include <QtCore/QSysInfo>
#include <QtCore/QLocale>
#include <QtCore/QEventLoop>
#include <QtGui/QDesktopServices>
#include <QtGui/QLineEdit>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkProxyFactory>
#include <QtNetwork/QNetworkAccessManager>

#include <QtCore/QXmlStreamReader>

#ifdef Q_OS_UNIX
#include <sys/utsname.h>
#endif

using namespace Core::Internal;

static const QString getOsString()
{
    QString osString;
#if defined(Q_OS_WIN)
    switch (QSysInfo::WindowsVersion) {
    case (QSysInfo::WV_4_0):
        osString += QLatin1String("WinNT4.0");
        break;
    case (QSysInfo::WV_5_0):
        osString += QLatin1String("Windows NT 5.0");
        break;
    case (QSysInfo::WV_5_1):
        osString += QLatin1String("Windows NT 5.1");
        break;
    case (QSysInfo::WV_5_2):
        osString += QLatin1String("Windows NT 5.2");
        break;
    case (QSysInfo::WV_6_0):
        osString += QLatin1String("Windows NT 6.0");
        break;
    case (QSysInfo::WV_6_1):
        osString += QLatin1String("Windows NT 6.1");
        break;
    default:
        osString += QLatin1String("Windows NT (Unknown)");
        break;
    }
#elif defined (Q_OS_MAC)
    if (QSysInfo::ByteOrder == QSysInfo::BigEndian)
        osString += QLatin1String("PPC ");
    else
        osString += QLatin1String("Intel ");
    osString += QLatin1String("Mac OS X ");
    switch (QSysInfo::MacintoshVersion) {
    case (QSysInfo::MV_10_3):
        osString += QLatin1String("10_3");
        break;
    case (QSysInfo::MV_10_4):
        osString += QLatin1String("10_4");
        break;
    case (QSysInfo::MV_10_5):
        osString += QLatin1String("10_5");
        break;
    case (QSysInfo::MV_10_6):
        osString += QLatin1String("10_6");
        break;
    default:
        osString += QLatin1String("(Unknown)");
        break;
    }
#elif defined (Q_OS_UNIX)
    struct utsname uts;
    if (uname(&uts) == 0) {
        osString += QLatin1String(uts.sysname);
        osString += QLatin1Char(' ');
        osString += QLatin1String(uts.release);
    } else {
        osString += QLatin1String("Unix (Unknown)");
    }
#else
    ossttring = QLatin1String("Unknown OS");
#endif
    return osString;
}

RssFetcher::RssFetcher(int maxItems)
    : QThread(0), m_maxItems(maxItems), m_items(0),
       m_requestCount(0), m_networkAccessManager(0)
{
    qRegisterMetaType<RssItem>("RssItem");
    moveToThread(this);
}

RssFetcher::~RssFetcher()
{
}

void RssFetcher::run()
{
    exec();
    delete m_networkAccessManager;
}

void RssFetcher::fetch(const QUrl &url)
{
    QString agentStr = QString::fromLatin1("Qt-Creator/%1 (QHttp %2; %3; %4; %5 bit)")
                    .arg(Core::Constants::IDE_VERSION_LONG).arg(qVersion())
                    .arg(getOsString()).arg(QLocale::system().name())
                    .arg(QSysInfo::WordSize);
    QNetworkRequest req(url);
    req.setRawHeader("User-Agent", agentStr.toLatin1());
    if (!m_networkAccessManager) {
        m_networkAccessManager = new QNetworkAccessManager;
        connect(m_networkAccessManager, SIGNAL(finished(QNetworkReply*)),
                SLOT(fetchingFinished(QNetworkReply*)));
    }
    m_requestCount++;
    m_networkAccessManager->get(req);
}

void RssFetcher::fetchingFinished(QNetworkReply *reply)
{
    const bool error = (reply->error() != QNetworkReply::NoError);
    if (!error) {
        parseXml(reply);
        m_items = 0;
    }
    if (--m_requestCount == 0)
        emit finished(error);
    reply->deleteLater();
}

RssFetcher::TagElement RssFetcher::tagElement(const QStringRef &r, TagElement prev)
{
    if (r == QLatin1String("item"))
        return itemElement;
    if (r == QLatin1String("title"))
        return titleElement;
    if (r == QLatin1String("category"))
        return categoryElement;
    if (r == QLatin1String("description"))
        return descriptionElement;
    if (r == QLatin1String("image"))
        return imageElement;
    if (r == QLatin1String("link")) {
        if (prev == imageElement)
            return imageLinkElement;
        else
            return linkElement;
    }
    return otherElement;
}

void RssFetcher::parseXml(QIODevice *device)
{
    QXmlStreamReader xmlReader(device);

    TagElement currentTag = otherElement;
    RssItem item;
    while (!xmlReader.atEnd()) {
        switch (xmlReader.readNext()) {
        case QXmlStreamReader::StartElement:
            currentTag = tagElement(xmlReader.name(), currentTag);
            if (currentTag == itemElement) {
                item = RssItem();
            }
            break;
            case QXmlStreamReader::EndElement:
            if (xmlReader.name() == QLatin1String("item")) {
                m_items++;
                if ((uint)m_items > (uint)m_maxItems)
                    return;
                emit newsItemReady(item.title, item.description, item.url);
                emit rssItemReady(item);
            }
            break;
            case QXmlStreamReader::Characters:
            if (!xmlReader.isWhitespace()) {
                switch (currentTag) {
                case titleElement:
                    item.title += xmlReader.text().toString();
                    break;
                case descriptionElement:
                    item.description += xmlReader.text().toString();
                    break;
                case categoryElement:
                    item.category += xmlReader.text().toString();
                    break;
                case linkElement:
                    item.url += xmlReader.text().toString();
                    break;
                case imageLinkElement:
                    item.imagePath += xmlReader.text().toString();
                    break;
                default:
                    break;
                }
            } // !xmlReader.isWhitespace()
            break;
            default:
            break;
        }
    }
    if (xmlReader.error() && xmlReader.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning("Welcome::Internal::RSSFetcher: XML ERROR: %d: %s (%s)",
                 int(xmlReader.lineNumber()),
                 qPrintable(xmlReader.errorString()),
                 qPrintable(item.title));
    }
}
