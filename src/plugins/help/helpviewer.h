/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2010 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef HELPVIEWER_H
#define HELPVIEWER_H

#include <find/ifindsupport.h>

#include <QtCore/qglobal.h>
#include <QtCore/QString>
#include <QtCore/QUrl>
#include <QtCore/QVariant>

#include <QtGui/QAction>
#include <QtGui/QFont>

#if defined(QT_NO_WEBKIT)
#include <QtGui/QTextBrowser>
#else
#include <QtWebKit/QWebView>
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
    Q_DISABLE_COPY(HelpViewer)

public:
    HelpViewer(qreal zoom, QWidget *parent = 0);
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

    bool findText(const QString &text, Find::IFindSupport::FindFlags flags,
        bool incremental, bool fromSearch);

    static QString AboutBlankPage;
    static QString PageNotFoundMessage;

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static bool launchWithExternalApp(const QUrl &url);

public slots:
    void copy();
    void home();

    void forward();
    void backward();

signals:
    void titleChanged();
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
    void setLoadFinished(bool ok);

private:
    bool eventFilter(QObject *obj, QEvent *event);
    void contextMenuEvent(QContextMenuEvent *event);
    QVariant loadResource(int type, const QUrl &name);
    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

private:
    HelpViewerPrivate *d;
};

    }   // namespace Help
}   // namespace Internal

#endif  // HELPVIEWER_H
