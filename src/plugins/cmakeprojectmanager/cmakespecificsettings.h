/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include <coreplugin/dialogs/ioptionspage.h>

#include <utils/aspects.h>

namespace CMakeProjectManager {
namespace Internal {

enum AfterAddFileAction : int {
    ASK_USER,
    COPY_FILE_PATH,
    NEVER_COPY_FILE_PATH
};

class CMakeSpecificSettings : public Utils::AspectContainer
{
    Q_DECLARE_TR_FUNCTIONS(CMakeProjectManager::Internal::CMakeSpecificSettings)

public:
    CMakeSpecificSettings();

    void setAfterAddFileSetting(AfterAddFileAction settings) {
        m_afterAddFileToProjectSetting.setValue(settings);
    }
    AfterAddFileAction afterAddFileSetting() const {
        return static_cast<AfterAddFileAction>(m_afterAddFileToProjectSetting.value());
    }

    Utils::FilePath ninjaPath() const { return m_ninjaPath.filePath(); }

    void setPackageManagerAutoSetup(bool checked) { m_packageManagerAutoSetup.setValue(checked); }
    bool packageManagerAutoSetup() const { return m_packageManagerAutoSetup.value(); }

    void setAskBeforeReConfigureInitialParams(bool doAsk) { m_askBeforeReConfigureInitialParams.setValue(doAsk); }
    bool askBeforeReConfigureInitialParams() const { return m_askBeforeReConfigureInitialParams.value(); }

private:
    friend class CMakeSpecificSettingWidget;

    Utils::SelectionAspect m_afterAddFileToProjectSetting;
    Utils::StringAspect m_ninjaPath;
    Utils::BoolAspect m_packageManagerAutoSetup;
    Utils::BoolAspect m_askBeforeReConfigureInitialParams;
};


class CMakeSpecificSettingsPage final : public Core::IOptionsPage
{
public:
    explicit CMakeSpecificSettingsPage(CMakeSpecificSettings *settings);
};

} // Internal
} // CMakeProjectManager
