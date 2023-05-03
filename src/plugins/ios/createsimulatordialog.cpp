// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "createsimulatordialog.h"

#include "iostr.h"
#include "simulatorcontrol.h"

#include <utils/algorithm.h>
#include <utils/async.h>
#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>

namespace Ios::Internal {

CreateSimulatorDialog::CreateSimulatorDialog(QWidget *parent)
    : QDialog(parent)
{
    resize(320, 160);
    setWindowTitle(Tr::tr("Create Simulator"));

    m_nameEdit = new QLineEdit(this);
    m_deviceTypeCombo = new QComboBox(this);
    m_runtimeCombo = new QComboBox(this);

    auto buttonBox = new QDialogButtonBox(QDialogButtonBox::Cancel|QDialogButtonBox::Ok);
    buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

    using namespace Layouting;

    Column {
        Form {
            Tr::tr("Simulator name:"), m_nameEdit, br,
            Tr::tr("Device type:"), m_deviceTypeCombo, br,
            Tr::tr("OS version:"), m_runtimeCombo, br,
        },
        buttonBox
    }.attachTo(this);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    const auto enableOk = [this, buttonBox] {
        buttonBox->button(QDialogButtonBox::Ok)->setEnabled(
                    !m_nameEdit->text().isEmpty() &&
                    m_deviceTypeCombo->currentIndex() > 0 &&
                    m_runtimeCombo->currentIndex() > 0);
    };

    connect(m_nameEdit, &QLineEdit::textChanged, this, enableOk);
    connect(m_runtimeCombo, &QComboBox::currentIndexChanged, this, enableOk);
    connect(m_deviceTypeCombo, &QComboBox::currentIndexChanged, this, [this, enableOk] {
        populateRuntimes(m_deviceTypeCombo->currentData().value<DeviceTypeInfo>());
        enableOk();
    });

    m_futureSync.addFuture(Utils::onResultReady(SimulatorControl::updateDeviceTypes(this), this,
                                                &CreateSimulatorDialog::populateDeviceTypes));

    QFuture<QList<RuntimeInfo>> runtimesfuture = SimulatorControl::updateRuntimes(this);
    Utils::onResultReady(runtimesfuture, this, [this](const QList<RuntimeInfo> &runtimes) {
        m_runtimes = runtimes;
    });
    m_futureSync.addFuture(runtimesfuture);
    populateRuntimes(DeviceTypeInfo());
}

CreateSimulatorDialog::~CreateSimulatorDialog()
{
    m_futureSync.waitForFinished();
}

/*!
    Returns the the simulator name entered by user.
 */
QString CreateSimulatorDialog::name() const
{
    return m_nameEdit->text();
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
    return m_runtimeCombo->currentData().value<RuntimeInfo>();
}

/*!
    Returns the the selected device type.
 */
DeviceTypeInfo CreateSimulatorDialog::deviceType() const
{
    return m_deviceTypeCombo->currentData().value<DeviceTypeInfo>();
}

/*!
    Populates the devices types. Similar device types are grouped together.
 */
void CreateSimulatorDialog::populateDeviceTypes(const QList<DeviceTypeInfo> &deviceTypes)
{
    m_deviceTypeCombo->clear();
    m_deviceTypeCombo->addItem(Tr::tr("None"));

    if (deviceTypes.isEmpty())
        return;

    m_deviceTypeCombo->insertSeparator(1);

    auto addItems = [this, deviceTypes](const QString &filter) {
        const auto filteredTypes = Utils::filtered(deviceTypes, [filter](const DeviceTypeInfo &type){
            return type.name.contains(filter, Qt::CaseInsensitive);
        });
        for (auto type : filteredTypes) {
            m_deviceTypeCombo->addItem(type.name, QVariant::fromValue<DeviceTypeInfo>(type));
        }
        return filteredTypes.count();
    };

    if (addItems(QStringLiteral("iPhone")) > 0)
        m_deviceTypeCombo->insertSeparator(m_deviceTypeCombo->count());
    if (addItems(QStringLiteral("iPad")) > 0)
        m_deviceTypeCombo->insertSeparator(m_deviceTypeCombo->count());
    if (addItems(QStringLiteral("TV")) > 0)
        m_deviceTypeCombo->insertSeparator(m_deviceTypeCombo->count());
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
    m_runtimeCombo->clear();
    m_runtimeCombo->addItem(Tr::tr("None"));

    if (deviceType.name.isEmpty())
        return;

    m_runtimeCombo->insertSeparator(1);

    auto addItems = [this](const QString &filter) {
        const auto filteredTypes = Utils::filtered(m_runtimes, [filter](const RuntimeInfo &runtime){
            return runtime.name.contains(filter, Qt::CaseInsensitive);
        });
        for (auto runtime : filteredTypes) {
            m_runtimeCombo->addItem(runtime.name, QVariant::fromValue<RuntimeInfo>(runtime));
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

} // Ios::Internal
