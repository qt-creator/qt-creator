// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "helpviewer.h"

#include <QWebEngineUrlRequestInterceptor>
#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>

namespace Help {
namespace Internal {

class WebEngineHelpViewer;

class HelpUrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
public:
    explicit HelpUrlSchemeHandler(QObject *parent = nullptr);
    void requestStarted(QWebEngineUrlRequestJob *job) override;
};

class HelpUrlRequestInterceptor : public QWebEngineUrlRequestInterceptor
{
public:
    explicit HelpUrlRequestInterceptor(QObject *parent = nullptr);
    void interceptRequest(QWebEngineUrlRequestInfo &info) override;
};

class WebEngineHelpPage : public QWebEnginePage
{
public:
    explicit WebEngineHelpPage(QObject *parent = nullptr);

protected:
    bool acceptNavigationRequest(const QUrl &url,
                                 QWebEnginePage::NavigationType type,
                                 bool isMainFrame) override;
};

class WebView : public QWebEngineView
{
public:
    explicit WebView(WebEngineHelpViewer *viewer);

    bool event(QEvent *ev) override;
    bool eventFilter(QObject *src, QEvent *e) override;

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    WebEngineHelpViewer *m_viewer;
};

class WebEngineHelpViewer : public HelpViewer
{
    Q_OBJECT
public:
    explicit WebEngineHelpViewer(QWidget *parent = nullptr);

    void setViewerFont(const QFont &font) override;
    void setScale(qreal scale) override;
    QString title() const override;
    QUrl source() const override;
    void setSource(const QUrl &url) override;
    void setHtml(const QString &html) override;
    QString selectedText() const override;
    bool isForwardAvailable() const override;
    bool isBackwardAvailable() const override;
    void addBackHistoryItems(QMenu *backMenu) override;
    void addForwardHistoryItems(QMenu *forwardMenu) override;
    bool findText(const QString &text, Utils::FindFlags flags, bool incremental, bool fromSearch, bool *wrapped) override;

    WebEngineHelpPage *page() const;

    void copy() override;
    void stop() override;
    void forward() override;
    void backward() override;
    void print(QPrinter *printer) override;

private:
    WebView *m_widget;
    QUrl m_previousUrlWithoutFragment;
};

} // namespace Internal
} // namespace Help
