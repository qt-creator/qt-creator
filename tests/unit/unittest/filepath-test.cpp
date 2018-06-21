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

#include "googletest.h"

#include <filepath.h>

namespace {

TEST(FilePath, CreateFromPathString)
{
    ClangBackEnd::FilePath filePath{Utils::PathString{"/file/pathOne"}};

    ASSERT_THAT(filePath.directory(), "/file");
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(FilePath, CreateFromDirectoryAndFileName)
{
    ClangBackEnd::FilePath filePath{Utils::PathString{"/file"}, Utils::PathString{"pathOne"}};

    ASSERT_THAT(filePath.directory(), "/file");
    ASSERT_THAT(filePath.name(), "pathOne");
    ASSERT_THAT(filePath.path(), "/file/pathOne");
}

TEST(FilePath, CreateFromCString)
{
    ClangBackEnd::FilePath filePath{"/file/pathOne"};

    ASSERT_THAT(filePath.directory(), "/file");
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(FilePath, CreateFromFilePathView)
{
    ClangBackEnd::FilePath filePath{ClangBackEnd::FilePathView{"/file/pathOne"}};

    ASSERT_THAT(filePath.directory(), "/file");
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(FilePath, CreateFromQString)
{
    ClangBackEnd::FilePath filePath{QString{"/file/pathOne"}};

    ASSERT_THAT(filePath.directory(), "/file");
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(FilePath, DefaultFilePath)
{
    ClangBackEnd::FilePath filePath;

    ASSERT_THAT(filePath.directory(), "");
    ASSERT_THAT(filePath.name(), "");
}

TEST(FilePath, EmptyFilePath)
{
    ClangBackEnd::FilePath filePath("");

    ASSERT_THAT(filePath.directory(), "");
    ASSERT_THAT(filePath.name(), "");
}

}
