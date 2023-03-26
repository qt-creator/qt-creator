// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "assetimportupdatetreeitem.h"

#include <utils/itemviews.h>

namespace QmlDesigner {
namespace Internal {

class AssetImportUpdateTreeModel;

class AssetImportUpdateTreeView : public Utils::TreeView
{
    Q_OBJECT

public:
    explicit AssetImportUpdateTreeView(QWidget *parent = nullptr);

    AssetImportUpdateTreeModel *model() const;

public slots:
    void clear();

protected:
    AssetImportUpdateTreeModel *m_model;
};

} // namespace Internal
} // namespace QmlDesigner
