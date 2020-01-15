/****************************************************************************
**
** Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
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

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

// StLinkUtilGdbServerProvider

class StLinkUtilGdbServerProvider final : public GdbServerProvider
{
public:
    enum TransportLayer { ScsiOverUsb = 1, RawUsb = 2 };

    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const IDebugServerProvider &other) const final;

    QString channelString() const final;
    Utils::CommandLine command() const final;

    QSet<StartupMode> supportedStartupModes() const final;
    bool isValid() const final;

private:
    StLinkUtilGdbServerProvider();

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    Utils::FilePath m_executableFile = Utils::FilePath::fromString("st-util");
    int m_verboseLevel = 0; // 0..99
    bool m_extendedMode = false; // Listening for connections after disconnect
    bool m_resetBoard = true;
    TransportLayer m_transport = RawUsb;

    friend class StLinkUtilGdbServerProviderConfigWidget;
    friend class StLinkUtilGdbServerProviderFactory;
};

// StLinkUtilGdbServerProviderFactory

class StLinkUtilGdbServerProviderFactory final
        : public IDebugServerProviderFactory
{
public:
    StLinkUtilGdbServerProviderFactory();
};

// StLinkUtilGdbServerProviderConfigWidget

class StLinkUtilGdbServerProviderConfigWidget final
        : public GdbServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit StLinkUtilGdbServerProviderConfigWidget(
            StLinkUtilGdbServerProvider *provider);

private:
    void apply() final;
    void discard() final;

    StLinkUtilGdbServerProvider::TransportLayer transportLayerFromIndex(int idx) const;
    StLinkUtilGdbServerProvider::TransportLayer transportLayer() const;
    void setTransportLayer(StLinkUtilGdbServerProvider::TransportLayer);

    void populateTransportLayers();
    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    Utils::PathChooser *m_executableFileChooser = nullptr;
    QSpinBox *m_verboseLevelSpinBox = nullptr;
    QCheckBox *m_extendedModeCheckBox = nullptr;
    QCheckBox *m_resetBoardCheckBox = nullptr;
    QComboBox *m_transportLayerComboBox = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // namespace Internal
} // namespace BareMetal
