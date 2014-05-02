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

#include <coreplugin/find/textfindconstants.h>

#include <QFont>
#include <QMenu>
#include <QPrinter>
#include <QString>
#include <QUrl>
#include <QWidget>

namespace Help {
namespace Internal {

class HelpViewer : public QWidget
{
    Q_OBJECT

public:
    explicit HelpViewer(QWidget *parent = 0);
    ~HelpViewer() { }

    virtual QFont viewerFont() const = 0;
    virtual void setViewerFont(const QFont &font) = 0;

    virtual void scaleUp() = 0;
    virtual void scaleDown() = 0;
    virtual void resetScale() = 0;

    virtual qreal scale() const = 0;

    virtual QString title() const = 0;
    virtual void setTitle(const QString &title) = 0;

    virtual QUrl source() const = 0;
    virtual void setSource(const QUrl &url) = 0;
    virtual void scrollToAnchor(const QString &anchor) = 0;
    virtual void highlightId(const QString &id) { Q_UNUSED(id) }

    virtual void setHtml(const QString &html) = 0;

    virtual QString selectedText() const = 0;
    virtual bool isForwardAvailable() const = 0;
    virtual bool isBackwardAvailable() const = 0;
    virtual void addBackHistoryItems(QMenu *backMenu) = 0;
    virtual void addForwardHistoryItems(QMenu *forwardMenu) = 0;
    virtual void setOpenInNewWindowActionVisible(bool visible) = 0;

    virtual bool findText(const QString &text, Core::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = 0) = 0;

    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static QString mimeFromUrl(const QUrl &url);
    static bool launchWithExternalApp(const QUrl &url);

public slots:
    void home();

    virtual void copy() = 0;
    virtual void stop() = 0;
    virtual void forward() = 0;
    virtual void backward() = 0;
    virtual void print(QPrinter *printer) = 0;

signals:
    void sourceChanged(const QUrl &);
    void titleChanged();
    void printRequested();
    void forwardAvailable(bool);
    void backwardAvailable(bool);
    void loadFinished();

protected slots:
    void slotLoadStarted();
    void slotLoadFinished();
};

}   // namespace Internal
}   // namespace Help

#endif  // HELPVIEWER_H
