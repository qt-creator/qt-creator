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

#include "cppquickfix_test_utils.h"

#include "cppeditor.h"
#include "cppeditorplugin.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"

#include <cpptools/cppcodestylepreferences.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpppreprocessor.h>
#include <cpptools/cpptoolssettings.h>
#include <cpptools/includeutils.h>

#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QtTest>

/*!
    Tests for quick-fixes.
 */
using namespace Core;
using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace IncludeUtils;
using namespace TextEditor;

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
        if (!QFileInfo(fileName).isAbsolute())
            return QDir::tempPath() + QLatin1Char('/') + fileName;
        return fileName;
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
    const int cursorMarkerPosition;

    CPPEditor *editor;
    CPPEditorWidget *editorWidget;
};

/**
 * Encapsulates the whole process of setting up an editor, getting the
 * quick-fix, applying it, and checking the result.
 */
class TestCase
{
public:
    TestCase(const QByteArray &originalSource, const QByteArray &expectedSource,
             const QStringList &includePaths = QStringList());
    TestCase(const QList<TestDocumentPtr> theTestFiles,
             const QStringList &includePaths = QStringList());
    ~TestCase();

    QuickFixOperation::Ptr getFix(CppQuickFixFactory *factory, CPPEditorWidget *editorWidget,
                                  int resultIndex = 0);
    TestDocumentPtr testFileWithCursorMarker() const;

    void run(CppQuickFixFactory *factory, int resultIndex = 0);

private:
    TestCase(const TestCase &);
    TestCase &operator=(const TestCase &);

    void init(const QStringList &includePaths);

private:
    QList<TestDocumentPtr> testFiles;

    CppCodeStylePreferences *cppCodeStylePreferences;
    QString cppCodeStylePreferencesOriginalDelegateId;

    QStringList includePathsToRestore;
    bool restoreIncludePaths;
};

/// Apply the factory on the source and get back the resultIndex'th result or a null pointer.
QuickFixOperation::Ptr TestCase::getFix(CppQuickFixFactory *factory, CPPEditorWidget *editorWidget,
                                        int resultIndex)
{
    CppQuickFixInterface qfi(new CppQuickFixAssistInterface(editorWidget, ExplicitlyInvoked));
    TextEditor::QuickFixOperations results;
    factory->match(qfi, results);
    return results.isEmpty() ? QuickFixOperation::Ptr() : results.at(resultIndex);
}

/// The '@' in the originalSource is the position from where the quick-fix discovery is triggered.
TestCase::TestCase(const QByteArray &originalSource, const QByteArray &expectedSource,
                   const QStringList &includePaths)
    : cppCodeStylePreferences(0), restoreIncludePaths(false)
{
    testFiles << TestDocument::create(originalSource, expectedSource, QLatin1String("file.cpp"));
    init(includePaths);
}

/// Exactly one TestFile must contain the cursor position marker '@' in the originalSource.
TestCase::TestCase(const QList<TestDocumentPtr> theTestFiles, const QStringList &includePaths)
    : testFiles(theTestFiles), cppCodeStylePreferences(0), restoreIncludePaths(false)
{
    init(includePaths);
}

