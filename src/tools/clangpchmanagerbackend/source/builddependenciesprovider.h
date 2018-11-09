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

namespace ClangBackEnd {

class BuildDependenciesStorageInterface;
class ModifiedTimeCheckerInterface;
class BuildDependenciesGeneratorInterface;

class BuildDependenciesProvider : public BuildDependenciesProviderInterface
{
public:
    BuildDependenciesProvider(BuildDependenciesStorageInterface &buildDependenciesStorage,
                              ModifiedTimeCheckerInterface &modifiedTimeChecker,
                              BuildDependenciesGeneratorInterface &buildDependenciesGenerator)
        : m_buildDependenciesStorage(buildDependenciesStorage),
          m_modifiedTimeChecker(modifiedTimeChecker),
          m_buildDependenciesGenerator(buildDependenciesGenerator)
    {
    }

    BuildDependency create(const V2::ProjectPartContainer &projectPart) const override;

private:
    BuildDependency createBuildDependencyFromStorage(SourceEntries &&includes) const;
    UsedMacros createUsedMacrosFromStorage(const SourceEntries &includes) const;
    SourceEntries createSourceEntriesFromStorage(const FilePathIds &sourcePathIds,
                                                 Utils::SmallStringView projectPartId) const;

private:
    BuildDependenciesStorageInterface &m_buildDependenciesStorage;
    ModifiedTimeCheckerInterface &m_modifiedTimeChecker;
    BuildDependenciesGeneratorInterface &m_buildDependenciesGenerator;
};

} // namespace ClangBackEnd
