/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include <QDialog>
#include <QFutureSynchronizer>

namespace Ios {
namespace Internal {

namespace Ui { class CreateSimulatorDialog; }
class SimulatorControl;
class RuntimeInfo;
class DeviceTypeInfo;

/*!
    A dialog to select the iOS Device type and the runtime for a new
    iOS simulator device.
 */
class CreateSimulatorDialog : public QDialog
{
    Q_OBJECT

public:
    explicit CreateSimulatorDialog(QWidget *parent = nullptr);
    ~CreateSimulatorDialog();

    QString name() const;
    RuntimeInfo runtime() const;
    DeviceTypeInfo deviceType() const;

private:
    void populateDeviceTypes(const QList<DeviceTypeInfo> &deviceTypes);
    void populateRuntimes(const DeviceTypeInfo &deviceType);

private:
    QFutureSynchronizer<void> m_futureSync;
    Ui::CreateSimulatorDialog *m_ui = nullptr;
    SimulatorControl *m_simControl = nullptr;
    QList<RuntimeInfo> m_runtimes;
};

} // namespace Internal
} // namespace Ios
