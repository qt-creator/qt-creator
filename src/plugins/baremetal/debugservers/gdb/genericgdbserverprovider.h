// Copyright (C) 2020 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "gdbserverprovider.h"

QT_BEGIN_NAMESPACE
class QCheckBox;
class QPlainTextEdit;
QT_END_NAMESPACE

namespace BareMetal::Internal {

// GenericGdbServerProvider

class GenericGdbServerProvider final : public GdbServerProvider
{
private:
    GenericGdbServerProvider();
    QSet<StartupMode> supportedStartupModes() const final;
    ProjectExplorer::RunWorker *targetRunner(ProjectExplorer::RunControl *runControl) const final {
        Q_UNUSED(runControl)
        // Generic Runner assumes GDB Server already running
        return nullptr;
    }

    friend class GenericGdbServerProviderConfigWidget;
    friend class GenericGdbServerProviderFactory;
    friend class BareMetalDevice;
};

// GenericGdbServerProviderFactory

class GenericGdbServerProviderFactory final : public IDebugServerProviderFactory
{
public:
    GenericGdbServerProviderFactory();
};

// GenericGdbServerProviderConfigWidget

class GenericGdbServerProviderConfigWidget final
        : public GdbServerProviderConfigWidget
{
public:
    explicit GenericGdbServerProviderConfigWidget(
            GenericGdbServerProvider *provider);

private:
    void apply() final;
    void discard() final;

    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    QCheckBox *m_useExtendedRemoteCheckBox = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // BareMetal::Internal
