// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosdsymbuildstep.h"

#include "iosconstants.h"
#include "iosconfigurations.h"
#include "iosrunconfiguration.h"
#include "iostr.h"

#include <extensionsystem/pluginmanager.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <qtsupport/qtkitaspect.h>
#include <qtsupport/qtparser.h>

#include <utils/process.h>
#include <utils/qtcassert.h>
#include <utils/stringutils.h>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios::Internal {

const char USE_DEFAULT_ARGS_PARTIAL_KEY[] = ".ArgumentsUseDefault";
const char COMMAND_PARTIAL_KEY[] = ".Command";
const char ARGUMENTS_PARTIAL_KEY[] = ".Arguments";
const char CLEAN_PARTIAL_KEY[] = ".Clean";

class IosDsymBuildStep : public AbstractProcessStep
{
public:
    IosDsymBuildStep(BuildStepList *parent, Id id);

    QWidget *createConfigWidget() override;
    void setArguments(const QStringList &args);
    QStringList arguments() const;
    QStringList defaultArguments() const;
    FilePath defaultCommand() const;
    FilePath command() const;
    void setCommand(const FilePath &command);
    bool isDefault() const;

private:
    void setupOutputFormatter(OutputFormatter *formatter) override;
    void toMap(Store &map) const override;
    void fromMap(const Store &map) override;

    QStringList defaultCleanCmdList() const;
    QStringList defaultCmdList() const;

    QStringList m_arguments;
    FilePath m_command;
    bool m_clean;
};

IosDsymBuildStep::IosDsymBuildStep(BuildStepList *parent, Id id) :
    AbstractProcessStep(parent, id),
    m_clean(parent->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
{
    setCommandLineProvider([this] { return CommandLine(command(), arguments()); });
    setUseEnglishOutput();

    // If we are cleaning, then build can fail with an error code, but that doesn't mean
    // we should stop the clean queue
    // That is mostly so that rebuild works on an already clean project
    setIgnoreReturnValue(m_clean);
}

void IosDsymBuildStep::toMap(Store &map) const
{
    AbstractProcessStep::toMap(map);

    map.insert(id().toKey() + ARGUMENTS_PARTIAL_KEY, arguments());
    map.insert(id().toKey() + USE_DEFAULT_ARGS_PARTIAL_KEY, isDefault());
    map.insert(id().toKey() + CLEAN_PARTIAL_KEY, m_clean);
    map.insert(id().toKey() + COMMAND_PARTIAL_KEY, command().toSettings());
}

void IosDsymBuildStep::fromMap(const Store &map)
{
    QVariant bArgs = map.value(id().toKey() + ARGUMENTS_PARTIAL_KEY);
    m_arguments = bArgs.toStringList();
    bool useDefaultArguments = map.value(id().toKey() + USE_DEFAULT_ARGS_PARTIAL_KEY).toBool();
    m_clean = map.value(id().toKey() + CLEAN_PARTIAL_KEY, m_clean).toBool();
    m_command = FilePath::fromSettings(map.value(id().toKey() + COMMAND_PARTIAL_KEY));
    if (useDefaultArguments) {
        m_command = defaultCommand();
        m_arguments = defaultArguments();
    }

    BuildStep::fromMap(map);
}

QStringList IosDsymBuildStep::defaultArguments() const
{
    if (m_clean)
        return defaultCleanCmdList().mid(1);
    return defaultCmdList().mid(1);
}

FilePath IosDsymBuildStep::defaultCommand() const
{
    if (m_clean)
        return FilePath::fromString(defaultCleanCmdList().at(0));
    else
        return FilePath::fromString(defaultCmdList().at(0));
}

QStringList IosDsymBuildStep::defaultCleanCmdList() const
{
    auto runConf = qobject_cast<IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return {"echo"});
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(".dSYM");
    return {"rm", "-rf", dsymPath};
}

QStringList IosDsymBuildStep::defaultCmdList() const
{
    QString dsymutilCmd = "dsymutil";
    const Utils::FilePath dsymUtilPath = IosConfigurations::developerPath()
            .pathAppended("Toolchains/XcodeDefault.xctoolchain/usr/bin/dsymutil");
    if (dsymUtilPath.exists())
        dsymutilCmd = dsymUtilPath.toUserOutput();
    auto runConf = qobject_cast<const IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return {"echo"});
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(".dSYM");
    return {dsymutilCmd, "-o", dsymPath, runConf->localExecutable().toUserOutput()};
}

