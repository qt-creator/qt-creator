/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "searchwidget.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/styledbar.h>

#include <QMap>
#include <QString>
#include <QStringList>

#include <QMenu>
#include <QLayout>
#include <QKeyEvent>
#include <QClipboard>
#include <QApplication>
#include <QTextBrowser>

#include <QHelpEngine>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>

using namespace Help::Internal;

SearchWidget::SearchWidget()
    : zoomCount(0)
    , m_progress(0)
    , searchEngine(0)
    , resultWidget(0)
{
}

SearchWidget::~SearchWidget()
{
}

void SearchWidget::zoomIn()
{
    QTextBrowser* browser = resultWidget->findChild<QTextBrowser*>();
    if (browser && zoomCount != 10) {
        zoomCount++;
        browser->zoomIn();
    }
}

void SearchWidget::zoomOut()
{
    QTextBrowser* browser = resultWidget->findChild<QTextBrowser*>();
    if (browser && zoomCount != -5) {
        zoomCount--;
        browser->zoomOut();
    }
}

void SearchWidget::resetZoom()
{
    if (zoomCount == 0)
        return;

    QTextBrowser* browser = resultWidget->findChild<QTextBrowser*>();
    if (browser) {
        browser->zoomOut(zoomCount);
        zoomCount = 0;
    }
}

void SearchWidget::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !searchEngine) {
        QVBoxLayout *vLayout = new QVBoxLayout(this);
        vLayout->setMargin(0);
        vLayout->setSpacing(0);

        searchEngine = (&LocalHelpManager::helpEngine())->searchEngine();

        Utils::StyledBar *toolbar = new Utils::StyledBar(this);
        toolbar->setSingleRow(false);
        QHelpSearchQueryWidget *queryWidget = searchEngine->queryWidget();
        QLayout *tbLayout = new QVBoxLayout();
        tbLayout->setSpacing(6);
        tbLayout->setMargin(4);
        tbLayout->addWidget(queryWidget);
        toolbar->setLayout(tbLayout);

        Utils::StyledBar *toolbar2 = new Utils::StyledBar(this);
        toolbar2->setSingleRow(false);
        tbLayout = new QVBoxLayout();
        tbLayout->setSpacing(0);
        tbLayout->setMargin(0);
        tbLayout->addWidget(resultWidget = searchEngine->resultWidget());
        toolbar2->setLayout(tbLayout);

        vLayout->addWidget(toolbar);
        vLayout->addWidget(toolbar2);

        setFocusProxy(queryWidget);

        connect(queryWidget, SIGNAL(search()), this, SLOT(search()));
        connect(resultWidget, SIGNAL(requestShowLink(QUrl)), this,
            SIGNAL(linkActivated(QUrl)));

        connect(searchEngine, SIGNAL(searchingStarted()), this,
            SLOT(searchingStarted()));
        connect(searchEngine, SIGNAL(searchingFinished(int)), this,
            SLOT(searchingFinished(int)));

        QTextBrowser* browser = resultWidget->findChild<QTextBrowser*>();
        browser->viewport()->installEventFilter(this);

        connect(searchEngine, SIGNAL(indexingStarted()), this,
            SLOT(indexingStarted()));
        connect(searchEngine, SIGNAL(indexingFinished()), this,
            SLOT(indexingFinished()));

        QMetaObject::invokeMethod(&LocalHelpManager::helpEngine(), "setupFinished",
            Qt::QueuedConnection);
    }
}

void SearchWidget::search() const
{
    static QStringList charsToEscapeList;
    if (charsToEscapeList.isEmpty()) {
        charsToEscapeList << QLatin1String("\\") << QLatin1String("+")
            << QLatin1String("-") << QLatin1String("!") << QLatin1String("(")
            << QLatin1String(")") << QLatin1String(":") << QLatin1String("^")
            << QLatin1String("[") << QLatin1String("]") << QLatin1String("{")
            << QLatin1String("}") << QLatin1String("~");
    }

    static QString escapeChar(QLatin1String("\\"));
    static QRegExp regExp(QLatin1String("[\\+\\-\\!\\(\\)\\^\\[\\]\\{\\}~:]"));

    QList<QHelpSearchQuery> escapedQueries;
    const QList<QHelpSearchQuery> queries = searchEngine->queryWidget()->query();
    foreach (const QHelpSearchQuery &query, queries) {
        QHelpSearchQuery escapedQuery;
        escapedQuery.fieldName = query.fieldName;
        foreach (QString word, query.wordList) {
            if (word.contains(regExp)) {
                foreach (const QString &charToEscape, charsToEscapeList)
                    word.replace(charToEscape, escapeChar + charToEscape);
            }
            escapedQuery.wordList.append(word);
        }
        escapedQueries.append(escapedQuery);
    }
    searchEngine->search(escapedQueries);
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
    Core::ICore::progressManager() ->addTask(m_progress->future(),
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

bool SearchWidget::eventFilter(QObject *o, QEvent *e)
{
    QTextBrowser *browser = resultWidget->findChild<QTextBrowser *>();
    if (browser && o == browser->viewport()
        && e->type() == QEvent::MouseButtonRelease){
        QMouseEvent *me = static_cast<QMouseEvent *>(e);
        QUrl link = resultWidget->linkAt(me->pos());
        if (!link.isEmpty() || link.isValid()) {
            bool controlPressed = me->modifiers() & Qt::ControlModifier;
            if ((me->button() == Qt::LeftButton && controlPressed)
                || (me->button() == Qt::MidButton)) {
                    OpenPagesManager::instance().createPageFromSearch(link);
            }
        }
    }
    return QWidget::eventFilter(o,e);
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *contextMenuEvent)
{
    QTextBrowser *browser = resultWidget->findChild<QTextBrowser *>();
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
    if (usedAction == openLink)
        browser->selectAll();
    else if (usedAction == openLinkInNewTab)
        OpenPagesManager::instance().createPageFromSearch(link);
    else if (usedAction == copyAnchorAction)
        QApplication::clipboard()->setText(link.toString());
}
