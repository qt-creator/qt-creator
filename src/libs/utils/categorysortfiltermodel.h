// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "utils_global.h"

#include <QSortFilterProxyModel>

namespace Utils {

class QTCREATOR_UTILS_EXPORT CategorySortFilterModel : public QSortFilterProxyModel
{
public:
    CategorySortFilterModel(QObject *parent = nullptr);

    // "New" items will always be accepted, regardless of the filter.
    void setNewItemRole(int role);

protected:
    bool filterAcceptsRow(int source_row, const QModelIndex &source_parent) const override;

private:
    int m_newItemRole = -1;
};

} // Utils
