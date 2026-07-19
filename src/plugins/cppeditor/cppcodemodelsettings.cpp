// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettings.h"

#include "compileroptionsbuilder.h"
#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cppmodelmanager.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>
#include <projectexplorer/projectpanelfactory.h>
#include <projectexplorer/projectsettings.h>
#include <projectexplorer/useglobalaspect.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>

#include <QCheckBox>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

CppCodeModelSettings::CppCodeModelSettings()
{
    setSettingsGroup(Constants::CPPEDITOR_SETTINGSGROUP);

    ignorePch.setSettingsKey(Constants::CPPEDITOR_MODEL_MANAGER_PCH_USAGE);
    ignorePch.setLabelText(Tr::tr("Ignore precompiled headers"));
    ignorePch.setToolTip(Tr::tr(
        "<html><head/><body><p>When precompiled headers are not ignored, the parsing for code "
        "completion and semantic highlighting will process the precompiled header before "
        "processing any file.</p></body></html>"));

    indexerFileSizeLimitInMb.setSettingsKey(Constants::CPPEDITOR_INDEXER_FILE_SIZE_LIMIT);
    indexerFileSizeLimitInMb.setDefaultValue(5);
    indexerFileSizeLimitInMb.setSuffix(Tr::tr("MB"));
    indexerFileSizeLimitInMb.setRange(1, 500);

    interpretAmbiguousHeadersAsC.setSettingsKey(Constants::CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS);
    interpretAmbiguousHeadersAsC.setLabelText(Tr::tr("Interpret ambiguous headers as C headers"));

    skipIndexingBigFiles.setSettingsKey(Constants::CPPEDITOR_SKIP_INDEXING_BIG_FILES);
    skipIndexingBigFiles.setDefaultValue(true);
    skipIndexingBigFiles.setLabelText(Tr::tr("Do not index files greater than"));

    useBuiltinPreprocessor.setSettingsKey(Constants::CPPEDITOR_USE_BUILTIN_PREPROCESSOR);
    useBuiltinPreprocessor.setDefaultValue(true);
    useBuiltinPreprocessor.setLabelText(Tr::tr("Use built-in preprocessor to show "
                                               "pre-processed files"));
    useBuiltinPreprocessor.setToolTip(Tr::tr("Uncheck this to invoke the actual compiler "
                                             "to show a pre-processed source file in the editor."));

    ignoreFiles.setSettingsKey(Constants::CPPEDITOR_IGNORE_FILES);
    ignoreFiles.setDefaultValue(false);
    ignoreFiles.setLabelText(Tr::tr("Ignore files"));
    ignoreFiles.setToolTip("<html><head/><body><p>"
               + Tr::tr("Ignore files that match these wildcard patterns, one wildcard per line.")
               + "</p></body></html>");

    ignorePattern.setSettingsKey(Constants::CPPEDITOR_IGNORE_PATTERN);
    ignorePattern.setToolTip(ignoreFiles.toolTip());
    ignorePattern.setDisplayStyle(StringAspect::TextEditDisplay);
    ignorePattern.setEnabler(&ignoreFiles);

    enableIndexing.setSettingsKey("EnableIndexing");
    enableIndexing.setDefaultValue(true);
    enableIndexing.setLabelText(Tr::tr("Enable indexing"));
    enableIndexing.setToolTip(Tr::tr(
        "Indexing should almost always be kept enabled, as disabling it will severely limit the "
        "capabilities of the code model."));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("General")),
                Column {
                    interpretAmbiguousHeadersAsC,
                    ignorePch,
                    useBuiltinPreprocessor,
                    enableIndexing,
                    Row { skipIndexingBigFiles, indexerFileSizeLimitInMb, st },
                    Row { Column { ignoreFiles, st }, ignorePattern },
                }
            },
            st
        };
    });
}

