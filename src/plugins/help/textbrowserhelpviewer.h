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
    explicit TextBrowserHelpViewer(QWidget *parent = 0);
    ~TextBrowserHelpViewer();

    QFont viewerFont() const;
    void setViewerFont(const QFont &font);

    qreal scale() const;
    void setScale(qreal scale);

    QString title() const;

    QUrl source() const;
    void setSource(const QUrl &url);

    void setHtml(const QString &html);

    QString selectedText() const;
    bool isForwardAvailable() const;
    bool isBackwardAvailable() const;
    void addBackHistoryItems(QMenu *backMenu);
    void addForwardHistoryItems(QMenu *forwardMenu);

    bool findText(const QString &text, Core::FindFlags flags,
                  bool incremental, bool fromSearch, bool *wrapped = 0);

    void scaleUp();
    void scaleDown();
    void resetScale();
    void copy();
    void stop();
    void forward();
    void backward();
    void print(QPrinter *printer);

private:
    void goToHistoryItem();

    TextBrowserHelpWidget *m_textBrowser;
};

class TextBrowserHelpWidget : public QTextBrowser
{
    Q_OBJECT

public:
    TextBrowserHelpWidget(TextBrowserHelpViewer *parent);

    QVariant loadResource(int type, const QUrl &name);

    void scaleUp();
    void scaleDown();

    void setSource(const QUrl &name);

protected:
    void contextMenuEvent(QContextMenuEvent *event);
    bool eventFilter(QObject *obj, QEvent *event);
    void wheelEvent(QWheelEvent *e);
    void mousePressEvent(QMouseEvent *e);
    void mouseReleaseEvent(QMouseEvent *e);

private:
    QString linkAt(const QPoint& pos);

    int zoomCount;
    bool forceFont;
    TextBrowserHelpViewer *m_parent;
    friend class Help::Internal::TextBrowserHelpViewer;
};

}   // namespace Internal
}   // namespace Help