void TestCase::init(const QStringList &includePaths)
{
    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    foreach (const TestDocumentPtr testFile, testFiles) {
        if (testFile->hasCursorMarkerPosition())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write files to disk
    foreach (TestDocumentPtr testFile, testFiles)
        testFile->writeToDisk();

    CppTools::Internal::CppModelManager *cmm = CppTools::Internal::CppModelManager::instance();

    // Set appropriate include paths
    if (!includePaths.isEmpty()) {
        restoreIncludePaths = true;
        includePathsToRestore = cmm->includePaths();
        cmm->setIncludePaths(includePaths);
    }

    // Update Code Model
    QStringList filePaths;
    foreach (const TestDocumentPtr &testFile, testFiles)
        filePaths << testFile->filePath();
    cmm->updateSourceFiles(filePaths);

    // Wait for the parser in the future to give us the document
    QStringList filePathsNotYetInSnapshot(filePaths);
    forever {
        Snapshot snapshot = cmm->snapshot();
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
    CppModelManagerInterface *mmi = CppModelManagerInterface::instance();
    mmi->GC();
    QCOMPARE(mmi->snapshot().size(), 0);

    // Restore include paths
    if (restoreIncludePaths)
        CppTools::Internal::CppModelManager::instance()->setIncludePaths(includePathsToRestore);

    // Remove created files from file system
    foreach (const TestDocumentPtr &testDocument, testFiles)
        QVERIFY(QFile::remove(testDocument->filePath()));
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

void TestCase::run(CppQuickFixFactory *factory, int resultIndex)
{
    // Run the fix in the file having the cursor marker
    TestDocumentPtr testFile;
    foreach (const TestDocumentPtr file, testFiles) {
        if (file->hasCursorMarkerPosition())
            testFile = file;
    }
    QVERIFY2(testFile, "No test file with cursor marker found");

    if (QuickFixOperation::Ptr fix = getFix(factory, testFile->editorWidget, resultIndex))
        fix->perform();
    else
        qDebug() << "Quickfix was not triggered";

    // Compare all files
    foreach (const TestDocumentPtr testFile, testFiles) {
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

/// Delegates directly to AddIncludeForUndefinedIdentifierOp for easier testing.
class AddIncludeForUndefinedIdentifierTestFactory : public CppQuickFixFactory
{
public:
    AddIncludeForUndefinedIdentifierTestFactory(const QString &include)
        : m_include(include) {}

    void match(const CppQuickFixInterface &cppQuickFixInterface, QuickFixOperations &result)
    {
        result += CppQuickFixOperation::Ptr(
            new AddIncludeForUndefinedIdentifierOp(cppQuickFixInterface, 0, m_include));
    }

private:
    const QString m_include;
};

QString includeBaseDirectory()
{
    return QLatin1String(SRCDIR)
        + QLatin1String("/../../../tests/auto/cplusplus/preprocessor/data/include-data");
}

QString globalQtCoreIncludePath()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/QtCore"));
}

QString globalIncludePath()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/global"));
}

QString directoryOfTestFile()
{
    return QDir::cleanPath(includeBaseDirectory() + QLatin1String("/local"));
}

} // anonymous namespace

/// Checks:
/// 1. If the name does not start with ("m_" or "_") and does not
///    end with "_", we are forced to prefix the getter with "get".
/// 2. Setter: Use pass by value on integer/float and pointer types.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithPrefix()
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

/// Checks: In addition to test_quickfix_GenerateGetterSetter_basicGetterWithPrefix
/// generated definitions should fit in the namespace.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespace()
{
    const QByteArray original =
        "namespace SomeNamespace {\n"
        "class Something\n"
        "{\n"
        "    int @it;\n"
        "};\n"
        "}\n";

    const QByteArray expected =
        "namespace SomeNamespace {\n"
        "class Something\n"
        "{\n"
        "    int it;\n"
        "\n"
        "public:\n"
        "    int getIt() const;\n"
        "    void setIt(int value);\n"
        "};\n"
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
        "}\n\n";

    GenerateGetterSetter factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Checks: In addition to test_quickfix_GenerateGetterSetter_basicGetterWithPrefix
/// generated definitions should fit in the namespace.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespaceToCpp()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace SomeNamespace {\n"
        "class Something\n"
        "{\n"
        "    int @it;\n"
        "};\n"
        "}\n";
    expected =
        "namespace SomeNamespace {\n"
        "class Something\n"
        "{\n"
        "    int it;\n"
        "\n"
        "public:\n"
        "    int getIt() const;\n"
        "    void setIt(int value);\n"
        "};\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "namespace SomeNamespace {\n"
        "}\n";
    expected =
        "#include \"file.h\"\n"
        "namespace SomeNamespace {\n"
        "int Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int value)\n"
        "{\n"
        "    it = value;\n"
        "}\n\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    GenerateGetterSetter factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Checks:
/// 1. Getter: "get" prefix is not necessary.
/// 2. Setter: Parameter name is base name.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithoutPrefix()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_customType()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_constMember()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_pointerToNonConst()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_pointerToConst()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_staticMember()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_secondDeclarator()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_triggeringRightAfterPointerSign()
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
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_notTriggeringOnMemberFunction()
{
    const QByteArray original = "class Something { void @f(); };";

    GenerateGetterSetter factory;
    TestCase data(original, original + "\n");
    data.run(&factory);
}

/// Check: Quick fix is not triggered on an member array;
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_notTriggeringOnMemberArray()
{
    const QByteArray original = "class Something { void @a[10]; };";

    GenerateGetterSetter factory;
    TestCase data(original, original + "\n");
    data.run(&factory);
}

/// Check: Do not offer the quick fix if there is already a member with the
/// getter or setter name we would generate.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_notTriggeringWhenGetterOrSetterExist()
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
void CppEditorPlugin::test_quickfix_ReformatPointerDeclaration()
{
    const QByteArray original = "char@*s;";
    const QByteArray expected = "char *s;\n";

    ReformatPointerDeclaration factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check from source file: If there is no header file, insert the definition after the class.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_basic()
{
    const QByteArray original =
        "struct Foo\n"
        "{\n"
        "    Foo();@\n"
        "};\n";
    const QByteArray expected = original +
        "\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n"
        "\n";

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check if definition is inserted right after class for insert definition outside
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_afterClass()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "    Foo();\n"
        "    void a@();\n"
        "};\n"
        "\n"
        "class Bar {};\n";
    expected =
        "class Foo\n"
        "{\n"
        "    Foo();\n"
        "    void a();\n"
        "};\n"
        "\n"
        "void Foo::a()\n"
        "{\n\n}\n"
        "\n"
        "class Bar {};\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory, 1);
}

/// Check from header file: If there is a source file, insert the definition in the source file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic1()
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
        "{\n\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check from source file: Insert in source file, not header file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic2()
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
        "{\n\n"
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
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace1()
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
        "{\n\n"
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
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace2()
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
            ;
    expected = original +
            "\n"
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n"
            "\n"
            ;
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

void CppEditorPlugin::test_quickfix_InsertDefFromDecl_freeFunction()
{
    const QByteArray original = "void free()@;\n";
    const QByteArray expected =
        "void free()\n"
        "{\n\n"
        "}\n"
        "\n"
        ;

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check definition insert inside class
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_insideClass()
{
    const QByteArray original =
        "class Foo {\n"
        "    void b@ar();\n"
        "};";
    const QByteArray expected =
        "class Foo {\n"
        "    void bar()\n"
        "    {\n\n"
        "    }\n"
        "};\n";

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory, 1);
}

/// Check not triggering when definition exists
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_notTriggeringWhenDefinitionExists()
{
    const QByteArray original =
            "class Foo {\n"
            "    void b@ar();\n"
            "};\n"
            "void Foo::bar() {}\n";
    const QByteArray expected = original + "\n";

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory, 1);
}

/// Check not triggering when it is a statement
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_notTriggeringStatement()
{
    const QByteArray original =
            "class Foo {\n"
            "public:\n"
            "    Foo() {}\n"
            "};\n"
            "void freeFunc() {\n"
            "    Foo @f();"
            "}\n";
    const QByteArray expected = original + "\n";

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Find right implementation file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_findRightImplementationFile()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "struct Foo\n"
        "{\n"
        "    Foo();\n"
        "    void a();\n"
        "    void b@();\n"
        "};\n"
        "}\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File #1
    original =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n"
            "\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));


    // Source File #2
    original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::a()\n"
            "{\n\n"
            "}\n";
    expected = original +
            "\n"
            "void Foo::b()\n"
            "{\n\n"
            "}\n"
            "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file2.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Ignore generated functions declarations when looking at the surrounding
/// functions declarations in order to find the right implementation file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_ignoreSurroundingGeneratedDeclarations()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "#define DECLARE_HIDDEN_FUNCTION void hidden();\n"
        "struct Foo\n"
        "{\n"
        "    void a();\n"
        "    DECLARE_HIDDEN_FUNCTION\n"
        "    void b@();\n"
        "};\n"
        "}\n";
    expected = original + '\n';
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File #1
    original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::a()\n"
            "{\n\n"
            "}\n";
    expected =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::a()\n"
            "{\n\n"
            "}\n"
            "\n"
            "void Foo::b()\n"
            "{\n\n"
            "}\n"
            "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    // Source File #2
    original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::hidden()\n"
            "{\n\n"
            "}\n";
    expected = original + '\n';
    testFiles << TestDocument::create(original, expected, QLatin1String("file2.cpp"));

    InsertDefFromDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check if whitespace is respected for operator functions
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_respectWsInOperatorNames1()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "    Foo &opera@tor =();\n"
        "};\n";
    QByteArray expected =
        "class Foo\n"
        "{\n"
        "    Foo &operator =();\n"
        "};\n"
        "\n"
        "\n"
        "Foo &Foo::operator =()\n"
        "{\n"
        "\n"
        "}\n"
        "\n";

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check if whitespace is respected for operator functions
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_respectWsInOperatorNames2()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "    Foo &opera@tor=();\n"
        "};\n";
    QByteArray expected =
        "class Foo\n"
        "{\n"
        "    Foo &operator=();\n"
        "};\n"
        "\n"
        "\n"
        "Foo &Foo::operator=()\n"
        "{\n"
        "\n"
        "}\n"
        "\n";

    InsertDefFromDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

// Function for one of InsertDeclDef section cases
void insertToSectionDeclFromDef(const QByteArray &section, int sectionIndex)
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        + section + ":\n" +
        "    Foo();\n"
        "@};\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo@()\n"
        "{\n"
        "}\n"
        "\n"
        ;
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertDeclFromDef factory;
    TestCase data(testFiles);
    data.run(&factory, sectionIndex);
}

/// Check from source file: Insert in header file.
void CppEditorPlugin::test_quickfix_InsertDeclFromDef()
{
    insertToSectionDeclFromDef("public", 0);
    insertToSectionDeclFromDef("public slots", 1);
    insertToSectionDeclFromDef("protected", 2);
    insertToSectionDeclFromDef("protected slots", 3);
    insertToSectionDeclFromDef("private", 4);
    insertToSectionDeclFromDef("private slots", 5);
}

QList<Include> includesForSource(const QByteArray &source)
{
    const QString fileName = directoryOfTestFile() + QLatin1String("/file.cpp");
    Utils::FileSaver srcSaver(fileName);
    srcSaver.write(source);
    srcSaver.finalize();

    using namespace CppTools::Internal;

    CppModelManager *cmm = CppModelManager::instance();
    cmm->GC();
    CppPreprocessor pp((QPointer<CppModelManager>(cmm)));
    pp.setIncludePaths(QStringList(globalIncludePath()));
    pp.run(fileName);

    Document::Ptr document = cmm->snapshot().document(fileName);
    return document->includes();
}

/// Check: Detection of include groups separated by new lines
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_detectIncludeGroupsByNewLines()
{
    // Source referencing those files
    QByteArray source =
        "#include \"header.h\"\n"
        "\n"
        "#include \"file.h\"\n"
        "#include \"fileother.h\"\n"
        "\n"
        "#include <lib/fileother.h>\n"
        "#include <lib/file.h>\n"
        "\n"
        "#include \"otherlib/file.h\"\n"
        "#include \"otherlib/fileother.h\"\n"
        "\n"
        "#include \"utils/utils.h\"\n"
        "\n"
        "#include <QDebug>\n"
        "#include <QDir>\n"
        "#include <QString>\n"
        "\n"
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <except>\n"
        "\n"
        "#include <iostream>\n"
        "#include \"stuff\"\n"
        "#include <except>\n"
        "\n"
        ;

    QList<Include> includes = includesForSource(source);
    QCOMPARE(includes.size(), 17);
    QList<IncludeGroup> includeGroups
        = IncludeGroup::detectIncludeGroupsByNewLines(includes);
    QCOMPARE(includeGroups.size(), 8);

    QCOMPARE(includeGroups.at(0).size(), 1);
    QVERIFY(includeGroups.at(0).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(0).hasOnlyIncludesOfType(Client::IncludeLocal));
    QVERIFY(includeGroups.at(0).isSorted());

    QCOMPARE(includeGroups.at(1).size(), 2);
    QVERIFY(!includeGroups.at(1).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(1).hasOnlyIncludesOfType(Client::IncludeLocal));
    QVERIFY(includeGroups.at(1).isSorted());

    QCOMPARE(includeGroups.at(2).size(), 2);
    QVERIFY(!includeGroups.at(2).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(2).hasOnlyIncludesOfType(Client::IncludeGlobal));
    QVERIFY(!includeGroups.at(2).isSorted());

    QCOMPARE(includeGroups.at(6).size(), 3);
    QVERIFY(includeGroups.at(6).commonPrefix().isEmpty());
    QVERIFY(includeGroups.at(6).hasOnlyIncludesOfType(Client::IncludeGlobal));
    QVERIFY(!includeGroups.at(6).isSorted());

    QCOMPARE(includeGroups.at(7).size(), 3);
    QVERIFY(includeGroups.at(7).commonPrefix().isEmpty());
    QVERIFY(!includeGroups.at(7).hasOnlyIncludesOfType(Client::IncludeLocal));
    QVERIFY(!includeGroups.at(7).hasOnlyIncludesOfType(Client::IncludeGlobal));
    QVERIFY(!includeGroups.at(7).isSorted());

    QCOMPARE(IncludeGroup::filterIncludeGroups(includeGroups, Client::IncludeLocal).size(), 4);
    QCOMPARE(IncludeGroup::filterIncludeGroups(includeGroups, Client::IncludeGlobal).size(), 3);
    QCOMPARE(IncludeGroup::filterMixedIncludeGroups(includeGroups).size(), 1);
}

/// Check: Detection of include groups separated by include dirs
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_detectIncludeGroupsByIncludeDir()
{
    QByteArray source =
        "#include \"file.h\"\n"
        "#include \"fileother.h\"\n"
        "#include <lib/file.h>\n"
        "#include <lib/fileother.h>\n"
        "#include \"otherlib/file.h\"\n"
        "#include \"otherlib/fileother.h\"\n"
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <except>\n"
        "\n"
        ;

    QList<Include> includes = includesForSource(source);
    QCOMPARE(includes.size(), 9);
    QList<IncludeGroup> includeGroups
        = IncludeGroup::detectIncludeGroupsByIncludeDir(includes);
    QCOMPARE(includeGroups.size(), 4);

    QCOMPARE(includeGroups.at(0).size(), 2);
    QVERIFY(includeGroups.at(0).commonIncludeDir().isEmpty());

    QCOMPARE(includeGroups.at(1).size(), 2);
    QCOMPARE(includeGroups.at(1).commonIncludeDir(), QLatin1String("lib/"));

    QCOMPARE(includeGroups.at(2).size(), 2);
    QCOMPARE(includeGroups.at(2).commonIncludeDir(), QLatin1String("otherlib/"));

    QCOMPARE(includeGroups.at(3).size(), 3);
    QCOMPARE(includeGroups.at(3).commonIncludeDir(), QLatin1String(""));
}

/// Check: Detection of include groups separated by include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_detectIncludeGroupsByIncludeType()
{
    QByteArray source =
        "#include \"file.h\"\n"
        "#include \"fileother.h\"\n"
        "#include <lib/file.h>\n"
        "#include <lib/fileother.h>\n"
        "#include \"otherlib/file.h\"\n"
        "#include \"otherlib/fileother.h\"\n"
        "#include <iostream>\n"
        "#include <string>\n"
        "#include <except>\n"
        "\n"
        ;

    QList<Include> includes = includesForSource(source);
    QCOMPARE(includes.size(), 9);
    QList<IncludeGroup> includeGroups
        = IncludeGroup::detectIncludeGroupsByIncludeDir(includes);
    QCOMPARE(includeGroups.size(), 4);

    QCOMPARE(includeGroups.at(0).size(), 2);
    QVERIFY(includeGroups.at(0).hasOnlyIncludesOfType(Client::IncludeLocal));

    QCOMPARE(includeGroups.at(1).size(), 2);
    QVERIFY(includeGroups.at(1).hasOnlyIncludesOfType(Client::IncludeGlobal));

    QCOMPARE(includeGroups.at(2).size(), 2);
    QVERIFY(includeGroups.at(2).hasOnlyIncludesOfType(Client::IncludeLocal));

    QCOMPARE(includeGroups.at(3).size(), 3);
    QVERIFY(includeGroups.at(3).hasOnlyIncludesOfType(Client::IncludeGlobal));
}

/// Check: Add include if there is already an include
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_normal()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "class Foo {};\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("afile.h"));

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Fo@o foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("afile.cpp"));

    // Do not use the test factory, at least once we want to go through the "full stack".
    AddIncludeForUndefinedIdentifier factory;
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Ignore *.moc includes
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_ignoremoc()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "void @f();\n"
        "#include \"file.moc\";\n"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "void f();\n"
        "#include \"file.moc\";\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert include at top for a sorted group
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_sortingTop()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"y.h\"\n"
        "#include \"z.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"file.h\"\n"
        "#include \"y.h\"\n"
        "#include \"z.h\"\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert include in the middle for a sorted group
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_sortingMiddle()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"a.h\"\n"
        "#include \"z.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"file.h\"\n"
        "#include \"z.h\"\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert include at bottom for a sorted group
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_sortingBottom()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"a.h\"\n"
        "#include \"b.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"b.h\"\n"
        "#include \"file.h\"\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: For an unsorted group the new include is appended
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_appendToUnsorted()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"b.h\"\n"
        "#include \"a.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"b.h\"\n"
        "#include \"a.h\"\n"
        "#include \"file.h\"\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert a local include at front if there are only global includes
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_firstLocalIncludeAtFront()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include <a.h>\n"
        "#include <b.h>\n"
        "\n@"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "#include <a.h>\n"
        "#include <b.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert a global include at back if there are only local includes
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_firstGlobalIncludeAtBack()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"a.h\"\n"
        "#include \"b.h\"\n"
        "\n"
        "void @f();\n"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"b.h\"\n"
        "\n"
        "#include <file.h>\n"
        "\n"
        "void f();\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<file.h>"));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Prefer group with longest matching prefix
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_preferGroupWithLongerMatchingPrefix()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"prefixa.h\"\n"
        "#include \"prefixb.h\"\n"
        "\n"
        "#include \"foo.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"prefixa.h\"\n"
        "#include \"prefixb.h\"\n"
        "#include \"prefixc.h\"\n"
        "\n"
        "#include \"foo.h\"\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"prefixc.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Create a new include group if there are only include groups with a different include dir
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_newGroupIfOnlyDifferentIncludeDirs()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"lib/file.h\"\n"
        "#include \"lib/fileother.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"lib/file.h\"\n"
        "#include \"lib/fileother.h\"\n"
        "\n"
        "#include \"file.h\"\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Include group with mixed include dirs, sorted --> insert properly
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedDirsSorted()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include <lib/file.h>\n"
        "#include <otherlib/file.h>\n"
        "#include <utils/file.h>\n"
        "\n@"
        ;
    expected =
        "#include <firstlib/file.h>\n"
        "#include <lib/file.h>\n"
        "#include <otherlib/file.h>\n"
        "#include <utils/file.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<firstlib/file.h>"));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Include group with mixed include dirs, unsorted --> append
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedDirsUnsorted()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include <otherlib/file.h>\n"
        "#include <lib/file.h>\n"
        "#include <utils/file.h>\n"
        "\n@"
        ;
    expected =
        "#include <otherlib/file.h>\n"
        "#include <lib/file.h>\n"
        "#include <utils/file.h>\n"
        "#include <lastlib/file.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<lastlib/file.h>"));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes1()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"a.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"z.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes2()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"a.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes3()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"z.h\"\n"
        "#include \"lib/file.h\"\n"
        "#include <global.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"lib/file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes4()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "#include <lib/file.h>\n"
        "\n\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<lib/file.h>"));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert very first include
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_noinclude()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "void @f();\n"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "void f();\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert very first include if there is a c++ style comment on top
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_veryFirstIncludeCppStyleCommentOnTop()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "\n"
        "// comment\n"
        "\n"
        "void @f();\n"
        ;
    expected =
        "\n"
        "// comment\n"
        "\n"
        "#include \"file.h\"\n"
        "\n"
        "void @f();\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: Insert very first include if there is a c style comment on top
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_veryFirstIncludeCStyleCommentOnTop()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "\n"
        "/*\n"
        " comment\n"
        " */\n"
        "\n"
        "void @f();\n"
        ;
    expected =
        "\n"
        "/*\n"
        " comment\n"
        " */\n"
        "\n"
        "#include \"file.h\"\n"
        "\n"
        "void @f();\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    TestCase data(testFiles, QStringList(globalIncludePath()));
    data.run(&factory);
}

