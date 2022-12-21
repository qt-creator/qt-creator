// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvtargetdriverselection.h"

#include <utils/basetreeview.h>
#include <utils/fileutils.h>
#include <utils/treemodel.h>

namespace BareMetal::Internal::Uv {

// DriverSelectionModel

class DriverSelectionItem;
class DriverSelectionModel final : public Utils::TreeModel<DriverSelectionItem>
{
    Q_OBJECT

public:
    explicit DriverSelectionModel(QObject *parent = nullptr);
    void fillDrivers(const Utils::FilePath &toolsIniFile, const QStringList &supportedDrivers);
};

// DriverSelectionView

class DriverSelectionView final : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit DriverSelectionView(QWidget *parent = nullptr);
    void setCurrentSelection(const QString &driverDll);

signals:
    void driverSelected(const DriverSelection &selection);

private:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) final;
};

} // BareMetal::Internal::Uv
