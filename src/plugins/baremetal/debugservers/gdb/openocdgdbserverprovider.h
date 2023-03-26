// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QPlainTextEdit;
QT_END_NAMESPACE

namespace Utils { class PathChooser; }

namespace BareMetal::Internal {

// OpenOcdGdbServerProvider

class OpenOcdGdbServerProvider final : public GdbServerProvider
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
    explicit OpenOcdGdbServerProvider();

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    Utils::FilePath m_executableFile = "openocd";
    Utils::FilePath m_rootScriptsDir;
    Utils::FilePath m_configurationFile;
    QString m_additionalArguments;

    friend class OpenOcdGdbServerProviderConfigWidget;
    friend class OpenOcdGdbServerProviderFactory;
};

// OpenOcdGdbServerProviderFactory

class OpenOcdGdbServerProviderFactory final
        : public IDebugServerProviderFactory
{
public:
    OpenOcdGdbServerProviderFactory();
};

// OpenOcdGdbServerProviderConfigWidget

class OpenOcdGdbServerProviderConfigWidget final
        : public GdbServerProviderConfigWidget
{
public:
    explicit OpenOcdGdbServerProviderConfigWidget(OpenOcdGdbServerProvider *provider);

private:
    void apply() final;
    void discard() final;

    void startupModeChanged();
    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    Utils::PathChooser *m_executableFileChooser = nullptr;
    Utils::PathChooser *m_rootScriptsDirChooser = nullptr;
    Utils::PathChooser *m_configurationFileChooser = nullptr;
    QLineEdit *m_additionalArgumentsLineEdit = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // BareMetal::Internal
