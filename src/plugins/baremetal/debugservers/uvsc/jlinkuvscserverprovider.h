// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "uvscserverprovider.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace BareMetal::Internal {

// JLinkUvscAdapterOptions

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

    QVariantMap toMap() const;
    bool fromMap(const QVariantMap &data);
    bool operator==(const JLinkUvscAdapterOptions &other) const;
};

// JLinkUvscServerProvider

class JLinkUvscServerProvider final : public UvscServerProvider
{
public:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

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

// JLinkUvscServerProviderFactory

class JLinkUvscServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    JLinkUvscServerProviderFactory();
};

// JLinkUvscServerProviderConfigWidget

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

} // BareMetal::Internal
