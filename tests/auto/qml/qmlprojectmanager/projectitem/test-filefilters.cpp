// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#include "common.h"
#include "projectitem/qmlprojectitem.h"

const Utils::FilePath testDataDir{
    Utils::FilePath::fromString(testDataRootDir.path() + "/file-filters")};
const Utils::FilePath projectFilePath{testDataDir.pathAppended("/MaterialBundle.qmlproject")};
const QmlProjectManager::QmlProjectItem projectItem{projectFilePath};
const Utils::FilePath fileListPath{testDataDir.pathAppended("/filelist.txt")};

TEST(QmlProjectItemFileFilterTests, TestFileFilters)
{
    QStringList fileNameList = QString::fromUtf8(fileListPath.fileContents().value()).replace("\r\n", "\n").split("\n");

    for (const Utils::FilePath &filePath : projectItem.files()) {
        const QString fileName{filePath.relativePathFrom(testDataDir).path()};
        const int index = fileNameList.indexOf(fileName);
        ASSERT_NE(index, -1) << "file_is_missing_in_the_filelist:: " + fileName.toStdString();
        fileNameList.remove(index);
    }

    ASSERT_EQ(fileNameList.size(), 0);
}
