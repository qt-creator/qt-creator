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

class CocoBuildStep : public ProjectExplorer::BuildStep
{
    Q_OBJECT

public:
    static CocoBuildStep *create(ProjectExplorer::BuildConfiguration *buildConfig);

    CocoBuildStep(ProjectExplorer::BuildStepList *bsl, Utils::Id id);

    bool init() override;
    void display(ProjectExplorer::BuildConfiguration *buildConfig);

signals:
    void setButtonState(bool enabled, const QString &text = {});

public slots:
    void buildSystemUpdated();
    void onButtonClicked();

protected:
    QWidget *createConfigWidget() override;

private:
    void updateDisplay();
    Tasking::GroupItem runRecipe() override;

    QPointer<BuildSettings> m_buildSettings;
    bool m_valid;
};

void setupCocoBuildSteps();

} // namespace Coco::Internal
