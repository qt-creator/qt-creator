/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#include "filestatus.h"
#include "projectpartentry.h"
#include "projectpartpch.h"
#include "projectpartartefact.h"
#include "sourcelocationentry.h"
#include "sourcedependency.h"
#include "symbolentry.h"
#include "usedmacro.h"

#include <sqlitetransaction.h>

#include <compilermacro.h>

namespace ClangBackEnd {

class SymbolStorageInterface
{
public:
    SymbolStorageInterface() = default;
    SymbolStorageInterface(const SymbolStorageInterface &) = delete;
    SymbolStorageInterface &operator=(const SymbolStorageInterface &) = delete;

    virtual void addSymbolsAndSourceLocations(const SymbolEntries &symbolEntries,
                                              const SourceLocationEntries &sourceLocations) = 0;
    virtual int insertOrUpdateProjectPart(Utils::SmallStringView projectPartName,
                                          const Utils::SmallStringVector &commandLineArguments,
                                          const CompilerMacros &compilerMacros,
                                          const Utils::SmallStringVector &includeSearchPaths) = 0;
    virtual void updateProjectPartSources(int projectPartId,
                                          const FilePathIds &sourceFilePathIds) = 0;
    virtual Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(FilePathId sourceId) const = 0;
    virtual Utils::optional<ProjectPartArtefact> fetchProjectPartArtefact(Utils::SmallStringView projectPartName) const = 0;
    virtual Utils::optional<ProjectPartPch> fetchPrecompiledHeader(int projectPartId) const = 0;

protected:
    ~SymbolStorageInterface() = default;
};

} // namespace ClangBackEnd
