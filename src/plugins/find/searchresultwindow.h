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

#ifndef SEARCHRESULTWINDOW_H
#define SEARCHRESULTWINDOW_H

#include "find_global.h"
#include "searchresulttreeview.h"

#include <coreplugin/ioutputpane.h>

#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtGui/QStackedWidget>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>

namespace Find {

class SearchResultWindow;

class FIND_EXPORT ResultWindowItem : public QObject
{
    Q_OBJECT

signals:
    void activated(const QString &fileName, int lineNumber, int column);

    friend class SearchResultWindow;
};

class FIND_EXPORT SearchResultWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    SearchResultWindow();
    ~SearchResultWindow();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets(void) const;

    QString name() const { return tr("Search Results"); }
    int priorityInStatusBar() const;
    void visibilityChanged(bool visible);
    bool isEmpty() const;
    int numberOfResults() const;
    bool hasFocus();
    bool canFocus();
    void setFocus();

public slots:
    void clearContents();
    void showNoMatchesFound();
    ResultWindowItem *addResult(const QString &fileName, int lineNumber, const QString &lineText,
        int searchTermStart, int searchTermLength);

private slots:
    void handleExpandCollapseToolButton(bool checked);
    void handleJumpToSearchResult(int index, const QString &fileName, int lineNumber,
        int searchTermStart, int searchTermLength);

private:
    void readSettings();
    void writeSettings();

    Internal::SearchResultTreeView *m_searchResultTreeView;
    QListWidget *m_noMatchesFoundDisplay;
    QToolButton *m_expandCollapseToolButton;
    static const bool m_initiallyExpand = false;
    QStackedWidget *m_widget;
    QList<ResultWindowItem *> m_items;
};

} // namespace Find

#endif // SEARCHRESULTWINDOW_H
