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

#include <projectstorage/sourcepathview.h>

namespace {

TEST(SourcePathView, FilePathSlashForEmptyPath)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(SourcePathView, FilePathSlashForSingleSlash)
{
    QmlDesigner::SourcePathView filePath("/");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(SourcePathView, FilePathSlashForFileInRoot)
{
    QmlDesigner::SourcePathView filePath("/file.h");

    ASSERT_THAT(filePath.slashIndex(), 0);
}

TEST(SourcePathView, FilePathSlashForSomeLongerPath)
{
    QmlDesigner::SourcePathView filePath("/path/to/some/file.h");

    ASSERT_THAT(filePath.slashIndex(), 13);
}

TEST(SourcePathView, FilePathSlashForFileNameOnly)
{
    QmlDesigner::SourcePathView filePath("file.h");

    ASSERT_THAT(filePath.slashIndex(), -1);
}

TEST(SourcePathView, DirectoryPathForEmptyPath)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.directory(), "");
}

TEST(SourcePathView, DirectoryPathForSingleSlashPath)
{
    QmlDesigner::SourcePathView filePath{"/"};

    ASSERT_THAT(filePath.directory(), "");
}

TEST(SourcePathView, DirectoryPathForLongerPath)
{
    QmlDesigner::SourcePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.directory(), "/path/to/some");
}

TEST(SourcePathView, DirectoryPathForFileNameOnly)
{
    QmlDesigner::SourcePathView filePath{"file.h"};

    ASSERT_THAT(filePath.directory(), IsEmpty());
}

TEST(SourcePathView, FileNameForEmptyPath)
{
    QmlDesigner::SourcePathView filePath("");

    ASSERT_THAT(filePath.name(), "");
}

TEST(SourcePathView, FileNameForSingleSlashPath)
{
    QmlDesigner::SourcePathView filePath{"/"};

    ASSERT_THAT(filePath.name(), "");
}

TEST(SourcePathView, FileNameForLongerPath)
{
    QmlDesigner::SourcePathView filePath{"/path/to/some/file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

TEST(SourcePathView, FileNameForFileNameOnly)
{
    QmlDesigner::SourcePathView filePath{"file.h"};

    ASSERT_THAT(filePath.name(), "file.h");
}

}
