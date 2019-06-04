/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "qdbdevicewizard.h"
#include "ui_qdbdevicewizardsettingspage.h"

#include "qdbconstants.h"
#include "qdbdevice.h"

#include <utils/qtcassert.h>

#include <QString>
#include <QWizardPage>

namespace {
enum PageId { SettingsPageId };
} // anonymous namespace

namespace Qdb {
namespace Internal {

class QdbSettingsPage : public QWizardPage
{
    Q_OBJECT
public:
    QdbSettingsPage(QdbDeviceWizard::DeviceType deviceType, QWidget *parent = 0)
        : QWizardPage(parent), m_deviceType(deviceType)
    {
        m_ui.setupUi(this);
        setTitle(tr("Device Settings"));
        connect(m_ui.nameLineEdit, &QLineEdit::textChanged, this, &QWizardPage::completeChanged);
        connect(m_ui.addressLineEdit, &QLineEdit::textChanged,
                this, &QWizardPage::completeChanged);
    }

    QdbDeviceWizard::DeviceType deviceType() const { return m_deviceType; }
    QString deviceName() const { return m_ui.nameLineEdit->text().trimmed(); }
    QString deviceAddress() const { return m_ui.addressLineEdit->text().trimmed(); }

private:
    bool isComplete() const final {
        return !deviceName().isEmpty() && !deviceAddress().isEmpty();
    }

    QdbDeviceWizard::DeviceType m_deviceType;
    Ui::QdbDeviceWizardSettingsPage m_ui;
};

class QdbDeviceWizard::DeviceWizardPrivate
{
public:
    DeviceWizardPrivate(DeviceType deviceType)
        : settingsPage(deviceType)
    {

    }

    QdbSettingsPage settingsPage;
};

QdbDeviceWizard::QdbDeviceWizard(DeviceType deviceType, QWidget *parent)
    : QWizard(parent), d(new DeviceWizardPrivate(deviceType))
{
    setWindowTitle(tr("Boot2Qt Network Device Setup"));
    d->settingsPage.setCommitPage(true);
    setPage(SettingsPageId, &d->settingsPage);
}

QdbDeviceWizard::~QdbDeviceWizard()
{
    delete d;
}

ProjectExplorer::IDevice::Ptr QdbDeviceWizard::device()
{
    QdbDevice::Ptr device = QdbDevice::create();

    device->setDisplayName(d->settingsPage.deviceName());
    device->setupId(ProjectExplorer::IDevice::ManuallyAdded, Core::Id());
    device->setType(Constants::QdbLinuxOsType);
    device->setMachineType(ProjectExplorer::IDevice::Hardware);

    device->setupDefaultNetworkSettings(d->settingsPage.deviceAddress());

    return device;
}

} // namespace Internal
} // namespace Qdb

#include "qdbdevicewizard.moc"
