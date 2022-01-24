/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
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
