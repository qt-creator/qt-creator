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
class QCheckBox;
QT_END_NAMESPACE

namespace BareMetal {
namespace Internal {

// SimulatorUvscServerProvider

class SimulatorUvscServerProvider final : public UvscServerProvider
{
public:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const IDebugServerProvider &other) const final;
    bool isSimulator() const final { return true; }

    Utils::FilePath optionsFilePath(Debugger::DebuggerRunTool *runTool,
                                    QString &errorMessage) const final;

private:
    explicit SimulatorUvscServerProvider();

    bool m_limitSpeed = false;

    friend class SimulatorUvscServerProviderConfigWidget;
    friend class SimulatorUvscServerProviderFactory;
    friend class SimulatorUvProjectOptions;
};

// SimulatorUvscServerProviderFactory

class SimulatorUvscServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    SimulatorUvscServerProviderFactory();
};

// SimulatorUvscServerProviderConfigWidget

class SimulatorUvscServerProviderConfigWidget final : public UvscServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit SimulatorUvscServerProviderConfigWidget(SimulatorUvscServerProvider *provider);

private:
    void apply() override;
    void discard() override;

    void setFromProvider();

    QCheckBox *m_limitSpeedCheckBox = nullptr;
};

} // namespace Internal
} // namespace BareMetal
