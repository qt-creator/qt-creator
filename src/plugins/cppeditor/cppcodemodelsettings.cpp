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
#include <projectexplorer/projectsettingswidget.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/macroexpander.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/store.h>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

static Key useGlobalSettingsKey() { return "useGlobalSettings"; }

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
    return settingsForProject(ProjectManager::projectWithProjectFilePath(projectFile));
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

class CppCodeModelProjectSettingsWidget : public ProjectSettingsWidget
{
public:
    explicit CppCodeModelProjectSettingsWidget(Project *project)
        : m_project(project)
    {
        setGlobalSettingsId(Constants::CPP_CODE_MODEL_SETTINGS_ID);

        if (project) {
            const Store data = storeFromVariant(m_project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
            m_useGlobalSettings = data.value(useGlobalSettingsKey(), true).toBool();
            m_customSettings.fromMap(data);
        }

        using namespace Layouting;
        Column {
            m_customSettings,
            noMargin
        }.attachTo(this);

        setUseGlobalSettings(m_useGlobalSettings);
        setEnabled(!useGlobalSettings());

        connect(this, &ProjectSettingsWidget::useGlobalSettingsChanged, this, [this](bool checked) {
            setEnabled(!checked);
            m_useGlobalSettings = checked;
            saveSettings();
        });

        connect(&m_customSettings, &AspectContainer::changed, this, [this] {
            saveSettings();
        });
    }

    void saveSettings()
    {
        if (m_project) {
            Store data;
            m_customSettings.toMap(data);
            data.insert(useGlobalSettingsKey(), m_useGlobalSettings);
            m_project->setNamedSettings(Constants::CPPEDITOR_SETTINGSGROUP, variantFromStore(data));
        }
        CppModelManager::handleSettingsChange(m_project);
    }

private:
    Project * const m_project;
    CppCodeModelSettings m_customSettings;
    bool m_useGlobalSettings = true;
};

} // namespace Internal

bool CppCodeModelSettings::hasCustomSettings(const Project *project)
{
    if (!project)
        return false;
    CppCodeModelSettings m_customSettings;
    const Store data = storeFromVariant(project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
    return !data.value(useGlobalSettingsKey(), true).toBool();
}

void CppCodeModelSettings::setSettingsForProject(Project *project, const CppCodeModelSettingsData &settings)
{
    if (project) {
        CppCodeModelSettings s;
        s.setData(settings);
        Store data;
        s.toMap(data);
        data.insert(useGlobalSettingsKey(), false);
        project->setNamedSettings(Constants::CPPEDITOR_SETTINGSGROUP, variantFromStore(data));
    }
    CppModelManager::handleSettingsChange(project);
}

CppCodeModelSettingsData CppCodeModelSettings::settingsForProject(const Project *project)
{
    if (!project)
        return CppCodeModelSettings::global().data();

    const Store data = storeFromVariant(project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
    if (data.value(useGlobalSettingsKey(), true).toBool())
        return CppCodeModelSettings::global().data();

    CppCodeModelSettings customSettings;
    customSettings.fromMap(data);
    return customSettings.data();
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
