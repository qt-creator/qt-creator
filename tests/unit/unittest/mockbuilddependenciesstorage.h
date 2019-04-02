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

#include "googletest.h"

#include <builddependenciesstorageinterface.h>

class MockBuildDependenciesStorage : public ClangBackEnd::BuildDependenciesStorageInterface
{
public:
    MOCK_METHOD2(insertOrUpdateSources,
                 void(const ClangBackEnd::SourceEntries &sources,
                      ClangBackEnd::ProjectPartId projectPartId));
    MOCK_METHOD1(insertOrUpdateUsedMacros,
                 void (const ClangBackEnd::UsedMacros &usedMacros));
    MOCK_METHOD1(insertOrUpdateFileStatuses, void(const ClangBackEnd::FileStatuses &fileStatuses));
    MOCK_METHOD1(insertOrUpdateSourceDependencies,
                 void (const ClangBackEnd::SourceDependencies &sourceDependencies));
    MOCK_CONST_METHOD1(fetchLowestLastModifiedTime,
                       long long (ClangBackEnd::FilePathId sourceId));
    MOCK_CONST_METHOD2(fetchDependSources,
                       ClangBackEnd::SourceEntries(ClangBackEnd::FilePathId sourceId,
                                                   ClangBackEnd::ProjectPartId projectPartId));
    MOCK_CONST_METHOD1(fetchUsedMacros,
                       ClangBackEnd::UsedMacros (ClangBackEnd::FilePathId sourceId));
    MOCK_METHOD1(fetchProjectPartId,
                 ClangBackEnd::ProjectPartId(Utils::SmallStringView projectPartName));
    MOCK_METHOD2(updatePchCreationTimeStamp,
                 void(long long pchCreationTimeStamp, ClangBackEnd::ProjectPartId projectPartId));
};

