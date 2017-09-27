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

#pragma once

#include <coreplugin/sidebar.h>

#include <QFutureInterface>
#include <QFutureWatcher>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QHelpSearchEngine;
class QHelpSearchResultWidget;
class QUrl;
QT_END_NAMESPACE

namespace Help {
namespace Internal {

class SearchSideBarItem : public Core::SideBarItem
{
    Q_OBJECT

public:
    SearchSideBarItem();

    QList<QToolButton *> createToolBarWidgets();

signals:
    void linkActivated(const QUrl &url, const QStringList &searchTerms, bool newPage);
};

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    SearchWidget();
    ~SearchWidget();

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void reindexDocumentation();

signals:
    void linkActivated(const QUrl &link, const QStringList &searchTerms, bool newPage);

protected:
    void showEvent(QShowEvent *event);

private:
    void search() const;

    void searchingStarted();
    void searchingFinished(int hits);

    void indexingStarted();
    void indexingFinished();

    bool eventFilter(QObject* o, QEvent *e);
    void contextMenuEvent(QContextMenuEvent *contextMenuEvent);
    QStringList currentSearchTerms() const;

    int zoomCount;

    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> *m_progress;

    QHelpSearchEngine *searchEngine;
    QHelpSearchResultWidget *resultWidget;
};

} // namespace Internal
} // namespace Help
