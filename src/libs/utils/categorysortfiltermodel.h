// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QSortFilterProxyModel>

namespace Utils {

class QTCREATOR_UTILS_EXPORT CategorySortFilterModel : public QSortFilterProxyModel
{
public:
    CategorySortFilterModel(QObject *parent = nullptr);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;
};

} // Utils
