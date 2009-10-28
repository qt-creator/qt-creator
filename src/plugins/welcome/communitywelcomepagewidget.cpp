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

#include "communitywelcomepagewidget.h"
#include "ui_communitywelcomepagewidget.h"

#include "rssfetcher.h"

#include <QtCore/QMap>
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
    ui->sitesTitleLabel->setStyledText(tr("Qt Websites"));

    connect(ui->newsTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));
    connect(ui->sitesTreeWidget, SIGNAL(activated(QString)), SLOT(slotUrlClicked(QString)));

    connect(m_rssFetcher, SIGNAL(newsItemReady(QString, QString, QString)),
        ui->newsTreeWidget, SLOT(slotAddNewsItem(QString, QString, QString)));
    //: Add localized feed here only if one exists
    m_rssFetcher->fetch(QUrl(tr("http://labs.trolltech.com/blogs/feed")));

    QList<QPair<QString, QString> > sites;
    sites << qMakePair(tr("Qt Home"), QString(QLatin1String("http://qt.nokia.com")));
    sites << qMakePair(tr("Qt Labs"), QString(QLatin1String("http://labs.qt.nokia.com")));
    sites << qMakePair(tr("Qt Git Hosting"), QString(QLatin1String("http://qt.gitorious.org")));
    sites << qMakePair(tr("Qt Centre"), QString(QLatin1String("http://www.qtcentre.org")));
    sites << qMakePair(tr("Qt Apps"), QString(QLatin1String("http://www.qt-apps.org")));
    sites << qMakePair(tr("Qt for Symbian at Forum Nokia"),  QString(QLatin1String("http://discussion.forum.nokia.com/forum/forumdisplay.php?f=196")));

    QListIterator<QPair<QString, QString> > it(sites);
    while (it.hasNext()) {
        QPair<QString, QString> pair = it.next();
        ui->sitesTreeWidget->addItem(pair.first, pair.second, pair.second);
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
