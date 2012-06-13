import qbs.base 1.0

DynamicLibrary {
    destination: {
        if (qbs.targetOS === "windows")
            return "bin"
        else
            return "lib/qtcreator"
    }
}
