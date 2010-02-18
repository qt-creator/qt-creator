/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef RSSFETCHER_H
#define RSSFETCHER_H

#include <QtCore/QUrl>
#include <QtCore/QXmlStreamReader>
#include <QtNetwork/QNetworkAccessManager>

QT_BEGIN_NAMESPACE
class QNetworkReply;
QT_END_NAMESPACE

namespace Welcome {
namespace Internal {

class RSSFetcher : public QObject
{
    Q_OBJECT
public:
    RSSFetcher(int maxItems, QObject *parent = 0);

signals:
    void newsItemReady(const QString& title, const QString& desciption, const QString& url);
    void finished(bool error);

public slots:
    void fetchingFinished(QNetworkReply *reply);
    void fetch(const QUrl &url);

private:
    void parseXml();

    QXmlStreamReader m_xml;
    QString m_currentTag;
    QString m_linkString;
    QString m_descriptionString;
    QString m_titleString;

    QNetworkAccessManager m_networkAccessManager;
    int m_items;
    int m_maxItems;
};

} // namespace Welcome
} // namespace Internal

#endif // RSSFETCHER_H

