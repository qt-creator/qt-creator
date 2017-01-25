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
var FileInfo = loadExtension("qbs.FileInfo")

function getGTestDir(str) {
    if (!str) {
        if (qbs.hostOS.contains("linux"))
            return "/usr/include/gtest";
    } else {
        return FileInfo.joinPaths(str, "googletest");
    }
    return "";
}

function getGMockDir(str) {
    if (!str) {
        if (qbs.hostOS.contains("linux"))
            return "/usr/include/gmock";
    } else {
        return FileInfo.joinPaths(str, "googlemock");
    }
    return "";
}

function getGTestAll(str) {
    var gtest = getGTestDir(str);
    if (!gtest)
        return [];
    return [FileInfo.joinPaths(gtest, "src/gtest-all.cc")];
}

function getGMockAll(str) {
    var gmock = getGMockDir(str);
    if (!gmock)
        return [];
    return [FileInfo.joinPaths(gmock, "src/gmock-all.cc")];
}

function getGTestIncludes(str) {
    var gtest = getGTestDir(str);
    if (!gtest)
        return [];
    return [gtest, FileInfo.joinPaths(gtest, "include")];
}

function getGMockIncludes(str) {
    var mock = getGMockDir(str);
    if (!mock)
        return [];
    return [mock, FileInfo.joinPaths(mock, "include")];
}
