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

#include "baremetalconstants.h"

#include "gdbserverprovidermanager.h"
#include "openocdgdbserverprovider.h"

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <coreplugin/variablechooser.h>

#include <QComboBox>
#include <QFileInfo>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>

using namespace Utils;

namespace BareMetal {
namespace Internal {

const char executableFileKeyC[] = "BareMetal.OpenOcdGdbServerProvider.ExecutableFile";
const char rootScriptsDirKeyC[] = "BareMetal.OpenOcdGdbServerProvider.RootScriptsDir";
const char configurationFileKeyC[] = "BareMetal.OpenOcdGdbServerProvider.ConfigurationPath";
const char additionalArgumentsKeyC[] = "BareMetal.OpenOcdGdbServerProvider.AdditionalArguments";

// OpenOcdGdbServerProvider

OpenOcdGdbServerProvider::OpenOcdGdbServerProvider()
    : GdbServerProvider(QLatin1String(Constants::OPENOCD_PROVIDER_ID))
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setDefaultChannel("localhost", 3333);
    setSettingsKeyBase("BareMetal.OpenOcdGdbServerProvider");
    setTypeDisplayName(OpenOcdGdbServerProviderFactory::tr("OpenOCD"));
}

QString OpenOcdGdbServerProvider::defaultInitCommands()
{
    return QLatin1String("set remote hardware-breakpoint-limit 6\n"
                         "set remote hardware-watchpoint-limit 4\n"
                         "monitor reset halt\n"
                         "load\n"
                         "monitor reset halt\n");
}

QString OpenOcdGdbServerProvider::defaultResetCommands()
{
    return QLatin1String("monitor reset halt\n");
}

QString OpenOcdGdbServerProvider::channelString() const
{
    switch (startupMode()) {
    case NoStartup:
        // fallback
    case StartupOnNetwork:
        // Just return as "host:port" form.
        return GdbServerProvider::channelString();
    case StartupOnPipe: {
        // In the pipe mode need to add quotes to each item of arguments;
        // otherwise running will be stuck.
        CommandLine cmd = command();
        QStringList args = {"|", cmd.executable().toString()};
        for (const QString &a : QtcProcess::splitArgs(cmd.arguments())) {
            if (a.startsWith('\"') && a.endsWith('\"'))
                args << a;
            else
                args << ('\"' + a + '\"');
        }
        return args.join(' ');
    }
    default: // wrong
        return {};
    }
}

CommandLine OpenOcdGdbServerProvider::command() const
{
    CommandLine cmd{m_executableFile};

    cmd.addArg("-c");
    if (startupMode() == StartupOnPipe)
        cmd.addArg("gdb_port pipe");
    else
        cmd.addArg("gdb_port " + QString::number(channel().port()));

    if (!m_rootScriptsDir.isEmpty())
        cmd.addArgs({"-s", m_rootScriptsDir});

    if (!m_configurationFile.isEmpty())
        cmd.addArgs({"-f", m_configurationFile});

    if (!m_additionalArguments.isEmpty())
        cmd.addArgs(m_additionalArguments, CommandLine::Raw);

    return cmd;
}

bool OpenOcdGdbServerProvider::canStartupMode(StartupMode m) const
{
    return m == NoStartup || m == StartupOnNetwork || m == StartupOnPipe;
}

bool OpenOcdGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    const StartupMode m = startupMode();

    if (m == NoStartup || m == StartupOnNetwork) {
        if (channel().host().isEmpty())
            return false;
    }

    if (m == StartupOnNetwork || m == StartupOnPipe) {
        if (m_executableFile.isEmpty())
            return false;
    }

    return true;
}

GdbServerProvider *OpenOcdGdbServerProvider::clone() const
{
    return new OpenOcdGdbServerProvider(*this);
}

QVariantMap OpenOcdGdbServerProvider::toMap() const
{
    QVariantMap data = GdbServerProvider::toMap();
    data.insert(QLatin1String(executableFileKeyC), m_executableFile.toVariant());
    data.insert(QLatin1String(rootScriptsDirKeyC), m_rootScriptsDir);
    data.insert(QLatin1String(configurationFileKeyC), m_configurationFile);
    data.insert(QLatin1String(additionalArgumentsKeyC), m_additionalArguments);
    return data;
}

bool OpenOcdGdbServerProvider::fromMap(const QVariantMap &data)
{
    if (!GdbServerProvider::fromMap(data))
        return false;

    m_executableFile = FilePath::fromVariant(data.value(QLatin1String(executableFileKeyC)));
    m_rootScriptsDir = data.value(QLatin1String(rootScriptsDirKeyC)).toString();
    m_configurationFile = data.value(QLatin1String(configurationFileKeyC)).toString();
    m_additionalArguments = data.value(QLatin1String(additionalArgumentsKeyC)).toString();
    return true;
}

bool OpenOcdGdbServerProvider::operator==(const GdbServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const OpenOcdGdbServerProvider *>(&other);
    return m_executableFile == p->m_executableFile
            && m_rootScriptsDir == p->m_rootScriptsDir
            && m_configurationFile == p->m_configurationFile
            && m_additionalArguments == p->m_additionalArguments;
}

GdbServerProviderConfigWidget *OpenOcdGdbServerProvider::configurationWidget()
{
    return new OpenOcdGdbServerProviderConfigWidget(this);
}

// OpenOcdGdbServerProviderFactory

