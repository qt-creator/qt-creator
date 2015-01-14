/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <coreplugin/sidebar.h>

#include <QFutureInterface>
#include <QFutureWatcher>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QHelpSearchEngine;
class QHelpSearchResultWidget;
class QMouseEvent;
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


public slots:
    void reindexDocumentation();

signals:
    void linkActivated(const QUrl &link, const QStringList &searchTerms, bool newPage);

protected:
    void showEvent(QShowEvent *event);

private slots:
    void search() const;

    void searchingStarted();
    void searchingFinished(int hits);

    void indexingStarted();
    void indexingFinished();

private:
    bool eventFilter(QObject* o, QEvent *e);
    void contextMenuEvent(QContextMenuEvent *contextMenuEvent);
    QStringList currentSearchTerms() const;

private:
    int zoomCount;

    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> *m_progress;

    QHelpSearchEngine *searchEngine;
    QHelpSearchResultWidget *resultWidget;
};

} // namespace Internal
} // namespace Help

#endif // SEARCHWIDGET_H
