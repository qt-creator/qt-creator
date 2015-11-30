/****************************************************************************
**
** Copyright (C) 2015 Tim Sander <tim@krieglstein.org>
** Copyright (C) 2015 Denis Shienkov <denis.shienkov@gmail.com>
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "baremetaldeviceconfigurationwidget.h"
#include "baremetaldevice.h"

#include "gdbserverproviderchooser.h"
#include "gdbserverprovider.h"

#include <utils/qtcassert.h>

#include <QFormLayout>

namespace BareMetal {
namespace Internal {

BareMetalDeviceConfigurationWidget::BareMetalDeviceConfigurationWidget(
        const ProjectExplorer::IDevice::Ptr &deviceConfig, QWidget *parent)
    : IDeviceWidget(deviceConfig, parent)
{
    const auto dev = qSharedPointerCast<const BareMetalDevice>(device());
    QTC_ASSERT(dev, return);

    auto formLayout = new QFormLayout(this);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);

    m_gdbServerProviderChooser = new GdbServerProviderChooser(true, this);
    m_gdbServerProviderChooser->populate();
    m_gdbServerProviderChooser->setCurrentProviderId(dev->gdbServerProviderId());
    formLayout->addRow(tr("GDB server provider:"), m_gdbServerProviderChooser);

    connect(m_gdbServerProviderChooser.data(), &GdbServerProviderChooser::providerChanged,
            this, &BareMetalDeviceConfigurationWidget::gdbServerProviderChanged);
}

void BareMetalDeviceConfigurationWidget::gdbServerProviderChanged()
{
    auto dev = qSharedPointerCast<BareMetalDevice>(device());
    QTC_ASSERT(dev, return);
    dev->setGdbServerProviderId(m_gdbServerProviderChooser->currentProviderId());
}

void BareMetalDeviceConfigurationWidget::updateDeviceFromUi()
{
    gdbServerProviderChanged();
}

} // namespace Internal
} // namespace BareMetal
