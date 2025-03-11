# Testing Qt Creator

There are four kinds of tests in Qt Creator, you can find some more documentation
[online here](https://doc-snapshots.qt.io/qtcreator-extending/plugin-tests.html).

## Manual Tests

Located in `tests/manual/...`, these need to be run manually as separate executables and require
user interaction for "playing around" with some piece of code.

## Squish-Based Tests

`tests/system/...` are run with [Squish](https://www.qt.io/product/quality-assurance/squish)
at irregular intervals and specifically checked for releases (RTA).

## Auto/Unit Tests

`tests/auto/...` are separate QTest-based executables (tst_...) that are run with tools like
cmake/ctest. They run automatically with the PRECHECK in Gerrit, etc.

## Plugin/Integration Tests

These tests are integrated into the plugins themselves and are executed within
the context of a fully running Qt Creator. They are run with the `-test <plugin>`
command line parameter of the qtcreator executable.
This starts Qt Creator with only that plugin and its dependencies loaded, executes any test
functions that the plugin registers, prints the QTest output, and then closes.

For plugin tests, plugins register test functions if Qt Creator was compiled with WITH_TESTS,
like this:

```c++
void TextEditorPlugin::initialize()
{
#ifdef WITH_TESTS
    addTestCreator(createFormatTextTest);
    addTestCreator(createTextDocumentTest);
    addTestCreator(createTextEditorTest);
    addTestCreator(createSnippetParserTest);
#endif
    ...
}
```

The "test creator" is a factory function that returns a QObject, which functions the same as the
tstSomething QObjects used in standalone unit tests.

```c++
QObject *createFormatTextTest()
{
    return new FormatTextTest;
}
```

Slots are executed as QTest tests, and there can be init and cleanup functions, etc.

```c++
class FormatTextTest final : public QObject
{
    Q_OBJECT

private slots:
    void testFormatting_data();
    void testFormatting();
};
```

These tests are executed after Qt Creator plugins have fully initialized and can access the full
Qt Creator state, open files, load projects, etc. (depending on the declared dependencies of
the tested plugin).
