/****************************************************************************
**
** Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
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

#include "cppquickfixsettings.h"
#include <projectexplorer/project.h>
#include <utils/fileutils.h>

namespace CppEditor {
namespace Internal {

class CppQuickFixProjectsSettings : public QObject
{
    Q_OBJECT
public:
    CppQuickFixProjectsSettings(ProjectExplorer::Project *project);
    CppQuickFixSettings *getSettings();
    bool isUsingGlobalSettings() const;
    const Utils::FilePath &filePathOfSettingsFile() const;

    using CppQuickFixProjectsSettingsPtr = QSharedPointer<CppQuickFixProjectsSettings>;
    static CppQuickFixProjectsSettingsPtr getSettings(ProjectExplorer::Project *project);
    static CppQuickFixSettings *getQuickFixSettings(ProjectExplorer::Project *project);

    Utils::FilePath searchForCppQuickFixSettingsFile();

    void useGlobalSettings();
    [[nodiscard]] bool useCustomSettings();
    void resetOwnSettingsToGlobal();
    bool saveOwnSettings();

private:
    void loadOwnSettingsFromFile();

    ProjectExplorer::Project *m_project;
    Utils::FilePath m_settingsFile;
    CppQuickFixSettings m_ownSettings;
    bool m_useGlobalSettings;
};

} // namespace Internal
} // namespace CppEditor

Q_DECLARE_METATYPE(QSharedPointer<CppEditor::Internal::CppQuickFixProjectsSettings>)
