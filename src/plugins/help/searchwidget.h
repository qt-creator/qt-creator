/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef SEARCHWIDGET_H
#define SEARCHWIDGET_H

#include <QtCore/QUrl>
#include <QtCore/QPoint>

#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class QMouseEvent;
class QHelpSearchEngine;
class QHelpSearchResultWidget;

QT_END_NAMESPACE

namespace Help {
namespace Internal {

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    SearchWidget(QHelpSearchEngine *engine, QWidget *parent = 0);
    ~SearchWidget();

    void zoomIn();
    void zoomOut();
    void resetZoom();

signals:
    void requestShowLink(const QUrl &url);
    void requestShowLinkInNewTab(const QUrl &url);
    void escapePressed();

private slots:
    void search() const;
    void searchingStarted();
    void searchingFinished(int hits);

private:
    void keyPressEvent(QKeyEvent *keyEvent);
    void contextMenuEvent(QContextMenuEvent *contextMenuEvent);

private:
    int zoomCount;
    QHelpSearchEngine *searchEngine;
    QHelpSearchResultWidget *resultWidget;
};

} // namespace Internal
} // namespace Help

#endif // SEARCHWIDGET_H
