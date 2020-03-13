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

#include "uvtargetdeviceselection.h"

#include <utils/basetreeview.h>
#include <utils/fileutils.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {
namespace Uv {

// DeviceSelectionModel

class DeviceSelectionItem;
class DeviceSelectionModel final : public Utils::TreeModel<DeviceSelectionItem>
{
    Q_OBJECT

public:
    explicit DeviceSelectionModel(QObject *parent = nullptr);
    void fillAllPacks(const Utils::FilePath &toolsIniFile);

private:
    void parsePackage(const QString &packageFile);
    void parsePackage(QXmlStreamReader &in, const QString &packageFile);
    void parseFamily(QXmlStreamReader &in, DeviceSelectionItem *parent);
    void parseSubFamily(QXmlStreamReader &in, DeviceSelectionItem *parent);
    void parseDevice(QXmlStreamReader &in, DeviceSelectionItem *parent);
    void parseDeviceVariant(QXmlStreamReader &in, DeviceSelectionItem *parent);

    Utils::FilePath m_toolsIniFile;
};

// DeviceSelectionView

class DeviceSelectionView final : public Utils::TreeView
{
    Q_OBJECT
public:
    explicit DeviceSelectionView(QWidget *parent = nullptr);

signals:
    void deviceSelected(const DeviceSelection &selection);

private:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous) final;

    DeviceSelection buildSelection(const DeviceSelectionItem *item) const;
};

} // namespace Uv
} // namespace Internal
} // namespace BareMetal
