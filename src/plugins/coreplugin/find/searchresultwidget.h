// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "searchresultwindow.h"

#include <utils/infobar.h>
#include <utils/searchresultitem.h>

#include <QWidget>

QT_BEGIN_NAMESPACE
class QFrame;
class QLabel;
class QLineEdit;
class QToolButton;
class QCheckBox;
QT_END_NAMESPACE

namespace Utils { class InfoLabel; }
namespace Core {

namespace Internal {
class SearchResultTreeView;

class SearchResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SearchResultWidget(QWidget *parent = nullptr);
    ~SearchResultWidget() override;

    void setInfo(const QString &label, const QString &toolTip, const QString &term);
    QWidget *additionalReplaceWidget() const;
    void setAdditionalReplaceWidget(QWidget *widget);

    void addResults(const Utils::SearchResultItems &items, SearchResult::AddMode mode);

    int count() const;
    bool isSearching() const { return m_searching; }

    void setSupportsReplace(bool replaceSupported, const QString &group);
    bool supportsReplace() const;
    void triggerReplace() { doReplace(); }

    void setTextToReplace(const QString &textToReplace);
    QString textToReplace() const;
    void setSupportPreserveCase(bool enabled);

    bool hasFocusInternally() const;
    void setFocusInternally();
    bool canFocusInternally() const;

    void notifyVisibilityChanged(bool visible);

    void setTextEditorFont(const QFont &font, const Utils::SearchResultColors &colors);
    void setTabWidth(int tabWidth);

    void setAutoExpandResults(bool expand);
    void expandAll();
    void collapseAll();

    void goToNext();
    void goToPrevious();

    void restart();

    void setSearchAgainSupported(bool supported);
    void setSearchAgainEnabled(bool enabled);
    void setFilter(SearchResultFilter *filter);
    bool hasFilter() const;
    void showFilterWidget(QWidget *parent);
    void setReplaceEnabled(bool enabled);
    Utils::SearchResultItems items(bool checkedOnly) const;

public slots:
    void finishSearch(bool canceled, const QString &reason);
    void sendRequestPopup();

signals:
    void activated(const Utils::SearchResultItem &item);
    void replaceButtonClicked(const QString &replaceText,
                              const Utils::SearchResultItems &checkedItems, bool preserveCase);
    void replaceTextChanged(const QString &replaceText);
    void searchAgainRequested();
    void canceled();
    void paused(bool paused);
    void restarted();
    void visibilityChanged(bool visible);
    void requestPopup(bool focus);
    void filterInvalidated();
    void filterChanged();

    void navigateStateChanged();

private:
    void handleJumpToSearchResult(const Utils::SearchResultItem &item);
    void handleReplaceButton();
    void doReplace();
    void cancel();
    void searchAgain();

    void setShowReplaceUI(bool visible);
    void continueAfterSizeWarning();
    void cancelAfterSizeWarning();

    void updateMatchesFoundLabel();

    SearchResultTreeView *m_searchResultTreeView = nullptr;
    bool m_searching = true;
    int m_count = 0;
    QString m_dontAskAgainGroup;
    QFrame *m_messageWidget = nullptr;
    Utils::InfoBar m_infoBar;
    Utils::InfoBarDisplay m_infoBarDisplay;
    QWidget *m_topReplaceWidget = nullptr;
    QLabel *m_replaceLabel = nullptr;
    QLineEdit *m_replaceTextEdit = nullptr;
    QToolButton *m_replaceButton = nullptr;
    QToolButton *m_searchAgainButton = nullptr;
    QCheckBox *m_preserveCaseCheck = nullptr;
    QWidget *m_additionalReplaceWidget = nullptr;
    QWidget *m_descriptionContainer = nullptr;
    QLabel *m_label = nullptr;
    QLabel *m_searchTerm = nullptr;
    Utils::InfoLabel *m_messageLabel = nullptr;
    QToolButton *m_cancelButton = nullptr;
    QLabel *m_matchesFoundLabel = nullptr;
    bool m_preserveCaseSupported = true;
    bool m_isShowingReplaceUI = false;
    bool m_searchAgainSupported = false;
    bool m_replaceSupported = false;
};

} // Internal
} // Find
