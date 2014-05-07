// see PluginSpecPrivate::loadLibrary()
function qtLibraryName(qbs, name)
{
    if (qbs.debugInformation) {
        if (qbs.targetOS.contains("windows"))
            return name + "d";
        else if (qbs.targetOS.contains("osx"))
            return name + "_debug";
    }
    return name;
}

function versionIsAtLeast(actualVersion, expectedVersion)
{
    var actualVersionParts = actualVersion.split('.').map(function(item) { return parseInt(item, 10); });
    var expectedVersionParts = expectedVersion.split('.').map(function(item) { return parseInt(item, 10); });
    for (var i = 0; i < expectedVersionParts.length; ++i) {
        if (actualVersionParts[i] > expectedVersionParts[i])
            return true;
        if (actualVersionParts[i] < expectedVersionParts[i])
            return false;
    }
    return i === expectedVersionParts.length || expectedVersionParts[i] === 0;
}

function commonCxxFlags(qbs)
{
    var flags = [];
    if (qbs.toolchain.contains("clang")) {
        flags.push("-std=c++11", "-stdlib=libc++");
    } else if (qbs.toolchain.contains("gcc")) {
        flags.push("-std=c++0x");
    }
    return flags;
}

function commonLinkerFlags(qbs)
{
    var flags = [];
    if (qbs.toolchain.contains("clang"))
        flags.push("-stdlib=libc++");
    return flags;
}

