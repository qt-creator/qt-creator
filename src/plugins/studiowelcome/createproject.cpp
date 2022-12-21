// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "createproject.h"

#include <projectexplorer/jsonwizard/jsonprojectpage.h>

#include <projectexplorer/jsonwizard/jsonfieldpage.h>
#include <projectexplorer/jsonwizard/jsonfieldpage_p.h>

using namespace StudioWelcome;

void CreateProject::execute()
{
    m_wizard.run([&](QWizardPage *page) {
        if (auto *p = dynamic_cast<ProjectExplorer::JsonProjectPage *>(page))
            processProjectPage(p);
        else if (auto *p = dynamic_cast<ProjectExplorer::JsonFieldPage *>(page))
            processFieldPage(p);
    });
}

void CreateProject::processProjectPage(ProjectExplorer::JsonProjectPage *page)
{
    page->setProjectName(m_projectName);
    page->setFilePath(m_projectLocation);

    page->setUseAsDefaultPath(m_saveAsDefaultLocation);
    page->fieldsUpdated();
}

void CreateProject::processFieldPage(ProjectExplorer::JsonFieldPage *page)
{
    if (page->jsonField("ScreenFactor"))
        m_wizard.setScreenSizeIndex(m_screenSizeIndex);

    if (page->jsonField("TargetQtVersion") && m_targetQtVersionIndex > -1)
        m_wizard.setTargetQtVersionIndex(m_targetQtVersionIndex);

    if (page->jsonField("ControlsStyle"))
        m_wizard.setStyleIndex(m_styleIndex);

    if (page->jsonField("UseVirtualKeyboard"))
        m_wizard.setUseVirtualKeyboard(m_useVirtualKeyboard);

    auto widthField = dynamic_cast<ProjectExplorer::LineEditField *>(page->jsonField("CustomScreenWidth"));
    auto heightField = dynamic_cast<ProjectExplorer::LineEditField *>(page->jsonField("CustomScreenHeight"));

    // TODO: use m_wizard to set these text items?
    if (widthField && heightField) {
        if (!m_customWidth.isEmpty() && !m_customHeight.isEmpty()) {
            widthField->setText(m_customWidth);
            heightField->setText(m_customHeight);
        }
    }
}
