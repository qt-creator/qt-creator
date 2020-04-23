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

#include "stlinkuvscserverprovider.h"

#include "uvproject.h"
#include "uvprojectwriter.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/debugserverprovidermanager.h>

#include <debugger/debuggerruncontrol.h>

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>

#include <fstream> // for std::ofstream

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal {
namespace Internal {

using namespace Uv;

constexpr char adapterOptionsKeyC[] = "BareMetal.StLinkUvscServerProvider.AdapterOptions";
constexpr char adapterPortKeyC[] = "BareMetal.StLinkUvscServerProvider.AdapterPort";
constexpr char adapterSpeedKeyC[] = "BareMetal.StLinkUvscServerProvider.AdapterSpeed";

static QString buildAdapterOptions(const StLinkUvscAdapterOptions &opts)
{
    QString s;
    if (opts.port == StLinkUvscAdapterOptions::JTAG)
        s += "-0142";
    else if (opts.port == StLinkUvscAdapterOptions::SWD)
        s += "-0206";

    s += " -S" + QString::number(opts.speed);
    return s;
}

static QString buildDllRegistryName(const DeviceSelection &device,
                                    const StLinkUvscAdapterOptions &opts)
{
    if (device.algorithmIndex < 0 || device.algorithmIndex >= int(device.algorithms.size()))
        return {};
    const DeviceSelection::Algorithm algorithm = device.algorithms.at(device.algorithmIndex);
    const QFileInfo path(algorithm.path);
    const QString flashStart = UvscServerProvider::adjustFlashAlgorithmProperty(algorithm.flashStart);
    const QString flashSize = UvscServerProvider::adjustFlashAlgorithmProperty(algorithm.flashSize);
    const QString adaptOpts = buildAdapterOptions(opts);
    return QStringLiteral(" %6 -FN1 -FF0%1 -FS0%2 -FL0%3 -FP0($$Device:%4$%5)")
            .arg(path.fileName(), flashStart, flashSize, device.name, path.filePath(), adaptOpts);
}

// StLinkUvProjectOptions

class StLinkUvProjectOptions final : public Uv::ProjectOptions
{
public:
    explicit StLinkUvProjectOptions(const StLinkUvscServerProvider *provider)
        : Uv::ProjectOptions(provider)
    {
        const DriverSelection driver = provider->driverSelection();
        const DeviceSelection device = provider->deviceSelection();
        m_debugOpt->appendProperty("nTsel", driver.index);
        m_debugOpt->appendProperty("pMon", driver.dll);

        // Fill 'TargetDriverDllRegistry' (required for dedugging).
        const auto dllRegistry = m_targetOption->appendPropertyGroup("TargetDriverDllRegistry");
        const auto setRegEntry = dllRegistry->appendPropertyGroup("SetRegEntry");
        setRegEntry->appendProperty("Number", 0);
        const QString key = UvscServerProvider::buildDllRegistryKey(driver);
        setRegEntry->appendProperty("Key", key);
        const QString name = buildDllRegistryName(device, provider->m_adapterOpts);
        setRegEntry->appendProperty("Name", name);
    }
};

// StLinkUvscAdapterOptions

QVariantMap StLinkUvscAdapterOptions::toMap() const
{
    QVariantMap map;
    map.insert(adapterPortKeyC, port);
    map.insert(adapterSpeedKeyC, speed);
    return map;
}

bool StLinkUvscAdapterOptions::fromMap(const QVariantMap &data)
{
    port = static_cast<Port>(data.value(adapterPortKeyC, SWD).toInt());
    speed = static_cast<Speed>(data.value(adapterSpeedKeyC, Speed_4MHz).toInt());
    return true;
}

bool StLinkUvscAdapterOptions::operator==(const StLinkUvscAdapterOptions &other) const
{
    return port == other.port && speed == other.speed;
}

// StLinkUvscServerProvider

StLinkUvscServerProvider::StLinkUvscServerProvider()
    : UvscServerProvider(Constants::UVSC_STLINK_PROVIDER_ID)
{
    setTypeDisplayName(UvscServerProvider::tr("uVision St-Link"));
    setConfigurationWidgetCreator([this] { return new StLinkUvscServerProviderConfigWidget(this); });
    setSupportedDrivers({"STLink\\ST-LINKIII-KEIL_SWO.dll"});
}

QVariantMap StLinkUvscServerProvider::toMap() const
{
    QVariantMap data = UvscServerProvider::toMap();
    data.insert(adapterOptionsKeyC, m_adapterOpts.toMap());
    return data;
}

bool StLinkUvscServerProvider::fromMap(const QVariantMap &data)
{
    if (!UvscServerProvider::fromMap(data))
        return false;
    m_adapterOpts.fromMap(data.value(adapterOptionsKeyC).toMap());
    return true;
}

bool StLinkUvscServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!UvscServerProvider::operator==(other))
        return false;
    const auto p = static_cast<const StLinkUvscServerProvider *>(&other);
    return m_adapterOpts == p->m_adapterOpts;
    return true;
}

