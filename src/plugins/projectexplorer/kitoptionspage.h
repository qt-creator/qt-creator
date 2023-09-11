// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/treemodel.h>

namespace Utils { class Id; }

namespace ProjectExplorer {

class PROJECTEXPLORER_EXPORT KitSettingsSortModel : public Utils::SortModel
{
public:
    using SortModel::SortModel;

    void setSortedCategories(const QStringList &categories) { m_sortedCategories = categories; }

private:
    bool lessThan(const QModelIndex &source_left, const QModelIndex &source_right) const override;

    QStringList m_sortedCategories;
};

namespace Internal {

void setSelectectKitId(const Utils::Id &kitId);

} // Internal
} // ProjectExplorer
