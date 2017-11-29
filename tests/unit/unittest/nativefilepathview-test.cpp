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

#include <filepathview.h>

#include <utils/hostosinfo.h>

namespace {

using ClangBackEnd::NativeFilePathView;

Utils::PathString native(Utils::PathString path)
{
    if (Utils::HostOsInfo::isWindowsHost())
        path.replace('/', '\\');

    return path;
}

TEST(NativeFilePathView, FilePathSlashForEmptyPath)
{
    NativeFilePathView filePath("");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(NativeFilePathView, FilePathSlashForSingleSlash)
{
    NativeFilePathView filePath(native("/"));

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(NativeFilePathView, FilePathSlashForFileInRoot)
{
    NativeFilePathView filePath(native("/file.h"));

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(NativeFilePathView, FilePathSlashForSomeLongerPath)
{
    NativeFilePathView filePath(native("/path/to/some/file.h"));

    ASSERT_THAT(filePath.slashIndex(), 13);
}

TEST(NativeFilePathView, FilePathSlashForFileNameOnly)
{
    NativeFilePathView filePath(native("file.h"));

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(NativeFilePathView, DirectoryPathForEmptyPath)
{
    NativeFilePathView filePath("");

    ASSERT_THAT(filePath.directory(), "");
}

TEST(NativeFilePathView, DirectoryPathForSingleSlashPath)
{
    NativeFilePathView filePath{native("/")};

    ASSERT_THAT(filePath.directory(), "");
}

TEST(NativeFilePathView, DirectoryPathForLongerPath)
{
    NativeFilePathView filePath{native("/path/to/some/file.h")};

    ASSERT_THAT(filePath.directory(), native("/path/to/some"));
}

TEST(NativeFilePathView, DirectoryPathForFileNameOnly)
{
    NativeFilePathView filePath{"file.h"};

    ASSERT_THAT(filePath.directory(), IsEmpty());
}

TEST(NativeFilePathView, FileNameForEmptyPath)
{
    NativeFilePathView filePath("");

    ASSERT_THAT(filePath.name(), "");
}

TEST(NativeFilePathView, FileNameForSingleSlashPath)
{
    NativeFilePathView filePath{native("/")};

    ASSERT_THAT(filePath.name(), "");
}

TEST(NativeFilePathView, FileNameForLongerPath)
{
    NativeFilePathView filePath{native("/path/to/some/file.h")};

    ASSERT_THAT(filePath.name(), "file.h");
}

TEST(NativeFilePathView, FileNameForFileNameOnly)
{
    NativeFilePathView filePath{"file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

}