OpenOcdGdbServerProviderFactory::OpenOcdGdbServerProviderFactory()
{
    setId(QLatin1String(Constants::OPENOCD_PROVIDER_ID));
    setDisplayName(tr("OpenOCD"));
}

GdbServerProvider *OpenOcdGdbServerProviderFactory::create()
{
    return new OpenOcdGdbServerProvider;
}

bool OpenOcdGdbServerProviderFactory::canRestore(const QVariantMap &data) const
{
    const QString id = idFromMap(data);
    return id.startsWith(QLatin1String(Constants::OPENOCD_PROVIDER_ID)
                         + QLatin1Char(':'));
}

GdbServerProvider *OpenOcdGdbServerProviderFactory::restore(const QVariantMap &data)
{
    const auto p = new OpenOcdGdbServerProvider;
    const auto updated = data;
    if (p->fromMap(updated))
        return p;
    delete p;
    return nullptr;
}

// OpenOcdGdbServerProviderConfigWidget

OpenOcdGdbServerProviderConfigWidget::OpenOcdGdbServerProviderConfigWidget(
        OpenOcdGdbServerProvider *p)
    : GdbServerProviderConfigWidget(p)
{
    Q_ASSERT(p);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(tr("Host:"), m_hostWidget);

    m_executableFileChooser = new Utils::PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_executableFileChooser->setCommandVersionArguments(QStringList(QLatin1String("--version")));
    m_mainLayout->addRow(tr("Executable file:"), m_executableFileChooser);

    m_rootScriptsDirChooser = new Utils::PathChooser;
    m_rootScriptsDirChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_mainLayout->addRow(tr("Root scripts directory:"), m_rootScriptsDirChooser);

    m_configurationFileChooser = new Utils::PathChooser;
    m_configurationFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_configurationFileChooser->setPromptDialogFilter(QLatin1String("*.cfg"));
    m_mainLayout->addRow(tr("Configuration file:"), m_configurationFileChooser);

    m_additionalArgumentsLineEdit = new QLineEdit(this);
    m_mainLayout->addRow(tr("Additional arguments:"), m_additionalArgumentsLineEdit);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(tr("Reset commands:"), m_resetCommandsTextEdit);

    addErrorLabel();
    setFromProvider();

    const auto chooser = new Core::VariableChooser(this);
    chooser->addSupportedWidget(m_initCommandsTextEdit);
    chooser->addSupportedWidget(m_resetCommandsTextEdit);

    connect(m_hostWidget, &HostWidget::dataChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_executableFileChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_rootScriptsDirChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_configurationFileChooser, &Utils::PathChooser::rawPathChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_additionalArgumentsLineEdit, &QLineEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_initCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);
    connect(m_resetCommandsTextEdit, &QPlainTextEdit::textChanged,
            this, &GdbServerProviderConfigWidget::dirty);

    connect(m_startupModeComboBox, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &OpenOcdGdbServerProviderConfigWidget::startupModeChanged);
}

void OpenOcdGdbServerProviderConfigWidget::startupModeChanged()
{
    const GdbServerProvider::StartupMode m = startupMode();
    const bool isStartup = m != GdbServerProvider::NoStartup;
    m_executableFileChooser->setVisible(isStartup);
    m_mainLayout->labelForField(m_executableFileChooser)->setVisible(isStartup);
    m_rootScriptsDirChooser->setVisible(isStartup);
    m_mainLayout->labelForField(m_rootScriptsDirChooser)->setVisible(isStartup);
    m_configurationFileChooser->setVisible(isStartup);
    m_mainLayout->labelForField(m_configurationFileChooser)->setVisible(isStartup);
    m_additionalArgumentsLineEdit->setVisible(isStartup);
    m_mainLayout->labelForField(m_additionalArgumentsLineEdit)->setVisible(isStartup);
    const bool isNetwork = m != GdbServerProvider::StartupOnPipe;
    m_hostWidget->setVisible(isNetwork);
    m_mainLayout->labelForField(m_hostWidget)->setVisible(isNetwork);
}

void OpenOcdGdbServerProviderConfigWidget::applyImpl()
{
    const auto p = static_cast<OpenOcdGdbServerProvider *>(provider());
    Q_ASSERT(p);

    p->setChannel(m_hostWidget->channel());
    p->m_executableFile = m_executableFileChooser->fileName();
    p->m_rootScriptsDir = m_rootScriptsDirChooser->fileName().toString();
    p->m_configurationFile = m_configurationFileChooser->fileName().toString();
    p->m_additionalArguments = m_additionalArgumentsLineEdit->text();
    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
}

void OpenOcdGdbServerProviderConfigWidget::discardImpl()
{
    setFromProvider();
}

void OpenOcdGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<OpenOcdGdbServerProvider *>(provider());
    Q_ASSERT(p);

    const QSignalBlocker blocker(this);
    startupModeChanged();
    m_hostWidget->setChannel(p->channel());
    m_executableFileChooser->setFileName(p->m_executableFile);
    m_rootScriptsDirChooser->setFileName(Utils::FilePath::fromString(p->m_rootScriptsDir));
    m_configurationFileChooser->setFileName(Utils::FilePath::fromString(p->m_configurationFile));
    m_additionalArgumentsLineEdit->setText(p->m_additionalArguments);
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
}

} // namespace Internal
} // namespace ProjectExplorer
