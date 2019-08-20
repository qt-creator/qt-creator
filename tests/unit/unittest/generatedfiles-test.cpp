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

#include "googletest.h"

#include <generatedfiles.h>

namespace {

using ClangBackEnd::V2::FileContainer;
using ClangBackEnd::V2::FileContainers;

class GeneratedFiles : public testing::Test
{
protected:
    FileContainer file1{"/file1", 1, "content1"};
    FileContainer file1b{"/file1", 1, "content1b"};
    FileContainer file2{"/file2", 2, "content2"};
    FileContainer file2b{"/file2", 2, "content2b"};
    FileContainer file3{"/file3", 3, "content3"};
    FileContainer file4{"/file4", 4, "content4"};
    ClangBackEnd::GeneratedFiles generatedFiles;
};

TEST_F(GeneratedFiles, AddGeneratedFiles)
{
    generatedFiles.update({file1, file2});

    ASSERT_THAT(generatedFiles.fileContainers(), ElementsAre(file1, file2));
}

TEST_F(GeneratedFiles, UpdateGeneratedFiles)
{
    generatedFiles.update({{file1, file3}});

    generatedFiles.update({file1b, file2b, file4});

    ASSERT_THAT(generatedFiles.fileContainers(), ElementsAre(file1b, file2b, file3, file4));
}

TEST_F(GeneratedFiles, RemoveGeneratedFiles)
{
    generatedFiles.update({file1, file2, file3, file4});

    generatedFiles.remove({"/file2", "/file4"});

    ASSERT_THAT(generatedFiles.fileContainers(), ElementsAre(file1, file3));
}

TEST_F(GeneratedFiles, IsValidForNoGeneratedFiles)
{
    ASSERT_TRUE(generatedFiles.isValid());
}

TEST_F(GeneratedFiles, IsNotValidIfFilesWithNotContentExists)
{
    generatedFiles.update({{"/file2", 2, ""}});

    ASSERT_FALSE(generatedFiles.isValid());
}

TEST_F(GeneratedFiles, IsValidIfAllFilesHasContent)
{
    generatedFiles.update({file1, file2, file3, file4});

    ASSERT_TRUE(generatedFiles.isValid());
}

TEST_F(GeneratedFiles, GetFilePathIds)
{
    generatedFiles.update({file3, file2, file1, file4});

    auto ids = generatedFiles.filePathIds();

    ASSERT_THAT(ids, ElementsAre(1, 2, 3, 4));
}

} // namespace
