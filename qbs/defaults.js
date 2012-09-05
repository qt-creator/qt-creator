function testsEnabled(qbs)
{
    return qbs.getenv("TEST") || qbs.buildVariant === "debug";
}

function defines(qbs)
{
    var list = [ 'IDE_LIBRARY_BASENAME="lib"' ]
    if (testsEnabled(qbs))
        list.push("WITH_TESTS")
    return list
}
