// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
