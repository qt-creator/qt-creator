// Copyright (C) 2019 Kovalev Dmitry <kovalevda1991@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal::Internal {

// JLinkGdbServerProvider

class JLinkGdbServerProvider final : public GdbServerProvider
{
public:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const IDebugServerProvider &other) const final;

    QString channelString() const final;
    Utils::CommandLine command() const final;

    QSet<StartupMode> supportedStartupModes() const final;
    bool isValid() const final;

private:
    JLinkGdbServerProvider();

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    Utils::FilePath m_executableFile;
    QString m_jlinkDevice;
    QString m_jlinkHost = {"USB"};
    QString m_jlinkHostAddr;
    QString m_jlinkTargetIface = {"SWD"};
    QString m_jlinkTargetIfaceSpeed = {"12000"};
    QString m_additionalArguments;

    friend class JLinkGdbServerProviderConfigWidget;
    friend class JLinkGdbServerProviderFactory;
};

// JLinkGdbServerProviderFactory

class JLinkGdbServerProviderFactory final
        : public IDebugServerProviderFactory
{
public:
    JLinkGdbServerProviderFactory();
};

// JLinkGdbServerProviderConfigWidget

class JLinkGdbServerProviderConfigWidget final
        : public GdbServerProviderConfigWidget
{
public:
    explicit JLinkGdbServerProviderConfigWidget(JLinkGdbServerProvider *provider);

private:
    void apply() final;
    void discard() final;

    void populateHostInterfaces();
    void populateTargetInterfaces();
    void populateTargetSpeeds();

    void setHostInterface(const QString &newIface);
    void setTargetInterface(const QString &newIface);
    void setTargetSpeed(const QString &newSpeed);

    void updateAllowedControls();

    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    Utils::PathChooser *m_executableFileChooser = nullptr;

    QWidget *m_hostInterfaceWidget = nullptr;
    QComboBox *m_hostInterfaceComboBox = nullptr;
    QLabel *m_hostInterfaceAddressLabel = nullptr;
    QLineEdit *m_hostInterfaceAddressLineEdit = nullptr;

    QWidget *m_targetInterfaceWidget = nullptr;
    QComboBox *m_targetInterfaceComboBox = nullptr;
    QLabel *m_targetInterfaceSpeedLabel = nullptr;
    QComboBox *m_targetInterfaceSpeedComboBox = nullptr;

    QLineEdit *m_jlinkDeviceLineEdit = nullptr;
    QPlainTextEdit *m_additionalArgumentsTextEdit = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // BareMetal::Internal
