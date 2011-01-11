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

#ifndef GETTINGSTARTEDWELCOMEPAGEWIDGET_H
#define GETTINGSTARTEDWELCOMEPAGEWIDGET_H

#include <QtGui/QWidget>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkReply>

QT_BEGIN_NAMESPACE
class QUrl;
class QLabel;
class QFile;
class QMenu;
QT_END_NAMESPACE

namespace Core {
class RssFetcher;
class RssItem;
}

namespace Qt4ProjectManager {
namespace Internal {

namespace Ui {
    class GettingStartedWelcomePageWidget;
}

typedef QHash<QString, QMenu*> QMenuHash;

class PixmapDownloader : public QNetworkAccessManager {
    Q_OBJECT
public:
    PixmapDownloader(const QUrl& url, QLabel* label, QObject *parent = 0)
        : QNetworkAccessManager(parent), m_url(url), m_label(label)
    {
        connect(this, SIGNAL(finished(QNetworkReply*)), SLOT(populatePixmap(QNetworkReply*)));
        get(QNetworkRequest(url));
    }
public slots:
    void populatePixmap(QNetworkReply* reply);

private:
    QUrl m_url;
    QLabel *m_label;

};

class GettingStartedWelcomePageWidget : public QWidget
{
    Q_OBJECT
public:
    GettingStartedWelcomePageWidget(QWidget *parent = 0);
    ~GettingStartedWelcomePageWidget();

public slots:
    void updateExamples(const QString &examplePath,
                        const QString &demosPath,
                        const QString &sourcePath);

private slots:
    void slotOpenHelpPage(const QString &url);
    void slotOpenContextHelpPage(const QString &url);
    void slotOpenExample();
    void slotNextTip();
    void slotPrevTip();
    void slotNextFeature();
    void slotPrevFeature();
    void slotCreateNewProject();
    void addToFeatures(const Core::RssItem&);
    void showFeature(int feature = -1);

signals:
    void startRssFetching(const QUrl&);

private:
    void parseXmlFile(QFile *file, QMenuHash &cppSubMenuHash, QMenuHash &qmlSubMenuHash,
                      const QString &examplePath, const QString &sourcePath);
    QStringList tipsOfTheDay();
    Ui::GettingStartedWelcomePageWidget *ui;
    int m_currentTip;
    int m_currentFeature;
    QList<Core::RssItem> m_featuredItems;
    Core::RssFetcher *m_rssFetcher;
};

} // namespace Internal
} // namespace Qt4ProjectManager

#endif // GETTINGSTARTEDWELCOMEPAGEWIDGET_H
