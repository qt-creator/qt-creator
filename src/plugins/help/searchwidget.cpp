// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "searchwidget.h"

#include "helpconstants.h"
#include "helptr.h"
#include "localhelpmanager.h"

#include <coreplugin/coreconstants.h>
#include <coreplugin/icore.h>
#include <coreplugin/progressmanager/progressmanager.h>

#include <utils/progressindicator.h>
#include <utils/stringutils.h>
#include <utils/styledbar.h>
#include <utils/utilsicons.h>

#include <QApplication>
#include <QHelpEngine>
#include <QHelpSearchEngine>
#include <QHelpSearchQueryWidget>
#include <QHelpSearchResultWidget>
#include <QKeyEvent>
#include <QLayout>
#include <QLabel>
#include <QMap>
#include <QMenu>
#include <QRegularExpression>
#include <QString>
#include <QStringList>
#include <QTextBrowser>
#include <QToolButton>

namespace Help::Internal {

SearchWidget::SearchWidget() = default;

SearchWidget::~SearchWidget() = default;

void SearchWidget::zoomIn()
{
    auto browser = resultWidget->findChild<QTextBrowser*>();
    if (browser && zoomCount != 10) {
        zoomCount++;
        browser->zoomIn();
    }
}

void SearchWidget::zoomOut()
{
    auto browser = resultWidget->findChild<QTextBrowser*>();
    if (browser && zoomCount != -5) {
        zoomCount--;
        browser->zoomOut();
    }
}

void SearchWidget::resetZoom()
{
    if (zoomCount == 0)
        return;

    auto browser = resultWidget->findChild<QTextBrowser*>();
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
        auto vLayout = new QVBoxLayout(this);
        vLayout->setContentsMargins(0, 0, 0, 0);
        vLayout->setSpacing(0);

        searchEngine = new QHelpSearchEngine(&LocalHelpManager::helpEngine(), this);

        auto toolbar = new Utils::StyledBar(this);
        toolbar->setSingleRow(false);
        m_queryWidget = searchEngine->queryWidget();
        QLayout *tbLayout = new QVBoxLayout();
        tbLayout->setSpacing(6);
        tbLayout->setContentsMargins(4, 4, 4, 4);
        tbLayout->addWidget(m_queryWidget);
        m_indexingDocumentationLabel = new QLabel(Tr::tr("Indexing Documentation"), toolbar);
        m_indexingDocumentationLabel->hide();
        tbLayout->addWidget(m_indexingDocumentationLabel);
        toolbar->setLayout(tbLayout);

        auto toolbar2 = new Utils::StyledBar(this);
        toolbar2->setSingleRow(false);
        tbLayout = new QVBoxLayout();
        tbLayout->setSpacing(0);
        tbLayout->setContentsMargins(0, 0, 0, 0);
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

        auto browser = resultWidget->findChild<const QTextBrowser*>();
        browser->viewport()->installEventFilter(this);

        connect(searchEngine, &QHelpSearchEngine::indexingStarted, this,
            &SearchWidget::indexingStarted);
        connect(searchEngine, &QHelpSearchEngine::indexingFinished, this,
            &SearchWidget::indexingFinished);

        QMetaObject::invokeMethod(&LocalHelpManager::helpEngine(), &QHelpEngine::setupFinished,
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
    Core::ProgressManager::addTask(m_progress->future(),
                                   Tr::tr("Indexing Documentation"),
                                   "Help.Indexer");
    m_progress->setProgressRange(0, 2);
    m_progress->setProgressValueAndText(1, Tr::tr("Indexing Documentation"));
    m_progress->reportStarted();

    connect(&m_watcher, &QFutureWatcherBase::canceled,
            searchEngine, &QHelpSearchEngine::cancelIndexing);
    m_watcher.setFuture(m_progress->future());

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
    auto browser = resultWidget->findChild<const QTextBrowser *>();
    if (browser && o == browser->viewport()
        && e->type() == QEvent::MouseButtonRelease){
        auto me = static_cast<const QMouseEvent *>(e);
        QUrl link = resultWidget->linkAt(me->pos());
        if (!link.isEmpty() || link.isValid()) {
            bool controlPressed = me->modifiers() & Qt::ControlModifier;
            if ((me->button() == Qt::LeftButton && controlPressed)
                || (me->button() == Qt::MiddleButton)) {
                    emit linkActivated(link, currentSearchTerms(), true/*newPage*/);
            }
        }
    }
    return QWidget::eventFilter(o,e);
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *contextMenuEvent)
{
    auto browser = resultWidget->findChild<QTextBrowser *>();
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
        openLink = menu.addAction(Tr::tr("Open Link"));
        openLinkInNewTab = menu.addAction(Tr::tr("Open Link as New Page"));
        copyAnchorAction = menu.addAction(Tr::tr("Copy Link"));
    } else if (browser->textCursor().hasSelection()) {
        connect(menu.addAction(Tr::tr("Copy")), &QAction::triggered, browser, &QTextEdit::copy);
    } else {
        connect(menu.addAction(Tr::tr("Reload")),
                &QAction::triggered,
                browser,
                &QTextBrowser::reload);
    }

    QAction *usedAction = menu.exec(mapToGlobal(contextMenuEvent->pos()));
    if (usedAction == openLink)
        emit linkActivated(link, currentSearchTerms(), false/*newPage*/);
    else if (usedAction == openLinkInNewTab)
        emit linkActivated(link, currentSearchTerms(), true/*newPage*/);
    else if (usedAction == copyAnchorAction)
        Utils::setClipboardAndSelection(link.toString());
}

QStringList SearchWidget::currentSearchTerms() const
{
    return searchEngine->searchInput().split(QRegularExpression("\\W+"), Qt::SkipEmptyParts);
}

// #pragma mark -- SearchSideBarItem

SearchSideBarItem::SearchSideBarItem()
    : SideBarItem(new SearchWidget, Constants::HELP_SEARCH)
{
    widget()->setWindowTitle(Tr::tr(Constants::SB_SEARCH));
    connect(static_cast<SearchWidget *>(widget()), &SearchWidget::linkActivated,
            this, &SearchSideBarItem::linkActivated);
}

QList<QToolButton *> SearchSideBarItem::createToolBarWidgets()
{
    auto reindexButton = new QToolButton;
    reindexButton->setIcon(Utils::Icons::RELOAD_TOOLBAR.icon());
    reindexButton->setToolTip(Tr::tr("Regenerate Index"));
    connect(reindexButton, &QAbstractButton::clicked,
            static_cast<SearchWidget *>(widget()), &SearchWidget::reindexDocumentation);
    return {reindexButton};
}

} // Help::Internal
