// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfix_test.h"

#include "../cppcodestylepreferences.h"
#include "../cppeditorwidget.h"
#include "../cppmodelmanager.h"
#include "../cppsourceprocessertesthelper.h"
#include "../cpptoolssettings.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QtTest>

/*!
    Tests for quick-fixes.
 */
using namespace Core;
using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

using CppEditor::Tests::TemporaryDir;
using CppEditor::Tests::Internal::TestIncludePaths;

typedef QByteArray _;

namespace CppEditor {
namespace Internal {
namespace Tests {

QList<TestDocumentPtr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected)
{
    return {CppTestDocument::create("file.cpp", original, expected)};
}

BaseQuickFixTestCase::BaseQuickFixTestCase(const QList<TestDocumentPtr> &testDocuments,
                                           const ProjectExplorer::HeaderPaths &headerPaths,
                                           const QByteArray &clangFormatSettings)
    : m_testDocuments(testDocuments)
    , m_cppCodeStylePreferences(0)
    , m_restoreHeaderPaths(false)
{
    QVERIFY(succeededSoFar());
    m_succeededSoFar = false;

    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (document->hasCursorMarker())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write documents to disk
    m_temporaryDirectory.reset(new TemporaryDir);
    QVERIFY(m_temporaryDirectory->isValid());
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (QFileInfo(document->m_fileName).isRelative())
            document->setBaseDirectory(m_temporaryDirectory->path());
        document->writeToDisk();
    }

    // Create .clang-format file
    if (!clangFormatSettings.isEmpty())
        m_temporaryDirectory->createFile(".clang-format", clangFormatSettings);

    // Set appropriate include paths
    if (!headerPaths.isEmpty()) {
        m_restoreHeaderPaths = true;
        m_headerPathsToRestore = CppModelManager::headerPaths();
        CppModelManager::setHeaderPaths(headerPaths);
    }

    // Update Code Model
    QSet<FilePath> filePaths;
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments))
        filePaths << document->filePath();
    QVERIFY(parseFiles(filePaths));

    // Open Files
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        QVERIFY(openCppEditor(document->filePath(), &document->m_editor,
                              &document->m_editorWidget));
        closeEditorAtEndOfTestCase(document->m_editor);

        // Set cursor position
        if (document->hasCursorMarker()) {
            if (document->hasAnchorMarker()) {
                document->m_editor->setCursorPosition(document->m_anchorPosition);
                document->m_editor->select(document->m_cursorPosition);
            } else {
                document->m_editor->setCursorPosition(document->m_cursorPosition);
            }
        } else {
            document->m_editor->setCursorPosition(0);
        }

        // Rehighlight
        waitForRehighlightedSemanticDocument(document->m_editorWidget);
    }

    // Enforce the default cpp code style, so we are independent of config file settings.
    // This is needed by e.g. the GenerateGetterSetter quick fix.
    m_cppCodeStylePreferences = CppToolsSettings::cppCodeStyle();
    QVERIFY(m_cppCodeStylePreferences);
    m_cppCodeStylePreferencesOriginalDelegateId = m_cppCodeStylePreferences->currentDelegateId();
    m_cppCodeStylePreferences->setCurrentDelegate("qt");

    // Find the document having the cursor marker
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (document->hasCursorMarker()){
            m_documentWithMarker = document;
            break;
        }
    }

    QVERIFY(m_documentWithMarker);
    m_succeededSoFar = true;
}

BaseQuickFixTestCase::~BaseQuickFixTestCase()
{
    // Restore default cpp code style
    if (m_cppCodeStylePreferences)
        m_cppCodeStylePreferences->setCurrentDelegate(m_cppCodeStylePreferencesOriginalDelegateId);

    // Restore include paths
    if (m_restoreHeaderPaths)
        CppModelManager::setHeaderPaths(m_headerPathsToRestore);

    // Remove created files from file system
    for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments))
        QVERIFY(testDocument->filePath().removeFile());
}

