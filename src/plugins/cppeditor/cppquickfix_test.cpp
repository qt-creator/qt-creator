/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppeditor.h"
#include "cppinsertdecldef.h"
#include "cppplugin.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"

#include <cpptools/cppcodestylepreferences.h>
#include <cpptools/cpptoolssettings.h>

#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QtTest>


/*!
    Tests for quick-fixes.
 */
using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;
using namespace Core;

namespace {

class TestDocument;
typedef QSharedPointer<TestDocument> TestDocumentPtr;

/**
 * Represents a test document before and after applying the quick fix.
 *
 * A TestDocument's originalSource may contain an '@' character to denote
 * the cursor position. This marker is removed before the Editor reads
 * the document.
 */
class TestDocument
{
public:
    TestDocument(const QByteArray &theOriginalSource, const QByteArray &theExpectedSource,
                 const QString &fileName)
        : originalSource(theOriginalSource)
        , expectedSource(theExpectedSource)
        , fileName(fileName)
        , cursorMarkerPosition(theOriginalSource.indexOf('@'))
        , editor(0)
        , editorWidget(0)
    {
        originalSource.remove(cursorMarkerPosition, 1);
        expectedSource.remove(theExpectedSource.indexOf('@'), 1);
    }

    static TestDocumentPtr create(const QByteArray &theOriginalSource,
        const QByteArray &theExpectedSource,const QString &fileName)
    {
        return TestDocumentPtr(new TestDocument(theOriginalSource, theExpectedSource, fileName));
    }

    bool hasCursorMarkerPosition() const { return cursorMarkerPosition != -1; }
    bool changesExpected() const { return originalSource != expectedSource; }

    QString filePath() const
    {
        if (directoryPath.isEmpty())
            qDebug() << "Warning: No directoryPath set!";
        return directoryPath + QLatin1Char('/') + fileName;
    }

    void writeToDisk() const
    {
        Utils::FileSaver srcSaver(filePath());
        srcSaver.write(originalSource);
        srcSaver.finalize();
    }

    QByteArray originalSource;
    QByteArray expectedSource;

    const QString fileName;
    QString directoryPath;
    const int cursorMarkerPosition;

    CPPEditor *editor;
    CPPEditorWidget *editorWidget;
};

/**
 * Encapsulates the whole process of setting up an editor, getting the
 * quick-fix, applying it, and checking the result.
 */
struct TestCase
{
    QList<TestDocumentPtr> testFiles;

    CppCodeStylePreferences *cppCodeStylePreferences;
    QString cppCodeStylePreferencesOriginalDelegateId;

    TestCase(const QByteArray &originalSource, const QByteArray &expectedSource);
    TestCase(const QList<TestDocumentPtr> theTestFiles);
    ~TestCase();

    QuickFixOperation::Ptr getFix(CppQuickFixFactory *factory, CPPEditorWidget *editorWidget);
    TestDocumentPtr testFileWithCursorMarker() const;

    void run(CppQuickFixFactory *factory);

private:
    TestCase(const TestCase &);
    TestCase &operator=(const TestCase &);

