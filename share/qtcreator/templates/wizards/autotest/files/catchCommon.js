// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

var File = require("qbs.File")
var FileInfo = require("qbs.FileInfo")

function getChildPath(qbs, baseFolder, childFolder)
{
    if (!baseFolder)
        return [];

    var childPath = FileInfo.joinPaths(baseFolder, childFolder);
    if (File.exists(childPath))
        return [childPath];
    return [];
}
