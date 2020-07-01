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

#include "jlinkuvscserverprovider.h"

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

constexpr char adapterOptionsKeyC[] = "BareMetal.JLinkUvscServerProvider.AdapterOptions";
constexpr char adapterPortKeyC[] = "BareMetal.JLinkUvscServerProvider.AdapterPort";
constexpr char adapterSpeedKeyC[] = "BareMetal.JLinkUvscServerProvider.AdapterSpeed";

static int decodeSpeedCode(JLinkUvscAdapterOptions::Speed speed)
{
    switch (speed) {
    case JLinkUvscAdapterOptions::Speed_50MHz:
        return 8;
    case JLinkUvscAdapterOptions::Speed_33MHz:
        return 9;
    case JLinkUvscAdapterOptions::Speed_25MHz:
        return 10;
    case JLinkUvscAdapterOptions::Speed_20MHz:
        return 0;
    case JLinkUvscAdapterOptions::Speed_10MHz:
        return 1;
    case JLinkUvscAdapterOptions::Speed_5MHz:
        return 2;
    case JLinkUvscAdapterOptions::Speed_3MHz:
        return 3;
    case JLinkUvscAdapterOptions::Speed_2MHz:
        return 4;
    case JLinkUvscAdapterOptions::Speed_1MHz:
        return 5;
    case JLinkUvscAdapterOptions::Speed_500kHz:
        return 6;
    case JLinkUvscAdapterOptions::Speed_200kHz:
        return 7;
    default:
        return 8;
    }
}

static QString buildAdapterOptions(const JLinkUvscAdapterOptions &opts)
{
    QString s;
    if (opts.port == JLinkUvscAdapterOptions::JTAG)
        s += "-O14";
    else if (opts.port == JLinkUvscAdapterOptions::SWD)
        s += "-O78";

    const int code = decodeSpeedCode(opts.speed);
    s += " -S" + QString::number(code) + " -ZTIFSpeedSel" + QString::number(opts.speed);
    return s;
}

static QString buildDllRegistryName(const DeviceSelection &device,
                                    const JLinkUvscAdapterOptions &opts)
{
    if (device.algorithmIndex < 0 || device.algorithmIndex >= int(device.algorithms.size()))
        return {};

    const DeviceSelection::Algorithm algorithm = device.algorithms.at(device.algorithmIndex);
    const QFileInfo path(algorithm.path);
    const QString flashStart = UvscServerProvider::adjustFlashAlgorithmProperty(algorithm.flashStart);
    const QString flashSize = UvscServerProvider::adjustFlashAlgorithmProperty(algorithm.flashSize);
    const QString adaptOpts = buildAdapterOptions(opts);

    QString content = QStringLiteral(" %6 -FN1 -FF0%1 -FS0%2 -FL0%3 -FP0($$Device:%4$%5)")
            .arg(path.fileName(), flashStart, flashSize, device.name, path.filePath(), adaptOpts);

    if (!algorithm.ramStart.isEmpty()) {
        const QString ramStart = UvscServerProvider::adjustFlashAlgorithmProperty(algorithm.ramStart);
        content += QStringLiteral(" -FD%1").arg(ramStart);
    }
    if (!algorithm.ramSize.isEmpty()) {
        const QString ramSize = UvscServerProvider::adjustFlashAlgorithmProperty(algorithm.ramSize);
        content += QStringLiteral(" -FC%1").arg(ramSize);
    }

    return content;
}

// JLinkUvProjectOptions

