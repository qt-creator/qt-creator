// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cmakecommandkeywords.h"

#include "cmaketoolmanager.h"

#include <projectexplorer/buildsystem.h>

#include <utils/algorithm.h>

namespace CMakeProjectManager::Internal {

void CommandKeywords::refresh()
{
    if (m_cmakeKeywords.functionArgs.isEmpty()) {
        const CMakeKeywords keywords = CMakeToolManager::defaultProjectOrDefaultCMakeKeyWords();
        if (!keywords.functionArgs.isEmpty()) {
            m_cmakeKeywords = keywords;
            m_perCommand.clear();
        }
    }

    auto buildSystem = qobject_cast<CMakeBuildSystem *>(
        ProjectExplorer::activeBuildSystemForCurrentProject());
    if (!buildSystem)
        return;

    const int generation = buildSystem->commandSignaturesGeneration();
    if (buildSystem == m_buildSystem && generation == m_generation)
        return;

    m_buildSystem = buildSystem;
    m_generation = generation;
    m_signatures = buildSystem->commandSignatures();
    m_perCommand.clear();
}

bool CommandKeywords::contains(const QString &command, const QString &argument)
{
    auto it = m_perCommand.find(command);
    if (it == m_perCommand.end()) {
        QSet<QString> keywords = Utils::toSet(
            m_cmakeKeywords.functionArgs.value(command.toLower()));
        keywords += Utils::toSet(m_signatures.signature(command).keywords());
        it = m_perCommand.insert(command, keywords);
    }
    return it->contains(argument);
}

} // namespace CMakeProjectManager::Internal
