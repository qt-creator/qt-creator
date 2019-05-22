var FileInfo = require("qbs.FileInfo")

function getGTestDir(Qbs, str) {
    if (!str) {
        if (Qbs.hostOS.contains("linux"))
            return "/usr/include/gtest";
    } else {
        return FileInfo.joinPaths(str, "googletest");
    }
    return "";
}

function getGMockDir(Qbs, str) {
    if (!str) {
        if (Qbs.hostOS.contains("linux"))
            return "/usr/include/gmock";
    } else {
        return FileInfo.joinPaths(str, "googlemock");
    }
    return "";
}

function getGTestAll(Qbs, str) {
    var gtest = getGTestDir(Qbs, str);
    if (!gtest)
        return [];
    return [FileInfo.joinPaths(gtest, "src/gtest-all.cc")];
}

function getGMockAll(Qbs, str) {
    var gmock = getGMockDir(Qbs, str);
    if (!gmock)
        return [];
    return [FileInfo.joinPaths(gmock, "src/gmock-all.cc")];
}

function getGTestIncludes(Qbs, str) {
    var gtest = getGTestDir(Qbs, str);
    if (!gtest)
        return [];
    return [gtest, FileInfo.joinPaths(gtest, "include")];
}

function getGMockIncludes(Qbs, str) {
    var mock = getGMockDir(Qbs, str);
    if (!mock)
        return [];
    return [mock, FileInfo.joinPaths(mock, "include")];
}
