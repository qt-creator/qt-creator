/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "cppcodemodelsettings.h"

#include "clangdiagnosticconfigsmodel.h"
#include "cppeditorconstants.h"
#include "cpptoolsreuse.h"

#include <coreplugin/icore.h>
#include <projectexplorer/project.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QSettings>

using namespace Utils;

namespace CppEditor {

static Id initialClangDiagnosticConfigId()
{ return Constants::CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM; }

static CppCodeModelSettings::PCHUsage initialPchUsage()
{ return CppCodeModelSettings::PchUse_BuildSystem; }

static QString clangDiagnosticConfigKey()
{ return QStringLiteral("ClangDiagnosticConfig"); }

static QString enableLowerClazyLevelsKey()
{ return QLatin1String("enableLowerClazyLevels"); }

static QString pchUsageKey()
{ return QLatin1String(Constants::CPPEDITOR_MODEL_MANAGER_PCH_USAGE); }

static QString interpretAmbiguousHeadersAsCHeadersKey()
{ return QLatin1String(Constants::CPPEDITOR_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS); }

static QString skipIndexingBigFilesKey()
{ return QLatin1String(Constants::CPPEDITOR_SKIP_INDEXING_BIG_FILES); }

static QString indexerFileSizeLimitKey()
{ return QLatin1String(Constants::CPPEDITOR_INDEXER_FILE_SIZE_LIMIT); }

static QString clangdSettingsKey() { return QLatin1String("ClangdSettings"); }
static QString useClangdKey() { return QLatin1String("UseClangd"); }
static QString clangdPathKey() { return QLatin1String("ClangdPath"); }
static QString clangdIndexingKey() { return QLatin1String("ClangdIndexing"); }
static QString clangdThreadLimitKey() { return QLatin1String("ClangdThreadLimit"); }
static QString clangdDocumentThresholdKey() { return QLatin1String("ClangdDocumentThreshold"); }
static QString clangdUseGlobalSettingsKey() { return QLatin1String("useGlobalSettings"); }

static FilePath g_defaultClangdFilePath;
static FilePath fallbackClangdFilePath()
{
    if (g_defaultClangdFilePath.exists())
        return g_defaultClangdFilePath;
    return "clangd";
}

static Id clangDiagnosticConfigIdFromSettings(QSettings *s)
{
    QTC_ASSERT(s->group() == QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP), return Id());

    return Id::fromSetting(
        s->value(clangDiagnosticConfigKey(), initialClangDiagnosticConfigId().toSetting()));
}

// Removed since Qt Creator 4.11
static ClangDiagnosticConfigs removedBuiltinConfigs()
{
    ClangDiagnosticConfigs configs;

    // Pedantic
    ClangDiagnosticConfig config;
    config.setId("Builtin.Pedantic");
    config.setDisplayName(QCoreApplication::translate("ClangDiagnosticConfigsModel",
                                                      "Pedantic checks"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{QStringLiteral("-Wpedantic")});
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    configs << config;

    // Everything with exceptions
    config = ClangDiagnosticConfig();
    config.setId("Builtin.EverythingWithExceptions");
    config.setDisplayName(QCoreApplication::translate(
                              "ClangDiagnosticConfigsModel",
                              "Checks for almost everything"));
    config.setIsReadOnly(true);
    config.setClangOptions(QStringList{
        QStringLiteral("-Weverything"),
        QStringLiteral("-Wno-c++98-compat"),
        QStringLiteral("-Wno-c++98-compat-pedantic"),
        QStringLiteral("-Wno-unused-macros"),
        QStringLiteral("-Wno-newline-eof"),
        QStringLiteral("-Wno-exit-time-destructors"),
        QStringLiteral("-Wno-global-constructors"),
        QStringLiteral("-Wno-gnu-zero-variadic-macro-arguments"),
        QStringLiteral("-Wno-documentation"),
        QStringLiteral("-Wno-shadow"),
        QStringLiteral("-Wno-switch-enum"),
        QStringLiteral("-Wno-missing-prototypes"), // Not optimal for C projects.
        QStringLiteral("-Wno-used-but-marked-unused"), // e.g. QTest::qWait
    });
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    configs << config;

    return configs;
}

static ClangDiagnosticConfig convertToCustomConfig(const Id &id)
{
    const ClangDiagnosticConfig config
        = findOrDefault(removedBuiltinConfigs(), [id](const ClangDiagnosticConfig &config) {
              return config.id() == id;
          });
    return ClangDiagnosticConfigsModel::createCustomConfig(config, config.displayName());
}

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP));

    setClangCustomDiagnosticConfigs(diagnosticConfigsFromSettings(s));
    setClangDiagnosticConfigId(clangDiagnosticConfigIdFromSettings(s));

    // Qt Creator 4.11 removes some built-in configs.
    bool write = false;
    const Id id = m_clangDiagnosticConfigId;
    if (id == "Builtin.Pedantic" || id == "Builtin.EverythingWithExceptions") {
        // If one of them was used, continue to use it, but convert it to a custom config.
        const ClangDiagnosticConfig customConfig = convertToCustomConfig(id);
        m_clangCustomDiagnosticConfigs.append(customConfig);
        m_clangDiagnosticConfigId = customConfig.id();
        write = true;
    }

    // Before Qt Creator 4.8, inconsistent settings might have been written.
    const ClangDiagnosticConfigsModel model = diagnosticConfigsModel(m_clangCustomDiagnosticConfigs);
    if (!model.hasConfigWithId(m_clangDiagnosticConfigId))
        setClangDiagnosticConfigId(initialClangDiagnosticConfigId());

    setEnableLowerClazyLevels(s->value(enableLowerClazyLevelsKey(), true).toBool());

    const QVariant pchUsageVariant = s->value(pchUsageKey(), initialPchUsage());
    setPCHUsage(static_cast<PCHUsage>(pchUsageVariant.toInt()));

    const QVariant interpretAmbiguousHeadersAsCHeaders
            = s->value(interpretAmbiguousHeadersAsCHeadersKey(), false);
    setInterpretAmbigiousHeadersAsCHeaders(interpretAmbiguousHeadersAsCHeaders.toBool());

    const QVariant skipIndexingBigFiles = s->value(skipIndexingBigFilesKey(), true);
    setSkipIndexingBigFiles(skipIndexingBigFiles.toBool());

    const QVariant indexerFileSizeLimit = s->value(indexerFileSizeLimitKey(), 5);
    setIndexerFileSizeLimitInMb(indexerFileSizeLimit.toInt());

    s->endGroup();

    if (write)
        toSettings(s);

    emit changed();
}

