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
#include "cppeditorplugin.h"

#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QtTest>


/*!
    Tests for Follow Symbol Under Cursor and Switch Between Method Declaration/Definition

    Section numbers refer to

        Working Draft, Standard for Programming Language C++
        Document Number: N3242=11-0012

    You can find potential test code for Follow Symbol Under Cursor on the bottom of this file.
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
 * Represents a test document.
 *
 * A TestDocument's source can contain special characters:
 *   - a '@' character denotes the initial text cursor position
 *   - a '$' character denotes the target text cursor position
 */
class TestDocument
{
public:
    TestDocument(const QByteArray &theSource, const QString &fileName)
        : source(theSource)
        , fileName(fileName)
        , initialCursorPosition(source.indexOf('@'))
        , targetCursorPosition(source.indexOf('$'))
        , editor(0)
        , editorWidget(0)
    {
        QVERIFY(initialCursorPosition != targetCursorPosition);
        if (initialCursorPosition > targetCursorPosition) {
            source.remove(initialCursorPosition, 1);
            if (targetCursorPosition != -1) {
                source.remove(targetCursorPosition, 1);
                --initialCursorPosition;
            }
        } else {
            source.remove(targetCursorPosition, 1);
            if (initialCursorPosition != -1) {
                source.remove(initialCursorPosition, 1);
                --targetCursorPosition;
            }
        }
    }

    static TestDocumentPtr create(const QByteArray &theOriginalSource, const QString &fileName)
    {
        return TestDocumentPtr(new TestDocument(theOriginalSource, fileName));
    }

    bool hasInitialCursorMarker() const { return initialCursorPosition != -1; }
    bool hasTargetCursorMarker() const { return targetCursorPosition != -1; }

    QString filePath() const
    {
        if (directoryPath.isEmpty())
            qDebug() << "directoryPath not set!";
        return directoryPath + QLatin1Char('/') + fileName;
    }

    void writeToDisk() const
    {
        Utils::FileSaver srcSaver(filePath());
        srcSaver.write(source);
        srcSaver.finalize();
    }

    QByteArray source;

    const QString fileName;
    QString directoryPath;
    int initialCursorPosition;
    int targetCursorPosition;

    CPPEditor *editor;
    CPPEditorWidget *editorWidget;
};

/**
 * Encapsulates the whole process of setting up several editors, positioning the cursor,
 * executing Follow Symbol Under Cursor or Switch Between Method Declaration/Definition
 * and checking the result.
 */
class TestCase
{
public:
    enum CppEditorAction {
        FollowSymbolUnderCursor,
        SwitchBetweenMethodDeclarationDefinition
    };

    TestCase(CppEditorAction action, const QByteArray &source);
    TestCase(CppEditorAction action, const QList<TestDocumentPtr> theTestFiles);
    ~TestCase();

    void run();

private:
    TestCase(const TestCase &);
    TestCase &operator=(const TestCase &);

    void init();

    TestDocumentPtr testFileWithInitialCursorMarker();
    TestDocumentPtr testFileWithTargetCursorMarker();

private:
    CppEditorAction m_action;
    QList<TestDocumentPtr> m_testFiles;
};

/// Convenience function for creating a TestDocument.
/// See TestDocument.
TestCase::TestCase(CppEditorAction action, const QByteArray &source)
    : m_action(action)
{
    m_testFiles << TestDocument::create(source, QLatin1String("file.cpp"));
    init();
}

/// Creates a test case with multiple test files.
/// Exactly one test document must be provided that contains '@', the initial position marker.
/// Exactly one test document must be provided that contains '$', the target position marker.
/// It can be the same document.
TestCase::TestCase(CppEditorAction action, const QList<TestDocumentPtr> theTestFiles)
    : m_action(action)
    , m_testFiles(theTestFiles)
{
    init();
}

