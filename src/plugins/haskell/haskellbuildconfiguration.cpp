// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellbuildconfiguration.h"

#include "haskellconstants.h"
#include "haskelltr.h"

#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/target.h>
#include <utils/algorithm.h>
#include <utils/detailswidget.h>
#include <utils/mimeutils.h>
#include <utils/pathchooser.h>
#include <utils/qtcassert.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

using namespace ProjectExplorer;

const char C_HASKELL_BUILDCONFIGURATION_ID[] = "Haskell.BuildConfiguration";

namespace Haskell {
namespace Internal {

HaskellBuildConfigurationFactory::HaskellBuildConfigurationFactory()
{
    registerBuildConfiguration<HaskellBuildConfiguration>(C_HASKELL_BUILDCONFIGURATION_ID);
    setSupportedProjectType(Constants::C_HASKELL_PROJECT_ID);
    setSupportedProjectMimeTypeName(Constants::C_HASKELL_PROJECT_MIMETYPE);

    setBuildGenerator([](const Kit *k, const Utils::FilePath &projectPath, bool forSetup)  {
        BuildInfo info;
        info.typeName = Tr::tr("Release");
        if (forSetup) {
            info.displayName = info.typeName;
            info.buildDirectory = projectPath.parentDir().pathAppended(".stack-work");
        }
        info.kitId = k->id();
        info.buildType = BuildConfiguration::BuildType::Release;
        return QList<BuildInfo>{info};
    });
}

HaskellBuildConfiguration::HaskellBuildConfiguration(Target *target, Utils::Id id)
    : BuildConfiguration(target, id)
{
    setInitializer([this](const BuildInfo &info) {
        setBuildDirectory(info.buildDirectory);
        setBuildType(info.buildType);
        setDisplayName(info.displayName);
    });
    appendInitialBuildStep(Constants::C_STACK_BUILD_STEP_ID);
}

NamedWidget *HaskellBuildConfiguration::createConfigWidget()
{
    return new HaskellBuildConfigurationWidget(this);
}

BuildConfiguration::BuildType HaskellBuildConfiguration::buildType() const
{
    return m_buildType;
}

void HaskellBuildConfiguration::setBuildType(BuildConfiguration::BuildType type)
{
    m_buildType = type;
}

HaskellBuildConfigurationWidget::HaskellBuildConfigurationWidget(HaskellBuildConfiguration *bc)
    : NamedWidget(Tr::tr("General"))
    , m_buildConfiguration(bc)
{
    setLayout(new QVBoxLayout);
    layout()->setContentsMargins(0, 0, 0, 0);
    auto box = new Utils::DetailsWidget;
    box->setState(Utils::DetailsWidget::NoSummary);
    layout()->addWidget(box);
    auto details = new QWidget;
    box->setWidget(details);
    details->setLayout(new QHBoxLayout);
    details->layout()->setContentsMargins(0, 0, 0, 0);
    details->layout()->addWidget(new QLabel(Tr::tr("Build directory:")));

    auto buildDirectoryInput = new Utils::PathChooser;
    buildDirectoryInput->setExpectedKind(Utils::PathChooser::Directory);
    buildDirectoryInput->setFilePath(m_buildConfiguration->buildDirectory());
    details->layout()->addWidget(buildDirectoryInput);

    connect(m_buildConfiguration,
            &BuildConfiguration::buildDirectoryChanged,
            buildDirectoryInput,
            [this, buildDirectoryInput] {
                buildDirectoryInput->setFilePath(m_buildConfiguration->buildDirectory());
            });
    connect(buildDirectoryInput,
            &Utils::PathChooser::textChanged,
            m_buildConfiguration,
            [this, buildDirectoryInput](const QString &) {
                m_buildConfiguration->setBuildDirectory(buildDirectoryInput->rawFilePath());
            });
}

} // namespace Internal
} // namespace Haskell
