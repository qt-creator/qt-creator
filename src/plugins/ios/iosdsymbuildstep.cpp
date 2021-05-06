/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "iosdsymbuildstep.h"

#include "iosconstants.h"
#include "iosconfigurations.h"
#include "iosrunconfiguration.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/target.h>
#include <projectexplorer/project.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/kitinformation.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/projectexplorerconstants.h>

#include <qtsupport/qtkitinformation.h>
#include <qtsupport/qtparser.h>

#include <utils/stringutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcprocess.h>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios {
namespace Internal {

static const char USE_DEFAULT_ARGS_PARTIAL_KEY[] = ".ArgumentsUseDefault";
static const char COMMAND_PARTIAL_KEY[] = ".Command";
static const char ARGUMENTS_PARTIAL_KEY[] = ".Arguments";
static const char CLEAN_PARTIAL_KEY[] = ".Clean";

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

QVariantMap IosDsymBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());

    map.insert(id().withSuffix(ARGUMENTS_PARTIAL_KEY).toString(),
               arguments());
    map.insert(id().withSuffix(USE_DEFAULT_ARGS_PARTIAL_KEY).toString(),
               isDefault());
    map.insert(id().withSuffix(CLEAN_PARTIAL_KEY).toString(), m_clean);
    map.insert(id().withSuffix(COMMAND_PARTIAL_KEY).toString(), command().toVariant());
    return map;
}

bool IosDsymBuildStep::fromMap(const QVariantMap &map)
{
    QVariant bArgs = map.value(id().withSuffix(ARGUMENTS_PARTIAL_KEY).toString());
    m_arguments = bArgs.toStringList();
    bool useDefaultArguments = map.value(
                id().withSuffix(USE_DEFAULT_ARGS_PARTIAL_KEY).toString()).toBool();
    m_clean = map.value(id().withSuffix(CLEAN_PARTIAL_KEY).toString(), m_clean).toBool();
    m_command = FilePath::fromVariant(map.value(id().withSuffix(COMMAND_PARTIAL_KEY).toString()));
    if (useDefaultArguments) {
        m_command = defaultCommand();
        m_arguments = defaultArguments();
    }

    return BuildStep::fromMap(map);
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
    QTC_ASSERT(runConf, return QStringList("echo"));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(".dSYM");
    return QStringList({"rm", "-rf", dsymPath});
}

QStringList IosDsymBuildStep::defaultCmdList() const
{
    QString dsymutilCmd = "dsymutil";
    const Utils::FilePath dsymUtilPath = IosConfigurations::developerPath()
            .pathAppended("Toolchains/XcodeDefault.xctoolchain/usr/bin/dsymutil");
    if (dsymUtilPath.exists())
        dsymutilCmd = dsymUtilPath.toUserOutput();
    auto runConf = qobject_cast<const IosRunConfiguration *>(target()->activeRunConfiguration());
    QTC_ASSERT(runConf, return QStringList("echo"));
    QString dsymPath = runConf->bundleDirectory().toUserOutput();
    dsymPath.chop(4);
    dsymPath.append(".dSYM");
    return QStringList({dsymutilCmd, "-o", dsymPath, runConf->localExecutable().toUserOutput()});
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

    auto commandLabel = new QLabel(tr("Command:"), widget);

    auto commandLineEdit = new QLineEdit(widget);
    commandLineEdit->setText(command().toString());

    auto argumentsTextEdit = new QPlainTextEdit(widget);
    argumentsTextEdit->setPlainText(Utils::ProcessArgs::joinArgs(arguments()));

    auto argumentsLabel = new QLabel(tr("Arguments:"), widget);

    auto resetDefaultsButton = new QPushButton(tr("Reset to Default"), widget);
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
        setArguments(Utils::ProcessArgs::splitArgs(argumentsTextEdit->toPlainText()));
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

//
// IosDsymBuildStepFactory
//

IosDsymBuildStepFactory::IosDsymBuildStepFactory()
{
    registerStep<IosDsymBuildStep>(Constants::IOS_DSYM_BUILD_STEP_ID);
    setSupportedDeviceTypes({Constants::IOS_DEVICE_TYPE,
                             Constants::IOS_SIMULATOR_TYPE});
    setDisplayName("dsymutil");
}

} // namespace Internal
} // namespace Ios
