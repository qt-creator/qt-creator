/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <utils/optional.h>

#include <qlitehtmlwidget.h>

#include <QTextBrowser>

namespace Help {
namespace Internal {

class LiteHtmlHelpViewer : public HelpViewer
{
    Q_OBJECT

public:
    explicit LiteHtmlHelpViewer(QWidget *parent = nullptr);
    ~LiteHtmlHelpViewer() override;

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

    bool eventFilter(QObject *src, QEvent *e) override;

private:
    void goForward(int count);
    void goBackward(int count);
    void setSourceInternal(const QUrl &url, Utils::optional<int> vscroll = Utils::nullopt);
    void showContextMenu(const QPoint &pos, const QUrl &url);

    struct HistoryItem
    {
        QUrl url;
        QString title;
        int vscroll;
    };
    HistoryItem currentHistoryItem() const;

    QLiteHtmlWidget *m_viewer;
    std::vector<HistoryItem> m_backItems;
    std::vector<HistoryItem> m_forwardItems;
};

}   // namespace Internal
}   // namespace Help
