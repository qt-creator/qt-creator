// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "buildsettings.h"

#include <projectexplorer/buildstep.h>
#include <projectexplorer/buildsteplist.h>
#include <projectexplorer/project.h>

#include <QPointer>

class QPushButton;

namespace Coco::Internal {

class QMakeStepFactory: public ProjectExplorer::BuildStepFactory
{
public:
    QMakeStepFactory();
};

class CMakeStepFactory: public ProjectExplorer::BuildStepFactory
{
public:
    CMakeStepFactory();
};

class CocoBuildStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT
public:
    static CocoBuildStep *create(ProjectExplorer::BuildConfiguration *buildConfig);

    CocoBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    bool init() override;
    void display(ProjectExplorer::BuildConfiguration *buildConfig);

public slots:
    void buildSystemUpdated();

private slots:
    void onReconfigureButtonClicked();

protected:
    QWidget *createConfigWidget() override;

private:
    void updateDisplay();
    Tasking::GroupItem runRecipe() override;

    QPointer<BuildSettings> m_buildSettings;
    bool m_valid;
    QPushButton *m_reconfigureButton;
};

} // namespace Coco::Internal