QuickFixOfferedOperationsTest::QuickFixOfferedOperationsTest(
        const QList<TestDocumentPtr> &testDocuments,
        CppQuickFixFactory *factory,
        const ProjectExplorer::HeaderPaths &headerPaths,
        const QStringList &expectedOperations)
    : BaseQuickFixTestCase(testDocuments, headerPaths)
{
    // Get operations
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations actualOperations;
    factory->match(quickFixInterface, actualOperations);

    // Convert to QStringList
    QStringList actualOperationsAsStringList;
    for (const QuickFixOperation::Ptr &operation : std::as_const(actualOperations))
        actualOperationsAsStringList << operation->description();

    QCOMPARE(actualOperationsAsStringList, expectedOperations);
}

/// Leading whitespace is not removed, so we can check if the indetation ranges
/// have been set correctly by the quick-fix.
static QString &removeTrailingWhitespace(QString &input)
{
    const QStringList lines = input.split(QLatin1Char('\n'));
    input.resize(0);
    for (int i = 0, total = lines.size(); i < total; ++i) {
        QString line = lines.at(i);
        while (line.length() > 0) {
            QChar lastChar = line[line.length() - 1];
            if (lastChar == QLatin1Char(' ') || lastChar == QLatin1Char('\t'))
                line.chop(1);
            else
                break;
        }
        input.append(line);

        const bool isLastLine = i == lines.size() - 1;
        if (!isLastLine)
            input.append(QLatin1Char('\n'));
    }
    return input;
}

QuickFixOperationTest::QuickFixOperationTest(const QList<TestDocumentPtr> &testDocuments,
                                             CppQuickFixFactory *factory,
                                             const ProjectExplorer::HeaderPaths &headerPaths,
                                             int operationIndex,
                                             const QByteArray &expectedFailMessage,
                                             const QByteArray &clangFormatSettings)
    : BaseQuickFixTestCase(testDocuments, headerPaths, clangFormatSettings)
{
    if (factory->clangdReplacement() && CppModelManager::isClangCodeModelActive())
        return;

    QVERIFY(succeededSoFar());

    // Perform operation if there is one
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations operations;
    factory->match(quickFixInterface, operations);
    if (operations.isEmpty()) {
        QEXPECT_FAIL("CompleteSwitchCaseStatement_QTCREATORBUG-25998", "FIXME", Abort);
        QVERIFY(testDocuments.first()->m_expectedSource.isEmpty());
        return;
    }

    QVERIFY(operationIndex < operations.size());
    const QuickFixOperation::Ptr operation = operations.at(operationIndex);
    operation->perform();

    // Compare all files
    for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments)) {
        // Check
        QString result = testDocument->m_editorWidget->document()->toPlainText();
        removeTrailingWhitespace(result);
        QEXPECT_FAIL("escape string literal: raw string literal", "FIXME", Continue);
        QEXPECT_FAIL("escape string literal: unescape adjacent literals", "FIXME", Continue);
        if (!expectedFailMessage.isEmpty())
            QEXPECT_FAIL("", expectedFailMessage.data(), Continue);
        else if (result != testDocument->m_expectedSource) {
            qDebug() << "---" << testDocument->m_expectedSource;
            qDebug() << "+++" << result;
        }
        QCOMPARE(result, testDocument->m_expectedSource);

        // Undo the change
        for (int i = 0; i < 100; ++i)
            testDocument->m_editorWidget->undo();
        result = testDocument->m_editorWidget->document()->toPlainText();
        QCOMPARE(result, testDocument->m_source);
    }
}

void QuickFixOperationTest::run(const QList<TestDocumentPtr> &testDocuments,
                                CppQuickFixFactory *factory,
                                const QString &headerPath,
                                int operationIndex)
{
    ProjectExplorer::HeaderPaths headerPaths;
    headerPaths.push_back(ProjectExplorer::HeaderPath::makeUser(headerPath));
    QuickFixOperationTest(testDocuments, factory, headerPaths, operationIndex);
}

} // namespace Tests
} // namespace Internal

typedef QSharedPointer<CppQuickFixFactory> CppQuickFixFactoryPtr;

} // namespace CppEditor

