// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/filepath.h>
#include <utils/id.h>
#include <utils/store.h>
#include <utils/qtcsettings.h>

#include <QStringList>

namespace ProjectExplorer { class Project; }

namespace CppEditor {
enum class UsePrecompiledHeaders;

class CPPEDITOR_EXPORT CppCodeModelSettings
{
public:
    enum PCHUsage {
        PchUse_None = 1,
        PchUse_BuildSystem = 2
    };

    CppCodeModelSettings() = default;
    CppCodeModelSettings(const Utils::Store &store) { fromMap(store); }

    friend bool operator==(const CppCodeModelSettings &s1, const CppCodeModelSettings &s2);
    friend bool operator!=(const CppCodeModelSettings &s1, const CppCodeModelSettings &s2)
    {
        return !(s1 == s2);
    }

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &store);

    static CppCodeModelSettings settingsForProject(const ProjectExplorer::Project *project);
    static CppCodeModelSettings settingsForProject(const Utils::FilePath &projectFile);
    static CppCodeModelSettings settingsForFile(const Utils::FilePath &file);

    static const CppCodeModelSettings &global() { return globalInstance(); }
    static void setGlobal(const CppCodeModelSettings &settings);

    static PCHUsage pchUsageForProject(const ProjectExplorer::Project *project);
    UsePrecompiledHeaders usePrecompiledHeaders() const;
    static UsePrecompiledHeaders usePrecompiledHeaders(const ProjectExplorer::Project *project);

    int effectiveIndexerFileSizeLimitInMb() const;

    static bool categorizeFindReferences();
    static void setCategorizeFindReferences(bool categorize);

    QString ignorePattern;
    PCHUsage pchUsage = PchUse_BuildSystem;
    int indexerFileSizeLimitInMb = 5;
    bool interpretAmbigiousHeadersAsC = false;
    bool skipIndexingBigFiles = true;
    bool useBuiltinPreprocessor = true;
    bool ignoreFiles = false;
    bool m_categorizeFindReferences = false; // Ephemeral!

private:
    CppCodeModelSettings(Utils::QtcSettings *s) { fromSettings(s); }
    static CppCodeModelSettings &globalInstance();

    void toSettings(Utils::QtcSettings *s);
    void fromSettings(Utils::QtcSettings *s);
};

class CppCodeModelProjectSettings
{
public:
    CppCodeModelProjectSettings(ProjectExplorer::Project *project);

    CppCodeModelSettings settings() const;
    void setSettings(const CppCodeModelSettings &settings);
    bool useGlobalSettings() const { return m_useGlobalSettings; }
    void setUseGlobalSettings(bool useGlobal);

private:
    void loadSettings();
    void saveSettings();

    ProjectExplorer::Project * const m_project;
    CppCodeModelSettings m_customSettings;
    bool m_useGlobalSettings = true;
};

namespace Internal {
void setupCppCodeModelSettingsPage();
void setupCppCodeModelProjectSettingsPanel();
} // namespace Internal

} // namespace CppEditor