void CppCodeModelSettings::toSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPEDITOR_SETTINGSGROUP));
    const ClangDiagnosticConfigs previousConfigs = diagnosticConfigsFromSettings(s);
    const Id previousConfigId = clangDiagnosticConfigIdFromSettings(s);

    diagnosticConfigsToSettings(s, m_clangCustomDiagnosticConfigs);

    s->setValue(clangDiagnosticConfigKey(), clangDiagnosticConfigId().toSetting());
    s->setValue(enableLowerClazyLevelsKey(), enableLowerClazyLevels());
    s->setValue(pchUsageKey(), pchUsage());

    s->setValue(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbigiousHeadersAsCHeaders());
    s->setValue(skipIndexingBigFilesKey(), skipIndexingBigFiles());
    s->setValue(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb());

    s->endGroup();

    QVector<Id> invalidated
        = ClangDiagnosticConfigsModel::changedOrRemovedConfigs(previousConfigs,
                                                               m_clangCustomDiagnosticConfigs);

    if (previousConfigId != clangDiagnosticConfigId() && !invalidated.contains(previousConfigId))
        invalidated.append(previousConfigId);

    if (!invalidated.isEmpty())
        emit clangDiagnosticConfigsInvalidated(invalidated);
    emit changed();
}

Id CppCodeModelSettings::clangDiagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(m_clangDiagnosticConfigId))
        return defaultClangDiagnosticConfigId();
    return m_clangDiagnosticConfigId;
}

void CppCodeModelSettings::setClangDiagnosticConfigId(const Id &configId)
{
    m_clangDiagnosticConfigId = configId;
}

Id CppCodeModelSettings::defaultClangDiagnosticConfigId()
{
    return initialClangDiagnosticConfigId();
}

const ClangDiagnosticConfig CppCodeModelSettings::clangDiagnosticConfig() const
{
    const ClangDiagnosticConfigsModel configsModel = diagnosticConfigsModel(
        m_clangCustomDiagnosticConfigs);

    return configsModel.configWithId(clangDiagnosticConfigId());
}

ClangDiagnosticConfigs CppCodeModelSettings::clangCustomDiagnosticConfigs() const
{
    return m_clangCustomDiagnosticConfigs;
}

void CppCodeModelSettings::setClangCustomDiagnosticConfigs(const ClangDiagnosticConfigs &configs)
{
    m_clangCustomDiagnosticConfigs = configs;
}

CppCodeModelSettings::PCHUsage CppCodeModelSettings::pchUsage() const
{
    return m_pchUsage;
}

void CppCodeModelSettings::setPCHUsage(CppCodeModelSettings::PCHUsage pchUsage)
{
    m_pchUsage = pchUsage;
}

