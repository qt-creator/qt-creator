// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "assetimportupdatetreeview.h"
#include "assetimportupdatetreemodel.h"
#include "assetimportupdatetreeitemdelegate.h"

#include <QHeaderView>

namespace QmlDesigner {
namespace Internal {

AssetImportUpdateTreeView::AssetImportUpdateTreeView(QWidget *parent)
    : Utils::TreeView(parent)
    , m_model(new AssetImportUpdateTreeModel(this))
{
    setModel(m_model);
    setItemDelegate(new AssetImportUpdateTreeItemDelegate(this));
    setExpandsOnDoubleClick(true);
    header()->hide();
}

void AssetImportUpdateTreeView::clear()
{
    m_model->clear();
}

AssetImportUpdateTreeModel *AssetImportUpdateTreeView::model() const
{
    return m_model;
}

} // namespace Internal
} // namespace QmlDesigner
