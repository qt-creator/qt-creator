// Copyright (c) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "haskellbuildconfiguration.h"

#include "haskellconstants.h"
#include "haskelltr.h"

#include <projectexplorer/buildconfiguration.h>
#include <projectexplorer/buildinfo.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/namedwidget.h>
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

namespace Haskell::Internal {

class HaskellBuildConfiguration final : public BuildConfiguration
{
public:
    HaskellBuildConfiguration(Target *target, Utils::Id id)
        : BuildConfiguration(target, id)
    {
        setInitializer([this](const BuildInfo &info) {
            setBuildDirectory(info.buildDirectory);
            setBuildType(info.buildType);
            setDisplayName(info.displayName);
        });
        appendInitialBuildStep(Constants::C_STACK_BUILD_STEP_ID);
    }

    NamedWidget *createConfigWidget() final;

    BuildType buildType() const final
    {
        return m_buildType;
    }

    void setBuildType(BuildType type)
    {
        m_buildType = type;
    }

private:
    BuildType m_buildType = BuildType::Release;
};

class HaskellBuildConfigurationWidget final : public NamedWidget
{
public:
    HaskellBuildConfigurationWidget(HaskellBuildConfiguration *bc)
        : NamedWidget(Tr::tr("General"))
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
        buildDirectoryInput->setFilePath(bc->buildDirectory());
        details->layout()->addWidget(buildDirectoryInput);

        connect(bc,
                &BuildConfiguration::buildDirectoryChanged,
                buildDirectoryInput,
                [bc, buildDirectoryInput] {
                    buildDirectoryInput->setFilePath(bc->buildDirectory());
                });
        connect(buildDirectoryInput,
                &Utils::PathChooser::textChanged,
                bc,
                [bc, buildDirectoryInput](const QString &) {
                    bc->setBuildDirectory(buildDirectoryInput->unexpandedFilePath());
                });
    }
};

NamedWidget *HaskellBuildConfiguration::createConfigWidget()
{
    return new HaskellBuildConfigurationWidget(this);
}

class HaskellBuildConfigurationFactory final : public BuildConfigurationFactory
{
public:
    HaskellBuildConfigurationFactory()
    {
        registerBuildConfiguration<HaskellBuildConfiguration>("Haskell.BuildConfiguration");

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
};

void setupHaskellBuildConfiguration()
{
    static HaskellBuildConfigurationFactory theHaskellBuildConfigurationFactory;
}

} // Haskell::Internal
