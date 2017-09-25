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

#include "searchresultitem.h"

#include <coreplugin/ioutputpane.h>

#include <QVariant>
#include <QStringList>
#include <QIcon>

QT_BEGIN_NAMESPACE
class QFont;
QT_END_NAMESPACE

namespace Core {
namespace Internal {
    class SearchResultWindowPrivate;
    class SearchResultWidget;
}
class SearchResultWindow;

class CORE_EXPORT SearchResult : public QObject
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
    QWidget *additionalReplaceWidget() const;

public slots:
    void addResult(const QString &fileName,
                   int lineNumber,
                   const QString &lineText,
                   int searchTermStart,
                   int searchTermLength,
                   const QVariant &userData = QVariant());
    void addResult(const QString &fileName,
                   const QString &lineText,
                   Search::TextRange mainRange,
                   const QVariant &userData = QVariant());
    void addResults(const QList<SearchResultItem> &items, AddMode mode);
    void finishSearch(bool canceled);
    void setTextToReplace(const QString &textToReplace);
    void restart();
    void setSearchAgainEnabled(bool enabled);
    void popup();

signals:
    void activated(const Core::SearchResultItem &item);
    void replaceButtonClicked(const QString &replaceText, const QList<Core::SearchResultItem> &checkedItems, bool preserveCase);
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

class CORE_EXPORT SearchResultWindow : public IOutputPane
{
    Q_OBJECT

public:
    enum SearchMode {
        SearchOnly,
        SearchAndReplace
    };

    enum PreserveCaseMode {
        PreserveCaseEnabled,
        PreserveCaseDisabled
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
    void setTabWidth(int width);
    void openNewSearchPanel();

    // The search result window owns the returned SearchResult
    // and might delete it any time, even while the search is running
    // (e.g. when the user clears the search result pane, or if the user opens so many other searches
    // that this search falls out of the history).
    SearchResult *startNewSearch(const QString &label,
                                 const QString &toolTip,
                                 const QString &searchTerm,
                                 SearchMode searchOrSearchAndReplace = SearchOnly,
                                 PreserveCaseMode preserveCaseMode = PreserveCaseEnabled,
                                 const QString &cfgGroup = QString());

public slots:
    void clearContents();

public: // Used by plugin, do not use
    void writeSettings();

private:
    void readSettings();

    Internal::SearchResultWindowPrivate *d;
    static SearchResultWindow *m_instance;
};

} // namespace Core
