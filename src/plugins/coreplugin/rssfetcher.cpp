/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include <QtCore/QDebug>
#include <QtGui/QDesktopServices>
#include <QtGui/QLineEdit>
#include <QtNetwork/QHttp>

#include "rssfetcher.h"

using namespace Core::Internal;

RSSFetcher::RSSFetcher(int maxItems, QObject *parent)
    : QObject(parent), m_items(0), m_maxItems(maxItems)
{
    connect(&m_http, SIGNAL(readyRead(const QHttpResponseHeader &)),
             this, SLOT(readData(const QHttpResponseHeader &)));

    connect(&m_http, SIGNAL(requestFinished(int, bool)),
             this, SLOT(finished(int, bool)));
}

void RSSFetcher::fetch(const QUrl &url)
{
    m_http.setHost(url.host());
    m_connectionId = m_http.get(url.path());
}

void RSSFetcher::readData(const QHttpResponseHeader &resp)
{
    if (resp.statusCode() != 200)
        m_http.abort();
    else {
        m_xml.addData(m_http.readAll());
        parseXml();
    }
}

void RSSFetcher::finished(int id, bool error)
{
    Q_UNUSED(id)
    m_items = 0;
    emit finished(error);
}

void RSSFetcher::parseXml()
{
    while (!m_xml.atEnd()) {
        m_xml.readNext();
        if (m_xml.isStartElement()) {
            if (m_xml.name() == "item")
                m_linkString = m_xml.attributes().value("rss:about").toString();
            m_currentTag = m_xml.name().toString();
        } else if (m_xml.isEndElement()) {
            if (m_xml.name() == "item") {
                m_items++;
                if (m_items > m_maxItems)
                    return;

                emit newsItemReady(m_titleString, m_linkString);
                m_titleString.clear();
                m_linkString.clear();
            }

        } else if (m_xml.isCharacters() && !m_xml.isWhitespace()) {
            if (m_currentTag == "title")
                m_titleString += m_xml.text().toString();
            else if (m_currentTag == "link")
                m_linkString += m_xml.text().toString();
        }
    }
    if (m_xml.error() && m_xml.error() != QXmlStreamReader::PrematureEndOfDocumentError) {
        qWarning() << "XML ERROR:" << m_xml.lineNumber() << ": " << m_xml.errorString();
        m_http.abort();
    }
}
