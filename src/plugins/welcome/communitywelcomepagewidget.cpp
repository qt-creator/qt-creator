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

#include "communitywelcomepagewidget.h"
#include "ui_communitywelcomepagewidget.h"

#include "rssfetcher.h"

#include <QtCore/QMap>
#include <QtCore/QUrl>
#include <QtGui/QDesktopServices>
#include <QtGui/QTreeWidgetItem>

namespace Welcome {
namespace Internal {
CommunityWelcomePageWidget::CommunityWelcomePageWidget(QWidget *parent) :
    QWidget(parent),
    m_rssFetcher(new RSSFetcher(7)),
    ui(new Ui::CommunityWelcomePageWidget)
{
    ui->setupUi(this);
    ui->labsTitleLabel->setStyledText(tr("News From the Qt Labs"));
    ui->supportSitesTitleLabel->setStyledText(tr("Qt Support Sites"));
    ui->miscSitesTitleLabel->setStyledText(tr("Qt Links"));

    connect(ui->newsTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(ui->miscSitesTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(ui->supportSitesTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));

    connect(m_rssFetcher, SIGNAL(newsItemReady(QString, QString, QString)),
        ui->newsTreeWidget, SLOT(slotAddNewsItem(QString, QString, QString)));
    //: Add localized feed here only if one exists
    m_rssFetcher->fetch(QUrl(tr("http://labs.trolltech.com/blogs/feed")));

    QList<QPair<QString, QString> > supportSites;
    QList<QPair<QString, QString> > sites;

    supportSites << qMakePair(tr("<b>Forum Nokia</b><br /><font color='gray'>Mobile Application Support</font>"),  QString(QLatin1String("http://www.forum.nokia.com/I_Want_To/Develop_Mobile_Applications/Technical_Support/")));
    supportSites << qMakePair(tr("<b>Qt GPL Support</b><br /><font color='gray'>Buy professional Qt support</font>"), QString(QLatin1String("http://shop.qt.nokia.com/en/support.html")));
    supportSites << qMakePair(tr("<b>Qt Centre</b><br /><font color='gray'>Community based Qt support</font>"), QString(QLatin1String("http://www.qtcentre.org")));

    sites << qMakePair(tr("<b>Qt Home</b><br /><font color='gray'>Qt by Nokia on the web</font>"), QString(QLatin1String("http://qt.nokia.com")));
    sites << qMakePair(tr("<b>Qt Git Hosting</b><br /><font color='gray'>Participate in Qt development</font>"), QString(QLatin1String("http://qt.gitorious.org")));
    sites << qMakePair(tr("<b>Qt Apps</b><br /><font color='gray'>Find free Qt-based apps</font>"), QString(QLatin1String("http://www.qt-apps.org")));

    QListIterator<QPair<QString, QString> > supportSitesIt(supportSites);
    while (supportSitesIt.hasNext()) {
        QPair<QString, QString> pair = supportSitesIt.next();
        ui->supportSitesTreeWidget->addItem(pair.first, pair.second, pair.second);
    }

    QListIterator<QPair<QString, QString> > sitesIt(sites);
    while (sitesIt.hasNext()) {
        QPair<QString, QString> pair = sitesIt.next();
        ui->miscSitesTreeWidget->addItem(pair.first, pair.second, pair.second);
    }
}

CommunityWelcomePageWidget::~CommunityWelcomePageWidget()
{
    delete m_rssFetcher;
    delete ui;
}

void CommunityWelcomePageWidget::slotUrlClicked(const QString &data)
{
    QDesktopServices::openUrl(QUrl(data));
}

} // namespace Internal
} // namespace Welcome
