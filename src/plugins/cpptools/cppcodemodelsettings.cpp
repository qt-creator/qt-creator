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

#include <utils/qtcassert.h>

#include <QSettings>

using namespace CppTools;

static Core::Id initialClangDiagnosticConfigId()
{ return Core::Id(Constants::CPP_CLANG_BUILTIN_CONFIG_ID_EVERYTHING_WITH_EXCEPTIONS); }

static CppCodeModelSettings::PCHUsage initialPchUsage()
{ return CppCodeModelSettings::PchUse_None; }

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

static QString pchUsageKey()
{ return QLatin1String(Constants::CPPTOOLS_MODEL_MANAGER_PCH_USAGE); }

static QString interpretAmbiguousHeadersAsCHeadersKey()
{ return QLatin1String(Constants::CPPTOOLS_INTERPRET_AMBIGIUOUS_HEADERS_AS_C_HEADERS); }

static QString skipIndexingBigFilesKey()
{ return QLatin1String(Constants::CPPTOOLS_SKIP_INDEXING_BIG_FILES); }

static QString indexerFileSizeLimitKey()
{ return QLatin1String(Constants::CPPTOOLS_INDEXER_FILE_SIZE_LIMIT); }

void CppCodeModelSettings::fromSettings(QSettings *s)
{
    s->beginGroup(QLatin1String(Constants::CPPTOOLS_SETTINGSGROUP));

    const int size = s->beginReadArray(clangDiagnosticConfigsArrayKey());
    for (int i = 0; i < size; ++i) {
        s->setArrayIndex(i);

        ClangDiagnosticConfig config;
        config.setId(Core::Id::fromSetting(s->value(clangDiagnosticConfigsArrayIdKey())));
        config.setDisplayName(s->value(clangDiagnosticConfigsArrayDisplayNameKey()).toString());
        config.setCommandLineWarnings(s->value(clangDiagnosticConfigsArrayWarningsKey()).toStringList());
        m_clangCustomDiagnosticConfigs.append(config);
    }
    s->endArray();

    const Core::Id diagnosticConfigId = Core::Id::fromSetting(
                                            s->value(clangDiagnosticConfigKey(),
                                                     initialClangDiagnosticConfigId().toSetting()));
    setClangDiagnosticConfigId(diagnosticConfigId);

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

    s->beginWriteArray(clangDiagnosticConfigsArrayKey());
    for (int i = 0, size = m_clangCustomDiagnosticConfigs.size(); i < size; ++i) {
        const ClangDiagnosticConfig &config = m_clangCustomDiagnosticConfigs.at(i);

        s->setArrayIndex(i);
        s->setValue(clangDiagnosticConfigsArrayIdKey(), config.id().toSetting());
        s->setValue(clangDiagnosticConfigsArrayDisplayNameKey(), config.displayName());
        s->setValue(clangDiagnosticConfigsArrayWarningsKey(), config.commandLineWarnings());
    }
    s->endArray();

    s->setValue(clangDiagnosticConfigKey(), clangDiagnosticConfigId().toSetting());
    s->setValue(pchUsageKey(), pchUsage());

    s->setValue(interpretAmbiguousHeadersAsCHeadersKey(), interpretAmbigiousHeadersAsCHeaders());
    s->setValue(skipIndexingBigFilesKey(), skipIndexingBigFiles());
    s->setValue(indexerFileSizeLimitKey(), indexerFileSizeLimitInMb());

    s->endGroup();

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

void CppCodeModelSettings::emitChanged()
{
    emit changed();
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
