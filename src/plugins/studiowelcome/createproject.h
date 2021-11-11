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

#pragma once

#include "wizardhandler.h"

namespace ProjectExplorer {
class JsonProjectPage;
}

namespace StudioWelcome {

class CreateProject
{
public:
    CreateProject(WizardHandler &wizard): m_wizard{wizard} {}

    CreateProject &withName(const QString &name) { m_projectName = name; return *this; }
    CreateProject &atLocation(const Utils::FilePath &location) { m_projectLocation = location; return *this; }
    CreateProject &withScreenSizes(int screenSizeIndex, const QString &customWidth, const QString &customHeight)
    {
        m_screenSizeIndex = screenSizeIndex;
        m_customWidth = customWidth;
        m_customHeight = customHeight;
        return *this;
    }

    CreateProject &withStyle(int styleIndex) { m_styleIndex = styleIndex; return *this; }
    CreateProject &useQtVirtualKeyboard(bool value) { m_useVirtualKeyboard = value; return *this; }
    CreateProject &saveAsDefaultLocation(bool value) { m_saveAsDefaultLocation = value; return *this; }
    CreateProject &withTargetQtVersion(int targetQtVersionIndex)
    { m_targetQtVersionIndex = targetQtVersionIndex; return *this; }

    void execute();

private:
    void processProjectPage(ProjectExplorer::JsonProjectPage *page);
    void processFieldPage(ProjectExplorer::JsonFieldPage *page);

private:
    WizardHandler &m_wizard;

    QString m_projectName;
    Utils::FilePath m_projectLocation;
    int m_screenSizeIndex = -1;
    QString m_customWidth;
    QString m_customHeight;
    int m_styleIndex = -1;
    bool m_useVirtualKeyboard = false;
    bool m_saveAsDefaultLocation = false;
    int m_targetQtVersionIndex = -1;
};

} // StudioWelcome
