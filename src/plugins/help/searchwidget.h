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

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QtCore/QFutureInterface>
#include <QtCore/QFutureWatcher>

#include <QtGui/QWidget>

QT_FORWARD_DECLARE_CLASS(QHelpSearchEngine)
QT_FORWARD_DECLARE_CLASS(QHelpSearchResultWidget)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)
QT_FORWARD_DECLARE_CLASS(QUrl)

namespace Help {
namespace Internal {

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    SearchWidget();
    ~SearchWidget();

    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void requestShowLink(const QUrl &url);
    void requestShowLinkInNewTab(const QUrl &url);
    void escapePressed();

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
    void keyPressEvent(QKeyEvent *keyEvent);
    void contextMenuEvent(QContextMenuEvent *contextMenuEvent);

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
