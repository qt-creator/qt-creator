/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "haskellbuildconfiguration.h"

#include "haskellconstants.h"

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
        info.typeName = HaskellBuildConfiguration::tr("Release");
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
    : NamedWidget(tr("General"))
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
    details->layout()->addWidget(new QLabel(tr("Build directory:")));

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
