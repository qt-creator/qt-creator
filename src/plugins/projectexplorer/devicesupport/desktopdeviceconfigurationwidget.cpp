/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "desktopdeviceconfigurationwidget.h"
#include "ui_desktopdeviceconfigurationwidget.h"
#include <projectexplorer/projectexplorerconstants.h>

#include <utils/utilsicons.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>

using namespace ProjectExplorer::Constants;

namespace ProjectExplorer {

DesktopDeviceConfigurationWidget::DesktopDeviceConfigurationWidget(const IDevice::Ptr &device,
                                                                   QWidget *parent) :
    IDeviceWidget(device, parent),
    m_ui(new Ui::DesktopDeviceConfigurationWidget)
{
    m_ui->setupUi(this);
    connect(m_ui->freePortsLineEdit, &QLineEdit::textChanged,
            this, &DesktopDeviceConfigurationWidget::updateFreePorts);

    initGui();
}

DesktopDeviceConfigurationWidget::~DesktopDeviceConfigurationWidget()
{
    delete m_ui;
}

void DesktopDeviceConfigurationWidget::updateDeviceFromUi()
{
    updateFreePorts();
}

void DesktopDeviceConfigurationWidget::updateFreePorts()
{
    device()->setFreePorts(Utils::PortList::fromString(m_ui->freePortsLineEdit->text()));
    m_ui->portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void DesktopDeviceConfigurationWidget::initGui()
{
    QTC_CHECK(device()->machineType() == IDevice::Hardware);
    m_ui->machineTypeValueLabel->setText(tr("Physical Device"));
    m_ui->freePortsLineEdit->setPlaceholderText(
                QString::fromLatin1("eg: %1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END));
    m_ui->portsWarningLabel->setPixmap(Utils::Icons::WARNING.pixmap());
    m_ui->portsWarningLabel->setToolTip(QLatin1String("<font color=\"red\">")
                                        + tr("You will need at least one port for QML debugging.")
                                        + QLatin1String("</font>"));
    QRegExpValidator * const portsValidator
            = new QRegExpValidator(QRegExp(Utils::PortList::regularExpression()), this);
    m_ui->freePortsLineEdit->setValidator(portsValidator);

    m_ui->freePortsLineEdit->setText(device()->freePorts().toString());
    updateFreePorts();
}

} // namespace ProjectExplorer
