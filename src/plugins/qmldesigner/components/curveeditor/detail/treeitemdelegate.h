// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "curveeditorstyle.h"

#include <QStyledItemDelegate>

namespace QmlDesigner {

class TreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    TreeItemDelegate(const CurveEditorStyle &style, QObject *parent = nullptr);

    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override;

    void paint(
        QPainter *painter,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) const override;

    void setStyle(const CurveEditorStyle &style);

protected:
    bool editorEvent(
        QEvent *event,
        QAbstractItemModel *model,
        const QStyleOptionViewItem &option,
        const QModelIndex &index) override;

private:
    CurveEditorStyle m_style;

    QPoint m_mousePos;
};

} // End namespace QmlDesigner.