FilePath StLinkUvscServerProvider::optionsFilePath(DebuggerRunTool *runTool,
                                                   QString &errorMessage) const
{
    const FilePath optionsPath = buildOptionsFilePath(runTool);
    std::ofstream ofs(optionsPath.toString().toStdString(), std::ofstream::out);
    Uv::ProjectOptionsWriter writer(&ofs);
    const StLinkUvProjectOptions projectOptions(this);
    if (!writer.write(&projectOptions)) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Unable to create a uVision project options template.");
        return {};
    }
    return optionsPath;
}

// StLinkUvscServerProviderFactory

StLinkUvscServerProviderFactory::StLinkUvscServerProviderFactory()
{
    setId(Constants::UVSC_STLINK_PROVIDER_ID);
    setDisplayName(UvscServerProvider::tr("uVision St-Link"));
    setCreator([] { return new StLinkUvscServerProvider; });
}

// StLinkUvscServerProviderConfigWidget

StLinkUvscServerProviderConfigWidget::StLinkUvscServerProviderConfigWidget(
        StLinkUvscServerProvider *p)
    : UvscServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_adapterOptionsWidget = new StLinkUvscAdapterOptionsWidget;
    m_mainLayout->addRow(tr("Adapter options:"), m_adapterOptionsWidget);

    setFromProvider();

    connect(m_adapterOptionsWidget, &StLinkUvscAdapterOptionsWidget::optionsChanged,
            this, &StLinkUvscServerProviderConfigWidget::dirty);
}

void StLinkUvscServerProviderConfigWidget::apply()
{
    const auto p = static_cast<StLinkUvscServerProvider *>(m_provider);
    Q_ASSERT(p);
    p->m_adapterOpts = adapterOptions();
    UvscServerProviderConfigWidget::apply();
}

void StLinkUvscServerProviderConfigWidget::discard()
{
    setFromProvider();
    UvscServerProviderConfigWidget::discard();
}

void StLinkUvscServerProviderConfigWidget::setAdapterOpitons(
        const StLinkUvscAdapterOptions &adapterOpts)
{
    m_adapterOptionsWidget->setAdapterOptions(adapterOpts);
}

StLinkUvscAdapterOptions StLinkUvscServerProviderConfigWidget::adapterOptions() const
{
    return m_adapterOptionsWidget->adapterOptions();
}

void StLinkUvscServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<StLinkUvscServerProvider *>(m_provider);
    Q_ASSERT(p);
    const QSignalBlocker blocker(this);
    setAdapterOpitons(p->m_adapterOpts);
}

// StLinkUvscAdapterOptionsWidget

StLinkUvscAdapterOptionsWidget::StLinkUvscAdapterOptionsWidget(QWidget *parent)
    : QWidget(parent)
{
    const auto layout = new QHBoxLayout;
    layout->setContentsMargins(0, 0, 0, 0);
    layout->addWidget(new QLabel(tr("Port:")));
    m_portBox = new QComboBox;
    layout->addWidget(m_portBox);
    layout->addWidget(new QLabel(tr("Speed:")));
    m_speedBox = new QComboBox;
    layout->addWidget(m_speedBox);
    setLayout(layout);

    populatePorts();

    connect(m_portBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        Q_UNUSED(index);
        populateSpeeds();
        emit optionsChanged();
    });
    connect(m_speedBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &StLinkUvscAdapterOptionsWidget::optionsChanged);
}

