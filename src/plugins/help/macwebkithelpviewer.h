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

#include <QMacCocoaViewContainer>

Q_FORWARD_DECLARE_OBJC_CLASS(DOMNode);
Q_FORWARD_DECLARE_OBJC_CLASS(DOMRange);
Q_FORWARD_DECLARE_OBJC_CLASS(NSString);
Q_FORWARD_DECLARE_OBJC_CLASS(WebView);

namespace Help {
namespace Internal {

class MacWebKitHelpViewer;
class MacWebKitHelpWidgetPrivate;

class MacWebKitHelpWidget : public QMacCocoaViewContainer
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

    QFont viewerFont() const override;
    void setViewerFont(const QFont &font) override;

    qreal scale() const override;
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

    bool findText(const QString &text, Core::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = nullptr) override;

    MacWebKitHelpWidget *widget() const { return m_widget; }

public:
    void scaleUp() override;
    void scaleDown() override;
    void resetScale() override;
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
