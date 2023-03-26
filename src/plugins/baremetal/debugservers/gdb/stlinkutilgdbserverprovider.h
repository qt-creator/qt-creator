// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal::Internal {

// StLinkUtilGdbServerProvider

class StLinkUtilGdbServerProvider final : public GdbServerProvider
{
public:
    enum TransportLayer { ScsiOverUsb = 1, RawUsb = 2, UnspecifiedTransport };

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

    Utils::FilePath m_executableFile = "st-util";
    int m_verboseLevel = 0; // 0..99
    bool m_extendedMode = false; // Listening for connections after disconnect
    bool m_resetBoard = true;
    bool m_connectUnderReset = false; // Makes it possible to connect to the device before code execution
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
public:
    explicit StLinkUtilGdbServerProviderConfigWidget(StLinkUtilGdbServerProvider *provider);

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
    QCheckBox *m_resetOnConnectCheckBox = nullptr;
    QCheckBox *m_resetBoardCheckBox = nullptr;
    QComboBox *m_transportLayerComboBox = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // BareMetal::Internal
