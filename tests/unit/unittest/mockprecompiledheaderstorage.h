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

#include "googletest.h"

#include <precompiledheaderstorageinterface.h>

class MockPrecompiledHeaderStorage : public ClangBackEnd::PrecompiledHeaderStorageInterface
{
public:
    MOCK_METHOD3(insertProjectPrecompiledHeader,
                 void(ClangBackEnd::ProjectPartId projectPartId,
                      Utils::SmallStringView pchPath,
                      ClangBackEnd::TimeStamp pchBuildTime));
    MOCK_METHOD2(deleteProjectPrecompiledHeader,
                 void(ClangBackEnd::ProjectPartId projectPartId, ClangBackEnd::TimeStamp buildTime));
    MOCK_METHOD1(deleteProjectPrecompiledHeaders,
                 void(const ClangBackEnd::ProjectPartIds &projectPartIds));
    MOCK_METHOD3(insertSystemPrecompiledHeaders,
                 void(const ClangBackEnd::ProjectPartIds &projectPartIds,
                      Utils::SmallStringView pchPath,
                      ClangBackEnd::TimeStamp pchBuildTime));
    MOCK_METHOD1(deleteSystemPrecompiledHeaders,
                 void(const ClangBackEnd::ProjectPartIds &projectPartIds));
    MOCK_METHOD1(deleteSystemAndProjectPrecompiledHeaders,
                 void(const ClangBackEnd::ProjectPartIds &projectPartIds));
    MOCK_METHOD1(fetchSystemPrecompiledHeaderPath,
                 ClangBackEnd::FilePath(ClangBackEnd::ProjectPartId projectPartId));
    MOCK_CONST_METHOD1(fetchPrecompiledHeader,
                       ClangBackEnd::FilePath(ClangBackEnd::ProjectPartId projectPartId));
    MOCK_CONST_METHOD1(fetchPrecompiledHeaders,
                       ClangBackEnd::PchPaths(ClangBackEnd::ProjectPartId projectPartId));
    MOCK_CONST_METHOD1(
        fetchTimeStamps,
        ClangBackEnd::PrecompiledHeaderTimeStamps(ClangBackEnd::ProjectPartId projectPartId));
    MOCK_CONST_METHOD0(fetchAllPchPaths, ClangBackEnd::FilePaths());
};
