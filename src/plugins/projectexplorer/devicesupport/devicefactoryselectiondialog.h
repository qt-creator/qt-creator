// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/id.h>

#include <QDialog>

namespace ProjectExplorer {
class IDeviceFactory;

namespace Internal {
namespace Ui { class DeviceFactorySelectionDialog; }

class DeviceFactorySelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceFactorySelectionDialog(QWidget *parent = nullptr);
    ~DeviceFactorySelectionDialog() override;
    Utils::Id selectedId() const;

private:
    void handleItemSelectionChanged();
    void handleItemDoubleClicked();
    Ui::DeviceFactorySelectionDialog *ui;
};

} // namespace Internal
} // namespace ProjectExplorer
