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
