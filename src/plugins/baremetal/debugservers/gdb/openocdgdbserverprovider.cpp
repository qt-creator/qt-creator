// Copyright (C) 2016 Denis Shienkov <denis.shienkov@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "openocdgdbserverprovider.h"

#include <baremetal/baremetalconstants.h>
#include <baremetal/baremetaltr.h>
#include <baremetal/debugserverprovidermanager.h>

#include <utils/fileutils.h>
#include <utils/pathchooser.h>
#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/variablechooser.h>

#include <QComboBox>
#include <QFormLayout>
#include <QLineEdit>
#include <QPlainTextEdit>

using namespace Utils;

namespace BareMetal::Internal {

const char executableFileKeyC[] = "ExecutableFile";
const char rootScriptsDirKeyC[] = "RootScriptsDir";
const char configurationFileKeyC[] = "ConfigurationPath";
const char additionalArgumentsKeyC[] = "AdditionalArguments";

// OpenOcdGdbServerProviderConfigWidget

class OpenOcdGdbServerProvider;

class OpenOcdGdbServerProviderConfigWidget final : public GdbServerProviderConfigWidget
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

// OpenOcdGdbServerProvider

class OpenOcdGdbServerProvider final : public GdbServerProvider
{
public:
    void toMap(Store &data) const final;
    void fromMap(const Store &data) final;

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


OpenOcdGdbServerProvider::OpenOcdGdbServerProvider()
    : GdbServerProvider(Constants::GDBSERVER_OPENOCD_PROVIDER_ID)
{
    setInitCommands(defaultInitCommands());
    setResetCommands(defaultResetCommands());
    setChannel("localhost", 3333);
    setTypeDisplayName(Tr::tr("OpenOCD"));
    setConfigurationWidgetCreator([this] { return new OpenOcdGdbServerProviderConfigWidget(this); });
}

QString OpenOcdGdbServerProvider::defaultInitCommands()
{
    return {"set remote hardware-breakpoint-limit 6\n"
                         "set remote hardware-watchpoint-limit 4\n"
                         "monitor reset halt\n"
                         "load\n"
                         "monitor reset halt\n"};
}

QString OpenOcdGdbServerProvider::defaultResetCommands()
{
    return {"monitor reset halt\n"};
}

QString OpenOcdGdbServerProvider::channelString() const
{
    switch (startupMode()) {
    case StartupOnNetwork:
        // Just return as "host:port" form.
        return GdbServerProvider::channelString();
    case StartupOnPipe: {
        // In the pipe mode need to add quotes to each item of arguments;
        // otherwise running will be stuck.
        CommandLine cmd = command();
        QStringList args = {"|", cmd.executable().toString()};
        for (const QString &a : ProcessArgs::splitArgs(cmd.arguments(), HostOsInfo::hostOs())) {
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
        cmd.addArgs({"-s", m_rootScriptsDir.path()});

    if (!m_configurationFile.isEmpty())
        cmd.addArgs({"-f", m_configurationFile.path()});

    if (!m_additionalArguments.isEmpty())
        cmd.addArgs(m_additionalArguments, CommandLine::Raw);

    return cmd;
}

QSet<GdbServerProvider::StartupMode>
OpenOcdGdbServerProvider::supportedStartupModes() const
{
    return {StartupOnNetwork, StartupOnPipe};
}

bool OpenOcdGdbServerProvider::isValid() const
{
    if (!GdbServerProvider::isValid())
        return false;

    const StartupMode m = startupMode();

    if (m == StartupOnNetwork) {
        if (channel().host().isEmpty())
            return false;
    }

    if (m == StartupOnNetwork || m == StartupOnPipe) {
        if (m_executableFile.isEmpty())
            return false;
    }

    return true;
}

void OpenOcdGdbServerProvider::toMap(Store &data) const
{
    GdbServerProvider::toMap(data);
    data.insert(executableFileKeyC, m_executableFile.toSettings());
    data.insert(rootScriptsDirKeyC, m_rootScriptsDir.toSettings());
    data.insert(configurationFileKeyC, m_configurationFile.toSettings());
    data.insert(additionalArgumentsKeyC, m_additionalArguments);
}

void OpenOcdGdbServerProvider::fromMap(const Store &data)
{
    GdbServerProvider::fromMap(data);
    m_executableFile = FilePath::fromSettings(data.value(executableFileKeyC));
    m_rootScriptsDir = FilePath::fromSettings(data.value(rootScriptsDirKeyC));
    m_configurationFile = FilePath::fromSettings(data.value(configurationFileKeyC));
    m_additionalArguments = data.value(additionalArgumentsKeyC).toString();
}

bool OpenOcdGdbServerProvider::operator==(const IDebugServerProvider &other) const
{
    if (!GdbServerProvider::operator==(other))
        return false;

    const auto p = static_cast<const OpenOcdGdbServerProvider *>(&other);
    return m_executableFile == p->m_executableFile
            && m_rootScriptsDir == p->m_rootScriptsDir
            && m_configurationFile == p->m_configurationFile
            && m_additionalArguments == p->m_additionalArguments;
}

// OpenOcdGdbServerProviderFactory

OpenOcdGdbServerProviderFactory::OpenOcdGdbServerProviderFactory()
{
    setId(Constants::GDBSERVER_OPENOCD_PROVIDER_ID);
    setDisplayName(Tr::tr("OpenOCD"));
    setCreator([] { return new OpenOcdGdbServerProvider; });
}

// OpenOcdGdbServerProviderConfigWidget

OpenOcdGdbServerProviderConfigWidget::OpenOcdGdbServerProviderConfigWidget(
        OpenOcdGdbServerProvider *provider)
    : GdbServerProviderConfigWidget(provider)
{
    Q_ASSERT(provider);

    m_hostWidget = new HostWidget(this);
    m_mainLayout->addRow(Tr::tr("Host:"), m_hostWidget);

    m_executableFileChooser = new Utils::PathChooser;
    m_executableFileChooser->setExpectedKind(Utils::PathChooser::ExistingCommand);
    m_executableFileChooser->setCommandVersionArguments({"--version"});
    m_mainLayout->addRow(Tr::tr("Executable file:"), m_executableFileChooser);

    m_rootScriptsDirChooser = new Utils::PathChooser;
    m_rootScriptsDirChooser->setExpectedKind(Utils::PathChooser::Directory);
    m_mainLayout->addRow(Tr::tr("Root scripts directory:"), m_rootScriptsDirChooser);

    m_configurationFileChooser = new Utils::PathChooser;
    m_configurationFileChooser->setExpectedKind(Utils::PathChooser::File);
    m_configurationFileChooser->setPromptDialogFilter("*.cfg");
    m_mainLayout->addRow(Tr::tr("Configuration file:"), m_configurationFileChooser);

    m_additionalArgumentsLineEdit = new QLineEdit(this);
    m_mainLayout->addRow(Tr::tr("Additional arguments:"), m_additionalArgumentsLineEdit);

    m_initCommandsTextEdit = new QPlainTextEdit(this);
    m_initCommandsTextEdit->setToolTip(defaultInitCommandsTooltip());
    m_mainLayout->addRow(Tr::tr("Init commands:"), m_initCommandsTextEdit);
    m_resetCommandsTextEdit = new QPlainTextEdit(this);
    m_resetCommandsTextEdit->setToolTip(defaultResetCommandsTooltip());
    m_mainLayout->addRow(Tr::tr("Reset commands:"), m_resetCommandsTextEdit);

    addErrorLabel();
    setFromProvider();

    const auto chooser = new VariableChooser(this);
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

    connect(m_startupModeComboBox, &QComboBox::currentIndexChanged,
            this, &OpenOcdGdbServerProviderConfigWidget::startupModeChanged);
}

void OpenOcdGdbServerProviderConfigWidget::apply()
{
    const auto p = static_cast<OpenOcdGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    p->setChannel(m_hostWidget->channel());
    p->m_executableFile = m_executableFileChooser->filePath();
    p->m_rootScriptsDir = m_rootScriptsDirChooser->filePath();
    p->m_configurationFile = m_configurationFileChooser->filePath();
    p->m_additionalArguments = m_additionalArgumentsLineEdit->text();
    p->setInitCommands(m_initCommandsTextEdit->toPlainText());
    p->setResetCommands(m_resetCommandsTextEdit->toPlainText());
    GdbServerProviderConfigWidget::apply();
}

void OpenOcdGdbServerProviderConfigWidget::discard()
{
    setFromProvider();
    GdbServerProviderConfigWidget::discard();
}

void OpenOcdGdbServerProviderConfigWidget::startupModeChanged()
{
    const GdbServerProvider::StartupMode m = startupMode();
    const bool isNetwork = m != GdbServerProvider::StartupOnPipe;
    m_hostWidget->setVisible(isNetwork);
    m_mainLayout->labelForField(m_hostWidget)->setVisible(isNetwork);
}

void OpenOcdGdbServerProviderConfigWidget::setFromProvider()
{
    const auto p = static_cast<OpenOcdGdbServerProvider *>(m_provider);
    Q_ASSERT(p);

    const QSignalBlocker blocker(this);
    startupModeChanged();
    m_hostWidget->setChannel(p->channel());
    m_executableFileChooser->setFilePath(p->m_executableFile);
    m_rootScriptsDirChooser->setFilePath(p->m_rootScriptsDir);
    m_configurationFileChooser->setFilePath(p->m_configurationFile);
    m_additionalArgumentsLineEdit->setText(p->m_additionalArguments);
    m_initCommandsTextEdit->setPlainText(p->initCommands());
    m_resetCommandsTextEdit->setPlainText(p->resetCommands());
}

} // BareMetal::Internal
