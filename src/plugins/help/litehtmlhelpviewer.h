// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpviewer.h"

#include <qlitehtmlwidget.h>

#include <QTextBrowser>
#include <QUrl>

#include <optional>

namespace Help {
namespace Internal {

class LiteHtmlHelpViewer : public HelpViewer
{
    Q_OBJECT

public:
    explicit LiteHtmlHelpViewer(QWidget *parent = nullptr);
    ~LiteHtmlHelpViewer() override;

    void setViewerFont(const QFont &font) override;
    void setScale(qreal scale) override;
    void setAntialias(bool on) final;

    QString title() const override;

    QUrl source() const override;
    void setSource(const QUrl &url) override;

    void setHtml(const QString &html) override;

    QString selectedText() const override;
    bool isForwardAvailable() const override;
    bool isBackwardAvailable() const override;
    void addBackHistoryItems(QMenu *backMenu) override;
    void addForwardHistoryItems(QMenu *forwardMenu) override;

    bool findText(const QString &text, Utils::FindFlags flags,
                  bool incremental, bool fromSearch, bool *wrapped = nullptr) override;

    void copy() override;
    void stop() override;
    void forward() override;
    void backward() override;
    void print(QPrinter *printer) override;

    bool eventFilter(QObject *src, QEvent *e) override;

private:
    void goForward(int count);
    void goBackward(int count);
    void setSourceInternal(const QUrl &url, std::optional<int> vscroll = std::nullopt);
    void showContextMenu(const QPoint &pos, const QUrl &url);

    struct HistoryItem
    {
        QUrl url;
        QString title;
        int vscroll;
    };
    HistoryItem currentHistoryItem() const;

    QLiteHtmlWidget *m_viewer;
    std::vector<HistoryItem> m_backItems;
    std::vector<HistoryItem> m_forwardItems;
    QUrl m_highlightedLink;
};

}   // namespace Internal
}   // namespace Help
