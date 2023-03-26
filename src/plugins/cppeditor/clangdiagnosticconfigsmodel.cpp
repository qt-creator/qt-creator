// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdiagnosticconfigsmodel.h"

#include "cppeditorconstants.h"
#include "cpptoolsreuse.h"

#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QUuid>

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
    copied.setId(Utils::Id::fromString(QUuid::createUuid().toString()));
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

int ClangDiagnosticConfigsModel::indexOfConfig(const Utils::Id &id) const
{
    return Utils::indexOf(m_diagnosticConfigs, [&](const ClangDiagnosticConfig &config) {
       return config.id() == id;
    });
}

} // namespace CppEditor
