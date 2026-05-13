// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QStyledItemDelegate>

namespace Autotest::Internal {

class TestTreeItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT
public:
    explicit TestTreeItemDelegate(QObject *parent = nullptr);

public:
    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const override;
};

} // namespace Autotest::Internal
