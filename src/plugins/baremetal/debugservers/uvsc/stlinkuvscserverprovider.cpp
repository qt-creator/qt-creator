// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "stlinkuvscserverprovider.h"

#include "uvproject.h"
#include "uvprojectwriter.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaltr.h>
#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/debugserverprovidermanager.h>

#include <utils/qtcassert.h>

#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLabel>

#include <fstream> // for std::ofstream

using namespace Debugger;
using namespace ProjectExplorer;
using namespace Utils;

namespace BareMetal::Internal {

using namespace Uv;

constexpr char adapterOptionsKeyC[] = "AdapterOptions";
constexpr char adapterPortKeyC[] = "AdapterPort";
constexpr char adapterSpeedKeyC[] = "AdapterSpeed";

// StLinkUvscAdapterOptions

class StLinkUvscAdapterOptions final
{
public:
    enum Port { JTAG, SWD };
    enum Speed {
        // SWD speeds.
        Speed_4MHz = 0, Speed_1_8MHz, Speed_950kHz, Speed_480kHz,
        Speed_240kHz, Speed_125kHz, Speed_100kHz, Speed_50kHz,
        Speed_25kHz, Speed_15kHz, Speed_5kHz,
        // JTAG speeds.
        Speed_9MHz = 256, Speed_4_5MHz, Speed_2_25MHz, Speed_1_12MHz,
        Speed_560kHz, Speed_280kHz, Speed_140kHz,
    };
    Port port = Port::SWD;
    Speed speed = Speed::Speed_4MHz;

    QVariantMap toMap() const;
    bool fromMap(const Store &data);
    bool operator==(const StLinkUvscAdapterOptions &other) const;
};

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

// StLinkUvscServerProvider

class StLinkUvscServerProvider final : public UvscServerProvider
{
public:
    void toMap(Store &data) const final;
    void fromMap(const Store &data) final;

    bool operator==(const IDebugServerProvider &other) const final;
    Utils::FilePath optionsFilePath(Debugger::DebuggerRunTool *runTool,
                                    QString &errorMessage) const final;
private:
    explicit StLinkUvscServerProvider();

    StLinkUvscAdapterOptions m_adapterOpts;

    friend class StLinkUvscServerProviderConfigWidget;
    friend class StLinkUvscServerProviderFactory;
    friend class StLinkUvProjectOptions;
};

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

QVariantMap StLinkUvscAdapterOptions::toMap() const
{
    QVariantMap map;
    map.insert(adapterPortKeyC, port);
    map.insert(adapterSpeedKeyC, speed);
    return map;
}

bool StLinkUvscAdapterOptions::fromMap(const Store &data)
{
    port = static_cast<Port>(data.value(adapterPortKeyC, SWD).toInt());
    speed = static_cast<Speed>(data.value(adapterSpeedKeyC, Speed_4MHz).toInt());
    return true;
}

bool StLinkUvscAdapterOptions::operator==(const StLinkUvscAdapterOptions &other) const
{
    return port == other.port && speed == other.speed;
}

// StLinkUvscServerProviderConfigWidget

class StLinkUvscAdapterOptionsWidget;
class StLinkUvscServerProviderConfigWidget final : public UvscServerProviderConfigWidget
{
public:
    explicit StLinkUvscServerProviderConfigWidget(StLinkUvscServerProvider *provider);

private:
    void apply() override;
    void discard() override;

    void setAdapterOpitons(const StLinkUvscAdapterOptions &adapterOpts);
    StLinkUvscAdapterOptions adapterOptions() const;
    void setFromProvider();

    StLinkUvscAdapterOptionsWidget *m_adapterOptionsWidget = nullptr;
};

// StLinkUvscAdapterOptionsWidget

class StLinkUvscAdapterOptionsWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit StLinkUvscAdapterOptionsWidget(QWidget *parent = nullptr);
    void setAdapterOptions(const StLinkUvscAdapterOptions &adapterOpts);
    StLinkUvscAdapterOptions adapterOptions() const;

signals:
    void optionsChanged();

private:
    StLinkUvscAdapterOptions::Port portAt(int index) const;
    StLinkUvscAdapterOptions::Speed speedAt(int index) const;

    void populatePorts();
    void populateSpeeds();

    QComboBox *m_portBox = nullptr;
    QComboBox *m_speedBox = nullptr;
};

// StLinkUvscServerProvider

StLinkUvscServerProvider::StLinkUvscServerProvider()
    : UvscServerProvider(Constants::UVSC_STLINK_PROVIDER_ID)
{
    setTypeDisplayName(Tr::tr("uVision St-Link"));
    setConfigurationWidgetCreator([this] { return new StLinkUvscServerProviderConfigWidget(this); });
    setSupportedDrivers({"STLink\\ST-LINKIII-KEIL_SWO.dll"});
}

