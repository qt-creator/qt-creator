/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
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

#include "builddependenciesprovider.h"

#include "builddependenciesstorageinterface.h"
#include "modifiedtimecheckerinterface.h"
#include "builddependencygeneratorinterface.h"

#include <sqlitetransaction.h>

#include <algorithm>

namespace ClangBackEnd {

template<class OutputContainer,
         class InputContainer1,
         class InputContainer2>
OutputContainer setUnion(InputContainer1 &&input1,
                         InputContainer2 &&input2)
{
    OutputContainer results;
    results.reserve(input1.size() + input2.size());

    std::set_union(std::begin(input1),
                   std::end(input1),
                   std::begin(input2),
                   std::end(input2),
                   std::back_inserter(results));

    return results;
}

BuildDependency BuildDependenciesProvider::create(const ProjectPartContainer &projectPart)
{
    return create(projectPart,
                  createSourceEntriesFromStorage(projectPart.sourcePathIds, projectPart.projectPartId));
}

BuildDependency BuildDependenciesProvider::create(const ProjectPartContainer &projectPart,
                                                  SourceEntries &&sourceEntries)
{
    m_ensureAliveMessageIsSentCallback();

    if (!m_modifiedTimeChecker.isUpToDate(sourceEntries)) {
        BuildDependency buildDependency = m_generator.create(projectPart);

        storeBuildDependency(buildDependency, projectPart.projectPartId);

        return buildDependency;
    }

    return createBuildDependencyFromStorage(std::move(sourceEntries));
}

BuildDependency BuildDependenciesProvider::createBuildDependencyFromStorage(
    SourceEntries &&includes) const
{
    BuildDependency buildDependency;

    buildDependency.usedMacros = createUsedMacrosFromStorage(includes);
    buildDependency.sources = std::move(includes);

    return buildDependency;
}

UsedMacros BuildDependenciesProvider::createUsedMacrosFromStorage(const SourceEntries &includes) const
{
    UsedMacros usedMacros;
    usedMacros.reserve(1024);

    Sqlite::DeferredTransaction transaction(m_transactionBackend);

    for (const SourceEntry &entry : includes) {
        UsedMacros macros = m_storage.fetchUsedMacros(entry.sourceId);
        std::sort(macros.begin(), macros.end());
        usedMacros.insert(usedMacros.end(),
                          std::make_move_iterator(macros.begin()),
                          std::make_move_iterator(macros.end()));
    }

    transaction.commit();

    return usedMacros;
}

SourceEntries BuildDependenciesProvider::createSourceEntriesFromStorage(
    const FilePathIds &sourcePathIds, ProjectPartId projectPartId) const
{
    SourceEntries includes;

    Sqlite::DeferredTransaction transaction(m_transactionBackend);

    for (FilePathId sourcePathId : sourcePathIds) {
        SourceEntries entries = m_storage.fetchDependSources(sourcePathId, projectPartId);
        SourceEntries mergedEntries = setUnion<SourceEntries>(includes, entries);

        includes = std::move(mergedEntries);
    }

    transaction.commit();

    return includes;
}

void BuildDependenciesProvider::storeBuildDependency(const BuildDependency &buildDependency,
                                                     ProjectPartId projectPartId)
{
    Sqlite::ImmediateTransaction transaction(m_transactionBackend);
    m_storage.insertOrUpdateSources(buildDependency.sources, projectPartId);
    m_storage.insertOrUpdateFileStatuses(buildDependency.fileStatuses);
    m_storage.insertOrUpdateSourceDependencies(buildDependency.sourceDependencies);
    m_storage.insertOrUpdateUsedMacros(buildDependency.usedMacros);

    transaction.commit();
}

} // namespace ClangBackEnd
