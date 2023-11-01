// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../ioutputpane.h"

#include <utils/searchresultitem.h>

#include <QVariant>
#include <QStringList>
#include <QIcon>

#include <functional>

QT_BEGIN_NAMESPACE
class QFont;
QT_END_NAMESPACE

namespace Core {

namespace Internal {
class SearchResultWindowPrivate;
class SearchResultWidget;
}
class SearchResultWindow;

class CORE_EXPORT SearchResultFilter : public QObject
{
    Q_OBJECT

public:
    virtual QWidget *createWidget() = 0;
    virtual bool matches(const Utils::SearchResultItem &item) const = 0;

signals:
    void filterChanged();
};

class CORE_EXPORT SearchResult : public QObject
{
    Q_OBJECT

public:
    enum AddMode {
        AddSortedByContent,
        AddSortedByPosition,
        AddOrdered
    };

    void setUserData(const QVariant &data);
    QVariant userData() const;
    bool supportsReplace() const;
    QString textToReplace() const;
    int count() const;
    void setSearchAgainSupported(bool supported);
    QWidget *additionalReplaceWidget() const;
    void setAdditionalReplaceWidget(QWidget *widget);
    void makeNonInteractive(const std::function<void()> &callback);
    bool isInteractive() const { return !m_finishedHandler; }
    Utils::SearchResultItems allItems() const;

public slots:
    void addResult(const Utils::SearchResultItem &item);
    void addResults(const Utils::SearchResultItems &items, AddMode mode);
    void setFilter(SearchResultFilter *filter); // Takes ownership
    void finishSearch(bool canceled, const QString &reason = {});
    void setTextToReplace(const QString &textToReplace);
    void restart();
    void setReplaceEnabled(bool enabled);
    void setSearchAgainEnabled(bool enabled);
    void popup();

signals:
    void activated(const Utils::SearchResultItem &item);
    void replaceButtonClicked(const QString &replaceText,
                              const Utils::SearchResultItems &checkedItems, bool preserveCase);
    void replaceTextChanged(const QString &replaceText);
    void canceled();
    void paused(bool paused);
    void visibilityChanged(bool visible);
    void countChanged(int count);
    void searchAgainRequested();

private:
    SearchResult(Internal::SearchResultWidget *widget);
    friend class SearchResultWindow; // for the constructor

private:
    Internal::SearchResultWidget *m_widget;
    QVariant m_userData;
    std::function<void()> m_finishedHandler;
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
    ~SearchResultWindow() override;
    static SearchResultWindow *instance();

    QWidget *outputWidget(QWidget *) override;
    QList<QWidget*> toolBarWidgets() const override;

    void visibilityChanged(bool visible) override;
    bool hasFocus() const override;
    bool canFocus() const override;
    void setFocus() override;

    bool canNext() const override;
    bool canPrevious() const override;
    void goToNext() override;
    void goToPrev() override;
    bool canNavigate() const override;

    void setTextEditorFont(const QFont &font, const Utils::SearchResultColors &colors);
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
    void clearContents() override;

public: // Used by plugin, do not use
    void writeSettings();

private:
    void readSettings();

    Internal::SearchResultWindowPrivate *d;
    static SearchResultWindow *m_instance;
};

} // namespace Core
