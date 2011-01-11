/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#ifndef RSSFETCHER_H
#define RSSFETCHER_H

#include "core_global.h"

#include <QtCore/QThread>

QT_BEGIN_NAMESPACE
class QUrl;
class QNetworkReply;
class QNetworkAccessManager;
class QIODevice;
QT_END_NAMESPACE

namespace Core {

class CORE_EXPORT RssItem
{
public:
    QString title;
    QString description;
    QString category;
    QString url;
    QString imagePath;
};

class CORE_EXPORT RssFetcher : public QThread
{
    Q_OBJECT
public:
    explicit RssFetcher(int maxItems = -1);
    virtual void run();
    virtual ~RssFetcher();

signals:
    void newsItemReady(const QString& title, const QString& desciption, const QString& url);
    void rssItemReady(const Core::RssItem& item);
    void finished(bool error);

public slots:
    void fetchingFinished(QNetworkReply *reply);
    void fetch(const QUrl &url);

private:
    enum  TagElement { itemElement, titleElement, descriptionElement, linkElement,
                       imageElement, imageLinkElement, categoryElement, otherElement };
    static TagElement tagElement(const QStringRef &, TagElement prev);
    void parseXml(QIODevice *);

    const int m_maxItems;
    int m_items;
    int m_requestCount;

    QNetworkAccessManager* m_networkAccessManager;
};

} // namespace Internal

#endif // RSSFETCHER_H

