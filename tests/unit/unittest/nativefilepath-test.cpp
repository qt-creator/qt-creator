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

#include <nativefilepath.h>

namespace {

Utils::PathString native(Utils::PathString path)
{
    if (Utils::HostOsInfo::isWindowsHost())
        path.replace('/', '\\');

    return path;
}

TEST(NativeFilePath, CreateFromPathString)
{
    ClangBackEnd::NativeFilePath filePath{native("/file/pathOne")};

    ASSERT_THAT(filePath.directory(), native("/file"));
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(NativeFilePath, CreateFromDirectoryAndFileName)
{
    ClangBackEnd::NativeFilePath filePath{Utils::PathString{native("/file")}, Utils::PathString{"pathOne"}};

    ASSERT_THAT(filePath.directory(), native("/file"));
    ASSERT_THAT(filePath.name(), "pathOne");
    ASSERT_THAT(filePath.path(), native("/file/pathOne"));
}

TEST(NativeFilePath, CreateFromCString)
{
    ClangBackEnd::NativeFilePath filePath{"/file/pathOne"};
    if (Utils::HostOsInfo::isWindowsHost())
        filePath = ClangBackEnd::NativeFilePath{"\\file\\pathOne"};

    ASSERT_THAT(filePath.directory(), native("/file"));
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(NativeFilePath, CreateFromFilePathView)
{
    ClangBackEnd::NativeFilePath filePath{ClangBackEnd::NativeFilePathView{native("/file/pathOne")}};

    ASSERT_THAT(filePath.directory(), native("/file"));
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(NativeFilePath, CreateFromQString)
{
    ClangBackEnd::NativeFilePath filePath{QString{native("/file/pathOne")}};

    ASSERT_THAT(filePath.directory(), native("/file"));
    ASSERT_THAT(filePath.name(), "pathOne");
}

TEST(NativeFilePath, DefaultNativeFilePath)
{
    ClangBackEnd::NativeFilePath filePath;

    ASSERT_THAT(filePath.directory(), "");
    ASSERT_THAT(filePath.name(), "");
}

TEST(NativeFilePath, EmptyNativeFilePath)
{
    ClangBackEnd::NativeFilePath filePath{""};

    ASSERT_THAT(filePath.directory(), "");
    ASSERT_THAT(filePath.name(), "");
}

}
