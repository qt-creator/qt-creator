// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