void TestCase::init()
{
    // Check if there are initial and target position markers
    QVERIFY2(testFileWithInitialCursorMarker(),
        "No test file with initial cursor marker is provided.");
    QVERIFY2(testFileWithTargetCursorMarker(),
        "No test file with target cursor marker is provided.");

    // Write files to disk
    const QString directoryPath = QDir::tempPath();
    foreach (TestDocumentPtr testFile, m_testFiles) {
        testFile->directoryPath = directoryPath;
        testFile->writeToDisk();
    }

    // Update Code Model
    QStringList filePaths;
    foreach (const TestDocumentPtr &testFile, m_testFiles)
        filePaths << testFile->filePath();
    CppTools::CppModelManagerInterface::instance()->updateSourceFiles(filePaths);

    // Wait for the indexer to process all files.
    // All these files are "Fast Checked", that is the function bodies are not processed.
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
    foreach (TestDocumentPtr testFile, m_testFiles) {
        testFile->editor
            = dynamic_cast<CPPEditor *>(EditorManager::openEditor(testFile->filePath()));
        QVERIFY(testFile->editor);

        testFile->editorWidget = dynamic_cast<CPPEditorWidget *>(testFile->editor->editorWidget());
        QVERIFY(testFile->editorWidget);

        // Wait until the indexer processed the just opened file.
        // The file is "Full Checked" since it is in the working copy now,
        // that is the function bodies are processed.
        forever {
            Snapshot snapshot = CppTools::CppModelManagerInterface::instance()->snapshot();
            if (Document::Ptr document = snapshot.document(testFile->filePath())) {
                if (document->checkMode() == Document::FullCheck)
                    break;
                QCoreApplication::processEvents();
            }
        }

        // Rehighlight
        testFile->editorWidget->semanticRehighlight(true);
        // Wait for the semantic info from the future
        while (testFile->editorWidget->semanticInfo().doc.isNull())
            QCoreApplication::processEvents();
    }
}

