// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jlinkuvscserverprovider.h"

#include "uvproject.h"
#include "uvprojectwriter.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaldebugsupport.h>
#include <baremetal/baremetaltr.h>
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

class JLinkUvscAdapterOptions final
{
public:
    enum Port { JTAG, SWD };
    enum Speed {
        Speed_50MHz = 50000, Speed_33MHz = 33000, Speed_25MHz = 25000,
        Speed_20MHz = 20000, Speed_10MHz = 10000, Speed_5MHz = 5000,
        Speed_3MHz = 3000, Speed_2MHz = 2000, Speed_1MHz = 1000,
        Speed_500kHz = 500, Speed_200kHz = 200, Speed_100kHz = 100,
    };
    Port port = Port::SWD;
    Speed speed = Speed::Speed_1MHz;

    Store toMap() const;
    bool fromMap(const Store &data);
    bool operator==(const JLinkUvscAdapterOptions &other) const;
};

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

// JLinkUvscAdapterOptions

Store JLinkUvscAdapterOptions::toMap() const
{
    Store map;
    map.insert(adapterPortKeyC, port);
    map.insert(adapterSpeedKeyC, speed);
    return map;
}

bool JLinkUvscAdapterOptions::fromMap(const Store &data)
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

class JLinkUvscServerProvider final : public UvscServerProvider
{
public:
    void toMap(Store &data) const final;
    void fromMap(const Store &data) final;

    bool operator==(const IDebugServerProvider &other) const final;
    Utils::FilePath optionsFilePath(Debugger::DebuggerRunTool *runTool,
                                    QString &errorMessage) const final;
private:
    explicit JLinkUvscServerProvider();

    JLinkUvscAdapterOptions m_adapterOpts;

    friend class JLinkUvscServerProviderConfigWidget;
    friend class JLinkUvscServerProviderFactory;
    friend class JLinkUvProjectOptions;
};

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

// JLinkUvscServerProviderFactory

class JLinkUvscAdapterOptionsWidget;

class JLinkUvscServerProviderConfigWidget final : public UvscServerProviderConfigWidget
{
public:
    explicit JLinkUvscServerProviderConfigWidget(JLinkUvscServerProvider *provider);

private:
    void apply() override;
    void discard() override;

    void setAdapterOpitons(const JLinkUvscAdapterOptions &adapterOpts);
    JLinkUvscAdapterOptions adapterOptions() const;
    void setFromProvider();

    JLinkUvscAdapterOptionsWidget *m_adapterOptionsWidget = nullptr;
};

// JLinkUvscAdapterOptionsWidget

class JLinkUvscAdapterOptionsWidget final : public QWidget
{
    Q_OBJECT

public:
    explicit JLinkUvscAdapterOptionsWidget(QWidget *parent = nullptr);
    void setAdapterOptions(const JLinkUvscAdapterOptions &adapterOpts);
    JLinkUvscAdapterOptions adapterOptions() const;

signals:
    void optionsChanged();

private:
    JLinkUvscAdapterOptions::Port portAt(int index) const;
    JLinkUvscAdapterOptions::Speed speedAt(int index) const;

    void populatePorts();
    void populateSpeeds();

    QComboBox *m_portBox = nullptr;
    QComboBox *m_speedBox = nullptr;
};

JLinkUvscServerProvider::JLinkUvscServerProvider()
    : UvscServerProvider(Constants::UVSC_JLINK_PROVIDER_ID)
{
    setTypeDisplayName(Tr::tr("uVision JLink"));
    setConfigurationWidgetCreator([this] { return new JLinkUvscServerProviderConfigWidget(this); });
    setSupportedDrivers({"Segger\\JL2CM3.dll"});
}

void JLinkUvscServerProvider::toMap(Store &data) const
{
    UvscServerProvider::toMap(data);
    data.insert(adapterOptionsKeyC, variantFromStore(m_adapterOpts.toMap()));
}

void JLinkUvscServerProvider::fromMap(const Store &data)
{
    UvscServerProvider::fromMap(data);
    m_adapterOpts.fromMap(storeFromVariant(data.value(adapterOptionsKeyC)));
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
        errorMessage = Tr::tr("Unable to create a uVision project options template.");
        return {};
    }
    return optionsPath;
}

// JLinkUvscServerProviderFactory

JLinkUvscServerProviderFactory::JLinkUvscServerProviderFactory()
{
    setId(Constants::UVSC_JLINK_PROVIDER_ID);
    setDisplayName(Tr::tr("uVision JLink"));
    setCreator([] { return new JLinkUvscServerProvider; });
}

// JLinkUvscServerProviderConfigWidget

JLinkUvscServerProviderConfigWidget::JLinkUvscServerProviderConfigWidget(
        JLinkUvscServerProvider *p)
    : UvscServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_adapterOptionsWidget = new JLinkUvscAdapterOptionsWidget;
    m_mainLayout->addRow(Tr::tr("Adapter options:"), m_adapterOptionsWidget);

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
    m_portBox->addItem(Tr::tr("JTAG"), JLinkUvscAdapterOptions::JTAG);
    m_portBox->addItem(Tr::tr("SWD"), JLinkUvscAdapterOptions::SWD);
}

void JLinkUvscAdapterOptionsWidget::populateSpeeds()
{
    m_speedBox->clear();
    m_speedBox->addItem(Tr::tr("50MHz"), JLinkUvscAdapterOptions::Speed_50MHz);
    m_speedBox->addItem(Tr::tr("33MHz"), JLinkUvscAdapterOptions::Speed_33MHz);
    m_speedBox->addItem(Tr::tr("25MHz"), JLinkUvscAdapterOptions::Speed_25MHz);
    m_speedBox->addItem(Tr::tr("20MHz"), JLinkUvscAdapterOptions::Speed_20MHz);
    m_speedBox->addItem(Tr::tr("10MHz"), JLinkUvscAdapterOptions::Speed_10MHz);
    m_speedBox->addItem(Tr::tr("5MHz"), JLinkUvscAdapterOptions::Speed_5MHz);
    m_speedBox->addItem(Tr::tr("3MHz"), JLinkUvscAdapterOptions::Speed_3MHz);
    m_speedBox->addItem(Tr::tr("2MHz"), JLinkUvscAdapterOptions::Speed_2MHz);
    m_speedBox->addItem(Tr::tr("1MHz"), JLinkUvscAdapterOptions::Speed_1MHz);
    m_speedBox->addItem(Tr::tr("500kHz"), JLinkUvscAdapterOptions::Speed_500kHz);
    m_speedBox->addItem(Tr::tr("200kHz"), JLinkUvscAdapterOptions::Speed_200kHz);
    m_speedBox->addItem(Tr::tr("100kHz"), JLinkUvscAdapterOptions::Speed_100kHz);
}

} // BareMetal::Internal

#include "jlinkuvscserverprovider.moc"
