// Copyright (C) 2016 Tim Sander <tim@krieglstein.org>
// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "baremetaldevice.h"

#include "baremetaldeviceconfigurationwidget.h"
#include "baremetaltr.h"
#include "debugserverproviderchooser.h"

#include <utils/qtcassert.h>
#include <utils/filepath.h>

#include <QFormLayout>

namespace BareMetal::Internal {

BareMetalDeviceConfigurationWidget::BareMetalDeviceConfigurationWidget(
        const ProjectExplorer::IDevice::Ptr &deviceConfig)
    : IDeviceWidget(deviceConfig)
{
    const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    QTC_ASSERT(dev, return);

    const auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_debugServerProviderChooser = new DebugServerProviderChooser(true, this);
    m_debugServerProviderChooser->populate();
    m_debugServerProviderChooser->setCurrentProviderId(dev->debugServerProviderId());
    formLayout->addRow(Tr::tr("Debug server provider:"), m_debugServerProviderChooser);

    connect(m_debugServerProviderChooser, &DebugServerProviderChooser::providerChanged,
            this, &BareMetalDeviceConfigurationWidget::debugServerProviderChanged);
}

void BareMetalDeviceConfigurationWidget::debugServerProviderChanged()
{
    const auto dev = qSharedPointerCast<BareMetalDevice>(device());
    QTC_ASSERT(dev, return);
    dev->setDebugServerProviderId(m_debugServerProviderChooser->currentProviderId());
}

void BareMetalDeviceConfigurationWidget::updateDeviceFromUi()
{
    debugServerProviderChanged();
}

} // BareMetal::Internal