void StLinkUvscAdapterOptionsWidget::setAdapterOptions(
        const StLinkUvscAdapterOptions &adapterOpts)
{
    for (auto index = 0; m_portBox->count(); ++index) {
        const StLinkUvscAdapterOptions::Port port = portAt(index);
        if (port == adapterOpts.port) {
            m_portBox->setCurrentIndex(index);
            break;
        }
    }

    populateSpeeds();

    for (auto index = 0; m_speedBox->count(); ++index) {
        const StLinkUvscAdapterOptions::Speed speed = speedAt(index);
        if (speed == adapterOpts.speed) {
            m_speedBox->setCurrentIndex(index);
            break;
        }
    }
}

StLinkUvscAdapterOptions StLinkUvscAdapterOptionsWidget::adapterOptions() const
{
    const auto port = portAt(m_portBox->currentIndex());
    const auto speed = speedAt(m_speedBox->currentIndex());
    return {port, speed};
}

StLinkUvscAdapterOptions::Port StLinkUvscAdapterOptionsWidget::portAt(int index) const
{
    return static_cast<StLinkUvscAdapterOptions::Port>(m_portBox->itemData(index).toInt());
}

StLinkUvscAdapterOptions::Speed StLinkUvscAdapterOptionsWidget::speedAt(int index) const
{
    return static_cast<StLinkUvscAdapterOptions::Speed>(m_speedBox->itemData(index).toInt());
}

void StLinkUvscAdapterOptionsWidget::populatePorts()
{
    m_portBox->addItem(tr("JTAG"), StLinkUvscAdapterOptions::JTAG);
    m_portBox->addItem(tr("SWD"), StLinkUvscAdapterOptions::SWD);
}

void StLinkUvscAdapterOptionsWidget::populateSpeeds()
{
    m_speedBox->clear();

    const auto port = portAt(m_portBox->currentIndex());
    if (port == StLinkUvscAdapterOptions::JTAG) {
        m_speedBox->addItem(tr("9MHz"), StLinkUvscAdapterOptions::Speed_9MHz);
        m_speedBox->addItem(tr("4.5MHz"), StLinkUvscAdapterOptions::Speed_4_5MHz);
        m_speedBox->addItem(tr("2.25MHz"), StLinkUvscAdapterOptions::Speed_2_25MHz);
        m_speedBox->addItem(tr("1.12MHz"), StLinkUvscAdapterOptions::Speed_1_12MHz);
        m_speedBox->addItem(tr("560kHz"), StLinkUvscAdapterOptions::Speed_560kHz);
        m_speedBox->addItem(tr("280kHz"), StLinkUvscAdapterOptions::Speed_280kHz);
        m_speedBox->addItem(tr("140kHz"), StLinkUvscAdapterOptions::Speed_140kHz);
    } else if (port == StLinkUvscAdapterOptions::SWD) {
        m_speedBox->addItem(tr("4MHz"), StLinkUvscAdapterOptions::Speed_4MHz);
        m_speedBox->addItem(tr("1.8MHz"), StLinkUvscAdapterOptions::Speed_1_8MHz);
        m_speedBox->addItem(tr("950kHz"), StLinkUvscAdapterOptions::Speed_950kHz);
        m_speedBox->addItem(tr("480kHz"), StLinkUvscAdapterOptions::Speed_480kHz);
        m_speedBox->addItem(tr("240kHz"), StLinkUvscAdapterOptions::Speed_240kHz);
        m_speedBox->addItem(tr("125kHz"), StLinkUvscAdapterOptions::Speed_125kHz);
        m_speedBox->addItem(tr("100kHz"), StLinkUvscAdapterOptions::Speed_100kHz);
        m_speedBox->addItem(tr("50kHz"), StLinkUvscAdapterOptions::Speed_50kHz);
        m_speedBox->addItem(tr("25kHz"), StLinkUvscAdapterOptions::Speed_25kHz);
        m_speedBox->addItem(tr("15kHz"), StLinkUvscAdapterOptions::Speed_15kHz);
        m_speedBox->addItem(tr("5kHz"), StLinkUvscAdapterOptions::Speed_5kHz);
    }
}

} // namespace Internal
} // namespace BareMetal
