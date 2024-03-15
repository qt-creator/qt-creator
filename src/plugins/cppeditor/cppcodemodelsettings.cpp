// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppcodemodelsettings.h"

#include "compileroptionsbuilder.h"
#include "cppeditorconstants.h"

#include <coreplugin/icore.h>

#include <projectexplorer/project.h>
#include <projectexplorer/projectmanager.h>

#include <utils/algorithm.h>
#include <utils/hostosinfo.h>
#include <utils/macroexpander.h>
#include <utils/qtcprocess.h>
#include <utils/qtcassert.h>
#include <utils/store.h>

#include <QDateTime>
#include <QDir>
#include <QHash>
#include <QPair>
#include <QSettings>

using namespace ProjectExplorer;
using namespace Utils;

namespace CppEditor {

static Key pchUsageKey() { return Constants::CPPEDITOR_MODEL_MANAGER_PCH_USAGE; }
static Key interpretAmbiguousHeadersAsCHeadersKey()
    { return Constants::CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS; }
static Key skipIndexingBigFilesKey() { return Constants::CPPEDITOR_SKIP_INDEXING_BIG_FILES; }
static Key ignoreFilesKey() { return Constants::CPPEDITOR_IGNORE_FILES; }
static Key ignorePatternKey() { return Constants::CPPEDITOR_IGNORE_PATTERN; }
static Key useBuiltinPreprocessorKey() { return Constants::CPPEDITOR_USE_BUILTIN_PREPROCESSOR; }
static Key indexerFileSizeLimitKey() { return Constants::CPPEDITOR_INDEXER_FILE_SIZE_LIMIT; }
static Key useGlobalSettingsKey() { return "useGlobalSettings"; }

bool operator==(const CppEditor::CppCodeModelSettings::Data &s1,
                const CppEditor::CppCodeModelSettings::Data &s2)
{
    return s1.pchUsage == s2.pchUsage
           && s1.interpretAmbigiousHeadersAsC == s2.interpretAmbigiousHeadersAsC
           && s1.skipIndexingBigFiles == s2.skipIndexingBigFiles
           && s1.useBuiltinPreprocessor == s2.useBuiltinPreprocessor
           && s1.indexerFileSizeLimitInMb == s2.indexerFileSizeLimitInMb
           && s1.categorizeFindReferences == s2.categorizeFindReferences
           && s1.ignoreFiles == s2.ignoreFiles && s1.ignorePattern == s2.ignorePattern;
}

Store CppCodeModelSettings::Data::toMap() const
{
    Store store;
    store.insert(pchUsageKey(), pchUsage);
    store.insert(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbigiousHeadersAsC);
    store.insert(skipIndexingBigFilesKey(), skipIndexingBigFiles);
    store.insert(ignoreFilesKey(), ignoreFiles);
    store.insert(ignorePatternKey(), ignorePattern);
    store.insert(useBuiltinPreprocessorKey(), useBuiltinPreprocessor);
    store.insert(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb);
    return store;
}

void CppCodeModelSettings::Data::fromMap(const Utils::Store &store)
{
    const CppCodeModelSettings::Data def;
    pchUsage = static_cast<PCHUsage>(store.value(pchUsageKey(), def.pchUsage).toInt());
    interpretAmbigiousHeadersAsC = store
                                       .value(interpretAmbiguousHeadersAsCHeadersKey(),
                                              def.interpretAmbigiousHeadersAsC)
                                       .toBool();
    skipIndexingBigFiles = store.value(skipIndexingBigFilesKey(), def.skipIndexingBigFiles).toBool();
    ignoreFiles = store.value(ignoreFilesKey(), def.ignoreFiles).toBool();
    ignorePattern = store.value(ignorePatternKey(), def.ignorePattern).toString();
    useBuiltinPreprocessor
        = store.value(useBuiltinPreprocessorKey(), def.useBuiltinPreprocessor).toBool();
    indexerFileSizeLimitInMb
        = store.value(indexerFileSizeLimitKey(), def.indexerFileSizeLimitInMb).toInt();
}

void CppCodeModelSettings::fromSettings(QtcSettings *s)
{
    m_data.fromMap(storeFromSettings(Constants::CPPEDITOR_SETTINGSGROUP, s));
}

void CppCodeModelSettings::toSettings(QtcSettings *s)
{
    storeToSettingsWithDefault(Constants::CPPEDITOR_SETTINGSGROUP, s, m_data.toMap(), Data().toMap());
}

CppCodeModelSettings &CppCodeModelSettings::globalInstance()
{
    static CppCodeModelSettings theCppCodeModelSettings(Core::ICore::settings());
    return theCppCodeModelSettings;
}

CppCodeModelSettings CppCodeModelSettings::settingsForProject(const ProjectExplorer::Project *project)
{
    return {CppCodeModelProjectSettings(const_cast<ProjectExplorer::Project *>(project)).data()};
}

CppCodeModelSettings CppCodeModelSettings::settingsForProject(const Utils::FilePath &projectFile)
{
    return settingsForProject(ProjectManager::projectWithProjectFilePath(projectFile));
}

CppCodeModelSettings CppCodeModelSettings::settingsForFile(const Utils::FilePath &file)
{
    return settingsForProject(ProjectManager::projectForFile(file));
}

void CppCodeModelSettings::setGlobalData(const Data &data)
{
    if (globalInstance().m_data == data)
        return;

    globalInstance().m_data = data;
    globalInstance().toSettings(Core::ICore::settings());
    emit globalInstance().changed(nullptr);
}

CppCodeModelSettings::PCHUsage CppCodeModelSettings::pchUsage(const Project *project)
{
    return CppCodeModelSettings::settingsForProject(project).pchUsage();
}

UsePrecompiledHeaders CppCodeModelSettings::usePrecompiledHeaders() const
{
    return pchUsage() == CppCodeModelSettings::PchUse_None ? UsePrecompiledHeaders::No
                                                           : UsePrecompiledHeaders::Yes;
}

UsePrecompiledHeaders CppCodeModelSettings::usePrecompiledHeaders(const Project *project)
{
    return CppCodeModelSettings::settingsForProject(project).usePrecompiledHeaders();
}

int CppCodeModelSettings::effectiveIndexerFileSizeLimitInMb() const
{
    return skipIndexingBigFiles() ? indexerFileSizeLimitInMb() : -1;
}

bool CppCodeModelSettings::categorizeFindReferences()
{
    return globalInstance().m_data.categorizeFindReferences;
}

void CppCodeModelSettings::setCategorizeFindReferences(bool categorize)
{
    globalInstance().m_data.categorizeFindReferences = categorize;
}

CppCodeModelProjectSettings::CppCodeModelProjectSettings(ProjectExplorer::Project *project)
    : m_project(project)
{
    loadSettings();
}

CppCodeModelSettings::Data CppCodeModelProjectSettings::data() const
{
    return m_useGlobalSettings ? CppCodeModelSettings::globalInstance().data() : m_customSettings;
}

void CppCodeModelProjectSettings::setData(const CppCodeModelSettings::Data &data)
{
    m_customSettings = data;
    saveSettings();
    emit CppCodeModelSettings::globalInstance().changed(m_project);
}

void CppCodeModelProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    m_useGlobalSettings = useGlobal;
    saveSettings();
    emit CppCodeModelSettings::globalInstance().changed(m_project);
}

void CppCodeModelProjectSettings::loadSettings()
{
    if (!m_project)
        return;
    const Store data = storeFromVariant(m_project->namedSettings(Constants::CPPEDITOR_SETTINGSGROUP));
    m_useGlobalSettings = data.value(useGlobalSettingsKey(), true).toBool();
    m_customSettings.fromMap(data);
}

void CppCodeModelProjectSettings::saveSettings()
{
    if (!m_project)
        return;
    Store data = m_customSettings.toMap();
    data.insert(useGlobalSettingsKey(), m_useGlobalSettings);
    m_project->setNamedSettings(Constants::CPPEDITOR_SETTINGSGROUP, variantFromStore(data));
}

} // namespace CppEditor