class JLinkUvProjectOptions final : public Uv::ProjectOptions
{
public:
    explicit JLinkUvProjectOptions(const JLinkUvscServerProvider *provider)
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

// JLinkUvscAdapterOptions

QVariantMap JLinkUvscAdapterOptions::toMap() const
{
    QVariantMap map;
    map.insert(adapterPortKeyC, port);
    map.insert(adapterSpeedKeyC, speed);
    return map;
}

bool JLinkUvscAdapterOptions::fromMap(const QVariantMap &data)
{
    port = static_cast<Port>(data.value(adapterPortKeyC, SWD).toInt());
    speed = static_cast<Speed>(data.value(adapterSpeedKeyC, Speed_1MHz).toInt());
    return true;
}

bool JLinkUvscAdapterOptions::operator==(const JLinkUvscAdapterOptions &other) const
{
    return port == other.port && speed == other.speed;
}

// JLinkUvscServerProvider

JLinkUvscServerProvider::JLinkUvscServerProvider()
    : UvscServerProvider(Constants::UVSC_JLINK_PROVIDER_ID)
{
    setTypeDisplayName(tr("uVision JLink"));
    setConfigurationWidgetCreator([this] { return new JLinkUvscServerProviderConfigWidget(this); });
    setSupportedDrivers({"Segger\\JL2CM3.dll"});
}

QVariantMap JLinkUvscServerProvider::toMap() const
{
    QVariantMap data = UvscServerProvider::toMap();
    data.insert(adapterOptionsKeyC, m_adapterOpts.toMap());
    return data;
}

bool JLinkUvscServerProvider::fromMap(const QVariantMap &data)
{
    if (!UvscServerProvider::fromMap(data))
        return false;
    m_adapterOpts.fromMap(data.value(adapterOptionsKeyC).toMap());
    return true;
}

bool JLinkUvscServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!UvscServerProvider::operator==(other))
        return false;
    const auto p = static_cast<const JLinkUvscServerProvider *>(&other);
    return m_adapterOpts == p->m_adapterOpts;
    return true;
}

FilePath JLinkUvscServerProvider::optionsFilePath(DebuggerRunTool *runTool,
                                                   QString &errorMessage) const
{
    const FilePath optionsPath = buildOptionsFilePath(runTool);
    std::ofstream ofs(optionsPath.toString().toStdString(), std::ofstream::out);
    Uv::ProjectOptionsWriter writer(&ofs);
    const JLinkUvProjectOptions projectOptions(this);
    if (!writer.write(&projectOptions)) {
        errorMessage = BareMetalDebugSupport::tr(
                    "Unable to create an uVision project options template.");
        return {};
    }
    return optionsPath;
}

// JLinkUvscServerProviderFactory

JLinkUvscServerProviderFactory::JLinkUvscServerProviderFactory()
{
    setId(Constants::UVSC_JLINK_PROVIDER_ID);
    setDisplayName(UvscServerProvider::tr("uVision JLink"));
    setCreator([] { return new JLinkUvscServerProvider; });
}

// JLinkUvscServerProviderConfigWidget

JLinkUvscServerProviderConfigWidget::JLinkUvscServerProviderConfigWidget(
        JLinkUvscServerProvider *p)
    : UvscServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_adapterOptionsWidget = new JLinkUvscAdapterOptionsWidget;
    m_mainLayout->addRow(tr("Adapter options:"), m_adapterOptionsWidget);

    setFromProvider();

    connect(m_adapterOptionsWidget, &JLinkUvscAdapterOptionsWidget::optionsChanged,
            this, &JLinkUvscServerProviderConfigWidget::dirty);
}

void JLinkUvscServerProviderConfigWidget::apply()
{
    const auto p = static_cast<JLinkUvscServerProvider *>(m_provider);
    Q_ASSERT(p);
    p->m_adapterOpts = adapterOptions();
    UvscServerProviderConfigWidget::apply();
}

void JLinkUvscServerProviderConfigWidget::discard()
{
    setFromProvider();
    UvscServerProviderConfigWidget::discard();
}

void JLinkUvscServerProviderConfigWidget::setAdapterOpitons(
        const JLinkUvscAdapterOptions &adapterOpts)
{
    m_adapterOptionsWidget->setAdapterOptions(adapterOpts);
}

