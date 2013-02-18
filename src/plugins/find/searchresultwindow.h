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

#ifndef SEARCHRESULTWINDOW_H
#define SEARCHRESULTWINDOW_H

#include "find_global.h"

#include <coreplugin/ioutputpane.h>

#include <QVariant>
#include <QStringList>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QFont;
QT_END_NAMESPACE

namespace Find {
namespace Internal {
    class SearchResultTreeView;
    class SearchResultWindowPrivate;
    class SearchResultWidget;
}
class SearchResultWindow;

struct FIND_EXPORT SearchResultItem
{
    SearchResultItem()
        : textMarkPos(-1),
        textMarkLength(0),
        lineNumber(-1),
        useTextEditorFont(false)
    {
    }

    SearchResultItem(const SearchResultItem &other)
        : path(other.path),
        text(other.text),
        textMarkPos(other.textMarkPos),
        textMarkLength(other.textMarkLength),
        icon(other.icon),
        lineNumber(other.lineNumber),
        useTextEditorFont(other.useTextEditorFont),
        userData(other.userData)
    {
    }

    QStringList path; // hierarchy to the parent item of this item
    QString text; // text to show for the item itself
    int textMarkPos; // 0-based starting position for a mark (-1 for no mark)
    int textMarkLength; // length of the mark (0 for no mark)
    QIcon icon; // icon to show in front of the item (by be null icon to hide)
    int lineNumber; // (0 or -1 for no line number)
    bool useTextEditorFont;
    QVariant userData; // user data for identification of the item
};

class FIND_EXPORT SearchResult : public QObject
{
    Q_OBJECT

public:
    enum AddMode {
        AddSorted,
        AddOrdered
    };

    void setUserData(const QVariant &data);
    QVariant userData() const;
    QString textToReplace() const;
    int count() const;
    void setSearchAgainSupported(bool supported);

public slots:
    void addResult(const QString &fileName, int lineNumber, const QString &lineText,
                   int searchTermStart, int searchTermLength, const QVariant &userData = QVariant());
    void addResults(const QList<SearchResultItem> &items, AddMode mode);
    void finishSearch(bool canceled);
    void setTextToReplace(const QString &textToReplace);
    void restart();
    void setSearchAgainEnabled(bool enabled);
    void popup();

signals:
    void activated(const Find::SearchResultItem &item);
    void replaceButtonClicked(const QString &replaceText, const QList<Find::SearchResultItem> &checkedItems, bool preserveCase);
    void cancelled();
    void paused(bool paused);
    void visibilityChanged(bool visible);
    void countChanged(int count);
    void searchAgainRequested();
    void requestEnabledCheck();

private:
    SearchResult(Internal::SearchResultWidget *widget);
    friend class SearchResultWindow; // for the constructor

private:
    Internal::SearchResultWidget *m_widget;
    QVariant m_userData;
};

class FIND_EXPORT SearchResultWindow : public Core::IOutputPane
{
    Q_OBJECT

public:
    enum SearchMode {
        SearchOnly,
        SearchAndReplace
    };


    SearchResultWindow(QWidget *newSearchPanel);
    virtual ~SearchResultWindow();
    static SearchResultWindow *instance();

    QWidget *outputWidget(QWidget *);
    QList<QWidget*> toolBarWidgets() const;

    QString displayName() const { return tr("Search Results"); }
    int priorityInStatusBar() const;
    void visibilityChanged(bool visible);
    bool hasFocus() const;
    bool canFocus() const;
    void setFocus();

    bool canNext() const;
    bool canPrevious() const;
    void goToNext();
    void goToPrev();
    bool canNavigate() const;

    void setTextEditorFont(const QFont &font,
                           const QColor &textForegroundColor,
                           const QColor &textBackgroundColor,
                           const QColor &highlightForegroundColor,
                           const QColor &highlightBackgroundColor);
    void openNewSearchPanel();

    // The search result window owns the returned SearchResult
    // and might delete it any time, even while the search is running
    // (e.g. when the user clears the search result pane, or if the user opens so many other searches
    // that this search falls out of the history).
    SearchResult *startNewSearch(const QString &label,
                                 const QString &toolTip,
                                 const QString &searchTerm,
                                 SearchMode searchOrSearchAndReplace = SearchOnly,
                                 const QString &cfgGroup = QString());

public slots:
    void clearContents();

private slots:
    void handleExpandCollapseToolButton(bool checked);

private:
    void readSettings();
    void writeSettings();

    Internal::SearchResultWindowPrivate *d;
    static SearchResultWindow *m_instance;
};

} // namespace Find

Q_DECLARE_METATYPE(Find::SearchResultItem)

#endif // SEARCHRESULTWINDOW_H
