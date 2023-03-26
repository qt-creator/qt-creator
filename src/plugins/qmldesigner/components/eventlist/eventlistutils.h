// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
#pragma once

#include "abstractview.h"
#include <string>

QT_FORWARD_DECLARE_CLASS(QAbstractItemModel)
QT_FORWARD_DECLARE_CLASS(QTableView)

namespace QmlDesigner {

class TabWalker : public QObject
{
    Q_OBJECT

public:
    TabWalker(QObject *parent = nullptr);
    bool eventFilter(QObject *obj, QEvent *event) override;
};

QString uniqueName(QAbstractItemModel *model, const QString &base);

std::string toString(AbstractView::PropertyChangeFlags flags);

void polishPalette(QTableView *view, const QColor &selectionColor);

void printPropertyType(const ModelNode &node, const PropertyName &name);

} // namespace QmlDesigner.
