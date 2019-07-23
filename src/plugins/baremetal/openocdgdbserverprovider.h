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

namespace Utils { class PathChooser; }

namespace BareMetal {
namespace Internal {

class OpenOcdGdbServerProviderConfigWidget;
class OpenOcdGdbServerProviderFactory;

// OpenOcdGdbServerProvider

class OpenOcdGdbServerProvider final : public GdbServerProvider
{
public:
    QVariantMap toMap() const final;
    bool fromMap(const QVariantMap &data) final;

    bool operator==(const GdbServerProvider &) const final;

    GdbServerProviderConfigWidget *configurationWidget() final;
    GdbServerProvider *clone() const final;

    QString channelString() const final;
    Utils::CommandLine command() const final;

    bool canStartupMode(StartupMode mode) const final;
    bool isValid() const final;

private:
    explicit OpenOcdGdbServerProvider();

    static QString defaultInitCommands();
    static QString defaultResetCommands();

    Utils::FilePath m_executableFile = Utils::FilePath::fromString("openocd");
    QString m_rootScriptsDir;
    QString m_configurationFile;
    QString m_additionalArguments;

    friend class OpenOcdGdbServerProviderConfigWidget;
    friend class OpenOcdGdbServerProviderFactory;
};

// OpenOcdGdbServerProviderFactory

class OpenOcdGdbServerProviderFactory final : public GdbServerProviderFactory
{
    Q_OBJECT

public:
    explicit OpenOcdGdbServerProviderFactory();

    GdbServerProvider *create() final;

    bool canRestore(const QVariantMap &data) const final;
    GdbServerProvider *restore(const QVariantMap &data) final;

    GdbServerProviderConfigWidget *configurationWidget(GdbServerProvider *);
};

// OpenOcdGdbServerProviderConfigWidget

class OpenOcdGdbServerProviderConfigWidget final : public GdbServerProviderConfigWidget
{
    Q_OBJECT

public:
    explicit OpenOcdGdbServerProviderConfigWidget(OpenOcdGdbServerProvider *);

private:
    void startupModeChanged();

    void applyImpl() final;
    void discardImpl() final;

    void setFromProvider();

    HostWidget *m_hostWidget = nullptr;
    Utils::PathChooser *m_executableFileChooser = nullptr;
    Utils::PathChooser *m_rootScriptsDirChooser = nullptr;
    Utils::PathChooser *m_configurationFileChooser = nullptr;
    QLineEdit *m_additionalArgumentsLineEdit = nullptr;
    QPlainTextEdit *m_initCommandsTextEdit = nullptr;
    QPlainTextEdit *m_resetCommandsTextEdit = nullptr;
};

} // namespace Internal
} // namespace BareMetal
