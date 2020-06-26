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
#include "cpptoolsconstants.h"
#include "cpptoolsreuse.h"

#include <utils/algorithm.h>
#include <utils/qtcassert.h>

#include <QSettings>

using namespace CppTools;

static Utils::Id initialClangDiagnosticConfigId()
{ return Constants::CPP_CLANG_DIAG_CONFIG_QUESTIONABLE; }

static CppCodeModelSettings::PCHUsage initialPchUsage()
{ return CppCodeModelSettings::PchUse_BuildSystem; }

static QString clangDiagnosticConfigKey()
{ return QStringLiteral("ClangDiagnosticConfig"); }

static QString enableLowerClazyLevelsKey()
{ return QLatin1String("enableLowerClazyLevels"); }

static QString pchUsageKey()
{ return QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE); }

static QString interpretAmbiguousHeadersAsCHeadersKey()
{ return QLatin1String(Constants::CPPTOOLS_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS); }

static QString skipIndexingBigFilesKey()
{ return QLatin1String(Constants::CPPTOOLS_SKIP_INDEXING_BIG_FILES); }

static QString indexerFileSizeLimitKey()
{ return QLatin1String(Constants::CPPTOOLS_INDEXER_FILE_SIZE_LIMIT); }

static Utils::Id clangDiagnosticConfigIdFromSettings(QSettings *s)
{
    QTC_ASSERT(s->group() == QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP), return Utils::Id());

    return Utils::Id::fromSetting(
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

static ClangDiagnosticConfig convertToCustomConfig(const Utils::Id &id)
{
    const ClangDiagnosticConfig config
        = Utils::findOrDefault(removedBuiltinConfigs(), [id](const ClangDiagnosticConfig &config) {
              return config.id() == id;
          });
    return ClangDiagnosticConfigsModel::createCustomConfig(config, config.displayName());
}

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));

    setClangCustomDiagnosticConfigs(diagnosticConfigsFromSettings(s));
    setClangDiagnosticConfigId(clangDiagnosticConfigIdFromSettings(s));

    // Qt Creator 4.11 removes some built-in configs.
    bool write = false;
    const Utils::Id id = m_clangDiagnosticConfigId;
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
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    const ClangDiagnosticConfigs previousConfigs = diagnosticConfigsFromSettings(s);
    const Utils::Id previousConfigId = clangDiagnosticConfigIdFromSettings(s);

    diagnosticConfigsToSettings(s, m_clangCustomDiagnosticConfigs);

    s->setValue(clangDiagnosticConfigKey(), clangDiagnosticConfigId().toSetting());
    s->setValue(enableLowerClazyLevelsKey(), enableLowerClazyLevels());
    s->setValue(pchUsageKey(), pchUsage());

    s->setValue(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbigiousHeadersAsCHeaders());
    s->setValue(skipIndexingBigFilesKey(), skipIndexingBigFiles());
    s->setValue(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb());

    s->endGroup();

    QVector<Utils::Id> invalidated
        = ClangDiagnosticConfigsModel::changedOrRemovedConfigs(previousConfigs,
                                                               m_clangCustomDiagnosticConfigs);

    if (previousConfigId != clangDiagnosticConfigId() && !invalidated.contains(previousConfigId))
        invalidated.append(previousConfigId);

    if (!invalidated.isEmpty())
        emit clangDiagnosticConfigsInvalidated(invalidated);
    emit changed();
}

Utils::Id CppCodeModelSettings::clangDiagnosticConfigId() const
{
    if (!diagnosticConfigsModel().hasConfigWithId(m_clangDiagnosticConfigId))
        return defaultClangDiagnosticConfigId();
    return m_clangDiagnosticConfigId;
}

void CppCodeModelSettings::setClangDiagnosticConfigId(const Utils::Id &configId)
{
    m_clangDiagnosticConfigId = configId;
}

Utils::Id CppCodeModelSettings::defaultClangDiagnosticConfigId()
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
