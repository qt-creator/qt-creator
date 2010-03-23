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

#include <QtCore/QUrl>
#include <QtCore/QVariant>
#include <QtGui/QTextBrowser>
#include <QtGui/QAction>

#if !defined(QT_NO_WEBKIT)
#include <QWebView>
#endif

QT_BEGIN_NAMESPACE


class QPoint;
class QString;
class QKeyEvent;
class QMouseEvent;
class QContextMenuEvent;

QT_END_NAMESPACE

namespace Help {
namespace Internal {
        class CentralWidget;
} // namespace Internal
} // namespace Help

#if !defined(QT_NO_WEBKIT)

class HelpViewer : public QWebView
{
    Q_OBJECT

public:
    HelpViewer(Help::Internal::CentralWidget *central, QWidget *parent);
    void setSource(const QUrl &url);

    inline QUrl source() const
    { return url(); }

    inline QString documentTitle() const
    { return title(); }

    inline bool hasSelection() const
    { return !selectedText().isEmpty(); } // ### this is suboptimal

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

    void resetZoom();
    int zoom() const;
    void setZoom(int zoom) { zoomIn(zoom); }

    inline void copy()
    { return triggerPageAction(QWebPage::Copy); }

    inline bool isForwardAvailable() const
    { return pageAction(QWebPage::Forward)->isEnabled(); }
    inline bool isBackwardAvailable() const
    { return pageAction(QWebPage::Back)->isEnabled(); }
    inline bool hasLoadFinished() const
    { return loadFinished; }

public Q_SLOTS:
    void home();
    void backward() { back(); }

Q_SIGNALS:
    void copyAvailable(bool enabled);
    void forwardAvailable(bool enabled);
    void backwardAvailable(bool enabled);
    void highlighted(const QString &);
    void sourceChanged(const QUrl &);

protected:
    void keyPressEvent(QKeyEvent *e);

    void wheelEvent(QWheelEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void mousePressEvent(QMouseEvent *event);

private Q_SLOTS:
    void actionChanged();
    void setLoadFinished(bool ok);

private:
    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

    QHelpEngine *helpEngine;
    Help::Internal::CentralWidget* parentWidget;
    QUrl homeUrl;
    bool multiTabsAllowed;
    bool loadFinished;
};

#else

class HelpViewer : public QTextBrowser
{
    Q_OBJECT

public:
    HelpViewer(Help::Internal::CentralWidget *central, QWidget *parent);
    void setSource(const QUrl &url);

    void zoomIn(int range = 1);
    void zoomOut(int range = 1);

    void resetZoom();
    int zoom() const { return zoomCount; }
    void setZoom(int zoom) { zoomCount = zoom; }

    inline bool hasSelection() const
    { return textCursor().hasSelection(); }

    bool launchedWithExternalApp(const QUrl &url);

public Q_SLOTS:
    void home();

protected:
    void wheelEvent(QWheelEvent *e);

private:
    QVariant loadResource(int type, const QUrl &name);
    void openLinkInNewTab(const QString &link);
    bool hasAnchorAt(const QPoint& pos);
    void contextMenuEvent(QContextMenuEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);

private slots:
    void openLinkInNewTab();

private:
    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

    int zoomCount;
    bool controlPressed;
    QString lastAnchor;
    Help::Internal::CentralWidget* parentWidget;
    QUrl homeUrl;
};

#endif

#endif
