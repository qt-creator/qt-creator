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

#include "helpviewer.h"

#include <QWebEngineUrlSchemeHandler>
#include <QWebEngineView>

namespace Help {
namespace Internal {

class WebEngineHelpViewer;

class HelpUrlSchemeHandler : public QWebEngineUrlSchemeHandler
{
public:
    explicit HelpUrlSchemeHandler(QObject *parent = 0);
    void requestStarted(QWebEngineUrlRequestJob *job) override;
};

class WebEngineHelpPage : public QWebEnginePage
{
public:
    explicit WebEngineHelpPage(QObject *parent = 0);
#if QT_VERSION < QT_VERSION_CHECK(5, 7, 0)
    QWebEnginePage *createWindow(QWebEnginePage::WebWindowType) override;
#endif
};

class WebView : public QWebEngineView
{
public:
    explicit WebView(WebEngineHelpViewer *viewer);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;

private:
    WebEngineHelpViewer *m_viewer;
};

class WebEngineHelpViewer : public HelpViewer
{
    Q_OBJECT
public:
    explicit WebEngineHelpViewer(QWidget *parent = 0);

    QFont viewerFont() const override;
    void setViewerFont(const QFont &font) override;
    qreal scale() const override;
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
    bool findText(const QString &text, Core::FindFlags flags, bool incremental, bool fromSearch, bool *wrapped) override;

    WebEngineHelpPage *page() const;

    void scaleUp() override;
    void scaleDown() override;
    void resetScale() override;
    void copy() override;
    void stop() override;
    void forward() override;
    void backward() override;
    void print(QPrinter *printer) override;

private:
    WebView *m_widget;
};

} // namespace Internal
} // namespace Help
