/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "uvtargetdriverselection.h"

#include <utils/basetreeview.h>
#include <utils/fileutils.h>
#include <utils/treemodel.h>

namespace BareMetal {
namespace Internal {
namespace Uv {

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

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
