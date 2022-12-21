// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <coreplugin/opendocumentstreeview.h>

namespace Help {
namespace Internal {

class OpenPagesWidget : public Core::OpenDocumentsTreeView
{
    Q_OBJECT

public:
    explicit OpenPagesWidget(QAbstractItemModel *model, QWidget *parent = nullptr);
    ~OpenPagesWidget() override;

    void selectCurrentPage(int index);
    void allowContextMenu(bool ok);

signals:
    void setCurrentPage(const QModelIndex &index);

    void closePage(const QModelIndex &index);
    void closePagesExcept(const QModelIndex &index);

private:
    void contextMenuRequested(QPoint pos);
    void handleActivated(const QModelIndex &index);
    void handleCloseActivated(const QModelIndex &index);
    void updateCloseButtonVisibility();

    bool m_allowContextMenu;
};

    } // namespace Internal
} // namespace Help