void StLinkUvscServerProvider::toMap(Store &data) const
{
    UvscServerProvider::toMap(data);
    data.insert(adapterOptionsKeyC, m_adapterOpts.toMap());
}

void StLinkUvscServerProvider::fromMap(const Store &data)
{
    UvscServerProvider::fromMap(data);
    m_adapterOpts.fromMap(storeFromVariant(data.value(adapterOptionsKeyC)));
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
        errorMessage = Tr::tr("Unable to create a uVision project options template.");
        return {};
    }
    return optionsPath;
}

// StLinkUvscServerProviderFactory

StLinkUvscServerProviderFactory::StLinkUvscServerProviderFactory()
{
    setId(Constants::UVSC_STLINK_PROVIDER_ID);
    setDisplayName(Tr::tr("uVision St-Link"));
    setCreator([] { return new StLinkUvscServerProvider; });
}

// StLinkUvscServerProviderConfigWidget

StLinkUvscServerProviderConfigWidget::StLinkUvscServerProviderConfigWidget(
        StLinkUvscServerProvider *p)
    : UvscServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_adapterOptionsWidget = new StLinkUvscAdapterOptionsWidget;
    m_mainLayout->addRow(Tr::tr("Adapter options:"), m_adapterOptionsWidget);

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
    layout->addWidget(new QLabel(Tr::tr("Port:")));
    m_portBox = new QComboBox;
    layout->addWidget(m_portBox);
    layout->addWidget(new QLabel(Tr::tr("Speed:")));
    m_speedBox = new QComboBox;
    layout->addWidget(m_speedBox);
    setLayout(layout);

    populatePorts();

    connect(m_portBox, &QComboBox::currentIndexChanged, this, [this] {
        populateSpeeds();
        emit optionsChanged();
    });
    connect(m_speedBox, &QComboBox::currentIndexChanged,
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
    m_portBox->addItem(Tr::tr("JTAG"), StLinkUvscAdapterOptions::JTAG);
    m_portBox->addItem(Tr::tr("SWD"), StLinkUvscAdapterOptions::SWD);
}

void StLinkUvscAdapterOptionsWidget::populateSpeeds()
{
    m_speedBox->clear();

    const auto port = portAt(m_portBox->currentIndex());
    if (port == StLinkUvscAdapterOptions::JTAG) {
        m_speedBox->addItem(Tr::tr("9MHz"), StLinkUvscAdapterOptions::Speed_9MHz);
        m_speedBox->addItem(Tr::tr("4.5MHz"), StLinkUvscAdapterOptions::Speed_4_5MHz);
        m_speedBox->addItem(Tr::tr("2.25MHz"), StLinkUvscAdapterOptions::Speed_2_25MHz);
        m_speedBox->addItem(Tr::tr("1.12MHz"), StLinkUvscAdapterOptions::Speed_1_12MHz);
        m_speedBox->addItem(Tr::tr("560kHz"), StLinkUvscAdapterOptions::Speed_560kHz);
        m_speedBox->addItem(Tr::tr("280kHz"), StLinkUvscAdapterOptions::Speed_280kHz);
        m_speedBox->addItem(Tr::tr("140kHz"), StLinkUvscAdapterOptions::Speed_140kHz);
    } else if (port == StLinkUvscAdapterOptions::SWD) {
        m_speedBox->addItem(Tr::tr("4MHz"), StLinkUvscAdapterOptions::Speed_4MHz);
        m_speedBox->addItem(Tr::tr("1.8MHz"), StLinkUvscAdapterOptions::Speed_1_8MHz);
        m_speedBox->addItem(Tr::tr("950kHz"), StLinkUvscAdapterOptions::Speed_950kHz);
        m_speedBox->addItem(Tr::tr("480kHz"), StLinkUvscAdapterOptions::Speed_480kHz);
        m_speedBox->addItem(Tr::tr("240kHz"), StLinkUvscAdapterOptions::Speed_240kHz);
        m_speedBox->addItem(Tr::tr("125kHz"), StLinkUvscAdapterOptions::Speed_125kHz);
        m_speedBox->addItem(Tr::tr("100kHz"), StLinkUvscAdapterOptions::Speed_100kHz);
        m_speedBox->addItem(Tr::tr("50kHz"), StLinkUvscAdapterOptions::Speed_50kHz);
        m_speedBox->addItem(Tr::tr("25kHz"), StLinkUvscAdapterOptions::Speed_25kHz);
        m_speedBox->addItem(Tr::tr("15kHz"), StLinkUvscAdapterOptions::Speed_15kHz);
        m_speedBox->addItem(Tr::tr("5kHz"), StLinkUvscAdapterOptions::Speed_5kHz);
    }
}

} // BareMetal::Internal

#include "stlinkuvscserverprovider.moc"
