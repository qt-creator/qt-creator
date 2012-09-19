function testsEnabled(qbs)
{
    return qbs.getenv("TEST") || qbs.buildVariant === "debug";
}

function defines(qbs)
{
    var list = [
        'IDE_LIBRARY_BASENAME="lib"',
        "QT_DISABLE_DEPRECATED_BEFORE=0x040900"
    ]
    if (testsEnabled(qbs))
        list.push("WITH_TESTS")
    return list
}
