// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "simulatoruvscserverprovider.h"

#include "uvproject.h"
#include "uvprojectwriter.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/baremetaltr.h>
#include <baremetal/debugserverprovidermanager.h>

#include <utils/qtcassert.h>

#include <QCheckBox>
#include <QFormLayout>

#include <fstream> // for std::ofstream

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

using namespace Uv;

const char limitSpeedKeyC[] = "LimitSpeed";

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
    setTypeDisplayName(Tr::tr("uVision Simulator"));
    setConfigurationWidgetCreator([this] { return new SimulatorUvscServerProviderConfigWidget(this); });
    setDriverSelection(defaultSimulatorDriverSelection());
}

void SimulatorUvscServerProvider::toMap(Store &data) const
{
    UvscServerProvider::toMap(data);
    data.insert(limitSpeedKeyC, m_limitSpeed);
}

void SimulatorUvscServerProvider::fromMap(const Store &data)
{
    UvscServerProvider::fromMap(data);
    m_limitSpeed = data.value(limitSpeedKeyC).toBool();
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
        errorMessage = Tr::tr("Unable to create a uVision project options template.");
        return {};
    }
    return optionsPath;
}

// SimulatorUvscServerProviderFactory

SimulatorUvscServerProviderFactory::SimulatorUvscServerProviderFactory()
{
    setId(Constants::UVSC_SIMULATOR_PROVIDER_ID);
    setDisplayName(Tr::tr("uVision Simulator"));
    setCreator([] { return new SimulatorUvscServerProvider; });
}

// SimulatorUvscServerProviderConfigWidget

SimulatorUvscServerProviderConfigWidget::SimulatorUvscServerProviderConfigWidget(
        SimulatorUvscServerProvider *p)
    : UvscServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_limitSpeedCheckBox = new QCheckBox;
    m_limitSpeedCheckBox->setToolTip(Tr::tr("Limit speed to real-time."));
    m_mainLayout->addRow(Tr::tr("Limit speed to real-time:"), m_limitSpeedCheckBox);

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

} // BareMetal::Internal