bool CppCodeModelSettings::interpretAmbigiousHeadersAsCHeaders() const
{
    return m_interpretAmbigiousHeadersAsCHeaders;
}

void CppCodeModelSettings::setInterpretAmbigiousHeadersAsCHeaders(bool yesno)
{
    m_interpretAmbigiousHeadersAsCHeaders = yesno;
}

bool CppCodeModelSettings::skipIndexingBigFiles() const
{
    return m_skipIndexingBigFiles;
}

void CppCodeModelSettings::setSkipIndexingBigFiles(bool yesno)
{
    m_skipIndexingBigFiles = yesno;
}

int CppCodeModelSettings::indexerFileSizeLimitInMb() const
{
    return m_indexerFileSizeLimitInMB;
}

void CppCodeModelSettings::setIndexerFileSizeLimitInMb(int sizeInMB)
{
    m_indexerFileSizeLimitInMB = sizeInMB;
}

bool CppCodeModelSettings::enableLowerClazyLevels() const
{
    return m_enableLowerClazyLevels;
}

void CppCodeModelSettings::setEnableLowerClazyLevels(bool yesno)
{
    m_enableLowerClazyLevels = yesno;
}


ClangdSettings &ClangdSettings::instance()
{
    static ClangdSettings settings;
    return settings;
}

void ClangdSettings::setDefaultClangdPath(const FilePath &filePath)
{
    g_defaultClangdFilePath = filePath;
}

FilePath ClangdSettings::clangdFilePath() const
{
    if (!m_data.executableFilePath.isEmpty())
        return m_data.executableFilePath;
    return fallbackClangdFilePath();
}

void ClangdSettings::setData(const Data &data)
{
    if (this == &instance() && data != m_data) {
        m_data = data;
        saveSettings();
        emit changed();
    }
}

void ClangdSettings::loadSettings()
{
    m_data.fromMap(Core::ICore::settings()->value(clangdSettingsKey()).toMap());
}

void ClangdSettings::saveSettings()
{
    Core::ICore::settings()->setValue(clangdSettingsKey(), m_data.toMap());
}

#ifdef WITH_TESTS
void ClangdSettings::setUseClangd(bool use) { instance().m_data.useClangd = use; }

void ClangdSettings::setClangdFilePath(const FilePath &filePath)
{
    instance().m_data.executableFilePath = filePath;
}
#endif

ClangdProjectSettings::ClangdProjectSettings(ProjectExplorer::Project *project) : m_project(project)
{
    loadSettings();
}

ClangdSettings::Data ClangdProjectSettings::settings() const
{
    if (m_useGlobalSettings)
        return ClangdSettings::instance().data();
    return m_customSettings;
}

void ClangdProjectSettings::setSettings(const ClangdSettings::Data &data)
{
    m_customSettings = data;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::setUseGlobalSettings(bool useGlobal)
{
    m_useGlobalSettings = useGlobal;
    saveSettings();
    emit ClangdSettings::instance().changed();
}

void ClangdProjectSettings::loadSettings()
{
    if (!m_project)
        return;
    const QVariantMap data = m_project->namedSettings(clangdSettingsKey()).toMap();
    m_useGlobalSettings = data.value(clangdUseGlobalSettingsKey(), true).toBool();
    if (!m_useGlobalSettings)
        m_customSettings.fromMap(data);
}

void ClangdProjectSettings::saveSettings()
{
    if (!m_project)
        return;
    QVariantMap data;
    if (!m_useGlobalSettings)
        data = m_customSettings.toMap();
    data.insert(clangdUseGlobalSettingsKey(), m_useGlobalSettings);
    m_project->setNamedSettings(clangdSettingsKey(), data);
}

QVariantMap ClangdSettings::Data::toMap() const
{
    QVariantMap map;
    map.insert(useClangdKey(), useClangd);
    if (executableFilePath != fallbackClangdFilePath())
        map.insert(clangdPathKey(), executableFilePath.toString());
    map.insert(clangdIndexingKey(), enableIndexing);
    map.insert(clangdThreadLimitKey(), workerThreadLimit);
    map.insert(clangdDocumentThresholdKey(), documentUpdateThreshold);
    return map;
}

void ClangdSettings::Data::fromMap(const QVariantMap &map)
{
    useClangd = map.value(useClangdKey(), false).toBool();
    executableFilePath = FilePath::fromString(map.value(clangdPathKey()).toString());
    enableIndexing = map.value(clangdIndexingKey(), true).toBool();
    workerThreadLimit = map.value(clangdThreadLimitKey(), 0).toInt();
    documentUpdateThreshold = map.value(clangdDocumentThresholdKey(), 500).toInt();
}

} // namespace CppEditor
