// Copyright (C) 2016 Alexander Drozdov.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <QComboBox>
#include <QStyledItemDelegate>

namespace CMakeProjectManager::Internal {

class ConfigModelItemDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ConfigModelItemDelegate(const Utils::FilePath &base, QObject *parent = nullptr);

    QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                          const QModelIndex &index) const final;
    void setEditorData(QWidget *editor, const QModelIndex &index) const final;
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &index) const final;
    QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const final;

private:
    Utils::FilePath m_base;
};

} // CMakeProjectManager::Internal
