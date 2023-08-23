// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "iosbuildstep.h"

#include "iosconstants.h"
#include "iostr.h"

#include <projectexplorer/abstractprocessstep.h>
#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/gcctoolchain.h>
#include <projectexplorer/gnumakeparser.h>
#include <projectexplorer/kitaspects.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <projectexplorer/toolchain.h>

#include <utils/filepath.h>
#include <utils/process.h>
#include <utils/qtcassert.h>

#include <QGridLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QPushButton>

using namespace Core;
using namespace ProjectExplorer;
using namespace Utils;

namespace Ios::Internal {

const char IOS_BUILD_STEP_ID[] = "Ios.IosBuildStep";
const char BUILD_USE_DEFAULT_ARGS_KEY[] = "Ios.IosBuildStep.XcodeArgumentsUseDefault";
const char BUILD_ARGUMENTS_KEY[] = "Ios.IosBuildStep.XcodeArguments";
const char CLEAN_KEY[] = "Ios.IosBuildStep.Clean";

class IosBuildStep final : public AbstractProcessStep
{
public:
    IosBuildStep(BuildStepList *stepList, Id id);

private:
    QWidget *createConfigWidget() final;
    void setBaseArguments(const QStringList &args);
    void setExtraArguments(const QStringList &extraArgs);
    QStringList baseArguments() const;
    QStringList allArguments() const;
    QStringList defaultArguments() const;
    Utils::FilePath buildCommand() const;

    bool init() final;
    void setupOutputFormatter(Utils::OutputFormatter *formatter) final;
    void fromMap(const Store &map) final;
    void toMap(Store &map) const final;

    QStringList m_baseBuildArguments;
    QStringList m_extraArguments;
    bool m_useDefaultArguments = true;
};

QWidget *IosBuildStep::createConfigWidget()
{
    auto widget = new QWidget;

    auto buildArgumentsLabel = new QLabel(Tr::tr("Base arguments:"), widget);

    auto buildArgumentsTextEdit = new QPlainTextEdit(widget);
    buildArgumentsTextEdit->setPlainText(ProcessArgs::joinArgs(baseArguments()));

    auto resetDefaultsButton = new QPushButton(widget);
    resetDefaultsButton->setLayoutDirection(Qt::RightToLeft);
    resetDefaultsButton->setText(Tr::tr("Reset Defaults"));
    resetDefaultsButton->setEnabled(!m_useDefaultArguments);

    auto extraArgumentsLabel = new QLabel(Tr::tr("Extra arguments:"), widget);

    auto extraArgumentsLineEdit = new QLineEdit(widget);
    extraArgumentsLineEdit->setText(ProcessArgs::joinArgs(m_extraArguments));

    auto gridLayout = new QGridLayout(widget);
    gridLayout->addWidget(buildArgumentsLabel, 0, 0, 1, 1);
    gridLayout->addWidget(buildArgumentsTextEdit, 0, 1, 2, 1);
    gridLayout->addWidget(resetDefaultsButton, 1, 2, 1, 1);
    gridLayout->addWidget(extraArgumentsLabel, 2, 0, 1, 1);
    gridLayout->addWidget(extraArgumentsLineEdit, 2, 1, 1, 1);

    setDisplayName(Tr::tr("iOS build", "iOS BuildStep display name."));

    auto updateDetails = [this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        setSummaryText(param.summary(displayName()));
    };

    updateDetails();

    connect(buildArgumentsTextEdit, &QPlainTextEdit::textChanged, this, [=] {
        setBaseArguments(ProcessArgs::splitArgs(buildArgumentsTextEdit->toPlainText(),
                                                HostOsInfo::hostOs()));
        resetDefaultsButton->setEnabled(!m_useDefaultArguments);
        updateDetails();
    });

    connect(resetDefaultsButton, &QAbstractButton::clicked, this, [=] {
        setBaseArguments(defaultArguments());
        buildArgumentsTextEdit->setPlainText(ProcessArgs::joinArgs(baseArguments()));
        resetDefaultsButton->setEnabled(!m_useDefaultArguments);
    });

    connect(extraArgumentsLineEdit, &QLineEdit::editingFinished, this, [=] {
        setExtraArguments(ProcessArgs::splitArgs(extraArgumentsLineEdit->text(),
                                                 HostOsInfo::hostOs()));
    });

    connect(ProjectExplorerPlugin::instance(), &ProjectExplorerPlugin::settingsChanged,
            this, updateDetails);
    connect(target(), &Target::kitChanged,
            this, updateDetails);
    connect(buildConfiguration(), &BuildConfiguration::environmentChanged,
            this, updateDetails);

    return widget;
}

IosBuildStep::IosBuildStep(BuildStepList *stepList, Id id)
    : AbstractProcessStep(stepList, id)
{
    setCommandLineProvider([this] { return CommandLine(buildCommand(), allArguments()); });
    setUseEnglishOutput();

    if (stepList->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN) {
        // If we are cleaning, then build can fail with an error code,
        // but that doesn't mean we should stop the clean queue
        // That is mostly so that rebuild works on an already clean project
        setIgnoreReturnValue(true);
        setExtraArguments(QStringList("clean"));
    }
}

bool IosBuildStep::init()
{
    if (!AbstractProcessStep::init())
        return false;

    ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit());
    if (!tc) {
        emit addTask(Task::compilerMissingTask());
        emitFaultyConfigurationMessage();
        return false;
    }

