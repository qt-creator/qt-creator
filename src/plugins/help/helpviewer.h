/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <find/ifindsupport.h>

#include <qglobal.h>
#include <QString>
#include <QUrl>
#include <QVariant>

#include <QAction>
#include <QFont>

#if defined(QT_NO_WEBKIT)
#include <QTextBrowser>
#else
#include <QWebPage>
#include <QWebView>
#endif

namespace Help {
namespace Internal {

#if !defined(QT_NO_WEBKIT)
class HelpViewer : public QWebView
#else
class HelpViewer : public QTextBrowser
#endif
{
    Q_OBJECT
    class HelpViewerPrivate;

public:
    explicit HelpViewer(qreal zoom, QWidget *parent = 0);
    ~HelpViewer();

    QFont viewerFont() const;
    void setViewerFont(const QFont &font);

    void scaleUp();
    void scaleDown();

    void resetScale();
    qreal scale() const;

    QString title() const;
    void setTitle(const QString &title);

    QUrl source() const;
    void setSource(const QUrl &url);

    QString selectedText() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;

    bool findText(const QString &text, Find::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = 0);

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static QString mimeFromUrl(const QUrl &url);
    static bool launchWithExternalApp(const QUrl &url);

public slots:
    void copy();
    void home();
    void stop();

    void forward();
    void backward();

signals:
    void titleChanged();
    void printRequested();
    void openFindToolBar();

#if !defined(QT_NO_WEBKIT)
    void sourceChanged(const QUrl &);
    void forwardAvailable(bool enabled);
    void backwardAvailable(bool enabled);
#else
    void loadFinished(bool finished);
#endif

protected:
    void keyPressEvent(QKeyEvent *e);
    void wheelEvent(QWheelEvent *event);
    void mousePressEvent(QMouseEvent *event);
    void mouseReleaseEvent(QMouseEvent *event);

private slots:
    void actionChanged();
    void slotLoadStarted();
    void slotLoadFinished(bool ok);
#if !defined(QT_NO_WEBKIT)
    void slotNetworkReplyFinished(QNetworkReply *reply);
#endif

private:
    bool eventFilter(QObject *obj, QEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    QVariant loadResource(int type, const QUrl &name);
    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

private:
    HelpViewerPrivate *d;
};

#ifndef QT_NO_WEBKIT
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

    friend class Help::Internal::HelpViewer;
    Qt::MouseButtons m_pressedButtons;
    Qt::KeyboardModifiers m_keyboardModifiers;
};
#endif // QT_NO_WEBKIT

}   // namespace Internal
}   // namespace Help

#endif  // HELPVIEWER_H
