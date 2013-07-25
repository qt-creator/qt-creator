function testsEnabled(qbs)
{
    return qbs.getenv("TEST") || qbs.buildVariant === "debug";
}

function defines(qbs)
{
    var list = [
        "QT_CREATOR",
        'IDE_LIBRARY_BASENAME="lib"',
        "QT_DISABLE_DEPRECATED_BEFORE=0x040900",
        "QT_NO_CAST_TO_ASCII",
        "QT_NO_CAST_FROM_ASCII"
    ]
    if (testsEnabled(qbs))
        list.push("WITH_TESTS")
    return list
}

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