    void init();
};

/// Apply the factory on the source and get back the first result or a null pointer.
QuickFixOperation::Ptr TestCase::getFix(CppQuickFixFactory *factory, CPPEditorWidget *editorWidget)
{
    CppQuickFixInterface qfi(new CppQuickFixAssistInterface(editorWidget, ExplicitlyInvoked));
    TextEditor::QuickFixOperations results;
    factory->match(qfi, results);
    return results.isEmpty() ? QuickFixOperation::Ptr() : results.first();
}

/// The '@' in the originalSource is the position from where the quick-fix discovery is triggered.
TestCase::TestCase(const QByteArray &originalSource, const QByteArray &expectedSource)
    : cppCodeStylePreferences(0)
{
    testFiles << TestDocument::create(originalSource, expectedSource, QLatin1String("file.cpp"));
    init();
}

/// Exactly one TestFile must contain the cursor position marker '@' in the originalSource.
TestCase::TestCase(const QList<TestDocumentPtr> theTestFiles)
    : testFiles(theTestFiles), cppCodeStylePreferences(0)
{
    init();
}

void TestCase::init()
{
    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    foreach (const TestDocumentPtr testFile, testFiles) {
        if (testFile->hasCursorMarkerPosition())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write files to disk
    const QString directoryPath = QDir::tempPath();
    foreach (TestDocumentPtr testFile, testFiles) {
        testFile->directoryPath = directoryPath;
        testFile->writeToDisk();
    }

    // Update Code Model
    QStringList filePaths;
    foreach (const TestDocumentPtr &testFile, testFiles)
        filePaths << testFile->filePath();
    CppTools::CppModelManagerInterface::instance()->updateSourceFiles(filePaths);

    // Wait for the parser in the future to give us the document
    QStringList filePathsNotYetInSnapshot(filePaths);
    forever {
        Snapshot snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
        foreach (const QString &filePath, filePathsNotYetInSnapshot) {
            if (snapshot.contains(filePath))
                filePathsNotYetInSnapshot.removeOne(filePath);
        }
        if (filePathsNotYetInSnapshot.isEmpty())
            break;
        QCoreApplication::processEvents();
    }

    // Open Files
    foreach (TestDocumentPtr testFile, testFiles) {
        testFile->editor
            = dynamic_cast<CPPEditor *>(EditorManager::openEditor(testFile->filePath()));
        QVERIFY(testFile->editor);

        // Set cursor position
        const int cursorPosition = testFile->hasCursorMarkerPosition()
                ? testFile->cursorMarkerPosition : 0;
        testFile->editor->setCursorPosition(cursorPosition);

        testFile->editorWidget = dynamic_cast<CPPEditorWidget *>(testFile->editor->editorWidget());
        QVERIFY(testFile->editorWidget);

        // Rehighlight
        testFile->editorWidget->semanticRehighlight(true);
        // Wait for the semantic info from the future
        while (testFile->editorWidget->semanticInfo().doc.isNull())
            QCoreApplication::processEvents();
    }

    // Enforce the default cpp code style, so we are independent of config file settings.
    // This is needed by e.g. the GenerateGetterSetter quick fix.
    cppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
    QVERIFY(cppCodeStylePreferences);
    cppCodeStylePreferencesOriginalDelegateId = cppCodeStylePreferences->currentDelegateId();
    cppCodeStylePreferences->setCurrentDelegate(QLatin1String("qt"));
}

TestCase::~TestCase()
{
    // Restore default cpp code style
    if (cppCodeStylePreferences)
        cppCodeStylePreferences->setCurrentDelegate(cppCodeStylePreferencesOriginalDelegateId);

    // Close editors
    QList<Core::IEditor *> editorsToClose;
    foreach (const TestDocumentPtr testFile, testFiles) {
        if (testFile->editor)
            editorsToClose << testFile->editor;
    }
    EditorManager::instance()->closeEditors(editorsToClose, false);
    QCoreApplication::processEvents(); // process any pending events

    // Remove the test files from the code-model
    CppModelManagerInterface *mmi = CppTools::CppModelManagerInterface::instance();
    mmi->GC();
    QCOMPARE(mmi->snapshot().size(), 0);
}

/// Leading whitespace is not removed, so we can check if the indetation ranges
/// have been set correctly by the quick-fix.
QByteArray &removeTrailingWhitespace(QByteArray &input)
{
    QList<QByteArray> lines = input.split('\n');
    input.resize(0);
    foreach (QByteArray line, lines) {
        while (line.length() > 0) {
            char lastChar = line[line.length() - 1];
            if (lastChar == ' ' || lastChar == '\t')
                line = line.left(line.length() - 1);
            else
                break;
        }
        input.append(line);
        input.append('\n');
    }
    return input;
}

void TestCase::run(CppQuickFixFactory *factory)
{
    // Run the fix in the file having the cursor marker
    TestDocumentPtr testFile;
    foreach (const TestDocumentPtr file, testFiles) {
        if (file->hasCursorMarkerPosition())
            testFile = file;
    }
    QVERIFY2(testFile, "No test file with cursor marker found");

    if (QuickFixOperation::Ptr fix = getFix(factory, testFile->editorWidget))
        fix->perform();
    else
        qDebug() << "Quickfix was not triggered";

    // Compare all files
    const int testFilesCount = testFiles.size();
    foreach (const TestDocumentPtr testFile, testFiles) {
        if (testFilesCount >= 2)
            qDebug() << "Checking" << testFile->filePath();

        // Check
        QByteArray result = testFile->editorWidget->document()->toPlainText().toUtf8();
        removeTrailingWhitespace(result);
        QCOMPARE(QLatin1String(result), QLatin1String(testFile->expectedSource));

        // Undo the change
        for (int i = 0; i < 100; ++i)
            testFile->editorWidget->undo();
        result = testFile->editorWidget->document()->toPlainText().toUtf8();
        QCOMPARE(result, testFile->originalSource);
    }
}

} // anonymous namespace

/// Checks:
/// 1. If the name does not start with ("m_" or "_") and does not
///    end with "_", we are forced to prefix the getter with "get".
/// 2. Setter: Use pass by value on integer/float and pointer types.
void CppPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithPrefix()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    int @it;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    int it;\n"
        "\n"
        "public:\n"
        "    int getIt() const;\n"
        "    void setIt(int value);\n"
        "};\n"
        "\n"
        "int Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Checks:
/// 1. Getter: "get" prefix is not necessary.
/// 2. Setter: Parameter name is base name.
void CppPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithoutPrefix()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    int @m_it;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    int m_it;\n"
        "\n"
        "public:\n"
        "    int it() const;\n"
        "    void setIt(int it);\n"
        "};\n"
        "\n"
        "int Something::it() const\n"
        "{\n"
        "    return m_it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int it)\n"
        "{\n"
        "    m_it = it;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Setter: Use pass by reference for parameters which
/// are not integer, float or pointers.
void CppPlugin::test_quickfix_GenerateGetterSetter_customType()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    MyType @it;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    MyType it;\n"
        "\n"
        "public:\n"
        "    MyType getIt() const;\n"
        "    void setIt(const MyType &value);\n"
        "};\n"
        "\n"
        "MyType Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(const MyType &value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Checks:
/// 1. Setter: No setter is generated for const members.
/// 2. Getter: Return a non-const type since it pass by value anyway.
void CppPlugin::test_quickfix_GenerateGetterSetter_constMember()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    const int @it;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    const int it;\n"
        "\n"
        "public:\n"
        "    int getIt() const;\n"
        "};\n"
        "\n"
        "int Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Checks: No special treatment for pointer to non const.
void CppPlugin::test_quickfix_GenerateGetterSetter_pointerToNonConst()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    int *it@;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    int *it;\n"
        "\n"
        "public:\n"
        "    int *getIt() const;\n"
        "    void setIt(int *value);\n"
        "};\n"
        "\n"
        "int *Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int *value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Checks: No special treatment for pointer to const.
void CppPlugin::test_quickfix_GenerateGetterSetter_pointerToConst()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    const int *it@;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    const int *it;\n"
        "\n"
        "public:\n"
        "    const int *getIt() const;\n"
        "    void setIt(const int *value);\n"
        "};\n"
        "\n"
        "const int *Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(const int *value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Checks:
/// 1. Setter: Setter is a static function.
/// 2. Getter: Getter is a static, non const function.
void CppPlugin::test_quickfix_GenerateGetterSetter_staticMember()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    static int @m_member;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    static int m_member;\n"
        "\n"
        "public:\n"
        "    static int member();\n"
        "    static void setMember(int member);\n"
        "};\n"
        "\n"
        "int Something::member()\n"
        "{\n"
        "    return m_member;\n"
        "}\n"
        "\n"
        "void Something::setMember(int member)\n"
        "{\n"
        "    m_member = member;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Check if it works on the second declarator
void CppPlugin::test_quickfix_GenerateGetterSetter_secondDeclarator()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    int *foo, @it;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    int *foo, it;\n"
        "\n"
        "public:\n"
        "    int getIt() const;\n"
        "    void setIt(int value);\n"
        "};\n"
        "\n"
        "int Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Quick fix is offered for "int *@it;" ('@' denotes the text cursor position)
void CppPlugin::test_quickfix_GenerateGetterSetter_triggeringRightAfterPointerSign()
{
    const QByteArray original =
        "\n"
        "class Something\n"
        "{\n"
        "    int *@it;\n"
        "};\n"
        ;
    const QByteArray expected =
        "\n"
        "class Something\n"
        "{\n"
        "    int *it;\n"
        "\n"
        "public:\n"
        "    int *getIt() const;\n"
        "    void setIt(int *value);\n"
        "};\n"
        "\n"
        "int *Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int *value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
        "\n"
        ;

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Quick fix is not triggered on a member function.
void CppPlugin::test_quickfix_GenerateGetterSetter_notTriggeringOnMemberFunction()
{
    const QByteArray original = "class Something { void @f(); };";

    GenerateGetterSetter factory;
    TestCase data(original, original + "\n");
    data.run(&factory);
}

/// Check: Quick fix is not triggered on an member array;
void CppPlugin::test_quickfix_GenerateGetterSetter_notTriggeringOnMemberArray()
{
    const QByteArray original = "class Something { void @a[10]; };";

    GenerateGetterSetter factory;
    TestCase data(original, original + "\n");
    data.run(&factory);
}

/// Check: Do not offer the quick fix if there is already a member with the
/// getter or setter name we would generate.
void CppPlugin::test_quickfix_GenerateGetterSetter_notTriggeringWhenGetterOrSetterExist()
{
    const QByteArray original =
        "class Something {\n"
        "     int @it;\n"
        "     void setIt();\n"
        "};\n";

    GenerateGetterSetter factory;
    TestCase data(original, original + "\n");
    data.run(&factory);
}

/// Check: Just a basic test since the main functionality is tested in
/// cpppointerdeclarationformatter_test.cpp
void CppPlugin::test_quickfix_ReformatPointerDeclaration()
{
    const QByteArray original = "char@*s;";
    const QByteArray expected = "char *s;\n";

    ReformatPointerDeclaration factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check from source file: If there is no header file, insert the definition after the class.
void CppPlugin::test_quickfix_InsertDefFromDecl_basic()
{
    const QByteArray original =
        "struct Foo\n"
        "{\n"
        "    Foo();@\n"
        "};\n"
        "\n"
        ;
    const QByteArray expected = original +
        "\n"
        "Foo::Foo()\n"
        "{\n"
        "}\n"
        "\n"
        ;

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check from header file: If there is a source file, insert the definition in the source file.
void CppPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic1()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original.resize(0);
    expected =
        "\n"
        "Foo::Foo()\n"
        "{\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check from source file: Insert in source file, not header file.
void CppPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic2()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Empty Header File
    testFiles << TestDocument::create("", "\n", QLatin1String("file.h"));

    // Source File
    original =
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n";
    expected = original +
        "\n"
        "\n"
        "Foo::Foo()\n"
        "{\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check from header file: If the class is in a namespace, the added function definition
/// name must be qualified accordingly.
void CppPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace1()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace N {\n"
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n"
        "}\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original.resize(0);
    expected =
        "\n"
        "N::Foo::Foo()\n"
        "{\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check from header file: If the class is in namespace N and the source file has a
/// "using namespace N" line, the function definition name must be qualified accordingly.
void CppPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace2()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace N {\n"
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n"
        "}\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "using namespace N;\n"
        "\n"
        ;
    expected = original +
        "\n"
        "Foo::Foo()\n"
        "{\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

void CppPlugin::test_quickfix_InsertDefFromDecl_freeFunction()
{
    const QByteArray original = "void free()@;\n";
    const QByteArray expected = original +
        "\n"
        "\n"
        "void free()\n"
        "{\n"
        "}\n"
        "\n"
        ;

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check normal add include if there is already a include
void CppPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_normal()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "class Foo {};\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"someheader.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Fo@o foo;\n"
        "}\n"
        ;
    expected =
        "#include \"someheader.h\"\n"
        "#include \"file.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifier factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check add include if no include is present
void CppPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_noinclude()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "class Foo {};\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "void f()\n"
        "{\n"
        "    Fo@o foo;\n"
        "}\n"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifier factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check add include if no include is present with comment on top
void CppPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_noincludeComment01()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "class Foo {};\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "\n"
        "// comment\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Fo@o foo;\n"
        "}\n"
        ;
    expected =
        "\n"
        "// comment\n"
        "\n"
        "#include \"file.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifier factory;
    TestCase data(testFiles);
    data.run(&factory);
}
/// Check add include if no include is present with comment on top
void CppPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_noincludeComment02()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "class Foo {};\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "\n"
        "/*\n"
        " comment\n"
        " */\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Fo@o foo;\n"
        "}\n"
        ;
    expected =
        "\n"
        "/*\n"
        " comment\n"
        " */\n"
        "\n"
        "#include \"file.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifier factory;
    TestCase data(testFiles);
    data.run(&factory);
}
