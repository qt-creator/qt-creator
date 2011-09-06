/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (info@qt.nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at info@qt.nokia.com.
**
**************************************************************************/

#ifndef SEARCHRESULTWIDGET_H
#define SEARCHRESULTWIDGET_H

#include "searchresultwindow.h"

#include <coreplugin/infobar.h>

#include <QtGui/QLabel>
#include <QtGui/QLineEdit>
#include <QtGui/QListWidget>
#include <QtGui/QToolButton>
#include <QtGui/QWidget>

namespace Find {
namespace Internal {

class SearchResultTreeView;

class SearchResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchResultWidget(QWidget *parent = 0);

    void addResult(const QString &fileName, int lineNumber, const QString &lineText,
                   int searchTermStart, int searchTermLength, const QVariant &userData = QVariant());
    void addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode);

    int count() const;

    QString dontAskAgainGroup() const;
    void setDontAskAgainGroup(const QString &group);

    void setTextToReplace(const QString &textToReplace);
    QString textToReplace() const;
    void setShowReplaceUI(bool visible);

    bool hasFocusInternally() const;
    void setFocusInternally();
    bool canFocusInternally() const;

    void notifyVisibilityChanged(bool visible);

    void setTextEditorFont(const QFont &font);

    void setAutoExpandResults(bool expand);
    void expandAll();
    void collapseAll();

    void goToNext();
    void goToPrevious();

    // TODO: temporary
    QList<QWidget*> toolBarWidgets() const { return QList<QWidget*>() << m_replaceLabel << m_replaceTextEdit << m_replaceButton; }

public slots:
    void finishSearch();
    void clear();
    void showNoMatchesFound();

signals:
    void activated(const Find::SearchResultItem &item);
    void replaceButtonClicked(const QString &replaceText, const QList<Find::SearchResultItem> &checkedItems);
    void visibilityChanged(bool visible);

    void navigateStateChanged();

private slots:
    void hideNoUndoWarning();
    void handleJumpToSearchResult(const SearchResultItem &item);
    void handleReplaceButton();

private:
    bool showWarningMessage() const;
    void setShowWarningMessage(bool showWarningMessage);
    QList<SearchResultItem> checkedItems() const;

    SearchResultTreeView *m_searchResultTreeView;
    int m_count;
    QString m_dontAskAgainGroup;
    Core::InfoBar m_infoBar;
    Core::InfoBarDisplay m_infoBarDisplay;
    QListWidget *m_noMatchesFoundDisplay;
    bool m_isShowingReplaceUI;
    QLabel *m_replaceLabel;
    QLineEdit *m_replaceTextEdit;
    QToolButton *m_replaceButton;
};

} // Internal
} // Find

#endif // SEARCHRESULTWIDGET_H
