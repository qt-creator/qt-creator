// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvtargetdeviceselection.h"

#include <utils/basetreeview.h>
#include <utils/filepath.h>
#include <utils/treemodel.h>

QT_BEGIN_NAMESPACE
class QXmlStreamReader;
QT_END_NAMESPACE

namespace BareMetal::Internal::Uv {

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

} // BareMetal::Internal::Uv
