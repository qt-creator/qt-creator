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

#pragma once

#include "googletest.h"

#include <filecontainerv2.h>
#include <pchcreatorinterface.h>
#include <projectpartpch.h>
#include <projectpartcontainerv2.h>

class MockPchCreator : public ClangBackEnd::PchCreatorInterface
{
public:
    MOCK_METHOD1(generatePchs,
                 void(const ClangBackEnd::V2::ProjectPartContainers &projectParts));
    MOCK_METHOD1(setGeneratedFiles,
                 void(const ClangBackEnd::V2::FileContainers &generatedFiles));
    MOCK_METHOD0(takeProjectsIncludes,
                 std::vector<ClangBackEnd::IdPaths>());

    void generatePchs(std::vector<ClangBackEnd::V2::ProjectPartContainer> &&projectParts) override
    {
        generatePchs(projectParts);
    }

    void setGeneratedFiles(ClangBackEnd::V2::FileContainers &&generatedFiles) override
    {
        setGeneratedFiles(generatedFiles);
    }
};
