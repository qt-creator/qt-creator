/****************************************************************************
**
** Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "simulatoruvscserverprovider.h"

#include "uvproject.h"
#include "uvprojectwriter.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/debugserverprovidermanager.h>

#include <debugger/debuggerruncontrol.h>

#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QFormLayout>

#include <fstream> // for std::ofstream

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

using namespace Uv;

const char limitSpeedKeyC[] = "BareMetal.SimulatorUvscServerProvider.LimitSpeed";

static DriverSelection defaultSimulatorDriverSelection()
{
    DriverSelection selection;
    // We don't use any driver DLL for a simulator,
    // we just use only one CPU DLL (yes?).
    selection.name = "None";
    selection.dll = "None";
    selection.index = 0;
    selection.cpuDlls = QStringList{"SARMCM3.DLL"};
    selection.cpuDllIndex = 0;
    return selection;
}

// SimulatorUvProjectOptionsWriter

class SimulatorUvProjectOptions final : public Uv::ProjectOptions
{
public:
    explicit SimulatorUvProjectOptions(const SimulatorUvscServerProvider *provider)
        : Uv::ProjectOptions(provider)
    {
        m_debugOpt->appendProperty("sLrtime", int(provider->m_limitSpeed));
    }
};

// SimulatorUvscServerProvider

SimulatorUvscServerProvider::SimulatorUvscServerProvider()
    : UvscServerProvider(Constants::UVSC_SIMULATOR_PROVIDER_ID)
{
    setTypeDisplayName(UvscServerProvider::tr("uVision Simulator"));
    setConfigurationWidgetCreator([this] { return new SimulatorUvscServerProviderConfigWidget(this); });
    setDriverSelection(defaultSimulatorDriverSelection());
}

QVariantMap SimulatorUvscServerProvider::toMap() const
{
    QVariantMap data = UvscServerProvider::toMap();
    data.insert(limitSpeedKeyC, m_limitSpeed);
    return data;
}

bool SimulatorUvscServerProvider::fromMap(const QVariantMap &data)
{
    if (!UvscServerProvider::fromMap(data))
        return false;
    m_limitSpeed = data.value(limitSpeedKeyC).toBool();
    return true;
}

bool SimulatorUvscServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!UvscServerProvider::operator==(other))
        return false;
    const auto p = static_cast<const SimulatorUvscServerProvider *>(&other);
    return m_limitSpeed == p->m_limitSpeed;
}

FilePath SimulatorUvscServerProvider::optionsFilePath(DebuggerRunTool *runTool,
                                                      QString &errorMessage) const
{
    const FilePath optionsPath = buildOptionsFilePath(runTool);
    std::ofstream ofs(optionsPath.toString().toStdString(), std::ofstream::out);
    Uv::ProjectOptionsWriter writer(&ofs);
    const SimulatorUvProjectOptions projectOptions(this);
    if (!writer.write(&projectOptions)) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Unable to create a uVision project options template.");
        return {};
    }
    return optionsPath;
}

// SimulatorUvscServerProviderFactory

SimulatorUvscServerProviderFactory::SimulatorUvscServerProviderFactory()
{
    setId(Constants::UVSC_SIMULATOR_PROVIDER_ID);
    setDisplayName(UvscServerProvider::tr("uVision Simulator"));
    setCreator([] { return new SimulatorUvscServerProvider; });
}

// SimulatorUvscServerProviderConfigWidget

SimulatorUvscServerProviderConfigWidget::SimulatorUvscServerProviderConfigWidget(
        SimulatorUvscServerProvider *p)
    : UvscServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_limitSpeedCheckBox = new QCheckBox;
    m_limitSpeedCheckBox->setToolTip(tr("Limit speed to real-time."));
    m_mainLayout->addRow(tr("Limit speed to real-time:"), m_limitSpeedCheckBox);

    setFromProvider();

    connect(m_limitSpeedCheckBox, &QAbstractButton::clicked,
            this, &SimulatorUvscServerProviderConfigWidget::dirty);
}

void SimulatorUvscServerProviderConfigWidget::apply()
{
    const auto p = static_cast<SimulatorUvscServerProvider *>(m_provider);
    Q_ASSERT(p);
    p->m_limitSpeed = m_limitSpeedCheckBox->isChecked();
    UvscServerProviderConfigWidget::apply();
}

void SimulatorUvscServerProviderConfigWidget::discard()
{
    setFromProvider();
    UvscServerProviderConfigWidget::discard();
}

void SimulatorUvscServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<SimulatorUvscServerProvider *>(m_provider);
    Q_ASSERT(p);
    const QSignalBlocker blocker(this);
    m_limitSpeedCheckBox->setChecked(p->m_limitSpeed);
}

} // namespace Internal
} // namespace BareMetal