TestCase::~TestCase()
{
    // Close editors
    QList<Core::IEditor *> editorsToClose;
    foreach (const TestDocumentPtr testFile, m_testFiles) {
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

TestDocumentPtr TestCase::testFileWithInitialCursorMarker()
{
    foreach (const TestDocumentPtr testFile, m_testFiles) {
        if (testFile->hasInitialCursorMarker())
            return testFile;
    }
    return TestDocumentPtr();
}

TestDocumentPtr TestCase::testFileWithTargetCursorMarker()
{
    foreach (const TestDocumentPtr testFile, m_testFiles) {
        if (testFile->hasTargetCursorMarker())
            return testFile;
    }
    return TestDocumentPtr();
}

void TestCase::run()
{
    TestDocumentPtr initialTestFile = testFileWithInitialCursorMarker();
    QVERIFY(initialTestFile);
    TestDocumentPtr targetTestFile = testFileWithTargetCursorMarker();
    QVERIFY(targetTestFile);

    // Activate editor of initial test file
    EditorManager::activateEditor(initialTestFile->editor);

    initialTestFile->editor->setCursorPosition(initialTestFile->initialCursorPosition);
//    qDebug() << "Initial line:" << initialTestFile->editor->currentLine();
//    qDebug() << "Initial column:" << initialTestFile->editor->currentColumn() - 1;

    // Trigger the action
    switch (m_action) {
    case FollowSymbolUnderCursor:
        initialTestFile->editorWidget->openLinkUnderCursor();
        break;
    case SwitchBetweenMethodDeclarationDefinition:
        CppEditorPlugin::instance()->switchDeclarationDefinition();
        break;
    default:
        QFAIL("Unknown test action");
        break;
    }

    QCoreApplication::processEvents();

    // Compare
    IEditor *currentEditor = EditorManager::instance()->currentEditor();
    BaseTextEditor *currentTextEditor = dynamic_cast<BaseTextEditor*>(currentEditor);
    QVERIFY(currentTextEditor);

    QCOMPARE(currentTextEditor->document()->fileName(), targetTestFile->filePath());
    int expectedLine, expectedColumn;
    currentTextEditor->convertPosition(targetTestFile->targetCursorPosition,
                                       &expectedLine, &expectedColumn);
//    qDebug() << "Expected line:" << expectedLine;
//    qDebug() << "Expected column:" << expectedColumn;
    QCOMPARE(currentTextEditor->currentLine(), expectedLine);
    QCOMPARE(currentTextEditor->currentColumn() - 1, expectedColumn);
}

} // anonymous namespace

void CppEditorPlugin::test_SwitchMethodDeclarationDefinition_fromFunctionDeclarationSymbol()
{
    QList<TestDocumentPtr> testFiles;

    const QByteArray headerContents =
        "class C\n"
        "{\n"
        "public:\n"
        "    C();\n"
        "    int @function();\n"  // Line 5
        "};\n"
        ;
    testFiles << TestDocument::create(headerContents, QLatin1String("file.h"));

    const QByteArray sourceContents =
        "#include \"file.h\"\n"
        "\n"
        "C::C()\n"
        "{\n"
        "}\n"                   // Line 5
        "\n"
        "int C::$function()\n"
        "{\n"
        "    return 1 + 1;\n"
        "}\n"                   // Line 10
        ;
    testFiles << TestDocument::create(sourceContents, QLatin1String("file.cpp"));

    TestCase test(TestCase::SwitchBetweenMethodDeclarationDefinition, testFiles);
    test.run();
}

void CppEditorPlugin::test_SwitchMethodDeclarationDefinition_fromFunctionDefinitionSymbol()
{
    QList<TestDocumentPtr> testFiles;

    const QByteArray headerContents =
        "class C\n"
        "{\n"
        "public:\n"
        "    C();\n"
        "    int $function();\n"
        "};\n"
        ;
    testFiles << TestDocument::create(headerContents, QLatin1String("file.h"));

    const QByteArray sourceContents =
        "#include \"file.h\"\n"
        "\n"
        "C::C()\n"
        "{\n"
        "}\n"
        "\n"
        "int C::@function()\n"
        "{\n"
        "    return 1 + 1;\n"
        "}\n"
        ;
    testFiles << TestDocument::create(sourceContents, QLatin1String("file.cpp"));

    TestCase test(TestCase::SwitchBetweenMethodDeclarationDefinition, testFiles);
    test.run();
}

void CppEditorPlugin::test_SwitchMethodDeclarationDefinition_fromFunctionBody()
{
    QList<TestDocumentPtr> testFiles;

    const QByteArray headerContents =
        "class C\n"
        "{\n"
        "public:\n"
        "    C();\n"
        "    int $function();\n"
        "};\n"
        ;
    testFiles << TestDocument::create(headerContents, QLatin1String("file.h"));

    const QByteArray sourceContents =
        "#include \"file.h\"\n"
        "\n"
        "C::C()\n"
        "{\n"
        "}\n"                   // Line 5
        "\n"
        "int C::function()\n"
        "{\n"
        "    return @1 + 1;\n"
        "}\n"                   // Line 10
        ;
    testFiles << TestDocument::create(sourceContents, QLatin1String("file.cpp"));

    TestCase test(TestCase::SwitchBetweenMethodDeclarationDefinition, testFiles);
    test.run();
}

void CppEditorPlugin::test_SwitchMethodDeclarationDefinition_fromReturnType()
{
    QList<TestDocumentPtr> testFiles;

    const QByteArray headerContents =
        "class C\n"
        "{\n"
        "public:\n"
        "    C();\n"
        "    int $function();\n"
        "};\n"
        ;
    testFiles << TestDocument::create(headerContents, QLatin1String("file.h"));

    const QByteArray sourceContents =
        "#include \"file.h\"\n"
        "\n"
        "C::C()\n"
        "{\n"
        "}\n"                   // Line 5
        "\n"
        "@int C::function()\n"
        "{\n"
        "    return 1 + 1;\n"
        "}\n"                   // Line 10
        ;
    testFiles << TestDocument::create(sourceContents, QLatin1String("file.cpp"));

    TestCase test(TestCase::SwitchBetweenMethodDeclarationDefinition, testFiles);
    test.run();
}

/// Check ...
void CppEditorPlugin::test_FollowSymbolUnderCursor_globalVarFromFunction()
{
    const QByteArray source =
        "int $j;\n"
        "int main()\n"
        "{\n"
        "    @j = 2;\n"
        "}\n"           // Line 5
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_funLocalVarHidesClassMember()
{
    // 3.3.10 Name hiding (par 3.), from text
    const QByteArray source =
        "struct C {\n"
        "    void f()\n"
        "    {\n"
        "        int $member; // hides C::member\n"
        "        ++@member;\n"                       // Line 5
        "    }\n"
        "    int member;\n"
        "};\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_funLocalVarHidesNamespaceMemberIntroducedByUsingDirective()
{
    // 3.3.10 Name hiding (par 4.), from text
    const QByteArray source =
        "namespace N {\n"
        "    int i;\n"
        "}\n"
        "\n"
        "int main()\n"                       // Line 5
        "{\n"
        "    using namespace N;\n"
        "    int $i;\n"
        "    ++i@; // refers to local i;\n"
        "}\n"                                // Line 10
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_loopLocalVarHidesOuterScopeVariable1()
{
    // 3.3.3 Block scope (par. 4), from text
    // Same for if, while, switch
    const QByteArray source =
        "int main()\n"
        "{\n"
        "    int i = 1;\n"
        "    for (int $i = 0; i < 10; ++i) { // 'i' refers to for's i\n"
        "        i = @i; // same\n"                                       // Line 5
        "    }\n"
        "}\n";
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_loopLocalVarHidesOuterScopeVariable2()
{
    // 3.3.3 Block scope (par. 4), from text
    // Same for if, while, switch
    const QByteArray source =
        "int main()\n"
        "{\n"
        "    int i = 1;\n"
        "    for (int $i = 0; @i < 10; ++i) { // 'i' refers to for's i\n"
        "        i = i; // same\n"                                         // Line 5
        "    }\n"
        "}\n";
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_subsequentDefinedClassMember()
{
    // 3.3.7 Class scope, part of the example
    const QByteArray source =
        "class X {\n"
        "    int f() { return @i; } // i refers to class's i\n"
        "    int $i;\n"
        "};\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_classMemberHidesOuterTypeDef()
{
    // 3.3.7 Class scope, part of the example
    // Variable name hides type name.
    const QByteArray source =
        "typedef int c;\n"
        "class X {\n"
        "    int f() { return @c; } // c refers to class' c\n"
        "    int $c; // hides typedef name\n"
        "};\n"                                                 // Line 5
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_globalVarFromEnum()
{
    // 3.3.2 Point of declaration (par. 1), copy-paste
    const QByteArray source =
        "const int $x = 12;\n"
        "int main()\n"
        "{\n"
        "    enum { x = @x }; // x refers to global x\n"
        "}\n"                                             // Line 5
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_selfInitialization()
{
    // 3.3.2 Point of declaration
    const QByteArray source =
        "int x = 12;\n"
        "int main()\n"
        "{\n"
        "   int $x = @x; // Second x refers to local x\n"
        "}\n"                                              // Line 5
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_pointerToClassInClassDefinition()
{
    // 3.3.2 Point of declaration (par. 3), from text
    const QByteArray source =
        "class $Foo {\n"
        "    @Foo *p; // Refers to above Foo\n"
        "};\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_previouslyDefinedMemberFromArrayDefinition()
{
    // 3.3.2 Point of declaration (par. 5), copy-paste
    const QByteArray source =
        "struct X {\n"
        "    enum E { $z = 16 };\n"
        "    int b[X::@z]; // z refers to defined z\n"
        "};\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_outerStaticMemberVariableFromInsideSubclass()
{
    // 3.3.7 Class scope (par. 2), from text
    const QByteArray source =
        "struct C\n"
        "{\n"
        "   struct I\n"
        "   {\n"
        "       void f()\n"                            // Line 5
        "       {\n"
        "           int i = @c; // refers to C's c\n"
        "       }\n"
        "   };\n"
        "\n"                                           // Line 10
        "   static int $c;\n"
        "};\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_memberVariableFollowingDotOperator()
{
    // 3.3.7 Class scope (par. 1), part of point 5
    const QByteArray source =
        "struct C\n"
        "{\n"
        "    int $member;\n"
        "};\n"
        "\n"                 // Line 5
        "int main()\n"
        "{\n"
        "    C c;\n"
        "    c.@member++;\n"
        "}\n"                // Line 10
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_memberVariableFollowingArrowOperator()
{
    // 3.3.7 Class scope (par. 1), part of point 5
    const QByteArray source =
        "struct C\n"
        "{\n"
        "    int $member;\n"
        "};\n"
        "\n"                    // Line 5
        "int main()\n"
        "{\n"
        "    C* c;\n"
        "    c->@member++;\n"
        "}\n"                   // Line 10
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_staticMemberVariableFollowingScopeOperator()
{
    // 3.3.7 Class scope (par. 1), part of point 5
    const QByteArray source =
        "struct C\n"
        "{\n"
        "    static int $member;\n"
        "};\n"
        "\n"                        // Line 5
        "int main()\n"
        "{\n"
        "    C::@member++;\n"
        "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_staticMemberVariableFollowingDotOperator()
{
    // 3.3.7 Class scope (par. 2), from text
    const QByteArray source =
        "struct C\n"
        "{\n"
        "    static int $member;\n"
        "};\n"
        "\n"                        // Line 5
        "int main()\n"
        "{\n"
        "    C c;\n"
        "    c.@member;\n"
        "}\n"                       // Line 10
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}


void CppEditorPlugin::test_FollowSymbolUnderCursor_staticMemberVariableFollowingArrowOperator()
{
    // 3.3.7 Class scope (par. 2), from text
    const QByteArray source =
        "struct C\n"
        "{\n"
        "    static int $member;\n"
        "};\n"
        "\n"                         // Line 5
        "int main()\n"
        "{\n"
        "    C *c;\n"
        "    c->@member++;\n"
        "}\n"                        // Line 10
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_previouslyDefinedEnumValueFromInsideEnum()
{
    // 3.3.8 Enumeration scope
    const QByteArray source =
        "enum {\n"
        "    $i = 0,\n"
        "    j = @i // refers to i above\n"
        "};\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_nsMemberHidesNsMemberIntroducedByUsingDirective()
{
    // 3.3.8 Enumeration scope
    const QByteArray source =
        "namespace A {\n"
        "  char x;\n"
        "}\n"
        "\n"
        "namespace B {\n"                               // Line 5
        "  using namespace A;\n"
        "  int $x; // hides A::x\n"
        "}\n"
        "\n"
        "int main()\n"                                  // Line 10
        "{\n"
        "    B::@x++; // refers to B's X, not A::x\n"
        "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_baseClassFunctionIntroducedByUsingDeclaration()
{
    // 3.3.10 Name hiding, from text
    // www.stroustrup.com/bs_faq2.html#overloadderived
    const QByteArray source =
        "struct B {\n"
        "    int $f(int) {}\n"
        "};\n"
        "\n"
        "class D : public B {\n"                              // Line 5
        "public:\n"
        "    using B::f; // make every f from B available\n"
        "    double f(double) {}\n"
        "};\n"
        "\n"                                                  // Line 10
        "int main()\n"
        "{\n"
        "    D* pd = new D;\n"
        "    pd->@f(2); // refers to B::f\n"
        "    pd->f(2.3); // refers to D::f\n"                 // Line 15
        "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_funWithSameNameAsBaseClassFunIntroducedByUsingDeclaration()
{
    // 3.3.10 Name hiding, from text
    // www.stroustrup.com/bs_faq2.html#overloadderived
    const QByteArray source =
        "struct B {\n"
        "    int f(int) {}\n"
        "};\n"
        "\n"
        "class D : public B {\n"                              // Line 5
        "public:\n"
        "    using B::f; // make every f from B available\n"
        "    double $f(double) {}\n"
        "};\n"
        "\n"                                                  // Line 10
        "int main()\n"
        "{\n"
        "    D* pd = new D;\n"
        "    pd->f(2); // refers to B::f\n"
        "    pd->@f(2.3); // refers to D::f\n"                // Line 15
        "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_funLocalVarHidesOuterClass()
{
    // 3.3.10 Name hiding (par 2.), from text
    // A class name (9.1) or enumeration name (7.2) can be hidden by the name of a variable, data member,
    // function, or enumerator declared in the same scope.
    const QByteArray source =
        "struct C {};\n"
        "\n"
        "int main()\n"
        "{\n"
        "    int $C; // hides type C\n"  // Line 5
        "    ++@C;\n"
        "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_using_QTCREATORBUG7903_globalNamespace()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "using NS::$Foo;\n"
            "void fun()\n"
            "{\n"
            "    @Foo foo;\n"
            "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_using_QTCREATORBUG7903_namespace()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "namespace NS1 {\n"
            "void fun()\n"
            "{\n"
            "    using NS::$Foo;\n"
            "    @Foo foo;\n"
            "}\n"
            "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

void CppEditorPlugin::test_FollowSymbolUnderCursor_using_QTCREATORBUG7903_insideFunction()
{
    const QByteArray source =
            "namespace NS {\n"
            "class Foo {};\n"
            "}\n"
            "void fun()\n"
            "{\n"
            "    using NS::$Foo;\n"
            "    @Foo foo;\n"
            "}\n"
        ;

    TestCase test(TestCase::FollowSymbolUnderCursor, source);
    test.run();
}

/*
Potential test cases improving name lookup.

If you fix one, add a test and remove the example from here.

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.1 Declarative regions and scopes, copy-paste (main added)
int j = 24;
int main()
{
    int i = j, j; // First j refers to global j, second j refers to just locally defined j
    j = 42; // Refers to locally defined j
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.2 Point of declaration (par. 2), copy-paste (main added)
const int i = 2;
int main()
{
    int i[i]; // Second i refers to global
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.2 Point of declaration (par. 9), copy-paste (main added)
typedef unsigned char T;
template<class T
= T // lookup finds the typedef name of unsigned char
, T // lookup finds the template parameter
N = 0> struct A { };

int main() {}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.9 Template parameter scope (par. 3), copy-paste (main added), part 1
template<class T, T* p, class U = T> class X {}; // second and third T refers to first one
template<class T> void f(T* p = new T);

int main() {}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.9 Template parameter scope (par. 3), copy-paste (main added), part 2
template<class T> class X : public Array<T> {};
template<class T> class Y : public T {};

int main() {}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.9 Template parameter scope (par. 4), copy-paste (main added), part 2
typedef int N;
template<N X, typename N, template<N Y> class T> struct A; // N refers to N above

int main() {}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.10 Name hiding (par 1.), from text, example 2a

// www.stroustrup.com/bs_faq2.html#overloadderived
// "In C++, there is no overloading across scopes - derived class scopes are not
// an exception to this general rule. (See D&E or TC++PL3 for details)."
struct B {
    int f(int) {}
};

struct D : public B {
    double f(double) {} // hides B::f
};

int main()
{
    D* pd = new D;
    pd->f(2); // refers to D::f
    pd->f(2.3); // refers to D::f
}

///////////////////////////////////////////////////////////////////////////////////////////////////

// 3.3.10 Name hiding (par 2.), from text
int C; // hides following type C, order is not important
struct C {};

int main()
{
    ++C;
}

*/