namespace CppEditor::Internal::Tests {

void QuickfixTest::testGeneric_data()
{
    QTest::addColumn<CppQuickFixFactoryPtr>("factory");
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    // Checks: All enum values are added as case statements for a blank switch.
    QTest::newRow("CompleteSwitchCaseStatement_basic1")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case V1:\n"
        "        break;\n"
        "    case V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_basic1_enum class")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case EnumType::V1:\n"
        "        break;\n"
        "    case EnumType::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above with the cursor somewhere in the body.
    QTest::newRow("CompleteSwitchCaseStatement_basic1_enum class, cursor in the body")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    @}\n"
        "}\n"
        ) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case EnumType::V1:\n"
        "        break;\n"
        "    case EnumType::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: All enum values are added as case statements for a blank switch when
    //         the variable is declared alongside the enum definition.
    QTest::newRow("CompleteSwitchCaseStatement_basic1_enum_with_declaration")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum EnumType { V1, V2 } t;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum EnumType { V1, V2 } t;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    switch (t) {\n"
        "    case V1:\n"
        "        break;\n"
        "    case V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_basic1_enum_with_declaration_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class EnumType { V1, V2 } t;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class EnumType { V1, V2 } t;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    switch (t) {\n"
        "    case EnumType::V1:\n"
        "        break;\n"
        "    case EnumType::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: All enum values are added as case statements for a blank switch
    //         for anonymous enums.
    QTest::newRow("CompleteSwitchCaseStatement_basic1_anonymous_enum")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum { V1, V2 } t;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum { V1, V2 } t;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    switch (t) {\n"
        "    case V1:\n"
        "        break;\n"
        "    case V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: All enum values are added as case statements for a blank switch with a default case.
    QTest::newRow("CompleteSwitchCaseStatement_basic2")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    @switch (t) {\n"
        "    default:\n"
        "    break;\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case V1:\n"
        "        break;\n"
        "    case V2:\n"
        "        break;\n"
        "    default:\n"
        "    break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_basic2_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    @switch (t) {\n"
        "    default:\n"
        "    break;\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case EnumType::V1:\n"
        "        break;\n"
        "    case EnumType::V2:\n"
        "        break;\n"
        "    default:\n"
        "    break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Enum type in class is found.
    QTest::newRow("CompleteSwitchCaseStatement_enumTypeInClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "struct C { enum EnumType { V1, V2 }; };\n"
        "\n"
        "void f(C::EnumType t) {\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "struct C { enum EnumType { V1, V2 }; };\n"
        "\n"
        "void f(C::EnumType t) {\n"
        "    switch (t) {\n"
        "    case C::V1:\n"
        "        break;\n"
        "    case C::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_enumClassInClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "struct C { enum class EnumType { V1, V2 }; };\n"
        "\n"
        "void f(C::EnumType t) {\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "struct C { enum class EnumType { V1, V2 }; };\n"
        "\n"
        "void f(C::EnumType t) {\n"
        "    switch (t) {\n"
        "    case C::EnumType::V1:\n"
        "        break;\n"
        "    case C::EnumType::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Enum type in namespace is found.
    QTest::newRow("CompleteSwitchCaseStatement_enumTypeInNamespace")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "namespace N { enum EnumType { V1, V2 }; };\n"
        "\n"
        "void f(N::EnumType t) {\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "namespace N { enum EnumType { V1, V2 }; };\n"
        "\n"
        "void f(N::EnumType t) {\n"
        "    switch (t) {\n"
        "    case N::V1:\n"
        "        break;\n"
        "    case N::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_enumClassInNamespace")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "namespace N { enum class EnumType { V1, V2 }; };\n"
        "\n"
        "void f(N::EnumType t) {\n"
        "    @switch (t) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "namespace N { enum class EnumType { V1, V2 }; };\n"
        "\n"
        "void f(N::EnumType t) {\n"
        "    switch (t) {\n"
        "    case N::EnumType::V1:\n"
        "        break;\n"
        "    case N::EnumType::V2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: The missing enum value is added.
    QTest::newRow("CompleteSwitchCaseStatement_oneValueMissing")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    @switch (t) {\n"
        "    case V2:\n"
        "        break;\n"
        "    default:\n"
        "        break;\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case V1:\n"
        "        break;\n"
        "    case V2:\n"
        "        break;\n"
        "    default:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_oneValueMissing_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    @switch (t) {\n"
        "    case EnumType::V2:\n"
        "        break;\n"
        "    default:\n"
        "        break;\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class EnumType { V1, V2 };\n"
        "\n"
        "void f()\n"
        "{\n"
        "    EnumType t;\n"
        "    switch (t) {\n"
        "    case EnumType::V1:\n"
        "        break;\n"
        "    case EnumType::V2:\n"
        "        break;\n"
        "    default:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Find the correct enum type despite there being a declaration with the same name.
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG10366_1")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum test { TEST_1, TEST_2 };\n"
        "\n"
        "void f() {\n"
        "    enum test test;\n"
        "    @switch (test) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum test { TEST_1, TEST_2 };\n"
        "\n"
        "void f() {\n"
        "    enum test test;\n"
        "    switch (test) {\n"
        "    case TEST_1:\n"
        "        break;\n"
        "    case TEST_2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG10366_1_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class test { TEST_1, TEST_2 };\n"
        "\n"
        "void f() {\n"
        "    enum test test;\n"
        "    @switch (test) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class test { TEST_1, TEST_2 };\n"
        "\n"
        "void f() {\n"
        "    enum test test;\n"
        "    switch (test) {\n"
        "    case test::TEST_1:\n"
        "        break;\n"
        "    case test::TEST_2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Find the correct enum type despite there being a declaration with the same name.
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG10366_2")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum test1 { Wrong11, Wrong12 };\n"
        "enum test { Right1, Right2 };\n"
        "enum test2 { Wrong21, Wrong22 };\n"
        "\n"
        "int main() {\n"
        "    enum test test;\n"
        "    @switch (test) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum test1 { Wrong11, Wrong12 };\n"
        "enum test { Right1, Right2 };\n"
        "enum test2 { Wrong21, Wrong22 };\n"
        "\n"
        "int main() {\n"
        "    enum test test;\n"
        "    switch (test) {\n"
        "    case Right1:\n"
        "        break;\n"
        "    case Right2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG10366_2_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class test1 { Wrong11, Wrong12 };\n"
        "enum class test { Right1, Right2 };\n"
        "enum class test2 { Wrong21, Wrong22 };\n"
        "\n"
        "int main() {\n"
        "    enum test test;\n"
        "    @switch (test) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class test1 { Wrong11, Wrong12 };\n"
        "enum class test { Right1, Right2 };\n"
        "enum class test2 { Wrong21, Wrong22 };\n"
        "\n"
        "int main() {\n"
        "    enum test test;\n"
        "    switch (test) {\n"
        "    case test::Right1:\n"
        "        break;\n"
        "    case test::Right2:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Do not crash on incomplete case statetement.
    QTest::newRow("CompleteSwitchCaseStatement_doNotCrashOnIncompleteCase")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum E {};\n"
        "void f(E o)\n"
        "{\n"
        "    @switch (o)\n"
        "    {\n"
        "    case\n"
        "    }\n"
        "}\n"
        ) << _(
        ""
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_doNotCrashOnIncompleteCase_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class E {};\n"
        "void f(E o)\n"
        "{\n"
        "    @switch (o)\n"
        "    {\n"
        "    case\n"
        "    }\n"
        "}\n"
        ) << _(
        ""
    );

    // Checks: complete switch statement where enum is goes via a template type parameter
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG-24752")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum E {EA, EB};\n"
        "template<typename T> struct S {\n"
        "    static T theType() { return T(); }\n"
        "};\n"
        "int main() {\n"
        "    @switch (S<E>::theType()) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum E {EA, EB};\n"
        "template<typename T> struct S {\n"
        "    static T theType() { return T(); }\n"
        "};\n"
        "int main() {\n"
        "    switch (S<E>::theType()) {\n"
        "    case EA:\n"
        "        break;\n"
        "    case EB:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Same as above for enum class.
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG-24752_enumClass")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "enum class E {A, B};\n"
        "template<typename T> struct S {\n"
        "    static T theType() { return T(); }\n"
        "};\n"
        "int main() {\n"
        "    @switch (S<E>::theType()) {\n"
        "    }\n"
        "}\n"
        ) << _(
        "enum class E {A, B};\n"
        "template<typename T> struct S {\n"
        "    static T theType() { return T(); }\n"
        "};\n"
        "int main() {\n"
        "    switch (S<E>::theType()) {\n"
        "    case E::A:\n"
        "        break;\n"
        "    case E::B:\n"
        "        break;\n"
        "    }\n"
        "}\n"
    );

    // Checks: Complete switch statement where enum is return type of a template function
    //         which is outside the scope of the return value.
    // TODO: Type minimization.
    QTest::newRow("CompleteSwitchCaseStatement_QTCREATORBUG-25998")
        << CppQuickFixFactoryPtr(new CompleteSwitchCaseStatement) << _(
        "template <typename T> T enumCast(int value) { return static_cast<T>(value); }\n"
        "class Test {\n"
        "    enum class E { V1, V2 };"
        "    void func(int i) {\n"
        "        @switch (enumCast<E>(i)) {\n"
        "        }\n"
        "    }\n"
        "};\n"
        ) << _(
        "template <typename T> T enumCast(int value) { return static_cast<T>(value); }\n"
        "class Test {\n"
        "    enum class E { V1, V2 };"
        "    void func(int i) {\n"
        "        switch (enumCast<E>(i)) {\n"
        "        case Test::E::V1:\n"
        "            break;\n"
        "        case Test::E::V2:\n"
        "            break;\n"
        "        }\n"
        "    }\n"
        "};\n"
    );

    // Check: Just a basic test since the main functionality is tested in
    // cpppointerdeclarationformatter_test.cpp
    QTest::newRow("ReformatPointerDeclaration")
        << CppQuickFixFactoryPtr(new ReformatPointerDeclaration)
        << _("char@*s;")
        << _("char *s;");

    QTest::newRow("convert to camel case: normal")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @lower_case_function();\n")
        << _("void lowerCaseFunction();\n");
    QTest::newRow("convert to camel case: already camel case")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @camelCaseFunction();\n")
        << _();
    QTest::newRow("convert to camel case: no underscores (lower case)")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @lowercasefunction();\n")
        << _();
    QTest::newRow("convert to camel case: no underscores (upper case)")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @UPPERCASEFUNCTION();\n")
        << _();
    QTest::newRow("convert to camel case: non-applicable underscore")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @m_a_member;\n")
        << _("void m_aMember;\n");
    QTest::newRow("convert to camel case: upper case")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @UPPER_CASE_FUNCTION();\n")
        << _("void upperCaseFunction();\n");
    QTest::newRow("convert to camel case: partially camel case already")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void mixed@_andCamelCase();\n")
        << _("void mixedAndCamelCase();\n");
    QTest::newRow("convert to camel case: wild mix")
        << CppQuickFixFactoryPtr(new ConvertToCamelCase(true))
        << _("void @WhAt_TODO_hErE();\n")
        << _("void WhAtTODOHErE();\n");
}

void QuickfixTest::testGeneric()
{
    QFETCH(CppQuickFixFactoryPtr, factory);
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    QuickFixOperationTest(singleDocument(original, expected), factory.data());
}

class CppCodeStyleSettingsChanger {
public:
    CppCodeStyleSettingsChanger(const CppCodeStyleSettings &settings);
    ~CppCodeStyleSettingsChanger(); // Restore original

    static CppCodeStyleSettings currentSettings();

private:
    void setSettings(const CppCodeStyleSettings &settings);

    CppCodeStyleSettings m_originalSettings;
};

CppCodeStyleSettingsChanger::CppCodeStyleSettingsChanger(const CppCodeStyleSettings &settings)
{
    m_originalSettings = currentSettings();
    setSettings(settings);
}

CppCodeStyleSettingsChanger::~CppCodeStyleSettingsChanger()
{
    setSettings(m_originalSettings);
}

void CppCodeStyleSettingsChanger::setSettings(const CppCodeStyleSettings &settings)
{
    QVariant variant;
    variant.setValue(settings);

    CppToolsSettings::cppCodeStyle()->currentDelegate()->setValue(variant);
}

CppCodeStyleSettings CppCodeStyleSettingsChanger::currentSettings()
{
    return CppToolsSettings::cppCodeStyle()->currentDelegate()->value().value<CppCodeStyleSettings>();
}

void QuickfixTest::testConvertToMetaMethodInvocation_data()
{
    QTest::addColumn<QByteArray>("input");
    QTest::addColumn<QByteArray>("expected");

    // ^ marks the cursor locations.
    // $ marks the replacement regions.
    // The quoted string in the comment is the data tag.
    // The rest of the comment is the replacement string.
    const QByteArray allCases = R"(
class C {
public:
    C() {
        $this->^aSignal()$; // "signal from region on pointer to object" QMetaObject::invokeMethod(this, "aSignal")
        C c;
        $c.^aSignal()$; // "signal from region on object value" QMetaObject::invokeMethod(&c, "aSignal")
        $(new C)->^aSignal()$; // "signal from region on expression" QMetaObject::invokeMethod((new C), "aSignal")
        $emit this->^aSignal()$; // "signal from region, with emit" QMetaObject::invokeMethod(this, "aSignal")
        $Q_EMIT this->^aSignal()$; // "signal from region, with Q_EMIT" QMetaObject::invokeMethod(this, "aSignal")
        $this->^aSlot()$; // "slot from region" QMetaObject::invokeMethod(this, "aSlot")
        $this->^noArgs()$; // "Q_SIGNAL, no arguments" QMetaObject::invokeMethod(this, "noArgs")
        $this->^oneArg(0)$; // "Q_SLOT, one argument" QMetaObject::invokeMethod(this, "oneArg", Q_ARG(int, 0))
        $this->^twoArgs(0, c)$; // "Q_INVOKABLE, two arguments" QMetaObject::invokeMethod(this, "twoArgs", Q_ARG(int, 0), Q_ARG(C, c))
        this->^notInvokable(); // "not invokable"
    }

signals:
    void aSignal();

private slots:
    void aSlot();

private:
    Q_SIGNAL void noArgs();
    Q_SLOT void oneArg(int index);
    Q_INVOKABLE void twoArgs(int index, const C &value);
    void notInvokable();
};
)";

    qsizetype nextCursor = allCases.indexOf('^');
    while (nextCursor != -1) {
        const int commentStart = allCases.indexOf("//", nextCursor);
        QVERIFY(commentStart != -1);
        const int tagStart = allCases.indexOf('"', commentStart + 2);
        QVERIFY(tagStart != -1);
        const int tagEnd = allCases.indexOf('"', tagStart + 1);
        QVERIFY(tagEnd != -1);
        QByteArray input = allCases;
        QByteArray output = allCases;
        input.replace(nextCursor, 1, "@");
        const QByteArray tag = allCases.mid(tagStart + 1, tagEnd - tagStart - 1);
        const int prevNewline = allCases.lastIndexOf('\n', nextCursor);
        const int regionStart = allCases.lastIndexOf('$', nextCursor);
        bool hasReplacement = false;
        if (regionStart != -1 && regionStart > prevNewline) {
            const int regionEnd = allCases.indexOf('$', regionStart + 1);
            QVERIFY(regionEnd != -1);
            const int nextNewline = allCases.indexOf('\n', tagEnd);
            QVERIFY(nextNewline != -1);
            const QByteArray replacement
                = allCases.mid(tagEnd + 1, nextNewline - tagEnd - 1).trimmed();
            output.replace(regionStart, regionEnd - regionStart, replacement);
            hasReplacement = true;
        }
        static const auto matcher = [](char c) { return c == '^' || c == '$'; };
        input.removeIf(matcher);
        if (hasReplacement) {
            output.removeIf(matcher);
            output.prepend("#include <QMetaObject>\n\n");
        } else {
            output.clear();
        }
        QTest::newRow(tag.data()) << input << output;
        nextCursor = allCases.indexOf('^', nextCursor + 1);
    }
}

void QuickfixTest::testConvertToMetaMethodInvocation()
{
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, expected);

    ConvertToMetaMethodCall factory;
    QuickFixOperationTest({CppTestDocument::create("file.cpp", input, expected)}, &factory);
}

} // namespace CppEditor::Internal::Tests
