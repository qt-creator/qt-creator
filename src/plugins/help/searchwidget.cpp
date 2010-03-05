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

#include <QtCore/QMap>
#include <QtCore/QString>
#include <QtCore/QStringList>

#include <QtGui/QMenu>
#include <QtGui/QLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QClipboard>
#include <QtGui/QApplication>
#include <QtGui/QTextBrowser>

#include <QtHelp/QHelpSearchEngine>
#include <QtHelp/QHelpSearchQueryWidget>
#include <QtHelp/QHelpSearchResultWidget>

using namespace Help::Internal;

SearchWidget::SearchWidget(QHelpSearchEngine *engine, QWidget *parent)
    : QWidget(parent)
    , zoomCount(0)
    , searchEngine(engine)
{
    QVBoxLayout *vLayout = new QVBoxLayout(this);
    vLayout->setMargin(4);

    resultWidget = searchEngine->resultWidget();
    QHelpSearchQueryWidget *queryWidget = searchEngine->queryWidget();

    vLayout->addWidget(queryWidget);
    vLayout->addWidget(resultWidget);

    setFocusProxy(queryWidget);

    connect(queryWidget, SIGNAL(search()), this, SLOT(search()));
    connect(resultWidget, SIGNAL(requestShowLink(QUrl)), this,
        SIGNAL(requestShowLink(QUrl)));

    connect(searchEngine, SIGNAL(searchingStarted()), this,
        SLOT(searchingStarted()));
    connect(searchEngine, SIGNAL(searchingFinished(int)), this,
        SLOT(searchingFinished(int)));

    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    browser->viewport()->installEventFilter(this);
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
                    emit requestShowLinkInNewTab(link);
            }
        }
    }
    return QWidget::eventFilter(o,e);
}

void SearchWidget::keyPressEvent(QKeyEvent *keyEvent)
{
    if (keyEvent->key() == Qt::Key_Escape)
        emit escapePressed();
}

void SearchWidget::contextMenuEvent(QContextMenuEvent *contextMenuEvent)
{
    QMenu menu;
    QPoint point = contextMenuEvent->globalPos();

    QTextBrowser* browser = qFindChild<QTextBrowser*>(resultWidget);
    if (!browser)
        return;

    point = browser->mapFromGlobal(point);
    if (!browser->rect().contains(point, true))
        return;

    QUrl link = browser->anchorAt(point);

    QKeySequence keySeq(QKeySequence::Copy);
    QAction *copyAction = menu.addAction(tr("&Copy") + QLatin1String("\t") +
        keySeq.toString(QKeySequence::NativeText));
    copyAction->setEnabled(QTextCursor(browser->textCursor()).hasSelection());

    QAction *copyAnchorAction = menu.addAction(tr("Copy &Link Location"));
    copyAnchorAction->setEnabled(!link.isEmpty() && link.isValid());

    keySeq = QKeySequence(Qt::CTRL);
    QAction *newTabAction = menu.addAction(tr("Open Link in New Tab") +
        QLatin1String("\t") + keySeq.toString(QKeySequence::NativeText) +
        QLatin1String("LMB"));
    newTabAction->setEnabled(!link.isEmpty() && link.isValid());

    menu.addSeparator();

    keySeq = QKeySequence::SelectAll;
    QAction *selectAllAction = menu.addAction(tr("Select All") +
        QLatin1String("\t") + keySeq.toString(QKeySequence::NativeText));

    QAction *usedAction = menu.exec(mapToGlobal(contextMenuEvent->pos()));
    if (usedAction == copyAction) {
        QTextCursor cursor = browser->textCursor();
        if (!cursor.isNull() && cursor.hasSelection()) {
            QString selectedText = cursor.selectedText();
            QMimeData *data = new QMimeData();
            data->setText(selectedText);
            QApplication::clipboard()->setMimeData(data);
        }
    }
    else if (usedAction == copyAnchorAction) {
        QApplication::clipboard()->setText(link.toString());
    }
    else if (usedAction == newTabAction) {
        emit requestShowLinkInNewTab(link);
    }
    else if (usedAction == selectAllAction) {
        browser->selectAll();
    }
}