/// Check: If a "Qt Class" was not found by the locator, check the header files in the Qt
/// include paths
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_checkQSomethingInQtIncludePaths()
{
    QList<TestDocumentPtr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "@QDir dir;\n"
        ;
    expected =
        "#include <QDir>\n"
        "\n"
        "QDir dir;\n"
        "\n"
        ;
    testFiles << TestDocument::create(original, expected, directoryOfTestFile() + QLatin1Char('/')
                                      + QLatin1String("file.cpp"));

    AddIncludeForUndefinedIdentifier factory;
    TestCase data(testFiles, QStringList(globalQtCoreIncludePath()));
    data.run(&factory);
}

/// Check: Move definition from header to cpp.
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCpp()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo {\n"
        "  inline int numbe@r() const\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "\n"
        "    void bar();\n"
        "};";
    expected =
        "class Foo {\n"
        "  inline int number() const;\n"
        "\n"
        "    void bar();\n"
        "};\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "int Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppInsideNS()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace SomeNamespace {\n"
        "class Foo {\n"
        "  int ba@r()\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "};\n"
        "}\n";
    expected =
        "namespace SomeNamespace {\n"
        "class Foo {\n"
        "  int ba@r();\n"
        "};\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "namespace SomeNamespace {\n"
        "\n"
        "}\n";
    expected =
        "#include \"file.h\"\n"
        "namespace SomeNamespace {\n"
        "\n"
        "int Foo::bar()\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Move definition outside class
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncOutside1()
{
    QByteArray original =
        "class Foo {\n"
        "    void f1();\n"
        "    inline int f2@() const\n"
        "    {\n"
        "        return 1;\n"
        "    }\n"
        "    void f3();\n"
        "    void f4();\n"
        "};\n"
        "\n"
        "void Foo::f4() {}\n";
    QByteArray expected =
        "class Foo {\n"
        "    void f1();\n"
        "    inline int f2@() const;\n"
        "    void f3();\n"
        "    void f4();\n"
        "};\n"
        "\n"
        "int Foo::f2() const\n"
        "{\n"
        "    return 1;\n"
        "}\n"
        "\n"
        "void Foo::f4() {}\n\n";

    MoveFuncDefOutside factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Move definition outside class
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncOutside2()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo {\n"
        "    void f1();\n"
        "    int f2@()\n"
        "    {\n"
        "        return 1;\n"
        "    }\n"
        "    void f3();\n"
        "};\n";
    expected =
        "class Foo {\n"
        "    void f1();\n"
        "    int f2();\n"
        "    void f3();\n"
        "};\n"
        "\n"
        "int Foo::f2()\n"
        "{\n"
        "    return 1;\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "void Foo::f1() {}\n"
        "void Foo::f3() {}\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory, 1);
}

/// Check: Move definition from header to cpp (with namespace).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int numbe@r() const\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "};\n"
        "}";
    expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "}\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "int MyNs::Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Move definition from header to cpp (with namespace + using).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int numbe@r() const\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "};\n"
        "}";
    expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "}\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "using namespace MyNs;\n";
    expected =
        "#include \"file.h\"\n"
        "using namespace MyNs;\n"
        "\n"
        "\n"
        "int Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Move definition outside class with Namespace
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncOutsideWithNs()
{
    QByteArray original =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int numbe@r() const\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "};}";
    QByteArray expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "\n"
        "int Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n}\n";

    MoveFuncDefOutside factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Move free function from header to cpp.
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_FreeFuncToCpp()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "int numbe@r() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected =
        "int number() const;\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "int number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Move free function from header to cpp (with namespace).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace MyNamespace {\n"
        "int numbe@r() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "}\n";
    expected =
        "namespace MyNamespace {\n"
        "int number() const;\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "int MyNamespace::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Move Ctor with member initialization list (QTCREATORBUG-9157).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_CtorWithInitialization1()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo {\n"
        "public:\n"
        "    Fo@o() : a(42), b(3.141) {}\n"
        "private:\n"
        "    int a;\n"
        "    float b;\n"
        "};";
    expected =
        "class Foo {\n"
        "public:\n"
        "    Foo();\n"
        "private:\n"
        "    int a;\n"
        "    float b;\n"
        "};\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original ="#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "Foo::Foo() : a(42), b(3.141) {}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Move Ctor with member initialization list (QTCREATORBUG-9462).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_CtorWithInitialization2()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    Fo@o() : member(2)\n"
        "    {\n"
        "    }\n"
        "\n"
        "    int member;\n"
        "};";

    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    Foo();\n"
        "\n"
        "    int member;\n"
        "};\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original ="#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "Foo::Foo() : member(2)\n"
        "{\n"
        "}\n"
        "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check if definition is inserted right after class for move definition outside
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_afterClass()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "    Foo();\n"
        "    void a@() {}\n"
        "};\n"
        "\n"
        "class Bar {};\n";
    expected =
        "class Foo\n"
        "{\n"
        "    Foo();\n"
        "    void a();\n"
        "};\n"
        "\n"
        "void Foo::a() {}\n"
        "\n"
        "class Bar {};\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n";
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefOutside factory;
    TestCase data(testFiles);
    data.run(&factory, 1);
}

/// Check if whitespace is respected for operator functions
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_respectWsInOperatorNames1()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "    Foo &opera@tor =() {}\n"
        "};\n";
    QByteArray expected =
        "class Foo\n"
        "{\n"
        "    Foo &operator =();\n"
        "};\n"
        "\n"
        "\n"
        "Foo &Foo::operator =() {}\n"
        "\n";

    MoveFuncDefOutside factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check if whitespace is respected for operator functions
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_respectWsInOperatorNames2()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "    Foo &opera@tor=() {}\n"
        "};\n";
    QByteArray expected =
        "class Foo\n"
        "{\n"
        "    Foo &operator=();\n"
        "};\n"
        "\n"
        "\n"
        "Foo &Foo::operator=() {}\n"
        "\n";

    MoveFuncDefOutside factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCpp()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFunc()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo {\n"
        "    inline int number() const;\n"
        "};\n";
    expected =
        "class Foo {\n"
        "    inline int number() const {return 5;}\n"
        "};\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int Foo::num@ber() const {return 5;}\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefToDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncOutside()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncOutside()
{
    QByteArray original =
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "\n"
        "int Foo::num@ber() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";

    QByteArray expected =
        "class Foo {\n"
        "    inline int number() const\n"
        "    {\n"
        "        return 5;\n"
        "    }\n"
        "};\n"
        "\n\n\n";

    MoveFuncDefToDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNS()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "}\n";
    expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "    inline int number() const\n"
        "    {\n"
        "        return 5;\n"
        "    }\n"
        "};\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int MyNs::Foo::num@ber() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected = "#include \"file.h\"\n\n\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefToDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNSUsing()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "}\n";
    expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "    inline int number() const\n"
        "    {\n"
        "        return 5;\n"
        "    }\n"
        "};\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "using namespace MyNs;\n"
        "\n"
        "int Foo::num@ber() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected =
        "#include \"file.h\"\n"
        "using namespace MyNs;\n"
        "\n\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefToDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncOutsideWithNs()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncOutsideWithNs()
{
    QByteArray original =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "\n"
        "int Foo::numb@er() const\n"
        "{\n"
        "    return 5;\n"
        "}"
        "\n}\n";
    QByteArray expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "    inline int number() const\n"
        "    {\n"
        "        return 5;\n"
        "    }\n"
        "};\n\n\n}\n\n";

    MoveFuncDefToDecl factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_FreeFuncToCpp()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_FreeFuncToCpp()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original = "int number() const;\n";
    expected =
        "int number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "int numb@er() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected = "#include \"file.h\"\n\n\n\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefToDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_FreeFuncToCppNS()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace MyNamespace {\n"
        "int number() const;\n"
        "}\n";
    expected =
        "namespace MyNamespace {\n"
        "int number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "}\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int MyNamespace::nu@mber() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefToDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_CtorWithInitialization()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_CtorWithInitialization()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo {\n"
        "public:\n"
        "    Foo();\n"
        "private:\n"
        "    int a;\n"
        "    float b;\n"
        "};";
    expected =
        "class Foo {\n"
        "public:\n"
        "    Foo() : a(42), b(3.141) {}\n"
        "private:\n"
        "    int a;\n"
        "    float b;\n"
        "};\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::F@oo() : a(42), b(3.141) {}"
        ;
    expected ="#include \"file.h\"\n\n\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    MoveFuncDefToDecl factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: Add local variable for a free function.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_freeFunction()
{
    const QByteArray original =
        "int foo() {return 1;}\n"
        "void bar() {fo@o();}";
    const QByteArray expected =
        "int foo() {return 1;}\n"
        "void bar() {int localFoo = foo();}\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Add local variable for a member function.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_memberFunction()
{
    const QByteArray original =
        "class Foo {public: int* fooFunc();}\n"
        "void bar() {\n"
        "    Foo *f = new Foo;\n"
        "    @f->fooFunc();\n"
        "}";
    const QByteArray expected =
        "class Foo {public: int* fooFunc();}\n"
        "void bar() {\n"
        "    Foo *f = new Foo;\n"
        "    int *localFooFunc = f->fooFunc();\n"
        "}\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Add local variable for a static member function.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_staticMemberFunction()
{
    const QByteArray original =
        "class Foo {public: static int* fooFunc();}\n"
        "void bar() {\n"
        "    Foo::fooF@unc();\n"
        "}";
    const QByteArray expected =
        "class Foo {public: static int* fooFunc();}\n"
        "void bar() {\n"
        "    int *localFooFunc = Foo::fooFunc();\n"
        "}\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Add local variable for a new Expression.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_newExpression()
{
    const QByteArray original =
        "class Foo {}\n"
        "void bar() {\n"
        "    new Fo@o;\n"
        "}";
    const QByteArray expected =
        "class Foo {}\n"
        "void bar() {\n"
        "    Foo *localFoo = new Foo;\n"
        "}\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

void CppEditorPlugin::test_quickfix_AssignToLocalVariable_templates()
{

    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "template <typename T>\n"
        "class List {\n"
        "public:\n"
        "    T first();"
        "};\n"
        ;
    expected = original + "\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original =
        "#include \"file.h\"\n"
        "void foo() {\n"
        "    List<int> list;\n"
        "    li@st.first();\n"
        "}";
    expected =
        "#include \"file.h\"\n"
        "void foo() {\n"
        "    List<int> list;\n"
        "    int localFirst = list.first();\n"
        "}\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    AssignToLocalVariable factory;
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: No trigger for function inside member initialization list.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noInitializationList()
{
    const QByteArray original =
        "class Foo\n"
        "{\n"
        "    public: Foo : m_i(fooF@unc()) {}\n"
        "    int fooFunc() {return 2;}\n"
        "    int m_i;\n"
        "};";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for void functions.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noVoidFunction()
{
    const QByteArray original =
        "void foo() {}\n"
        "void bar() {fo@o();}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for void member functions.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noVoidMemberFunction()
{
    const QByteArray original =
        "class Foo {public: void fooFunc();}\n"
        "void bar() {\n"
        "    Foo *f = new Foo;\n"
        "    @f->fooFunc();\n"
        "}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for void static member functions.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noVoidStaticMemberFunction()
{
    const QByteArray original =
        "class Foo {public: static void fooFunc();}\n"
        "void bar() {\n"
        "    Foo::fo@oFunc();\n"
        "}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions in expressions.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noFunctionInExpression()
{
    const QByteArray original =
        "int foo(int a) {return a;}\n"
        "int bar() {return 1;}"
        "void baz() {foo(@bar() + bar());}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions in functions. (QTCREATORBUG-9510)
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noFunctionInFunction()
{
    const QByteArray original =
        "int foo(int a, int b) {return a + b;}\n"
        "int bar(int a) {return a;}\n"
        "void baz() {\n"
        "    int a = foo(ba@r(), bar());\n"
        "}\n";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions in return statements (classes).
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noReturnClass1()
{
    const QByteArray original =
        "class Foo {public: static void fooFunc();}\n"
        "Foo* bar() {\n"
        "    return new Fo@o;\n"
        "}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions in return statements (classes). (QTCREATORBUG-9525)
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noReturnClass2()
{
    const QByteArray original =
        "class Foo {public: int fooFunc();}\n"
        "int bar() {\n"
        "    return (new Fo@o)->fooFunc();\n"
        "}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions in return statements (functions).
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noReturnFunc1()
{
    const QByteArray original =
        "class Foo {public: int fooFunc();}\n"
        "int bar() {\n"
        "    return Foo::fooFu@nc();\n"
        "}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions in return statements (functions). (QTCREATORBUG-9525)
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noReturnFunc2()
{
    const QByteArray original =
        "int bar() {\n"
        "    return list.firs@t().foo;\n"
        "}\n";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: No trigger for functions which does not match in signature.
void CppEditorPlugin::test_quickfix_AssignToLocalVariable_noSignatureMatch()
{
    const QByteArray original =
        "int someFunc(int);\n"
        "\n"
        "void f()\n"
        "{\n"
        "    some@Func();\n"
        "}";
    const QByteArray expected = original + "\n";

    AssignToLocalVariable factory;
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert only declarations
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_onlyDecl()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOnlyDeclarations, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert only declarations vithout virtual keyword
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_onlyDeclWithoutVirtual()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    int virtualFuncA();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOnlyDeclarations, false));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Are access specifiers considered
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_Access()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "protected:\n"
        "    virtual int b();\n"
        "private:\n"
        "    virtual int c();\n"
        "public slots:\n"
        "    virtual int d();\n"
        "protected slots:\n"
        "    virtual int e();\n"
        "private slots:\n"
        "    virtual int f();\n"
        "signals:\n"
        "    virtual int g();\n"
        "};\n\n"
        "class Der@ived : public BaseA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "protected:\n"
        "    virtual int b();\n"
        "private:\n"
        "    virtual int c();\n"
        "public slots:\n"
        "    virtual int d();\n"
        "protected slots:\n"
        "    virtual int e();\n"
        "private slots:\n"
        "    virtual int f();\n"
        "signals:\n"
        "    virtual int g();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n\n"
        "protected:\n"
        "    virtual int b();\n\n"
        "private:\n"
        "    virtual int c();\n\n"
        "public slots:\n"
        "    virtual int d();\n\n"
        "protected slots:\n"
        "    virtual int e();\n\n"
        "private slots:\n"
        "    virtual int f();\n\n"
        "signals:\n"
        "    virtual int g();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOnlyDeclarations, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Is a base class of a base class considered.
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_Superclass()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b();\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int b();\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseB interface\n"
        "public:\n"
        "    virtual int b();\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOnlyDeclarations, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Do not insert reimplemented functions twice.
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_SuperclassOverride()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class BaseB : public BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class Der@ived : public BaseB {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOnlyDeclarations, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert only declarations for pure virtual function
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_PureVirtualOnlyDecl()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOnlyDeclarations, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert pure virtual functions inside class
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_PureVirtualInside()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA() = 0;\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA()\n"
        "    {\n"
        "    }\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeInsideClass, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert inside class
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_inside()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA()\n"
        "    {\n"
        "    }\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeInsideClass, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert outside class
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_outside()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "int Derived::virtualFuncA()\n"
        "{\n"
        "}\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOutsideClass, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Insert in implementation file
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_implementationFile()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "public:\n"
        "    Derived();\n"
        "};";
    expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n\n"
        "class Derived : public BaseA {\n"
        "public:\n"
        "    Derived();\n"
        "\n"
        "    // BaseA interface\n"
        "public:\n"
        "    virtual int a();\n"
        "};\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original = "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n"
        "int Derived::a()\n"
        "{\n}\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeImplementationFile, true));
    TestCase data(testFiles);
    data.run(&factory);
}

/// Check: No trigger: all implemented
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_notrigger_allImplemented()
{
    const QByteArray original =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};";
    const QByteArray expected =
        "class BaseA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n\n"
        "class Derived : public Bas@eA {\n"
        "public:\n"
        "    virtual int virtualFuncA();\n"
        "};\n";

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeOutsideClass, true));
    TestCase data(original, expected);
    data.run(&factory);
}

/// Check: Qualified names.
void CppEditorPlugin::test_quickfix_InsertVirtualMethods_BaseClassInNamespace()
{
    QList<TestDocumentPtr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "namespace BaseNS {enum BaseEnum {EnumA = 1};}\n"
        "namespace BaseNS {\n"
        "class Base {\n"
        "public:\n"
        "    virtual BaseEnum a(BaseEnum e);\n"
        "};\n"
        "}\n"
        "class Deri@ved : public BaseNS::Base {\n"
        "public:\n"
        "    Derived();\n"
        "};";
    expected =
        "namespace BaseNS {enum BaseEnum {EnumA = 1};}\n"
        "namespace BaseNS {\n"
        "class Base {\n"
        "public:\n"
        "    virtual BaseEnum a(BaseEnum e);\n"
        "};\n"
        "}\n"
        "class Deri@ved : public BaseNS::Base {\n"
        "public:\n"
        "    Derived();\n"
        "\n"
        "    // Base interface\n"
        "public:\n"
        "    virtual BaseNS::BaseEnum a(BaseNS::BaseEnum e);\n"
        "};\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.h"));

    // Source File
    original = "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n"
        "BaseNS::BaseEnum Derived::a(BaseNS::BaseEnum e)\n"
        "{\n}\n";
    testFiles << TestDocument::create(original, expected, QLatin1String("file.cpp"));

    InsertVirtualMethods factory(new InsertVirtualMethodsDialogTest(
                                     InsertVirtualMethodsDialog::ModeImplementationFile, true));
    TestCase data(testFiles);
    data.run(&factory);
}
