// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocobuildstep.h"

#include "cocopluginconstants.h"
#include "cocotr.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <settings/cocoinstallation.h>
#include <settings/globalsettingspage.h>
#include <solutions/tasking/tasktree.h>
#include <utils/layoutbuilder.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>

#include <QPushButton>

namespace Coco::Internal {

using namespace ProjectExplorer;

QMakeStepFactory::QMakeStepFactory()
{
    registerStep<CocoBuildStep>(Utils::Id{Constants::COCO_STEP_ID});
    setSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setRepeatable(false);
}

CMakeStepFactory::CMakeStepFactory()
{
    registerStep<CocoBuildStep>(Utils::Id{Constants::COCO_STEP_ID});
    setSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
    setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
    setRepeatable(false);
}

CocoBuildStep *CocoBuildStep::create(BuildConfiguration *buildConfig)
{
    // The "new" command creates a small memory leak which we can tolerate.
    return new CocoBuildStep(
        new BuildStepList(buildConfig, Constants::COCO_STEP_ID), Utils::Id(Constants::COCO_STEP_ID));
}

CocoBuildStep::CocoBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id)
    , m_reconfigureButton{new QPushButton}
{}

bool CocoBuildStep::init()
{
    return true;
}

void CocoBuildStep::buildSystemUpdated()
{
    updateDisplay();
}

void CocoBuildStep::onReconfigureButtonClicked()
{
    m_valid = !m_valid;

    setSummaryText(Tr::tr("Coco Code Coverage: Reconfiguring..."));
    m_reconfigureButton->setEnabled(false);
    m_buildSettings->setCoverage(m_valid);
    m_buildSettings->provideFile();
    m_buildSettings->reconfigure();
}

QWidget *CocoBuildStep::createConfigWidget()
{
    connect(
        m_reconfigureButton,
        &QPushButton::clicked,
        this,
        &CocoBuildStep::onReconfigureButtonClicked);

    Layouting::Form builder;
    builder.addRow({m_reconfigureButton, new QLabel});
    builder.setNoMargins();

    return builder.emerge();
}

void CocoBuildStep::updateDisplay()
{
    CocoInstallation coco;
    if (!coco.isValid()) {
        setSummaryText("<i>" + Tr::tr("Coco Code Coverage: No working Coco installation") + "</i>");
        m_reconfigureButton->setEnabled(false);
        return;
    }

    m_valid = m_buildSettings->validSettings();

    if (m_valid) {
        setSummaryText("<b>" + Tr::tr("Coco Code Coverage: Enabled") + "</b>");
        m_reconfigureButton->setText(Tr::tr("Disable Coverage"));
    } else {
        setSummaryText(Tr::tr("Coco Code Coverage: Disabled"));
        m_reconfigureButton->setText(Tr::tr("Enable Coverage"));
    }

    m_reconfigureButton->setEnabled(true);
}

void CocoBuildStep::display(BuildConfiguration *buildConfig)
{
    Q_ASSERT( m_buildSettings.isNull() );

    m_buildSettings = BuildSettings::createdFor(*buildConfig);
    m_buildSettings->read();
    m_buildSettings->connectToBuildStep(this);

    setImmutable(true);
    updateDisplay();
}

Tasking::GroupItem CocoBuildStep::runRecipe()
{
    return Tasking::GroupItem({});
}

} // namespace Coco::Internal
