/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://www.qt.io/licensing.  For further information
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
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTBROWSERHELPVIEWER_H
#define TEXTBROWSERHELPVIEWER_H

#include "centralwidget.h"
#include "helpviewer.h"
#include "openpagesmanager.h"

#include <QTextBrowser>

namespace Help {
namespace Internal {

class TextBrowserHelpWidget;

class TextBrowserHelpViewer : public HelpViewer
{
    Q_OBJECT

public:
    explicit TextBrowserHelpViewer(qreal zoom, QWidget *parent = 0);
    ~TextBrowserHelpViewer();

    QFont viewerFont() const;
    void setViewerFont(const QFont &font);

    qreal scale() const;

    QString title() const;

    QUrl source() const;
    void setSource(const QUrl &url);
    void scrollToAnchor(const QString &anchor);

    void setHtml(const QString &html);

    QString selectedText() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;
    void addBackHistoryItems(QMenu *backMenu);
    void addForwardHistoryItems(QMenu *forwardMenu);
    void setOpenInNewWindowActionVisible(bool visible);

    bool findText(const QString &text, Core::FindFlags flags,
                  bool incremental, bool fromSearch, bool *wrapped = 0);

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
    void goToHistoryItem();

private:
    QVariant loadResource(int type, const QUrl &name);

    TextBrowserHelpWidget *m_textBrowser;
};

class TextBrowserHelpWidget : public QTextBrowser
{
    Q_OBJECT

public:
    TextBrowserHelpWidget(int zoom, TextBrowserHelpViewer *parent);

    QVariant loadResource(int type, const QUrl &name);

    bool hasAnchorAt(const QPoint& pos);
    void openLink(bool newPage);
    void scaleUp();
    void scaleDown();

public slots:
    void openLink();
    void openLinkInNewPage();

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void wheelEvent(QWheelEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

public:
    int zoomCount;
    bool forceFont;
    QString lastAnchor;
    bool showOpenInNewWindowAction;
    TextBrowserHelpViewer *m_parent;
};

}   // namespace Internal
}   // namespace Help

#endif // TEXTBROWSERHELPVIEWER_H
