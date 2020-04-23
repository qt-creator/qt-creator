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
    Q_OBJECT

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

} // namespace Internal
} // namespace BareMetal
