// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cocobuildstep.h"

#include "cocopluginconstants.h"
#include "cocotr.h"
#include "globalsettings.h"

#include <cmakeprojectmanager/cmakeprojectconstants.h>
#include <projectexplorer/projectexplorerconstants.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/target.h>
#include <qmakeprojectmanager/qmakeprojectmanagerconstants.h>
#include <solutions/tasking/tasktree.h>
#include <utils/layoutbuilder.h>

#include <QPushButton>

namespace Coco::Internal {

using namespace ProjectExplorer;

class ButtonWidget : public QWidget
// The configuration button of the CocoBuildstep must be part of a separated object
// because it may be several times recreated by createConfigWidget().
{
public:
    explicit ButtonWidget(CocoBuildStep *step);

private slots:
    void setButtonState(bool enabled, const QString &text);

private:
    QPushButton *m_button;
};

ButtonWidget::ButtonWidget(CocoBuildStep *step)
    : m_button{new QPushButton}
{
    connect(m_button, &QPushButton::clicked, step, &CocoBuildStep::onButtonClicked);
    connect(step, &CocoBuildStep::setButtonState, this, &ButtonWidget::setButtonState);

    Layouting::Form builder;
    builder.addRow({m_button, new QLabel});
    builder.setNoMargins();
    builder.attachTo(this);
}

void ButtonWidget::setButtonState(bool enabled, const QString &text)
{
    m_button->setEnabled(enabled);
    if (!text.isEmpty())
        m_button->setText(text);
}

CocoBuildStep *CocoBuildStep::create(BuildConfiguration *buildConfig)
{
    // The "new" command creates a small memory leak which we can tolerate.
    return new CocoBuildStep(
        new BuildStepList(buildConfig, Constants::COCO_STEP_ID), Utils::Id(Constants::COCO_STEP_ID));
}

CocoBuildStep::CocoBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id)
    : BuildStep(bsl, id)
{}

bool CocoBuildStep::init()
{
    return true;
}

void CocoBuildStep::buildSystemUpdated()
{
    updateDisplay();
}

void CocoBuildStep::onButtonClicked()
{
    m_valid = !m_valid;

    setSummaryText(Tr::tr("Coco Code Coverage: Reconfiguring..."));
    emit setButtonState(false);

    m_buildSettings->setCoverage(m_valid);
    m_buildSettings->provideFile();
    m_buildSettings->reconfigure();
}

QWidget *CocoBuildStep::createConfigWidget()
{
    auto widget = new ButtonWidget{this};
    updateDisplay();

    return widget;
}

void CocoBuildStep::updateDisplay()
{
    if (!cocoSettings().isValid()) {
        setSummaryText("<i>" + Tr::tr("Coco Code Coverage: No working Coco installation") + "</i>");
        emit setButtonState(false);
        return;
    }

    m_valid = m_buildSettings->validSettings();

    if (m_valid) {
        setSummaryText("<b>" + Tr::tr("Coco Code Coverage: Enabled") + "</b>");
        emit setButtonState(true, Tr::tr("Disable Coverage"));
    } else {
        setSummaryText(Tr::tr("Coco Code Coverage: Disabled"));
        // m_reconfigureButton->setText(Tr::tr("Enable Coverage"));
        emit setButtonState(true, Tr::tr("Enable Coverage"));
    }
}

void CocoBuildStep::display(BuildConfiguration *buildConfig)
{
    Q_ASSERT( m_buildSettings.isNull() );

    m_buildSettings = BuildSettings::createdFor(buildConfig);
    m_buildSettings->read();
    m_buildSettings->connectToBuildStep(this);

    setImmutable(true);
    updateDisplay();
}

Tasking::GroupItem CocoBuildStep::runRecipe()
{
    return Tasking::GroupItem({});
}

// Factories

class QMakeStepFactory final : public BuildStepFactory
{
public:
    QMakeStepFactory()
    {
        registerStep<CocoBuildStep>(Utils::Id{Constants::COCO_STEP_ID});
        setSupportedProjectType(QmakeProjectManager::Constants::QMAKEPROJECT_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        setRepeatable(false);
    }
};

class CMakeStepFactory final : public BuildStepFactory
{
public:
    CMakeStepFactory()
    {
        registerStep<CocoBuildStep>(Utils::Id{Constants::COCO_STEP_ID});
        setSupportedProjectType(CMakeProjectManager::Constants::CMAKE_PROJECT_ID);
        setSupportedStepList(ProjectExplorer::Constants::BUILDSTEPS_BUILD);
        setRepeatable(false);
    }
};

static void addBuildStep(Target *target)
{
    for (BuildConfiguration *config : target->buildConfigurations()) {
        if (BuildSettings::supportsBuildConfig(*config)) {
            BuildStepList *steps = config->buildSteps();

            if (!steps->contains(Constants::COCO_STEP_ID))
                steps->insertStep(0, CocoBuildStep::create(config));

            steps->firstOfType<CocoBuildStep>()->display(config);
        }
    }
}

void setupCocoBuildSteps()
{
    static QMakeStepFactory theQmakeStepFactory;
    static CMakeStepFactory theCmakeStepFactory;

    QObject::connect(ProjectManager::instance(), &ProjectManager::projectAdded, [](Project *project) {
        if (Target *target = project->activeTarget())
            addBuildStep(target);

        QObject::connect(project, &Project::addedTarget, [](Target *target) {
            addBuildStep(target);
        });
    });
}

} // namespace Coco::Internal
