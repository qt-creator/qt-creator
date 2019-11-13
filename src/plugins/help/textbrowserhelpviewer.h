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
#include "openpagesmanager.h"

#include <QTextBrowser>

namespace Help {
namespace Internal {

class TextBrowserHelpWidget;

class TextBrowserHelpViewer : public HelpViewer
{
    Q_OBJECT

public:
    explicit TextBrowserHelpViewer(QWidget *parent = nullptr);
    ~TextBrowserHelpViewer() override;

    QFont viewerFont() const override;
    void setViewerFont(const QFont &font) override;

    qreal scale() const override;
    void setScale(qreal scale) override;

    QString title() const override;

    QUrl source() const override;
    void setSource(const QUrl &url) override;

    void setHtml(const QString &html) override;

    QString selectedText() const override;
    bool isForwardAvailable() const override;
    bool isBackwardAvailable() const override;
    void addBackHistoryItems(QMenu *backMenu) override;
    void addForwardHistoryItems(QMenu *forwardMenu) override;

    bool findText(const QString &text, Core::FindFlags flags,
                  bool incremental, bool fromSearch, bool *wrapped = nullptr) override;

    void scaleUp() override;
    void scaleDown() override;
    void resetScale() override;
    void copy() override;
    void stop() override;
    void forward() override;
    void backward() override;
    void print(QPrinter *printer) override;

private:
    void goToHistoryItem();

    TextBrowserHelpWidget *m_textBrowser;
};

class TextBrowserHelpWidget : public QTextBrowser
{
    Q_OBJECT

public:
    TextBrowserHelpWidget(TextBrowserHelpViewer *parent);

    QVariant loadResource(int type, const QUrl &name) override;

    void scaleUp();
    void scaleDown();

    void withFixedTopPosition(const std::function<void()> &action);

protected:
    void contextMenuEvent(QContextMenuEvent *event) override;
    bool eventFilter(QObject *obj, QEvent *event) override;
    void mousePressEvent(QMouseEvent *e) override;
    void mouseReleaseEvent(QMouseEvent *e) override;
    void resizeEvent(QResizeEvent *e) override;

private:
    QString linkAt(const QPoint &pos);
    void scrollToTextPosition(int position);

    int zoomCount;
    bool forceFont;
    TextBrowserHelpViewer *m_parent;
    friend class Help::Internal::TextBrowserHelpViewer;
};

}   // namespace Internal
}   // namespace Help
