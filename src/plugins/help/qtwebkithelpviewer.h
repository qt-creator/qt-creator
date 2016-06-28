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

#ifndef QT_NO_WEBKIT

#include <QWebPage>
#include <QWebView>

namespace Help {
namespace Internal {

class QtWebKitHelpPage;
class QtWebKitHelpWidget;

class QtWebKitHelpViewer : public HelpViewer
{
    Q_OBJECT
public:
    explicit QtWebKitHelpViewer(QWidget *parent = 0);
    ~QtWebKitHelpViewer() { }

    QFont viewerFont() const;
    void setViewerFont(const QFont &font);

    qreal scale() const;
    void setScale(qreal scale);

    QString title() const;

    QUrl source() const;
    void setSource(const QUrl &url);
    void highlightId(const QString &id);

    void setHtml(const QString &html);

    QString selectedText() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;
    void addBackHistoryItems(QMenu *backMenu);
    void addForwardHistoryItems(QMenu *forwardMenu);
    void setOpenInNewPageActionVisible(bool visible);

    bool findText(const QString &text, Core::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = 0);

    QtWebKitHelpPage *page() const;

    void scaleUp();
    void scaleDown();
    void resetScale();
    void copy();
    void stop();
    void forward();
    void backward();
    void print(QPrinter *printer);

private:
    void goToBackHistoryItem();
    void goToForwardHistoryItem();
    void goToHistoryItem(bool forward);

    QString m_oldHighlightId;
    QString m_oldHighlightStyle;
    QtWebKitHelpWidget *m_webView;
};

class QtWebKitHelpWidget : public QWebView
{
    Q_OBJECT

public:
    explicit QtWebKitHelpWidget(QtWebKitHelpViewer *parent = 0);
    ~QtWebKitHelpWidget();

    void scaleUp();
    void scaleDown();
    void copy();

signals:
    void forwardAvailable(bool enabled);
    void backwardAvailable(bool enabled);

protected:
    void keyPressEvent(QKeyEvent *e);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);

private:
    void actionChanged();
    void slotNetworkReplyFinished(QNetworkReply *reply);
    bool eventFilter(QObject *obj, QEvent *event);

    QtWebKitHelpViewer *m_parent;
};

class QtWebKitHelpPage : public QWebPage
{
    Q_OBJECT
public:
    QtWebKitHelpPage(QObject *parent);

protected:
    virtual QWebPage *createWindow(QWebPage::WebWindowType);
    virtual void triggerAction(WebAction action, bool checked = false);

    virtual bool acceptNavigationRequest(QWebFrame *frame,
        const QNetworkRequest &request, NavigationType type);

private:
    void onHandleUnsupportedContent(QNetworkReply *reply);

    QUrl m_loadingUrl;
    bool closeNewTabIfNeeded;

    friend class Help::Internal::QtWebKitHelpWidget;
    Qt::MouseButtons m_pressedButtons;
    Qt::KeyboardModifiers m_keyboardModifiers;
};

}   // namespace Internal
}   // namespace Help

#endif // !QT_NO_WEBKIT
