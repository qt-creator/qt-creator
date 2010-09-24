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

#ifndef RSSFETCHER_H
#define RSSFETCHER_H

#include "core_global.h"

#include <QtCore/QThread>
#include <QtCore/QUrl>

QT_BEGIN_NAMESPACE
class QNetworkReply;
class QNetworkAccessManager;
class QIODevice;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

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
    void rssItemReady(const RssItem& item);
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

} // namespace Core
} // namespace Internal

#endif // RSSFETCHER_H

