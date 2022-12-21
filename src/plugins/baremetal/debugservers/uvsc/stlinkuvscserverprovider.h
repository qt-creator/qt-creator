// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvscserverprovider.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace BareMetal::Internal {

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
    bool fromMap(const QVariantMap &data);
    bool operator==(const StLinkUvscAdapterOptions &other) const;
};

// StLinkUvscServerProvider

class StLinkUvscServerProvider final : public UvscServerProvider
{
public:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

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

// StLinkUvscServerProviderFactory

class StLinkUvscServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    StLinkUvscServerProviderFactory();
};

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

} // BareMetal::Internal