JLinkUvscAdapterOptions JLinkUvscServerProviderConfigWidget::adapterOptions() const
{
    return m_adapterOptionsWidget->adapterOptions();
}

void JLinkUvscServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<JLinkUvscServerProvider *>(m_provider);
    Q_ASSERT(p);
    const QSignalBlocker blocker(this);
    setAdapterOpitons(p->m_adapterOpts);
}

// JLinkUvscAdapterOptionsWidget

JLinkUvscAdapterOptionsWidget::JLinkUvscAdapterOptionsWidget(QWidget *parent)
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
            this, &JLinkUvscAdapterOptionsWidget::optionsChanged);
}

void JLinkUvscAdapterOptionsWidget::setAdapterOptions(
        const JLinkUvscAdapterOptions &adapterOpts)
{
    for (auto index = 0; m_portBox->count(); ++index) {
        const JLinkUvscAdapterOptions::Port port = portAt(index);
        if (port == adapterOpts.port) {
            m_portBox->setCurrentIndex(index);
            break;
        }
    }

    populateSpeeds();

    for (auto index = 0; m_speedBox->count(); ++index) {
        const JLinkUvscAdapterOptions::Speed speed = speedAt(index);
        if (speed == adapterOpts.speed) {
            m_speedBox->setCurrentIndex(index);
            break;
        }
    }
}

JLinkUvscAdapterOptions JLinkUvscAdapterOptionsWidget::adapterOptions() const
{
    const auto port = portAt(m_portBox->currentIndex());
    const auto speed = speedAt(m_speedBox->currentIndex());
    return {port, speed};
}

JLinkUvscAdapterOptions::Port JLinkUvscAdapterOptionsWidget::portAt(int index) const
{
    return static_cast<JLinkUvscAdapterOptions::Port>(m_portBox->itemData(index).toInt());
}

JLinkUvscAdapterOptions::Speed JLinkUvscAdapterOptionsWidget::speedAt(int index) const
{
    return static_cast<JLinkUvscAdapterOptions::Speed>(m_speedBox->itemData(index).toInt());
}

void JLinkUvscAdapterOptionsWidget::populatePorts()
{
    m_portBox->addItem(tr("JTAG"), JLinkUvscAdapterOptions::JTAG);
    m_portBox->addItem(tr("SWD"), JLinkUvscAdapterOptions::SWD);
}

void JLinkUvscAdapterOptionsWidget::populateSpeeds()
{
    m_speedBox->clear();
    m_speedBox->addItem(tr("50MHz"), JLinkUvscAdapterOptions::Speed_50MHz);
    m_speedBox->addItem(tr("33MHz"), JLinkUvscAdapterOptions::Speed_33MHz);
    m_speedBox->addItem(tr("25MHz"), JLinkUvscAdapterOptions::Speed_25MHz);
    m_speedBox->addItem(tr("20MHz"), JLinkUvscAdapterOptions::Speed_20MHz);
    m_speedBox->addItem(tr("10MHz"), JLinkUvscAdapterOptions::Speed_10MHz);
    m_speedBox->addItem(tr("5MHz"), JLinkUvscAdapterOptions::Speed_5MHz);
    m_speedBox->addItem(tr("3MHz"), JLinkUvscAdapterOptions::Speed_3MHz);
    m_speedBox->addItem(tr("2MHz"), JLinkUvscAdapterOptions::Speed_2MHz);
    m_speedBox->addItem(tr("1MHz"), JLinkUvscAdapterOptions::Speed_1MHz);
    m_speedBox->addItem(tr("500kHz"), JLinkUvscAdapterOptions::Speed_500kHz);
    m_speedBox->addItem(tr("200kHz"), JLinkUvscAdapterOptions::Speed_200kHz);
    m_speedBox->addItem(tr("100kHz"), JLinkUvscAdapterOptions::Speed_100kHz);
}

} // namespace Internal
} // namespace BareMetal
