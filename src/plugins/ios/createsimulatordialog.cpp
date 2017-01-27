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

#include "createsimulatordialog.h"
#include "ui_createsimulatordialog.h"
#include "simulatorcontrol.h"

#include <utils/algorithm.h>
#include <utils/runextensions.h>

#include <QPushButton>
#include <QVariant>

namespace Ios {
namespace Internal {

using namespace std::placeholders;

CreateSimulatorDialog::CreateSimulatorDialog(QWidget *parent) :
    QDialog(parent),
    m_ui(new Ui::CreateSimulatorDialog),
    m_simControl(new SimulatorControl(this))
{
    m_ui->setupUi(this);
    m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    const auto enableOk = [this]() {
        m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                    !m_ui->nameEdit->text().isEmpty() &&
                    m_ui->deviceTypeCombo->currentIndex() > 0 &&
                    m_ui->runtimeCombo->currentIndex() > 0);
    };

    const auto indexChanged = static_cast<void(QComboBox::*)(int)>(&QComboBox::currentIndexChanged);
    connect(m_ui->nameEdit, &QLineEdit::textChanged, enableOk);
    connect(m_ui->runtimeCombo, indexChanged, enableOk);
    connect(m_ui->deviceTypeCombo, indexChanged, [this, enableOk]() {
        populateRuntimes(m_ui->deviceTypeCombo->currentData().value<DeviceTypeInfo>());
        enableOk();
    });

    m_futureSync.setCancelOnWait(true);
    m_futureSync.addFuture(Utils::onResultReady(SimulatorControl::updateDeviceTypes(), this,
                                                &CreateSimulatorDialog::populateDeviceTypes));

    QFuture<QList<RuntimeInfo>> runtimesfuture = SimulatorControl::updateRuntimes();
    Utils::onResultReady(runtimesfuture, this, [this](const QList<RuntimeInfo> &runtimes) {
        m_runtimes = runtimes;
    });
    m_futureSync.addFuture(runtimesfuture);
    populateRuntimes(DeviceTypeInfo());
}

CreateSimulatorDialog::~CreateSimulatorDialog()
{
    m_futureSync.waitForFinished();
    delete m_ui;
}

/*!
    Returns the the simulator name entered by user.
 */
QString CreateSimulatorDialog::name() const
{
    return m_ui->nameEdit->text();
}

/*!
    Returns the the simulator runtime (OS version) selected by user.
    Though the runtimes are filtered by the selected device type but the runtime camppatibility is
    not checked. i.e. User can select the Runtime iOS 10.2 for iPhone 4 but the combination is not
    possible as iOS 10.2 is not compatible with iPhone 4. In this case the command to create
    simulator shall fail with an error message describing the compatibility.
 */
RuntimeInfo CreateSimulatorDialog::runtime() const
{
    return m_ui->runtimeCombo->currentData().value<RuntimeInfo>();
}

/*!
    Returns the the selected device type.
 */
DeviceTypeInfo CreateSimulatorDialog::deviceType() const
{
    return m_ui->deviceTypeCombo->currentData().value<DeviceTypeInfo>();
}

/*!
    Populates the devices types. Similar device types are grouped together.
 */
void CreateSimulatorDialog::populateDeviceTypes(const QList<DeviceTypeInfo> &deviceTypes)
{
    m_ui->deviceTypeCombo->clear();
    m_ui->deviceTypeCombo->addItem(tr("None"));

    if (deviceTypes.isEmpty())
        return;

    m_ui->deviceTypeCombo->insertSeparator(1);

    auto addItems = [this, deviceTypes](const QString &filter) {
        auto filteredTypes = Utils::filtered(deviceTypes, [filter](const DeviceTypeInfo &type){
            return type.name.contains(filter, Qt::CaseInsensitive);
        });
        foreach (auto type, filteredTypes) {
            m_ui->deviceTypeCombo->addItem(type.name, QVariant::fromValue<DeviceTypeInfo>(type));
        }
        return filteredTypes.count();
    };

    if (addItems(QStringLiteral("iPhone")) > 0)
        m_ui->deviceTypeCombo->insertSeparator(m_ui->deviceTypeCombo->count());
    if (addItems(QStringLiteral("iPad")) > 0)
        m_ui->deviceTypeCombo->insertSeparator(m_ui->deviceTypeCombo->count());
    if (addItems(QStringLiteral("TV")) > 0)
        m_ui->deviceTypeCombo->insertSeparator(m_ui->deviceTypeCombo->count());
    addItems(QStringLiteral("Watch"));
}

/*!
    Populates the available runtimes. Though the runtimes are filtered by the selected device type
    but the runtime camppatibility is not checked. i.e. User can select the Runtime iOS 10.2 for
    iPhone 4 but the combination is not possible as iOS 10.2 is not compatible with iPhone 4. In
    this case the command to create simulator shall fail with an error message describing the
    compatibility issue.
 */
void CreateSimulatorDialog::populateRuntimes(const DeviceTypeInfo &deviceType)
{
    m_ui->runtimeCombo->clear();
    m_ui->runtimeCombo->addItem(tr("None"));

    if (deviceType.name.isEmpty())
        return;

    m_ui->runtimeCombo->insertSeparator(1);

    auto addItems = [this](const QString &filter) {
        auto filteredTypes = Utils::filtered(m_runtimes, [filter](const RuntimeInfo &runtime){
            return runtime.name.contains(filter, Qt::CaseInsensitive);
        });
        foreach (auto runtime, filteredTypes) {
            m_ui->runtimeCombo->addItem(runtime.name, QVariant::fromValue<RuntimeInfo>(runtime));
        }
    };

    if (deviceType.name.contains(QStringLiteral("iPhone")))
        addItems(QStringLiteral("iOS"));
    else if (deviceType.name.contains(QStringLiteral("iPad")))
        addItems(QStringLiteral("iOS"));
    else if (deviceType.name.contains(QStringLiteral("TV")))
        addItems(QStringLiteral("tvOS"));
    else if (deviceType.name.contains(QStringLiteral("Watch")))
        addItems(QStringLiteral("watchOS"));
}

} // namespace Internal
} // namespace Ios
