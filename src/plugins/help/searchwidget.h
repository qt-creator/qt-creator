// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/sidebar.h>

#include <QFutureInterface>
#include <QFutureWatcher>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QHelpSearchEngine;
class QHelpSearchQueryWidget;
class QHelpSearchResultWidget;
class QUrl;
QT_END_NAMESPACE

namespace Utils { class ProgressIndicator; }

namespace Help {
namespace Internal {

class SearchSideBarItem : public Core::SideBarItem
{
    Q_OBJECT

public:
    SearchSideBarItem();

    QList<QToolButton *> createToolBarWidgets() override;

signals:
    void linkActivated(const QUrl &url, const QStringList &searchTerms, bool newPage);
};

class SearchWidget : public QWidget
{
    Q_OBJECT

public:
    SearchWidget();
    ~SearchWidget() override;

    void zoomIn();
    void zoomOut();
    void resetZoom();

    void reindexDocumentation();

signals:
    void linkActivated(const QUrl &link, const QStringList &searchTerms, bool newPage);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void search() const;

    void searchingStarted();
    void searchingFinished(int hits);

    void indexingStarted();
    void indexingFinished();

    bool eventFilter(QObject* o, QEvent *e) override;
    void contextMenuEvent(QContextMenuEvent *contextMenuEvent) override;
    QStringList currentSearchTerms() const;

    int zoomCount = 0;

    QFutureWatcher<void> m_watcher;
    QFutureInterface<void> *m_progress = nullptr;

    QHelpSearchEngine *searchEngine = nullptr;
    QHelpSearchResultWidget *resultWidget = nullptr;
    QHelpSearchQueryWidget *m_queryWidget = nullptr;
    QWidget *m_indexingDocumentationLabel = nullptr;
    Utils::ProgressIndicator *m_indexingIndicator = nullptr;
};

} // namespace Internal
} // namespace Help
