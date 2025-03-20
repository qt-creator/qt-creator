// Copyright (C) 2020 Alexis Jeandet.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mesonbuildstep.h"

#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "mesonprojectmanagertr.h"
#include "mesonoutputparser.h"
#include "settings.h"
#include "toolkitaspectwidget.h"

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

namespace MesonProjectManager::Internal {

const char TARGETS_KEY[] = "MesonProjectManager.BuildStep.BuildTargets";
const char TOOL_ARGUMENTS_KEY[] = "MesonProjectManager.BuildStep.AdditionalArguments";

MesonBuildStep::MesonBuildStep(BuildStepList *bsl, Id id)
    : AbstractProcessStep{bsl, id}
{
    if (m_targetName.isEmpty())
        setBuildTarget(defaultBuildTarget());
    setLowPriority();

    setCommandLineProvider([this] { return command(); });
    setUseEnglishOutput();

    connect(buildSystem(), &BuildSystem::parsingFinished, this, &MesonBuildStep::update);
    connect(&settings().verboseBuild, &BaseAspect::changed,
            this, &MesonBuildStep::commandChanged);
}

QWidget *MesonBuildStep::createConfigWidget()
{
    auto widget = new QWidget;
    setDisplayName(Tr::tr("Build", "MesonProjectManager::MesonBuildStepConfigWidget display name."));

    auto buildTargetsList = new QListWidget(widget);
    buildTargetsList->setMinimumHeight(200);
    buildTargetsList->setFrameShape(QFrame::StyledPanel);
    buildTargetsList->setFrameShadow(QFrame::Raised);

    auto toolArguments = new QLineEdit(widget);
    toolArguments->setText(m_commandArgs);

    auto wrapper = Core::ItemViewFind::createSearchableWrapper(buildTargetsList,
                                                               Core::ItemViewFind::LightColored);

    auto formLayout = new QFormLayout(widget);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(Tr::tr("Tool arguments:"), toolArguments);
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
            connect(button, &QRadioButton::toggled,
                    this, [this, target, updateDetails](bool toggled) {
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

    connect(this, &MesonBuildStep::commandChanged, this, updateDetails);

    connect(this, &MesonBuildStep::targetListChanged, widget, updateTargetList);

    connect(toolArguments, &QLineEdit::textEdited, this, [this, updateDetails](const QString &text) {
        setCommandArgs(text);
        updateDetails();
    });

    connect(buildTargetsList, &QListWidget::itemChanged,
            this, [this, updateDetails](QListWidgetItem *item) {
        if (item->checkState() == Qt::Checked) {
            setBuildTarget(item->data(Qt::UserRole).toString());
            updateDetails();
        }
    });

    return widget;
}

CommandLine MesonBuildStep::command()
{
    if (auto tool = MesonToolKitAspect::mesonTool(kit()))
    {
        return tool->compile(buildDirectory(), m_targetName, settings().verboseBuild()).command;
    }
    return {};
}

QStringList MesonBuildStep::projectTargets()
{
    return static_cast<MesonBuildSystem *>(buildSystem())->targetList();
}

void MesonBuildStep::update(bool parsingSuccessful)
{
    if (parsingSuccessful) {
        if (!projectTargets().contains(m_targetName)) {
            m_targetName = defaultBuildTarget();
        }
        emit targetListChanged();
    }
}

QString MesonBuildStep::defaultBuildTarget() const
{
    const ProjectExplorer::BuildStepList *const bsl = stepList();
    QTC_ASSERT(bsl, return {});
    const Utils::Id parentId = bsl->id();
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_CLEAN)
        return {Constants::Targets::clean};
    if (parentId == ProjectExplorer::Constants::BUILDSTEPS_DEPLOY)
        return {Constants::Targets::install};
    return {Constants::Targets::all};
}

void MesonBuildStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    auto mesonOutputParser = new MesonOutputParser;
    mesonOutputParser->setSourceDirectory(project()->projectDirectory());
    formatter->addLineParser(mesonOutputParser);
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

void MesonBuildStep::setBuildTarget(const QString &targetName)
{
    m_targetName = targetName;
}

void MesonBuildStep::setCommandArgs(const QString &args)
{
    m_commandArgs = args.trimmed();
}

void MesonBuildStep::toMap(Store &map) const
{
    AbstractProcessStep::toMap(map);
    map.insert(TARGETS_KEY, m_targetName);
    map.insert(TOOL_ARGUMENTS_KEY, m_commandArgs);
}

void MesonBuildStep::fromMap(const Store &map)
{
    m_targetName = map.value(TARGETS_KEY).toString();
    m_commandArgs = map.value(TOOL_ARGUMENTS_KEY).toString();
    return AbstractProcessStep::fromMap(map);
}

// MesonBuildStepFactory

class MesonBuildStepFactory final : public BuildStepFactory
{
public:
    MesonBuildStepFactory()
    {
        registerStep<MesonBuildStep>(Constants::MESON_BUILD_STEP_ID);
        setSupportedProjectType(Constants::Project::ID);
        setDisplayName(Tr::tr("Meson Build"));
    }
};

void setupMesonBuildStep()
{
    static MesonBuildStepFactory theMesonBuildStepFactory;
}

} // MesonProjectManager::Internal
