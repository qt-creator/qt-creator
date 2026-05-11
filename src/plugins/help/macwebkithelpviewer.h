// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpviewer.h"

Q_FORWARD_DECLARE_OBJC_CLASS(WKWebView);

namespace Help {
namespace Internal {

class MacWebKitHelpViewer;
class MacWebKitHelpWidgetPrivate;

class MacWebKitHelpWidget : public QWidget
{
    Q_OBJECT

public:
    MacWebKitHelpWidget(MacWebKitHelpViewer *parent);
    ~MacWebKitHelpWidget() override;

    WKWebView *webView() const;
    void startToolTipTimer(const QPoint &pos, const QString &text);
    void hideToolTip();
    void setContextMenuOpen(bool open);
    MacWebKitHelpViewer *viewer() const;

protected:
    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;
    void resizeEvent(QResizeEvent *) override;
    bool event(QEvent *) override;

private:
    void showToolTip();
    void updateWebViewFrame();
    MacWebKitHelpWidgetPrivate *d;
};

class MacWebKitHelpViewer : public HelpViewer
{
    Q_OBJECT

public:
    explicit MacWebKitHelpViewer(QWidget *parent = nullptr);
    ~MacWebKitHelpViewer() override;

    void setViewerFont(const QFont &font) override;

    void setScale(qreal scale) override;

    QString title() const override;

    QUrl source() const override;
    void setSource(const QUrl &url) override;
    void scrollToAnchor(const QString &anchor);

    void setHtml(const QString &html) override;

    QString selectedText() const override;
    bool isForwardAvailable() const override;
    bool isBackwardAvailable() const override;
    void addBackHistoryItems(QMenu *backMenu) override;
    void addForwardHistoryItems(QMenu *forwardMenu) override;
    void setActionVisible(bool visible);

    bool findText(const QString &text, Utils::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = nullptr) override;

    MacWebKitHelpWidget *widget() const { return m_widget; }

public:
    void copy() override;
    void stop() override;
    void forward() override;
    void backward() override;
    void print(QPrinter *printer) override;

    void slotLoadStarted();
    void slotLoadFinished();

private:
    void goToHistoryItem();
    void applyFont();

    MacWebKitHelpWidget *m_widget;
    QFont m_viewerFont;
    bool m_fontSet = false;
    QUrl m_pendingUrl;
};

}   // namespace Internal
}   // namespace Help
