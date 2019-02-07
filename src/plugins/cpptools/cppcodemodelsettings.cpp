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

#include <utils/qtcassert.h>

#include <QSettings>

using namespace CppTools;

static Core::Id initialClangDiagnosticConfigId()
{ return {Constants::CPP_CLANG_BUILTIN_CONFIG_ID_EVERYTHING_WITH_EXCEPTIONS}; }

static CppCodeModelSettings::PCHUsage initialPchUsage()
{ return CppCodeModelSettings::PchUse_BuildSystem; }

static QString clangDiagnosticConfigKey()
{ return QStringLiteral("ClangDiagnosticConfig"); }

static QString clangDiagnosticConfigsArrayKey()
{ return QStringLiteral("ClangDiagnosticConfigs"); }

static QString clangDiagnosticConfigsArrayIdKey()
{ return QLatin1String("id"); }

static QString clangDiagnosticConfigsArrayDisplayNameKey()
{ return QLatin1String("displayName"); }

static QString clangDiagnosticConfigsArrayWarningsKey()
{ return QLatin1String("diagnosticOptions"); }

static QString clangDiagnosticConfigsArrayClangTidyChecksKey()
{ return QLatin1String("clangTidyChecks"); }

static QString clangDiagnosticConfigsArrayClangTidyModeKey()
{ return QLatin1String("clangTidyMode"); }

static QString clangDiagnosticConfigsArrayClazyChecksKey()
{ return QLatin1String("clazyChecks"); }

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

static QString convertToNewClazyChecksFormat(const QString &checks)
{
    // Before Qt Creator 4.9 valid values for checks were: "", "levelN".
    // Starting with Qt Creator 4.9, checks are a comma-separated string of checks: "x,y,z".

    if (checks.isEmpty())
        return checks;

    if (checks.size() == 6 && checks.startsWith("level")) {
        bool ok = false;
        const int level = checks.midRef(5).toInt(&ok);
        QTC_ASSERT(ok, return QString());
        return clazyChecksForLevel(level);
    }

    return checks;
}

static ClangDiagnosticConfigs customDiagnosticConfigsFromSettings(QSettings *s)
{
    QTC_ASSERT(s->group() == QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP),
               return ClangDiagnosticConfigs());

    ClangDiagnosticConfigs configs;

    const int size = s->beginReadArray(clangDiagnosticConfigsArrayKey());
    for (int i = 0; i < size; ++i) {
        s->setArrayIndex(i);

        ClangDiagnosticConfig config;
        config.setId(Core::Id::fromSetting(s->value(clangDiagnosticConfigsArrayIdKey())));
        config.setDisplayName(s->value(clangDiagnosticConfigsArrayDisplayNameKey()).toString());
        config.setClangOptions(s->value(clangDiagnosticConfigsArrayWarningsKey()).toStringList());
        config.setClangTidyMode(static_cast<ClangDiagnosticConfig::TidyMode>(
                                    s->value(clangDiagnosticConfigsArrayClangTidyModeKey()).toInt()));
        config.setClangTidyChecks(
                    s->value(clangDiagnosticConfigsArrayClangTidyChecksKey()).toString());

        const QString clazyChecks = s->value(clangDiagnosticConfigsArrayClazyChecksKey()).toString();
        config.setClazyChecks(convertToNewClazyChecksFormat(clazyChecks));
        configs.append(config);
    }
    s->endArray();

    return configs;
}

static Core::Id clangDiagnosticConfigIdFromSettings(QSettings *s)
{
    QTC_ASSERT(s->group() == QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP), return Core::Id());

    return Core::Id::fromSetting(
        s->value(clangDiagnosticConfigKey(), initialClangDiagnosticConfigId().toSetting()));
}

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));

    setClangCustomDiagnosticConfigs(customDiagnosticConfigsFromSettings(s));
    setClangDiagnosticConfigId(clangDiagnosticConfigIdFromSettings(s));

    { // Before Qt Creator 4.8, inconsistent settings might have been written.
        const ClangDiagnosticConfigsModel model(m_clangCustomDiagnosticConfigs);
        if (!model.hasConfigWithId(m_clangDiagnosticConfigId))
            setClangDiagnosticConfigId(initialClangDiagnosticConfigId());
    }

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

    emit changed();
}

void CppCodeModelSettings::toSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));
    const ClangDiagnosticConfigs previousConfigs = customDiagnosticConfigsFromSettings(s);
    const Core::Id previousConfigId = clangDiagnosticConfigIdFromSettings(s);

    s->beginWriteArray(clangDiagnosticConfigsArrayKey());
    for (int i = 0, size = m_clangCustomDiagnosticConfigs.size(); i < size; ++i) {
        const ClangDiagnosticConfig &config = m_clangCustomDiagnosticConfigs.at(i);

        s->setArrayIndex(i);
        s->setValue(clangDiagnosticConfigsArrayIdKey(), config.id().toSetting());
        s->setValue(clangDiagnosticConfigsArrayDisplayNameKey(), config.displayName());
        s->setValue(clangDiagnosticConfigsArrayWarningsKey(), config.clangOptions());
        s->setValue(clangDiagnosticConfigsArrayClangTidyModeKey(),
                    static_cast<int>(config.clangTidyMode()));
        s->setValue(clangDiagnosticConfigsArrayClangTidyChecksKey(),
                    config.clangTidyChecks());
        s->setValue(clangDiagnosticConfigsArrayClazyChecksKey(), config.clazyChecks());
    }
    s->endArray();

    s->setValue(clangDiagnosticConfigKey(), clangDiagnosticConfigId().toSetting());
    s->setValue(enableLowerClazyLevelsKey(), enableLowerClazyLevels());
    s->setValue(pchUsageKey(), pchUsage());

    s->setValue(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbigiousHeadersAsCHeaders());
    s->setValue(skipIndexingBigFilesKey(), skipIndexingBigFiles());
    s->setValue(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb());

    s->endGroup();

    QVector<Core::Id> invalidated
        = ClangDiagnosticConfigsModel::changedOrRemovedConfigs(previousConfigs,
                                                               m_clangCustomDiagnosticConfigs);

    if (previousConfigId != clangDiagnosticConfigId() && !invalidated.contains(previousConfigId))
        invalidated.append(previousConfigId);

    if (!invalidated.isEmpty())
        emit clangDiagnosticConfigsInvalidated(invalidated);
    emit changed();
}

Core::Id CppCodeModelSettings::clangDiagnosticConfigId() const
{
    return m_clangDiagnosticConfigId;
}

void CppCodeModelSettings::setClangDiagnosticConfigId(const Core::Id &configId)
{
    m_clangDiagnosticConfigId = configId;
}

void CppCodeModelSettings::resetClangDiagnosticConfigId()
{
    m_clangDiagnosticConfigId = initialClangDiagnosticConfigId();
}

const ClangDiagnosticConfig CppCodeModelSettings::clangDiagnosticConfig() const
{
    const ClangDiagnosticConfigsModel configsModel(m_clangCustomDiagnosticConfigs);

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
