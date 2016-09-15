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

#include "scxmltypes.h"

#include <QTreeView>

QT_FORWARD_DECLARE_CLASS(QEvent)
QT_FORWARD_DECLARE_CLASS(QMouseEvent)

namespace ScxmlEditor {

namespace Common {

/**
 * @brief The TreeView class provides a default model/view implementation of a tree view.
 *
 * It extend the normal QTreeView by adding the signals mouseExited, currentIndexChanged and rightButtonClicked.
 */
class TreeView : public QTreeView
{
    Q_OBJECT

public:
    TreeView(QWidget *parent = nullptr);

signals:
    void mouseExited();
    void currentIndexChanged(const QModelIndex &index);
    void rightButtonClicked(const QModelIndex &index, const QPoint &globalPos);

protected:
    void leaveEvent(QEvent *e) override;
    void currentChanged(const QModelIndex &index, const QModelIndex &prev) override;
    void mousePressEvent(QMouseEvent *event) override;
};

} // namespace Common
} // namespace ScxmlEditor
