// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "desktopdeviceconfigurationwidget.h"

#include "../projectexplorerconstants.h"
#include "../projectexplorertr.h"
#include "idevice.h"

#include <utils/infolabel.h>
#include <utils/layoutbuilder.h>
#include <utils/portlist.h>
#include <utils/qtcassert.h>
#include <utils/utilsicons.h>

#include <QLineEdit>
#include <QRegularExpressionValidator>

using namespace ProjectExplorer::Constants;

namespace ProjectExplorer {

DesktopDeviceConfigurationWidget::DesktopDeviceConfigurationWidget(const IDevicePtr &device) :
    IDeviceWidget(device)
{
    m_freePortsLineEdit = new QLineEdit;
    m_portsWarningLabel = new Utils::InfoLabel(
                Tr::tr("You will need at least one port for QML debugging."),
                Utils::InfoLabel::Warning);

    using namespace Layouting;
    Form {
        Tr::tr("Machine type:"), Tr::tr("Physical Device"), br,
        Tr::tr("Free ports:"), m_freePortsLineEdit, br,
        empty, m_portsWarningLabel, br,
        noMargin,
    }.attachTo(this);

    connect(m_freePortsLineEdit, &QLineEdit::textChanged,
            this, &DesktopDeviceConfigurationWidget::updateFreePorts);

    initGui();
}

void DesktopDeviceConfigurationWidget::updateDeviceFromUi()
{
    updateFreePorts();
}

void DesktopDeviceConfigurationWidget::updateFreePorts()
{
    device()->setFreePorts(Utils::PortList::fromString(m_freePortsLineEdit->text()));
    m_portsWarningLabel->setVisible(!device()->freePorts().hasMore());
}

void DesktopDeviceConfigurationWidget::initGui()
{
    QTC_CHECK(device()->machineType() == IDevice::Hardware);
    m_freePortsLineEdit->setPlaceholderText(
                QString::fromLatin1("eg: %1-%2").arg(DESKTOP_PORT_START).arg(DESKTOP_PORT_END));
    const auto portsValidator = new QRegularExpressionValidator(
        QRegularExpression(Utils::PortList::regularExpression()), this);
    m_freePortsLineEdit->setValidator(portsValidator);

    m_freePortsLineEdit->setText(device()->freePorts().toString());
    updateFreePorts();
}

} // namespace ProjectExplorer
