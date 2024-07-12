// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigsmodel.h"

#include "cppeditorconstants.h"
#include "cppeditortr.h"
#include "cpptoolsreuse.h"

#include <utils/algorithm.h>

#include <QCoreApplication>

namespace CppEditor {

ClangDiagnosticConfigsModel::ClangDiagnosticConfigsModel(const ClangDiagnosticConfigs &configs)
{
    m_diagnosticConfigs.append(configs);
}

int ClangDiagnosticConfigsModel::size() const
{
    return m_diagnosticConfigs.size();
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsModel::at(int index) const
{
    return m_diagnosticConfigs.at(index);
}

void ClangDiagnosticConfigsModel::appendOrUpdate(const ClangDiagnosticConfig &config)
{
    const int index = indexOfConfig(config.id());

    if (index >= 0 && index < m_diagnosticConfigs.size())
        m_diagnosticConfigs.replace(index, config);
    else
        m_diagnosticConfigs.append(config);
}

void ClangDiagnosticConfigsModel::removeConfigWithId(const Utils::Id &id)
{
    m_diagnosticConfigs.removeOne(configWithId(id));
}

ClangDiagnosticConfigs ClangDiagnosticConfigsModel::allConfigs() const
{
    return m_diagnosticConfigs;
}

ClangDiagnosticConfigs ClangDiagnosticConfigsModel::customConfigs() const
{
    return Utils::filtered(allConfigs(), [](const ClangDiagnosticConfig &config) {
        return !config.isReadOnly();
    });
}

bool ClangDiagnosticConfigsModel::hasConfigWithId(const Utils::Id &id) const
{
    return indexOfConfig(id) != -1;
}

const ClangDiagnosticConfig &ClangDiagnosticConfigsModel::configWithId(const Utils::Id &id) const
{
    return m_diagnosticConfigs.at(indexOfConfig(id));
}

ClangDiagnosticConfig ClangDiagnosticConfigsModel::createCustomConfig(
    const ClangDiagnosticConfig &baseConfig, const QString &displayName)
{
    ClangDiagnosticConfig copied = baseConfig;
    copied.setId(Utils::Id::generate());
    copied.setDisplayName(displayName);
    copied.setIsReadOnly(false);

    return copied;
}

QStringList ClangDiagnosticConfigsModel::globalDiagnosticOptions()
{
    return {
        // Avoid undesired warnings from e.g. Q_OBJECT
        QStringLiteral("-Wno-unknown-pragmas"),
        QStringLiteral("-Wno-unknown-warning-option"),

        // qdoc commands
        QStringLiteral("-Wno-documentation-unknown-command")
    };
}

void ClangDiagnosticConfigsModel::addBuiltinConfigs()
{
    ClangDiagnosticConfig config;

    // Questionable constructs
    config = ClangDiagnosticConfig();
    config.setId(Constants::CPP_CLANG_DIAG_CONFIG_QUESTIONABLE);
    config.setDisplayName(Tr::tr("Checks for questionable constructs"));
    config.setIsReadOnly(true);
    config.setClangOptions({
        "-Wall",
        "-Wextra",
    });
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    appendOrUpdate(config);

    // Warning flags from build system
    config = ClangDiagnosticConfig();
    config.setId(Constants::CPP_CLANG_DIAG_CONFIG_BUILDSYSTEM);
    config.setDisplayName(Tr::tr("Build-system warnings"));
    config.setIsReadOnly(true);
    config.setClazyMode(ClangDiagnosticConfig::ClazyMode::UseCustomChecks);
    config.setClangTidyMode(ClangDiagnosticConfig::TidyMode::UseCustomChecks);
    config.setUseBuildSystemWarnings(true);
    appendOrUpdate(config);
}

int ClangDiagnosticConfigsModel::indexOfConfig(const Utils::Id &id) const
{
    return Utils::indexOf(m_diagnosticConfigs, [&](const ClangDiagnosticConfig &config) {
       return config.id() == id;
    });
}

} // namespace CppEditor
