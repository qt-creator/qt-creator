// Copyright (C) 2019 Andrey Sobol <andrey.sobol.nn@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal::Internal {

// EBlinkGdbServerProvider

class EBlinkGdbServerProvider final : public GdbServerProvider
{
public:
    enum InterfaceType { SWD, JTAG };

    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const IDebugServerProvider &other) const final;

    QString channelString() const final;
    Utils::CommandLine command() const final;

    QSet<StartupMode> supportedStartupModes() const final;
    bool isValid() const final;

private:
    EBlinkGdbServerProvider();

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    Utils::FilePath m_executableFile = "eblink"; // server execute filename
    int  m_verboseLevel = 0;                // verbose <0..7>  Specify generally verbose logging
    InterfaceType m_interfaceType = SWD;    // -I stlink ;swd(default) jtag
    Utils::FilePath m_deviceScript = "stm32-auto.script";  // -D <script> ;Select the device script <>.script
    bool m_interfaceResetOnConnect = true;  // (inversed)-I stlink,dr ;Disable reset at connection (hotplug)
    int  m_interfaceSpeed = 4000;           // -I stlink,speed=4000
    QString m_interfaceExplicidDevice;      // device=<usb_bus>:<usb_addr> ; Set device explicit
    QString m_targetName = {"cortex-m"};    // -T cortex-m(default)
    bool m_targetDisableStack = false;      // -T cortex-m,nu ;Disable stack unwind at exception
    bool m_gdbShutDownAfterDisconnect = true;// -G S ; Shutdown after disconnect
    bool m_gdbNotUseCache = false;           // -G nc ; Don't use EBlink flash cache

    QString scriptFileWoExt() const;

    friend class EBlinkGdbServerProviderConfigWidget;
    friend class EBlinkGdbServerProviderFactory;
};

// EBlinkGdbServerProviderFactory

class EBlinkGdbServerProviderFactory final
        : public IDebugServerProviderFactory
{
public:
    explicit EBlinkGdbServerProviderFactory();
};

// EBlinkGdbServerProviderConfigWidget

class EBlinkGdbServerProviderConfigWidget final
        : public GdbServerProviderConfigWidget
{
public:
    explicit EBlinkGdbServerProviderConfigWidget(
            EBlinkGdbServerProvider *provider);

private:
    void apply() final;
    void discard() final;

    EBlinkGdbServerProvider::InterfaceType interfaceTypeToWidget(int idx) const;
    EBlinkGdbServerProvider::InterfaceType interfaceTypeFromWidget() const;

    void populateInterfaceTypes();
    void setFromProvider();

    HostWidget *m_gdbHostWidget = nullptr;
    Utils::PathChooser *m_executableFileChooser = nullptr;
    QSpinBox *m_verboseLevelSpinBox = nullptr;
    QCheckBox *m_resetOnConnectCheckBox = nullptr;
    QCheckBox *m_notUseCacheCheckBox = nullptr;
    QCheckBox *m_shutDownAfterDisconnectCheckBox = nullptr;
    QComboBox *m_interfaceTypeComboBox = nullptr;
    //QLineEdit *m_deviceScriptLineEdit = nullptr;
    Utils::PathChooser *m_scriptFileChooser = nullptr;
    QSpinBox  *m_interfaceSpeedSpinBox = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // BareMetal::Internal

