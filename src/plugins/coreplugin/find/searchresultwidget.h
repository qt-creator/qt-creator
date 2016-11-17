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

#include "searchresultwindow.h"

#include <coreplugin/infobar.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFrame;
class QLabel;
class QLineEdit;
class QToolButton;
class QCheckBox;
QT_END_NAMESPACE

namespace Core {
namespace Internal {

class SearchResultTreeView;
class SearchResultColor;

class SearchResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchResultWidget(QWidget *parent = 0);
    ~SearchResultWidget();

    void setInfo(const QString &label, const QString &toolTip, const QString &term);

    void addResult(const QString &fileName,
                   const QString &lineText,
                   Search::TextRange mainRange,
                   const QVariant &userData = QVariant());
    void addResults(const QList<SearchResultItem> &items, SearchResult::AddMode mode);

    int count() const;

    void setSupportsReplace(bool replaceSupported, const QString &group);

    void setTextToReplace(const QString &textToReplace);
    QString textToReplace() const;
    void setSupportPreserveCase(bool enabled);

    bool hasFocusInternally() const;
    void setFocusInternally();
    bool canFocusInternally() const;

    void notifyVisibilityChanged(bool visible);

    void setTextEditorFont(const QFont &font, const SearchResultColor &color);
    void setTabWidth(int tabWidth);

    void setAutoExpandResults(bool expand);
    void expandAll();
    void collapseAll();

    void goToNext();
    void goToPrevious();

    void restart();

    void setSearchAgainSupported(bool supported);
    void setSearchAgainEnabled(bool enabled);

public slots:
    void finishSearch(bool canceled);
    void sendRequestPopup();

signals:
    void activated(const Core::SearchResultItem &item);
    void replaceButtonClicked(const QString &replaceText, const QList<Core::SearchResultItem> &checkedItems, bool preserveCase);
    void searchAgainRequested();
    void cancelled();
    void paused(bool paused);
    void restarted();
    void visibilityChanged(bool visible);
    void requestPopup(bool focus);

    void navigateStateChanged();

private:
    void handleJumpToSearchResult(const SearchResultItem &item);
    void handleReplaceButton();
    void cancel();
    void searchAgain();

    void setShowReplaceUI(bool visible);
    void continueAfterSizeWarning();
    void cancelAfterSizeWarning();

    QList<SearchResultItem> checkedItems() const;
    void updateMatchesFoundLabel();

    SearchResultTreeView *m_searchResultTreeView;
    int m_count;
    QString m_dontAskAgainGroup;
    QFrame *m_messageWidget;
    InfoBar m_infoBar;
    InfoBarDisplay m_infoBarDisplay;
    QWidget *m_topReplaceWidget;
    QLabel *m_replaceLabel;
    QLineEdit *m_replaceTextEdit;
    QToolButton *m_replaceButton;
    QToolButton *m_searchAgainButton;
    QCheckBox *m_preserveCaseCheck;
    QWidget *m_descriptionContainer;
    QLabel *m_label;
    QLabel *m_searchTerm;
    QToolButton *m_cancelButton;
    QLabel *m_matchesFoundLabel;
    bool m_preserveCaseSupported;
    bool m_isShowingReplaceUI;
    bool m_searchAgainSupported;
    bool m_replaceSupported;
};

} // Internal
} // Find
