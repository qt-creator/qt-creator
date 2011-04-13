/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
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

#include "communitywelcomepagewidget.h"
#include "ui_communitywelcomepagewidget.h"

#include <coreplugin/rssfetcher.h>

#include <QtCore/QMap>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QTreeWidgetItem>

struct Site {
    const char *description;
    const char *url;
};

static const Site supportSites[] = {
    { QT_TRANSLATE_NOOP("Welcome::Internal::CommunityWelcomePageWidget",
                        "<b>Forum Nokia</b><br /><font color='gray'>Mobile application support</font>"),
      "http://www.forum.nokia.com/Support/"},
    { QT_TRANSLATE_NOOP("Welcome::Internal::CommunityWelcomePageWidget",
                        "<b>Qt LGPL Support</b><br /><font color='gray'>Buy commercial Qt support</font>"),
      "http://shop.qt.nokia.com/en/support.html"},
    { QT_TRANSLATE_NOOP("Welcome::Internal::CommunityWelcomePageWidget",
                        "<b>Qt DevNet</b><br /><font color='gray'>Qt Developer Resources</font>"),
      "http://developer.qt.nokia.com" }
};

static const Site sites[] = {
    { QT_TRANSLATE_NOOP("Welcome::Internal::CommunityWelcomePageWidget",
                        "<b>Qt Home</b><br /><font color='gray'>Qt by Nokia on the web</font>"),
      "http://qt.nokia.com" },
    { QT_TRANSLATE_NOOP("Welcome::Internal::CommunityWelcomePageWidget",
                        "<b>Qt Git Hosting</b><br /><font color='gray'>Participate in Qt development</font>"),
      "http://qt.gitorious.org"},
    { QT_TRANSLATE_NOOP("Welcome::Internal::CommunityWelcomePageWidget",
                        "<b>Qt Apps</b><br /><font color='gray'>Find free Qt-based apps</font>"),
      "http://www.qt-apps.org"}
};

namespace Welcome {
namespace Internal {

static inline void populateWelcomeTreeWidget(const Site *sites, int count, Utils::WelcomeModeTreeWidget *wt)
{
    for (int s = 0; s < count; s++) {
        const QString description = CommunityWelcomePageWidget::tr(sites[s].description);
        const QString url = QLatin1String(sites[s].url);
        wt->addItem(description, url, url);
    }
}

CommunityWelcomePageWidget::CommunityWelcomePageWidget(QWidget *parent) :
    QWidget(parent),
    m_rssFetcher(new Core::RssFetcher(7)),
    ui(new Ui::CommunityWelcomePageWidget)
{
    ui->setupUi(this);

    connect(ui->newsTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(ui->miscSitesTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(ui->supportSitesTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));

    connect(m_rssFetcher, SIGNAL(newsItemReady(QString, QString, QString)),
            ui->newsTreeWidget, SLOT(addNewsItem(QString, QString, QString)), Qt::QueuedConnection);
    connect(this, SIGNAL(startRssFetching(QUrl)), m_rssFetcher, SLOT(fetch(QUrl)), Qt::QueuedConnection);

    m_rssFetcher->start(QThread::LowestPriority);
    //: Add localized feed here only if one exists
    emit startRssFetching(QUrl(tr("http://labs.trolltech.com/blogs/feed")));

    populateWelcomeTreeWidget(supportSites, sizeof(supportSites)/sizeof(Site), ui->supportSitesTreeWidget);
    populateWelcomeTreeWidget(sites, sizeof(sites)/sizeof(Site), ui->miscSitesTreeWidget);
}

CommunityWelcomePageWidget::~CommunityWelcomePageWidget()
{
    m_rssFetcher->exit();
    m_rssFetcher->wait();
    delete m_rssFetcher;
    delete ui;
}

void CommunityWelcomePageWidget::slotUrlClicked(const QString &data)
{
    QDesktopServices::openUrl(QUrl(data));
}

} // namespace Internal
} // namespace Welcome
