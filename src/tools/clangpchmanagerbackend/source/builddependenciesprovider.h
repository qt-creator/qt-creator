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

#pragma once

#include "builddependenciesproviderinterface.h"

#include <modifiedtimecheckerinterface.h>

namespace Sqlite {
class TransactionInterface;
}

namespace ClangBackEnd {

class BuildDependenciesStorageInterface;
class BuildDependencyGeneratorInterface;

class BuildDependenciesProvider : public BuildDependenciesProviderInterface
{
public:
    BuildDependenciesProvider(BuildDependenciesStorageInterface &buildDependenciesStorage,
                              ModifiedTimeCheckerInterface<> &modifiedTimeChecker,
                              BuildDependencyGeneratorInterface &buildDependenciesGenerator,
                              Sqlite::TransactionInterface &transactionBackend)
        : m_storage(buildDependenciesStorage)
        , m_modifiedTimeChecker(modifiedTimeChecker)
        , m_generator(buildDependenciesGenerator)
        , m_transactionBackend(transactionBackend)
    {}

    BuildDependency create(const ProjectPartContainer &projectPart) override;
    BuildDependency create(const ProjectPartContainer &projectPart,
                           SourceEntries &&sourceEntries) override;

    void setEnsureAliveMessageIsSentCallback(std::function<void()> &&callback)
    {
        m_ensureAliveMessageIsSentCallback = std::move(callback);
    }

    SourceEntries createSourceEntriesFromStorage(const FilePathIds &sourcePathIds,
                                                 ProjectPartId projectPartId) const override;

private:
    BuildDependency createBuildDependencyFromStorage(SourceEntries &&includes) const;
    UsedMacros createUsedMacrosFromStorage(const SourceEntries &includes) const;
    void storeBuildDependency(const BuildDependency &buildDependency, ProjectPartId projectPartId);

private:
    BuildDependenciesStorageInterface &m_storage;
    ModifiedTimeCheckerInterface<> &m_modifiedTimeChecker;
    BuildDependencyGeneratorInterface &m_generator;
    Sqlite::TransactionInterface &m_transactionBackend;
    std::function<void()> m_ensureAliveMessageIsSentCallback;
};

} // namespace ClangBackEnd
