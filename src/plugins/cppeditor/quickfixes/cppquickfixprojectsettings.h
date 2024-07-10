// Copyright (C) 2020 Leander Schulten <Leander.Schulten@rwth-aachen.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
