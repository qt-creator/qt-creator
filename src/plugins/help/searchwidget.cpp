/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "searchwidget.h"
#include "helpconstants.h"
#include "helpplugin.h"
#include "localhelpmanager.h"
#include "openpagesmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>
#include <utils/progressindicator.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QClipboard>
#include <QHelpEngine>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QKeyEvent>
#include <QLayout>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QRegExp>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QToolButton>

using namespace Help::Internal;

SearchWidget::SearchWidget()
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

void SearchWidget::reindexDocumentation()
{
    if (searchEngine)
        searchEngine->reindexDocumentation();
}

void SearchWidget::showEvent(QShowEvent *event)
{
    if (!event->spontaneous() && !searchEngine) {
        QVBoxLayout *vLayout = new QVBoxLayout(this);
        vLayout->setMargin(0);
        vLayout->setSpacing(0);

        searchEngine = new QHelpSearchEngine(&LocalHelpManager::helpEngine(), this);

        Utils::StyledBar *toolbar = new Utils::StyledBar(this);
        toolbar->setSingleRow(false);
        m_queryWidget = searchEngine->queryWidget();
        QLayout *tbLayout = new QVBoxLayout();
        tbLayout->setSpacing(6);
        tbLayout->setMargin(4);
        tbLayout->addWidget(m_queryWidget);
        m_indexingDocumentationLabel = new QLabel(tr("Indexing Documentation"), toolbar);
        m_indexingDocumentationLabel->hide();
        tbLayout->addWidget(m_indexingDocumentationLabel);
        toolbar->setLayout(tbLayout);

        Utils::StyledBar *toolbar2 = new Utils::StyledBar(this);
        toolbar2->setSingleRow(false);
        tbLayout = new QVBoxLayout();
        tbLayout->setSpacing(0);
        tbLayout->setMargin(0);
        tbLayout->addWidget(resultWidget = searchEngine->resultWidget());
        toolbar2->setLayout(tbLayout);

        m_indexingIndicator = new Utils::ProgressIndicator(Utils::ProgressIndicatorSize::Medium,
                                                           resultWidget);
        m_indexingIndicator->attachToWidget(resultWidget);
        m_indexingIndicator->hide();

        vLayout->addWidget(toolbar);
        vLayout->addWidget(toolbar2);

        setFocusProxy(m_queryWidget);

        connect(m_queryWidget, &QHelpSearchQueryWidget::search, this, &SearchWidget::search);
        connect(resultWidget, &QHelpSearchResultWidget::requestShowLink, this,
                [this](const QUrl &url) {
                    emit linkActivated(url, currentSearchTerms(), false/*newPage*/);
                });

        connect(searchEngine, &QHelpSearchEngine::searchingStarted, this,
            &SearchWidget::searchingStarted);
        connect(searchEngine, &QHelpSearchEngine::searchingFinished, this,
            &SearchWidget::searchingFinished);

        QTextBrowser* browser = resultWidget->findChild<QTextBrowser*>();
        browser->viewport()->installEventFilter(this);

        connect(searchEngine, &QHelpSearchEngine::indexingStarted, this,
            &SearchWidget::indexingStarted);
        connect(searchEngine, &QHelpSearchEngine::indexingFinished, this,
            &SearchWidget::indexingFinished);

        QMetaObject::invokeMethod(&LocalHelpManager::helpEngine(), "setupFinished",
            Qt::QueuedConnection);
    }
}

void SearchWidget::search() const
{
    searchEngine->search(searchEngine->queryWidget()->searchInput());
}

void SearchWidget::searchingStarted()
{
    QGuiApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
}

void SearchWidget::searchingFinished(int hits)
{
    Q_UNUSED(hits)
    QGuiApplication::restoreOverrideCursor();
}

void SearchWidget::indexingStarted()
{
    Q_ASSERT(!m_progress);
    m_progress = new QFutureInterface<void>();
    Core::ProgressManager::addTask(m_progress->future(), tr("Indexing Documentation"), "Help.Indexer");
    m_progress->setProgressRange(0, 2);
    m_progress->setProgressValueAndText(1, tr("Indexing Documentation"));
    m_progress->reportStarted();

    m_watcher.setFuture(m_progress->future());
    connect(&m_watcher, &QFutureWatcherBase::canceled,
            searchEngine, &QHelpSearchEngine::cancelIndexing);

    m_queryWidget->hide();
    m_indexingDocumentationLabel->show();
    m_indexingIndicator->show();
}

void SearchWidget::indexingFinished()
{
    m_progress->reportFinished();

    delete m_progress;
    m_progress = nullptr;

    m_queryWidget->show();
    m_indexingDocumentationLabel->hide();
    m_indexingIndicator->hide();
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
                    emit linkActivated(link, currentSearchTerms(), true/*newPage*/);
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

    QAction *openLink = nullptr;
    QAction *openLinkInNewTab = nullptr;
    QAction *copyAnchorAction = nullptr;

    QMenu menu;
    QUrl link = browser->anchorAt(point);
    if (!link.isEmpty() && link.isValid()) {
        if (link.isRelative())
            link = browser->source().resolved(link);
        openLink = menu.addAction(tr("Open Link"));
        openLinkInNewTab = menu.addAction(tr("Open Link as New Page"));
        copyAnchorAction = menu.addAction(tr("Copy Link"));
    } else if (browser->textCursor().hasSelection()) {
        connect(menu.addAction(tr("Copy")), &QAction::triggered, browser, &QTextEdit::copy);
    } else {
        connect(menu.addAction(tr("Reload")), &QAction::triggered, browser, &QTextBrowser::reload);
    }

    QAction *usedAction = menu.exec(mapToGlobal(contextMenuEvent->pos()));
    if (usedAction == openLink)
        emit linkActivated(link, currentSearchTerms(), false/*newPage*/);
    else if (usedAction == openLinkInNewTab)
        emit linkActivated(link, currentSearchTerms(), true/*newPage*/);
    else if (usedAction == copyAnchorAction)
        QApplication::clipboard()->setText(link.toString());
}

QStringList SearchWidget::currentSearchTerms() const
{
    return searchEngine->searchInput().split(QRegExp("\\W+"), QString::SkipEmptyParts);
}

// #pragma mark -- SearchSideBarItem

SearchSideBarItem::SearchSideBarItem()
    : SideBarItem(new SearchWidget, Constants::HELP_SEARCH)
{
    widget()->setWindowTitle(HelpPlugin::tr(Constants::SB_SEARCH));
    connect(static_cast<SearchWidget *>(widget()), &SearchWidget::linkActivated,
            this, &SearchSideBarItem::linkActivated);
}

QList<QToolButton *> SearchSideBarItem::createToolBarWidgets()
{
    QToolButton *reindexButton = new QToolButton;
    reindexButton->setIcon(Utils::Icons::RELOAD.icon());
    reindexButton->setToolTip(tr("Regenerate Index"));
    connect(reindexButton, &QAbstractButton::clicked,
            static_cast<SearchWidget *>(widget()), &SearchWidget::reindexDocumentation);
    return QList<QToolButton *>() << reindexButton;
}
