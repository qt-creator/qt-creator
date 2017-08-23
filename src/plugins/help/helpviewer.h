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
    enum class Action {
        NewPage = 0x01,
        ExternalWindow = 0x02
    };
    Q_DECLARE_FLAGS(Actions, Action)

    explicit HelpViewer(QWidget *parent = 0);
    ~HelpViewer();

    virtual QFont viewerFont() const = 0;
    virtual void setViewerFont(const QFont &font) = 0;

    virtual qreal scale() const = 0;
    virtual void setScale(qreal scale) = 0;

    virtual QString title() const = 0;

    virtual QUrl source() const = 0;
    // metacall in HelpPlugin::updateSideBarSource
    Q_INVOKABLE virtual void setSource(const QUrl &url) = 0;
    virtual void highlightId(const QString &id) { Q_UNUSED(id) }

    virtual void setHtml(const QString &html) = 0;

    virtual QString selectedText() const = 0;
    virtual bool isForwardAvailable() const = 0;
    virtual bool isBackwardAvailable() const = 0;
    virtual void addBackHistoryItems(QMenu *backMenu) = 0;
    virtual void addForwardHistoryItems(QMenu *forwardMenu) = 0;
    void setActionVisible(Action action, bool visible);
    bool isActionVisible(Action action);

    virtual bool findText(const QString &text, Core::FindFlags flags,
        bool incremental, bool fromSearch, bool *wrapped = 0) = 0;

    bool handleForwardBackwardMouseButtons(QMouseEvent *e);

    static bool isLocalUrl(const QUrl &url);
    static bool canOpenPage(const QString &url);
    static QString mimeFromUrl(const QUrl &url);
    static bool launchWithExternalApp(const QUrl &url);

    void home();

    virtual void scaleUp() = 0;
    virtual void scaleDown() = 0;
    virtual void resetScale() = 0;
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
    void newPageRequested(const QUrl &url);
    void externalPageRequested(const QUrl &url);

protected:
    void slotLoadStarted();
    void slotLoadFinished();

    void restoreOverrideCursor();

    Actions m_visibleActions = 0;
    int m_loadOverrideStack = 0;
};

}   // namespace Internal
}   // namespace Help
