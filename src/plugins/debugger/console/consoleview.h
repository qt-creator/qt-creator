// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/fileinprojectfinder.h>
#include <utils/itemviews.h>

namespace Debugger::Internal {

class ConsoleItemModel;

class ConsoleView : public Utils::TreeView
{
public:
    ConsoleView(ConsoleItemModel *model, QWidget *parent);

    void onScrollToBottom();
    void populateFileFinder();

protected:
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *e) override;
    void drawBranches(QPainter *painter, const QRect &rect,
                      const QModelIndex &index) const override;
    void contextMenuEvent(QContextMenuEvent *event) override;
    void focusInEvent(QFocusEvent *event) override;

private:
    void onRowActivated(const QModelIndex &index);
    void copyToClipboard(const QModelIndex &index);
    bool canShowItemInTextEditor(const QModelIndex &index);

    ConsoleItemModel *m_model;
    Utils::FileInProjectFinder m_finder;
};

} // Debugger::Internal
