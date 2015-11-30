/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef QTWEBKITHELPVIEWER_H
#define QTWEBKITHELPVIEWER_H

#include "helpviewer.h"

#ifndef QT_NO_WEBKIT

#include <QWebPage>
#include <QWebView>

namespace Help {
namespace Internal {

class HelpPage;
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
    void scrollToAnchor(const QString &anchor);
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

    HelpPage *page() const;

public slots:
    void scaleUp();
    void scaleDown();
    void resetScale();
    void copy();
    void stop();
    void forward();
    void backward();
    void print(QPrinter *printer);

private slots:
    void goToBackHistoryItem();
    void goToForwardHistoryItem();
    void goToHistoryItem(bool forward);

private:
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

public slots:
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

private slots:
    void actionChanged();
    void slotNetworkReplyFinished(QNetworkReply *reply);

private:
    bool eventFilter(QObject *obj, QEvent *event);

    QtWebKitHelpViewer *m_parent;
};

class HelpPage : public QWebPage
{
    Q_OBJECT
public:
    HelpPage(QObject *parent);

protected:
    virtual QWebPage *createWindow(QWebPage::WebWindowType);
    virtual void triggerAction(WebAction action, bool checked = false);

    virtual bool acceptNavigationRequest(QWebFrame *frame,
        const QNetworkRequest &request, NavigationType type);

private slots:
    void onHandleUnsupportedContent(QNetworkReply *reply);

private:
    QUrl m_loadingUrl;
    bool closeNewTabIfNeeded;

    friend class Help::Internal::QtWebKitHelpWidget;
    Qt::MouseButtons m_pressedButtons;
    Qt::KeyboardModifiers m_keyboardModifiers;
};

}   // namespace Internal
}   // namespace Help

#endif // !QT_NO_WEBKIT

#endif // QTWEBKITHELPVIEWER_H
