/*
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
**/
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

function getGTestDir(qbs, str) {
    if (!str) {
        if (qbs.hostOS.contains("linux") && File.exists("/usr/src/gtest"))
            return "/usr/src/gtest";
    } else {
        return FileInfo.joinPaths(str, "googletest");
    }
    return "";
}

function getGMockDir(qbs, str) {
    if (!str) {
        if (qbs.hostOS.contains("linux") && File.exists("/usr/src/gmock"))
            return "/usr/src/gmock";
    } else {
        return FileInfo.joinPaths(str, "googlemock");
    }
    return "";
}

function getGTestAll(qbs, str) {
    var gtest = getGTestDir(qbs, str);
    if (!gtest)
        return [];
    return [FileInfo.joinPaths(gtest, "src/gtest-all.cc")];
}

function getGMockAll(qbs, str) {
    var gmock = getGMockDir(qbs, str);
    if (!gmock)
        return [];
    return [FileInfo.joinPaths(gmock, "src/gmock-all.cc")];
}

function getGTestIncludes(qbs, str) {
    var gtest = getGTestDir(qbs, str);
    if (!gtest)
        return [];
    return [gtest, FileInfo.joinPaths(gtest, "include")];
}

function getGMockIncludes(qbs, str) {
    var mock = getGMockDir(qbs, str);
    if (!mock)
        return [];
    return [mock, FileInfo.joinPaths(mock, "include")];
}
