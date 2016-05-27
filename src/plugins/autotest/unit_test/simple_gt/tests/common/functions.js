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
