/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.2, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

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

class QHelpEngine;
class CentralWidget;

class QPoint;
class QString;
class QKeyEvent;
class QMouseEvent;
class QContextMenuEvent;

#if !defined(QT_NO_WEBKIT)

class HelpViewer : public QWebView
{
    Q_OBJECT

public:
    HelpViewer(QHelpEngine *helpEngine, CentralWidget *parent);
    void setSource(const QUrl &url);

    inline QUrl source() const
    { return url(); }

    inline QString documentTitle() const
    { return title(); }

    inline bool hasSelection() const
    { return !selectedText().isEmpty(); } // ### this is suboptimal

    void resetZoom();
    void zoomIn(qreal range = 1);
    void zoomOut(qreal range = 1);

    inline void copy()
    { return triggerPageAction(QWebPage::Copy); }

    inline bool isForwardAvailable() const
    { return pageAction(QWebPage::Forward)->isEnabled(); }
    inline bool isBackwardAvailable() const
    { return pageAction(QWebPage::Back)->isEnabled(); }

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
    virtual void wheelEvent(QWheelEvent *);
    void mouseReleaseEvent(QMouseEvent *e);

private Q_SLOTS:
    void actionChanged();

private:
    QHelpEngine *helpEngine;
    CentralWidget* parentWidget;
    QUrl homeUrl;
};

#else

class HelpViewer : public QTextBrowser
{
    Q_OBJECT

public:
    HelpViewer(QHelpEngine *helpEngine, CentralWidget *parent);
    void setSource(const QUrl &url);

    void resetZoom();
    void zoomIn(int range = 1);
    void zoomOut(int range = 1);
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
    void mouseReleaseEvent(QMouseEvent *e);
    void keyPressEvent(QKeyEvent *e);

private slots:
    void openLinkInNewTab();

private:
    int zoomCount;
    bool controlPressed;
    QString lastAnchor;
    QHelpEngine *helpEngine;
    CentralWidget* parentWidget;
    QUrl homeUrl;
};

#endif

QT_END_NAMESPACE

#endif
