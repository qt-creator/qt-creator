// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpviewer.h"

Q_FORWARD_DECLARE_OBJC_CLASS(DOMNode);
Q_FORWARD_DECLARE_OBJC_CLASS(DOMRange);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
Q_FORWARD_DECLARE_OBJC_CLASS(WebView);

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

    WebView *webView() const;
    void startToolTipTimer(const QPoint &pos, const QString &text);
    void hideToolTip();
    MacWebKitHelpViewer *viewer() const;

protected:
    void hideEvent(QHideEvent *) override;
    void showEvent(QShowEvent *) override;

private:
    void showToolTip();
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

    DOMRange *findText(NSString *text, bool forward, bool caseSensitive, DOMNode *startNode,
                       int startOffset);
    MacWebKitHelpWidget *m_widget;
};

}   // namespace Internal
}   // namespace Help
