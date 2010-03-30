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

#include "searchwidget.h"
#include "helpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <QtGui/QMenu>
#include <QtGui/QLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QTextBrowser>

#include <QtHelp/QHelpEngine>
#include <QtHelp/QHelpSearchEngine>
#include <QtHelp/QHelpSearchQueryWidget>
#include <QtHelp/QHelpSearchResultWidget>

using namespace Help::Internal;

SearchWidget::SearchWidget()
    : zoomCount(0)
    , m_progress(0)
    , searchEngine(0)
{
}

SearchWidget::~SearchWidget()
{
}

void SearchWidget::zoomIn()
{
    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    if (browser && zoomCount != 10) {
        zoomCount++;
        browser->zoomIn();
    }
}

void SearchWidget::zoomOut()
{
    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    if (browser && zoomCount != -5) {
        zoomCount--;
        browser->zoomOut();
    }
}

void SearchWidget::resetZoom()
{
    if (zoomCount == 0)
        return;

    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    if (browser) {
        browser->zoomOut(zoomCount);
        zoomCount = 0;
    }
}

void SearchWidget::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !searchEngine) {
        QVBoxLayout *vLayout = new QVBoxLayout(this);
        vLayout->setMargin(4);

        searchEngine = (&HelpManager::helpEngine())->searchEngine();
        resultWidget = searchEngine->resultWidget();
        QHelpSearchQueryWidget *queryWidget = searchEngine->queryWidget();

        vLayout->addWidget(queryWidget);
        vLayout->addWidget(resultWidget);

        setFocusProxy(queryWidget);

        connect(queryWidget, SIGNAL(search()), this, SLOT(search()));
        connect(resultWidget, SIGNAL(requestShowLink(QUrl)), this,
            SIGNAL(linkActivated(QUrl)));

        connect(searchEngine, SIGNAL(searchingStarted()), this,
            SLOT(searchingStarted()));
        connect(searchEngine, SIGNAL(searchingFinished(int)), this,
            SLOT(searchingFinished(int)));

        QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
        browser->viewport()->installEventFilter(this);

        connect(searchEngine, SIGNAL(indexingStarted()), this,
            SLOT(indexingStarted()));
        connect(searchEngine, SIGNAL(indexingFinished()), this,
            SLOT(indexingFinished()));

        QMetaObject::invokeMethod(&HelpManager::helpEngine(), "setupFinished",
            Qt::QueuedConnection);
    }
}

void SearchWidget::search() const
{
    QList<QHelpSearchQuery> query = searchEngine->queryWidget()->query();
    searchEngine->search(query);
}

void SearchWidget::searchingStarted()
{
    qApp->setOverrideCursor(QCursor(Qt::WaitCursor));
}

void SearchWidget::searchingFinished(int hits)
{
    Q_UNUSED(hits)
    qApp->restoreOverrideCursor();
}

void SearchWidget::indexingStarted()
{
    Q_ASSERT(!m_progress);
    m_progress = new QFutureInterface<void>();
    Core::ICore::instance()->progressManager() ->addTask(m_progress->future(),
        tr("Indexing"), QLatin1String("Help.Indexer"));
    m_progress->setProgressRange(0, 2);
    m_progress->setProgressValueAndText(1, tr("Indexing Documentation..."));
    m_progress->reportStarted();

    m_watcher.setFuture(m_progress->future());
    connect(&m_watcher, SIGNAL(canceled()), searchEngine, SLOT(cancelIndexing()));
}

void SearchWidget::indexingFinished()
{
    m_progress->reportFinished();

    delete m_progress;
    m_progress = NULL;
}

bool SearchWidget::eventFilter(QObject* o, QEvent *e)
{
    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    if (browser && o == browser->viewport()
        && e->type() == QEvent::MouseButtonRelease){
        QMouseEvent *me = static_cast<QMouseEvent*>(e);
        QUrl link = resultWidget->linkAt(me->pos());
        if (!link.isEmpty() || link.isValid()) {
            bool controlPressed = me->modifiers() & Qt::ControlModifier;
            if((me->button() == Qt::LeftButton && controlPressed)
                || (me->button() == Qt::MidButton)) {
                    OpenPagesManager::instance().createPageFromSearch(link);
            }
        }
    }
    return QWidget::eventFilter(o,e);
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *contextMenuEvent)
{
    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    if (!browser)
        return;

    QPoint point = browser->mapFromGlobal(contextMenuEvent->globalPos());
    if (!browser->rect().contains(point, true))
        return;

    QAction *openLink = 0;
    QAction *openLinkInNewTab = 0;
    QAction *copyAnchorAction = 0;

    QMenu menu;
    QUrl link = browser->anchorAt(point);
    if (!link.isEmpty() && link.isValid()) {
        if (link.isRelative())
            link = browser->source().resolved(link);
        openLink = menu.addAction(tr("Open Link"));
        openLinkInNewTab = menu.addAction(tr("Open Link as New Page"));
        copyAnchorAction = menu.addAction(tr("Copy Link"));
    } else if (browser->textCursor().hasSelection()) {
        menu.addAction(tr("Copy"), browser, SLOT(copy()));
    } else {
        menu.addAction(tr("Reload"), browser, SLOT(reload()));
    }

    QAction *usedAction = menu.exec(mapToGlobal(contextMenuEvent->pos()));
    if (usedAction == openLink) {
        browser->selectAll();
    } else if (usedAction == openLinkInNewTab) {
        OpenPagesManager::instance().createPageFromSearch(link);
    } else if (usedAction == copyAnchorAction) {
        QApplication::clipboard()->setText(link.toString());
    }
}
