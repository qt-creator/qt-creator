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

#include "clangdiagnosticconfigsmodel.h"

#include "cpptoolsreuse.h"
#include "cpptoolsconstants.h"

#include <utils/algorithm.h>

#include <QCoreApplication>
#include <QUuid>

namespace CppTools {

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

QVector<Utils::Id> ClangDiagnosticConfigsModel::changedOrRemovedConfigs(
    const ClangDiagnosticConfigs &oldConfigs, const ClangDiagnosticConfigs &newConfigs)
{
    ClangDiagnosticConfigsModel newConfigsModel(newConfigs);
    QVector<Utils::Id> changedConfigs;

    for (const ClangDiagnosticConfig &old: oldConfigs) {
        const int i = newConfigsModel.indexOfConfig(old.id());
        if (i == -1)
            changedConfigs.append(old.id()); // Removed
        else if (newConfigsModel.allConfigs().value(i) != old)
            changedConfigs.append(old.id()); // Changed
    }

    return changedConfigs;
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

} // namespace CppTools
