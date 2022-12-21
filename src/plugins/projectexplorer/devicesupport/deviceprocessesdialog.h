// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../projectexplorer_export.h"

#include <projectexplorer/devicesupport/idevicefwd.h>

#include <QDialog>

#include <memory>

namespace Utils { class ProcessInfo; }

namespace ProjectExplorer {

class KitChooser;

namespace Internal { class DeviceProcessesDialogPrivate; }

class PROJECTEXPLORER_EXPORT DeviceProcessesDialog : public QDialog
{
    Q_OBJECT

public:
    explicit DeviceProcessesDialog(QWidget *parent = nullptr);
    ~DeviceProcessesDialog() override;
    void addAcceptButton(const QString &label);
    void addCloseButton();

    void setDevice(const IDeviceConstPtr &device);
    void showAllDevices();
    Utils::ProcessInfo currentProcess() const;
    KitChooser *kitChooser() const;
    void logMessage(const QString &line);
    DeviceProcessesDialog(KitChooser *chooser, QWidget *parent);

private:
    void setKitVisible(bool);

    const std::unique_ptr<Internal::DeviceProcessesDialogPrivate> d;
};

} // namespace ProjectExplorer
