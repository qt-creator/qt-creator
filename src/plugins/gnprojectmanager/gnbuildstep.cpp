// Copyright (C) 2024 The Qt Company Ltd.
// Copyright (C) 2026 BogDan Vatra <bogdan@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "gnbuildstep.h"

#include "gnbuildsystem.h"
#include "gnpluginconstants.h"
#include "gnprojectmanagertr.h"

#include <coreplugin/find/itemviewfind.h>

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/processparameters.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>

#include <QFormLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QRadioButton>

using namespace ProjectExplorer;
using namespace Utils;

namespace GNProjectManager::Internal {

const char TARGETS_KEY[] = "GNProjectManager.BuildStep.BuildTargets";
const char TOOL_ARGUMENTS_KEY[] = "GNProjectManager.BuildStep.AdditionalArguments";

GNBuildStep::GNBuildStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep{bsl, id}
{
    if (m_targetName.isEmpty())
        setBuildTarget(defaultBuildTarget());
    setLowPriority();

    setCommandLineProvider([this] { return command(); });
    setUseEnglishOutput();

    connect(buildSystem(), &BuildSystem::parsingFinished, this, &GNBuildStep::update);
}

QWidget *GNBuildStep::createConfigWidget()
{
    auto widget = new QWidget;
    setDisplayName(Tr::tr("Build", "GNProjectManager::GNBuildStepConfigWidget display name."));

    auto buildTargetsList = new QListWidget(widget);
    buildTargetsList->setMinimumHeight(200);
    buildTargetsList->setFrameShape(QFrame::StyledPanel);
    buildTargetsList->setFrameShadow(QFrame::Raised);

    auto toolArguments = new QLineEdit(widget);
    toolArguments->setText(m_commandArgs);

    auto wrapper = Core::ItemViewFind::
        createSearchableWrapper(buildTargetsList, Core::ItemViewFind::LightColored);

    auto formLayout = new QFormLayout(widget);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(Tr::tr("Ninja arguments:"), toolArguments);
    formLayout->addRow(Tr::tr("Targets:"), wrapper);

    auto updateDetails = [this] {
        ProcessParameters param;
        setupProcessParameters(&param);
        setSummaryText(param.summary(displayName()));
    };

    auto updateTargetList = [this, buildTargetsList, updateDetails] {
        buildTargetsList->clear();
        for (const QString &target : projectTargets()) {
            auto item = new QListWidgetItem(buildTargetsList);
            auto button = new QRadioButton(target);
            connect(button,
                    &QRadioButton::toggled,
                    this,
                    [this, target, updateDetails](bool toggled) {
                        if (toggled) {
                            setBuildTarget(target);
                            updateDetails();
                        }
                    });
            button->setChecked(targetName() == target);
            buildTargetsList->setItemWidget(item, button);
            item->setData(Qt::UserRole, target);
        }
    };

    updateDetails();
    updateTargetList();

    connect(this, &GNBuildStep::commandChanged, this, updateDetails);
    connect(this, &GNBuildStep::targetListChanged, widget, updateTargetList);

    connect(toolArguments, &QLineEdit::textEdited, this, [this, updateDetails](const QString &text) {
        setCommandArgs(text);
        updateDetails();
    });

    connect(buildTargetsList,
            &QListWidget::itemChanged,
            this,
            [this, updateDetails](QListWidgetItem *item) {
                if (item->checkState() == Qt::Checked) {
                    setBuildTarget(item->data(Qt::UserRole).toString());
                    updateDetails();
                }
            });

    return widget;
}

CommandLine GNBuildStep::command()
{
    // GN uses ninja as the build backend
    const Environment env = buildConfiguration()->environment();
    const FilePath ninjaExe = env.searchInPath("ninja");

    if (ninjaExe.isEmpty())
        return {};

    QStringList args;
    args << "-C" << buildDirectory().toFSPathString();

    if (!m_commandArgs.isEmpty())
        args << ProcessArgs::splitArgs(m_commandArgs, HostOsInfo::hostOs());

    // "all" is the default target for ninja, so we don't need to specify it
    if (m_targetName == Constants::Targets::clean) {
        args << "-t" << "clean";
    } else if (m_targetName != Constants::Targets::all && !m_targetName.isEmpty()) {
        args << m_targetName;
    }

    return CommandLine{ninjaExe, args};
}

QStringList GNBuildStep::projectTargets()
{
    return static_cast<GNBuildSystem *>(buildSystem())->targetList();
}

void GNBuildStep::update(bool parsingSuccessful)
{
    if (parsingSuccessful) {
        if (!projectTargets().contains(m_targetName)) {
            m_targetName = defaultBuildTarget();
        }
        emit targetListChanged();
    }
}

QString GNBuildStep::defaultBuildTarget() const
{
    const BuildStepList *const bsl = stepList();
    QTC_ASSERT(bsl, return {});
    const Id parentId = bsl->id();
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        return {Constants::Targets::clean};
    return {Constants::Targets::all};
}

void GNBuildStep::setupOutputFormatter(OutputFormatter *formatter)
{
    m_ninjaParser = new NinjaParser;
    m_ninjaParser->setSourceDirectory(project()->projectDirectory());
    formatter->addLineParser(m_ninjaParser);
    const QList<OutputLineParser *> additionalParsers = kit()->createOutputParsers();
    for (const auto parser : additionalParsers) {
        parser->setRedirectionDetector(m_ninjaParser);
    }
    formatter->addLineParsers(additionalParsers);
    formatter->addSearchDir(processParameters()->effectiveWorkingDirectory());
    AbstractProcessStep::setupOutputFormatter(formatter);

    connect(m_ninjaParser, &NinjaParser::reportProgress, this, [this](int percent) {
        emit progress(percent, QString());
    });
}

void GNBuildStep::setBuildTarget(const QString &targetName)
{
    m_targetName = targetName;
}

void GNBuildStep::setCommandArgs(const QString &args)
{
    m_commandArgs = args.trimmed();
}

void GNBuildStep::toMap(Store &map) const
{
    AbstractProcessStep::toMap(map);
    map.insert(TARGETS_KEY, m_targetName);
    map.insert(TOOL_ARGUMENTS_KEY, m_commandArgs);
}

void GNBuildStep::fromMap(const Store &map)
{
    m_targetName = map.value(TARGETS_KEY).toString();
    m_commandArgs = map.value(TOOL_ARGUMENTS_KEY).toString();
    return AbstractProcessStep::fromMap(map);
}

// GNBuildStepFactory

class GNBuildStepFactory final : public BuildStepFactory
{
public:
    GNBuildStepFactory()
    {
        registerStep<GNBuildStep>(Constants::GN_BUILD_STEP_ID);
        setSupportedProjectType(Constants::GN_PROJECT_ID);
        setDisplayName(Tr::tr("GN Build"));
    }
};

void setupGNBuildStep()
{
    static GNBuildStepFactory theGNBuildStepFactory;
}

} // namespace GNProjectManager::Internal
