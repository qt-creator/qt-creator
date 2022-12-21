// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "idevice.h"

#include <utils/theme/theme.h>

#include <QDialog>

#include <memory>

namespace ProjectExplorer {
namespace Internal {

class DeviceTestDialog : public QDialog
{
    Q_OBJECT

public:
    DeviceTestDialog(const IDevice::Ptr &deviceConfiguration, QWidget *parent = nullptr);
    ~DeviceTestDialog() override;

    void reject() override;

private:
    void handleProgressMessage(const QString &message);
    void handleErrorMessage(const QString &message);
    void handleTestFinished(ProjectExplorer::DeviceTester::TestResult result);

    void addText(const QString &text, Utils::Theme::Color color, bool bold);

    class DeviceTestDialogPrivate;
    const std::unique_ptr<DeviceTestDialogPrivate> d;
};

} // namespace Internal
} // namespace ProjectExplorer
