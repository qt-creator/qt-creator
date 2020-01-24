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

#pragma once

#include "uvscserverprovider.h"

QT_BEGIN_NAMESPACE
class QComboBox;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

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
    Q_OBJECT

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

} // namespace Internal
} // namespace BareMetal
