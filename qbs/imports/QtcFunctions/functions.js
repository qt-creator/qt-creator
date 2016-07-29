// see PluginSpecPrivate::loadLibrary()
function qtLibraryName(qbs, name)
{
    if (qbs.debugInformation) {
        if (qbs.targetOS.contains("windows"))
            return name + "d";
        else if (qbs.targetOS.contains("macos"))
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