FilePath IosDsymBuildStep::command() const
{
    if (m_command.isEmpty())
        return defaultCommand();
    return m_command;
}

void IosDsymBuildStep::setCommand(const FilePath &command)
{
    if (command == m_command)
        return;
    if (command.isEmpty() || command == defaultCommand()) {
        if (arguments() == defaultArguments())
            m_command.clear();
        else
            m_command = defaultCommand();
    } else if (m_command.isEmpty()) {
        m_arguments = defaultArguments();
        m_command = command;
    } else {
        m_command = command;
    }
}

bool IosDsymBuildStep::isDefault() const
{
    return arguments() == defaultArguments() && command() == defaultCommand();
}

void IosDsymBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->setLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

void IosDsymBuildStep::setArguments(const QStringList &args)
{
    if (arguments() == args)
        return;
    if (args == defaultArguments() && command() == defaultCommand())
        m_command.clear();
    else {
        if (m_command.isEmpty())
            m_command = defaultCommand();
        m_arguments = args;
    }
}

QStringList IosDsymBuildStep::arguments() const
{
    if (m_command.isEmpty())
        return defaultArguments();
    return m_arguments;
}

QWidget *IosDsymBuildStep::createConfigWidget()
{
    auto widget = new QWidget;

    auto commandLabel = new QLabel(Tr::tr("Command:"), widget);

    auto commandLineEdit = new QLineEdit(widget);
    commandLineEdit->setText(command().toString());

    auto argumentsTextEdit = new QPlainTextEdit(widget);
    argumentsTextEdit->setPlainText(Utils::ProcessArgs::joinArgs(arguments()));

    auto argumentsLabel = new QLabel(Tr::tr("Arguments:"), widget);

    auto resetDefaultsButton = new QPushButton(Tr::tr("Reset to Default"), widget);
    resetDefaultsButton->setLayoutDirection(Qt::RightToLeft);
    resetDefaultsButton->setEnabled(!isDefault());

    auto gridLayout = new QGridLayout(widget);
    gridLayout->addWidget(commandLabel, 0, 0, 1, 1);
    gridLayout->addWidget(commandLineEdit, 0, 2, 1, 1);
    gridLayout->addWidget(argumentsLabel, 1, 0, 1, 1);
    gridLayout->addWidget(argumentsTextEdit, 1, 2, 2, 1);
    gridLayout->addWidget(resetDefaultsButton, 2, 3, 1, 1);

    auto updateDetails = [this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        setSummaryText(param.summary(displayName()));
    };

    updateDetails();

    connect(argumentsTextEdit, &QPlainTextEdit::textChanged, this,
            [this, argumentsTextEdit, resetDefaultsButton, updateDetails] {
        setArguments(ProcessArgs::splitArgs(argumentsTextEdit->toPlainText(),
                                            HostOsInfo::hostOs()));
        resetDefaultsButton->setEnabled(!isDefault());
        updateDetails();
    });

    connect(commandLineEdit, &QLineEdit::editingFinished, this,
            [this, commandLineEdit, resetDefaultsButton, updateDetails] {
        setCommand(FilePath::fromString(commandLineEdit->text()));
        resetDefaultsButton->setEnabled(!isDefault());
        updateDetails();
    });

    connect(resetDefaultsButton, &QAbstractButton::clicked, this,
            [this, commandLineEdit, resetDefaultsButton, argumentsTextEdit, updateDetails] {
        setCommand(defaultCommand());
        setArguments(defaultArguments());
        commandLineEdit->setText(command().toString());
        argumentsTextEdit->setPlainText(Utils::ProcessArgs::joinArgs(arguments()));
        resetDefaultsButton->setEnabled(!isDefault());
        updateDetails();
    });

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, updateDetails);
    connect(target(), &Target::kitChanged,
            this, updateDetails);
    connect(buildConfiguration(), &BuildConfiguration::enabledChanged,
            this, updateDetails);

    return widget;
}

// IosDsymBuildStepFactory

IosDsymBuildStepFactory::IosDsymBuildStepFactory()
{
    registerStep<IosDsymBuildStep>(Constants::IOS_DSYM_BUILD_STEP_ID);
    setSupportedDeviceTypes({Constants::IOS_DEVICE_TYPE,
                             Constants::IOS_SIMULATOR_TYPE});
    setDisplayName("dsymutil");
}

} // Ios::Internal
