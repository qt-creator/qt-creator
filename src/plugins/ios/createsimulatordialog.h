// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/futuresynchronizer.h>

#include <QDialog>

QT_BEGIN_NAMESPACE
class QComboBox;
class QLineEdit;
QT_END_NAMESPACE

namespace Ios::Internal {

class DeviceTypeInfo;
class RuntimeInfo;

/*!
    A dialog to select the iOS Device type and the runtime for a new
    iOS simulator device.
 */
class CreateSimulatorDialog : public QDialog
{
public:
    explicit CreateSimulatorDialog(QWidget *parent = nullptr);
    ~CreateSimulatorDialog() override;

    QString name() const;
    RuntimeInfo runtime() const;
    DeviceTypeInfo deviceType() const;

private:
    void populateDeviceTypes(const QList<DeviceTypeInfo> &deviceTypes);
    void populateRuntimes(const DeviceTypeInfo &deviceType);

    Utils::FutureSynchronizer m_futureSync;
    QList<RuntimeInfo> m_runtimes;

    QLineEdit *m_nameEdit;
    QComboBox *m_deviceTypeCombo;
    QComboBox *m_runtimeCombo;
};

} // Ios::Internal
