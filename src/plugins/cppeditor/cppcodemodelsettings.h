// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <utils/aspects.h>

namespace ProjectExplorer { class Project; }

namespace CppEditor {

enum class UsePrecompiledHeaders;

enum class PchUsage {
    PchUse_None = 1,
    PchUse_BuildSystem = 2
};

class CPPEDITOR_EXPORT CppCodeModelSettingsData
{
public:
    int effectiveIndexerFileSizeLimitInMb() const;
    UsePrecompiledHeaders usePrecompiledHeaders() const;

    QString ignorePattern;
    PchUsage pchUsage = PchUsage::PchUse_BuildSystem;
    int indexerFileSizeLimitInMb = 5;
    bool interpretAmbiguousHeadersAsC = false;
    bool skipIndexingBigFiles = true;
    bool useBuiltinPreprocessor = true;
    bool ignoreFiles = false;
    bool enableIndexing = true;

private:
    friend bool operator==(const CppCodeModelSettingsData &s1, const CppCodeModelSettingsData &s2);
};

class IgnorePchAspect final : public Utils::BoolAspect
{
private:
    using Utils::BoolAspect::BoolAspect;
    QVariant toSettingsValue(const QVariant &valueToSave) const final;
    QVariant fromSettingsValue(const QVariant &savedValue) const final;
};

class CPPEDITOR_EXPORT CppCodeModelSettings : public Utils::AspectContainer
{
public:
    CppCodeModelSettings();

    static CppCodeModelSettingsData settingsForProject(const ProjectExplorer::Project *project);
    static CppCodeModelSettingsData settingsForProject(const Utils::FilePath &projectFile);
    static CppCodeModelSettingsData settingsForFile(const Utils::FilePath &file);
    static bool hasCustomSettings(const ProjectExplorer::Project *project);
    static void setSettingsForProject(ProjectExplorer::Project *project,
                                      const CppCodeModelSettingsData &settings);

    static CppCodeModelSettings &global();
    static void setGlobal(const CppCodeModelSettingsData &settings);

    static PchUsage pchUsageForProject(const ProjectExplorer::Project *project);
    static UsePrecompiledHeaders usePrecompiledHeaders(const ProjectExplorer::Project *project);

    static bool categorizeFindReferences();
    static void setCategorizeFindReferences(bool categorize);

    static bool isInteractiveFollowSymbol();
    static void setInteractiveFollowSymbol(bool interactive);

    CppCodeModelSettingsData data() const;
    void setData(const CppCodeModelSettingsData &data);

    Utils::StringAspect ignorePattern{this};
    Utils::BoolAspect ignorePch{this};
    Utils::IntegerAspect indexerFileSizeLimitInMb{this};
    Utils::BoolAspect interpretAmbiguousHeadersAsC{this};
    Utils::BoolAspect skipIndexingBigFiles{this};
    Utils::BoolAspect useBuiltinPreprocessor{this};
    Utils::BoolAspect ignoreFiles{this};
    Utils::BoolAspect enableIndexing{this};

private:
    bool m_categorizeFindReferences = false;
    bool interactiveFollowSymbol = true;
};

namespace Internal {
void setupCppCodeModelSettingsPage();
void setupCppCodeModelProjectSettingsPanel();

class NonInteractiveFollowSymbolMarker
{
public:
    NonInteractiveFollowSymbolMarker() { CppCodeModelSettings::setInteractiveFollowSymbol(false); }
    ~NonInteractiveFollowSymbolMarker() { CppCodeModelSettings::setInteractiveFollowSymbol(true); }
};

} // namespace Internal

} // namespace CppEditor
