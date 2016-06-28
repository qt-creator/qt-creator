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

#include <coreplugin/opendocumentstreeview.h>

namespace Help {
namespace Internal {

class OpenPagesModel;

class OpenPagesWidget : public Core::OpenDocumentsTreeView
{
    Q_OBJECT

public:
    explicit OpenPagesWidget(OpenPagesModel *model, QWidget *parent = 0);
    ~OpenPagesWidget();

    void selectCurrentPage();
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