    return true;
}

void IosBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    formatter->addLineParser(new GnuMakeParser);
    formatter->addLineParsers(kit()->createOutputParsers());
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);
}

void IosBuildStep::toMap(Store &map) const
{
    AbstractProcessStep::toMap(map);

    map.insert(BUILD_ARGUMENTS_KEY, m_baseBuildArguments);
    map.insert(BUILD_USE_DEFAULT_ARGS_KEY, m_useDefaultArguments);

    // Not used anymore since 4.14. But make sure older versions of Creator can read this.
    map.insert(CLEAN_KEY, stepList()->id() == ProjectExplorer::Constants::BUILDSTEPS_CLEAN);
}

void IosBuildStep::fromMap(const Store &map)
{
    QVariant bArgs = map.value(BUILD_ARGUMENTS_KEY);
    m_baseBuildArguments = bArgs.toStringList();
    m_useDefaultArguments = map.value(BUILD_USE_DEFAULT_ARGS_KEY).toBool();

    BuildStep::fromMap(map);
}

QStringList IosBuildStep::allArguments() const
{
    return baseArguments() + m_extraArguments;
}

QStringList IosBuildStep::defaultArguments() const
{
    QStringList res;
    Kit *kit = target()->kit();
    ToolChain *tc = ToolChainKitAspect::cxxToolChain(kit);
    switch (buildConfiguration()->buildType()) {
    case BuildConfiguration::Debug :
        res << "-configuration" << "Debug";
        break;
    case BuildConfiguration::Release :
    case BuildConfiguration::Profile :
        res << "-configuration" << "Release";
        break;
    case BuildConfiguration::Unknown :
        break;
    default:
        qCWarning(iosLog) << "IosBuildStep had an unknown buildType " << buildType();
    }
    if (tc->typeId() == ProjectExplorer::Constants::GCC_TOOLCHAIN_TYPEID
            || tc->typeId() == ProjectExplorer::Constants::CLANG_TOOLCHAIN_TYPEID) {
        auto gtc = static_cast<GccToolChain *>(tc);
        res << gtc->platformCodeGenFlags();
    }
    if (!SysRootKitAspect::sysRoot(kit).isEmpty())
        res << "-sdk" << SysRootKitAspect::sysRoot(kit).toString();
    res << "SYMROOT=" + buildDirectory().toString();
    return res;
}

FilePath IosBuildStep::buildCommand() const
{
    return "xcodebuild"; // add path?
}

void IosBuildStep::setBaseArguments(const QStringList &args)
{
    m_baseBuildArguments = args;
    m_useDefaultArguments = (args == defaultArguments());
}

void IosBuildStep::setExtraArguments(const QStringList &extraArgs)
{
    m_extraArguments = extraArgs;
}

QStringList IosBuildStep::baseArguments() const
{
    if (m_useDefaultArguments)
        return defaultArguments();
    return m_baseBuildArguments;
}

//
// IosBuildStepFactory
//

IosBuildStepFactory::IosBuildStepFactory()
{
    registerStep<IosBuildStep>(IOS_BUILD_STEP_ID);
    setSupportedDeviceTypes({Constants::IOS_DEVICE_TYPE,
                             Constants::IOS_SIMULATOR_TYPE});
    setSupportedStepLists({ProjectExplorer::Constants::BUILDSTEPS_CLEAN,
                           ProjectExplorer::Constants::BUILDSTEPS_BUILD});
    setDisplayName(Tr::tr("xcodebuild"));
}

} // Ios::Internal