bool operator==(const CppEditor::CppCodeModelSettingsData &s1,
                const CppEditor::CppCodeModelSettingsData &s2)
{
    return s1.pchUsage == s2.pchUsage
        && s1.interpretAmbiguousHeadersAsC == s2.interpretAmbiguousHeadersAsC
        && s1.enableIndexing == s2.enableIndexing
        && s1.skipIndexingBigFiles == s2.skipIndexingBigFiles
        && s1.useBuiltinPreprocessor == s2.useBuiltinPreprocessor
        && s1.indexerFileSizeLimitInMb == s2.indexerFileSizeLimitInMb
        && s1.ignoreFiles == s2.ignoreFiles
        && s1.ignorePattern == s2.ignorePattern;
}

CppCodeModelSettings &CppCodeModelSettings::global()
{
    static CppCodeModelSettings theCppCodeModelSettings;
    return theCppCodeModelSettings;
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForProject(const Utils::FilePath &projectFile)
{
    return settingsForProject(ProjectManager::projectWithProjectFile(projectFile, true));
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForFile(const Utils::FilePath &file)
{
    return settingsForProject(ProjectManager::projectForFile(file));
}

void CppCodeModelSettings::setGlobal(const CppCodeModelSettingsData &settings)
{
    if (global().data() == settings)
        return;

    global().setData(settings);
    global().writeSettings();
    CppModelManager::handleSettingsChange(nullptr);
}

PchUsage CppCodeModelSettings::pchUsageForProject(const Project *project)
{
    return CppCodeModelSettings::settingsForProject(project).pchUsage;
}

UsePrecompiledHeaders CppCodeModelSettingsData::usePrecompiledHeaders() const
{
    return pchUsage == PchUsage::PchUse_None ? UsePrecompiledHeaders::No
                                             : UsePrecompiledHeaders::Yes;
}

UsePrecompiledHeaders CppCodeModelSettings::usePrecompiledHeaders(const Project *project)
{
    return CppCodeModelSettings::settingsForProject(project).usePrecompiledHeaders();
}

int CppCodeModelSettingsData::effectiveIndexerFileSizeLimitInMb() const
{
    return skipIndexingBigFiles ? indexerFileSizeLimitInMb : -1;
}

bool CppCodeModelSettings::categorizeFindReferences()
{
    return global().m_categorizeFindReferences;
}

void CppCodeModelSettings::setCategorizeFindReferences(bool categorize)
{
    global().m_categorizeFindReferences = categorize;
}

bool CppCodeModelSettings::isInteractiveFollowSymbol()
{
    return global().interactiveFollowSymbol;
}

void CppCodeModelSettings::setInteractiveFollowSymbol(bool interactive)
{
    global().interactiveFollowSymbol = interactive;
}

CppCodeModelSettingsData CppCodeModelSettings::data() const
{
    CppCodeModelSettingsData d;
    d.ignorePattern = ignorePattern();
    d.pchUsage = ignorePch() ? PchUsage::PchUse_None : PchUsage::PchUse_BuildSystem;
    d.indexerFileSizeLimitInMb = indexerFileSizeLimitInMb();
    d.interpretAmbiguousHeadersAsC = interpretAmbiguousHeadersAsC();
    d.skipIndexingBigFiles = skipIndexingBigFiles();
    d.useBuiltinPreprocessor = useBuiltinPreprocessor();
    d.ignoreFiles = ignoreFiles();
    d.enableIndexing = enableIndexing();
    return d;
}

void CppCodeModelSettings::setData(const CppCodeModelSettingsData &data)
{
    ignorePattern.setValue(data.ignorePattern);
    ignorePch.setValue(data.pchUsage == PchUsage::PchUse_None);
    indexerFileSizeLimitInMb.setValue(data.indexerFileSizeLimitInMb);
    interpretAmbiguousHeadersAsC.setValue(data.interpretAmbiguousHeadersAsC);
    skipIndexingBigFiles.setValue(data.skipIndexingBigFiles);
    useBuiltinPreprocessor.setValue(data.useBuiltinPreprocessor);
    ignoreFiles.setValue(data.ignoreFiles);
    enableIndexing.setValue(data.enableIndexing);
}

namespace Internal {

class CppCodeModelSettingsPage final : public Core::IOptionsPage
{
public:
    CppCodeModelSettingsPage()
    {
        setId(Constants::CPP_CODE_MODEL_SETTINGS_ID);
        setDisplayName(Tr::tr("Code Model"));
        setCategory(Constants::CPP_SETTINGS_CATEGORY);
        setSettingsProvider([] { return &CppCodeModelSettings::global(); });
    }
};

void setupCppCodeModelSettingsPage()
{
    static CppCodeModelSettingsPage theCppCodeModelSettingsPage;
    CppCodeModelSettings::global().readSettings();
    CppCodeModelSettings::global().setAutoApply(false);
}

class CppCodeModelProjectSettings : public CppCodeModelSettings
{
public:
    explicit CppCodeModelProjectSettings(Project *project)
        : m_project(project)
    {
        setAutoApply(true);

        loadProjectSettings(*this, useGlobalSettings, project, Constants::CPPEDITOR_SETTINGSGROUP);

        setupUseGlobalSettings(this, &useGlobalSettings, [this] { save(); });
    }

    void save()
    {
        saveProjectSettings(*this, useGlobalSettings, m_project, Constants::CPPEDITOR_SETTINGSGROUP);
        CppModelManager::handleSettingsChange(m_project);
    }

    static Key extraDataKey() { return "CppCodeModelProjectSettings"; }

    UseGlobalAspect useGlobalSettings{Constants::CPP_CODE_MODEL_SETTINGS_ID};

private:
    Project * const m_project;
};

static CppCodeModelProjectSettings *cppCodeModelProjectSettings(Project *project)
{
    return projectSettings<CppCodeModelProjectSettings>(project);
}

class CppCodeModelProjectSettingsWidget : public QWidget
{
public:
    explicit CppCodeModelProjectSettingsWidget(Project *project)
    {
        CppCodeModelProjectSettings * const ps = cppCodeModelProjectSettings(project);
        using namespace Layouting;
        Column {
            ps->useGlobalSettings,
            *ps,
            noMargin
        }.attachTo(this);
    }
};

} // namespace Internal

bool CppCodeModelSettings::hasCustomSettings(const Project *project)
{
    if (!project)
        return false;
    return !Internal::cppCodeModelProjectSettings(const_cast<Project *>(project))->useGlobalSettings();
}

void CppCodeModelSettings::setSettingsForProject(Project *project, const CppCodeModelSettingsData &settings)
{
    if (project) {
        Internal::CppCodeModelProjectSettings * const ps
            = Internal::cppCodeModelProjectSettings(project);
        {
            QSignalBlocker blocker(ps);
            ps->setData(settings);
        }
        if (ps->useGlobalSettings())
            ps->useGlobalSettings.setValue(false);
        else
            ps->save();
    } else {
        CppModelManager::handleSettingsChange(nullptr);
    }
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForProject(const Project *project)
{
    if (!project)
        return global().data();
    const auto *ps = Internal::cppCodeModelProjectSettings(const_cast<Project *>(project));
    return ps->useGlobalSettings() ? global().data() : ps->data();
}

namespace Internal {

class CppCodeModelProjectSettingsPanelFactory final : public ProjectPanelFactory
{
public:
    CppCodeModelProjectSettingsPanelFactory()
    {
        setPriority(100);
        setDisplayName(Tr::tr("C++ Code Model"));
        setCreateWidgetFunction([](Project *project) {
            return new CppCodeModelProjectSettingsWidget(project);
        });
    }
};

void setupCppCodeModelProjectSettingsPanel()
{
    static CppCodeModelProjectSettingsPanelFactory factory;
}

} // namespace Internal
} // namespace CppEditor
