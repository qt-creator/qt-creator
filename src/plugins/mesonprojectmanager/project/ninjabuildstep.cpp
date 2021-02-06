/****************************************************************************
**
** Copyright (C) 2020 Alexis Jeandet.
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

#include "ninjabuildstep.h"

#include "mesonbuildconfiguration.h"
#include "mesonbuildsystem.h"
#include "mesonpluginconstants.h"
#include "outputparsers/mesonoutputparser.h"
#include "settings/general/settings.h"
#include "settings/tools/kitaspect/ninjatoolkitaspect.h"

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

namespace MesonProjectManager {
namespace Internal {

const char TARGETS_KEY[] = "MesonProjectManager.BuildStep.BuildTargets";
const char TOOL_ARGUMENTS_KEY[] = "MesonProjectManager.BuildStep.AdditionalArguments";

NinjaBuildStep::NinjaBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id)
    : ProjectExplorer::AbstractProcessStep{bsl, id}
{
    if (m_targetName.isEmpty())
        setBuildTarget(defaultBuildTarget());
    setLowPriority();

    setCommandLineProvider([this] { return command(); });
    setUseEnglishOutput();

    connect(target(), &ProjectExplorer::Target::parsingFinished, this, &NinjaBuildStep::update);
    connect(Settings::instance(),
            &Settings::verboseNinjaChanged,
            this,
            &NinjaBuildStep::commandChanged);
}

QWidget *NinjaBuildStep::createConfigWidget()
{
    auto widget = new QWidget;
    setDisplayName(tr("Build", "MesonProjectManager::MesonBuildStepConfigWidget display name."));

    auto buildTargetsList = new QListWidget(widget);
    buildTargetsList->setMinimumHeight(200);
    buildTargetsList->setFrameShape(QFrame::StyledPanel);
    buildTargetsList->setFrameShadow(QFrame::Raised);

    auto toolArguments = new QLineEdit(widget);

    auto wrapper = Core::ItemViewFind::createSearchableWrapper(buildTargetsList,
                                                               Core::ItemViewFind::LightColored);

    auto formLayout = new QFormLayout(widget);
    formLayout->setFieldGrowthPolicy(QFormLayout::ExpandingFieldsGrow);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->addRow(tr("Tool arguments:"), toolArguments);
    formLayout->addRow(tr("Targets:"), wrapper);

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

    connect(this, &NinjaBuildStep::commandChanged, this, updateDetails);

    connect(this, &NinjaBuildStep::targetListChanged, widget, updateTargetList);

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

// --verbose is only supported since
// https://github.com/ninja-build/ninja/commit/bf7517505ad1def03e13bec2b4131399331bc5c4
// TODO check when to switch back to --verbose
Utils::CommandLine NinjaBuildStep::command()
{
    Utils::CommandLine cmd = [this] {
        auto tool = NinjaToolKitAspect::ninjaTool(kit());
        if (tool)
            return Utils::CommandLine{tool->exe()};
        return Utils::CommandLine{};
    }();
    if (!m_commandArgs.isEmpty())
        cmd.addArgs(m_commandArgs, Utils::CommandLine::RawType::Raw);
    if (Settings::verboseNinja())
        cmd.addArg("-v");
    cmd.addArg(m_targetName);
    return cmd;
}

QStringList NinjaBuildStep::projectTargets()
{
    return static_cast<MesonBuildSystem *>(buildSystem())->targetList();
}

void NinjaBuildStep::update(bool parsingSuccessful)
{
    if (parsingSuccessful) {
        if (!projectTargets().contains(m_targetName)) {
            m_targetName = defaultBuildTarget();
        }
        emit targetListChanged();
    }
}

QString NinjaBuildStep::defaultBuildTarget() const
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

void NinjaBuildStep::doRun()
{
    AbstractProcessStep::doRun();
}

void NinjaBuildStep::setupOutputFormatter(Utils::OutputFormatter *formatter)
{
    auto mesonOutputParser = new MesonOutputParser;
    mesonOutputParser->setSourceDirectory(project()->projectDirectory());
    formatter->addLineParser(mesonOutputParser);
    m_ninjaParser = new NinjaParser;
    m_ninjaParser->setSourceDirectory(project()->projectDirectory());
    formatter->addLineParser(m_ninjaParser);
    auto additionalParsers = kit()->createOutputParsers();
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

MesonBuildStepFactory::MesonBuildStepFactory()
{
    registerStep<NinjaBuildStep>(Constants::MESON_BUILD_STEP_ID);
    setSupportedProjectType(Constants::Project::ID);
    setDisplayName(NinjaBuildStep::tr("Meson Build"));
}

void MesonProjectManager::Internal::NinjaBuildStep::setBuildTarget(const QString &targetName)
{
    m_targetName = targetName;
}

void NinjaBuildStep::setCommandArgs(const QString &args)
{
    m_commandArgs = args.trimmed();
}

QVariantMap NinjaBuildStep::toMap() const
{
    QVariantMap map(AbstractProcessStep::toMap());
    map.insert(TARGETS_KEY, m_targetName);
    map.insert(TOOL_ARGUMENTS_KEY, m_commandArgs);
    return map;
}

bool NinjaBuildStep::fromMap(const QVariantMap &map)
{
    m_targetName = map.value(TARGETS_KEY).toString();
    m_commandArgs = map.value(TOOL_ARGUMENTS_KEY).toString();
    return AbstractProcessStep::fromMap(map);
}

} // namespace Internal
} // namespace MesonProjectManager
