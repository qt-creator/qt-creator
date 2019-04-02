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

#include "sourceentry.h"

#include <builddependency.h>
#include <filepathid.h>
#include <filestatus.h>
#include <projectpartid.h>
#include <sourcedependency.h>
#include <usedmacro.h>

#include <utils/optional.h>

namespace ClangBackEnd {

class BuildDependenciesStorageInterface
{
public:
    BuildDependenciesStorageInterface() = default;
    BuildDependenciesStorageInterface(const BuildDependenciesStorageInterface &) = delete;
    BuildDependenciesStorageInterface &operator=(const BuildDependenciesStorageInterface &) = delete;

    virtual void insertOrUpdateSources(const SourceEntries &sourceIds, ProjectPartId projectPartId) = 0;
    virtual void insertOrUpdateUsedMacros(const UsedMacros &usedMacros) = 0;
    virtual void
    insertOrUpdateFileStatuses(const FileStatuses &fileStatuses) = 0;
    virtual void insertOrUpdateSourceDependencies(const SourceDependencies &sourceDependencies) = 0;
    virtual long long fetchLowestLastModifiedTime(FilePathId sourceId) const = 0;
    virtual SourceEntries fetchDependSources(FilePathId sourceId,
                                             ProjectPartId projectPartId) const = 0;
    virtual UsedMacros fetchUsedMacros(FilePathId sourceId) const = 0;
    virtual ProjectPartId fetchProjectPartId(Utils::SmallStringView projectPartName) = 0;
    virtual void updatePchCreationTimeStamp(long long pchCreationTimeStamp, ProjectPartId projectPartId) = 0;

protected:
    ~BuildDependenciesStorageInterface() = default;
};

} // namespace ClangBackEnd
