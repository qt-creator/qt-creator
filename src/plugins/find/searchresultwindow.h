/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef SEARCHRESULTWINDOW_H
#define SEARCHRESULTWINDOW_H

#include "find_global.h"
#include "searchresulttreeview.h"

#include <coreplugin/ioutputpane.h>
#include <coreplugin/icore.h>

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
    SearchResultWindow(Core::ICore *core);
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
    Core::ICore *m_core;
    QToolButton *m_expandCollapseToolButton;
    static const bool m_initiallyExpand = false;
    QStackedWidget *m_widget;
    QList<ResultWindowItem *> m_items;
};

} // namespace Find

#endif // SEARCHRESULTWINDOW_H
