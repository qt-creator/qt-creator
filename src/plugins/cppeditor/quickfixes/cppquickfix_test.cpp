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

namespace CppEditor::Internal::Tests {
typedef QList<TestDocumentPtr> QuickFixTestDocuments;
}
Q_DECLARE_METATYPE(CppEditor::Internal::Tests::QuickFixTestDocuments)

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

/// Delegates directly to AddIncludeForUndefinedIdentifierOp for easier testing.
class AddIncludeForUndefinedIdentifierTestFactory : public CppQuickFixFactory
{
public:
    AddIncludeForUndefinedIdentifierTestFactory(const QString &include)
        : m_include(include) {}

    void doMatch(const CppQuickFixInterface &cppQuickFixInterface, QuickFixOperations &result) override
    {
        result << new AddIncludeForUndefinedIdentifierOp(cppQuickFixInterface, 0, m_include);
    }

private:
    const QString m_include;
};

class AddForwardDeclForUndefinedIdentifierTestFactory : public CppQuickFixFactory
{
public:
    AddForwardDeclForUndefinedIdentifierTestFactory(const QString &className, int symbolPos)
        : m_className(className), m_symbolPos(symbolPos) {}

    void doMatch(const CppQuickFixInterface &cppQuickFixInterface, QuickFixOperations &result) override
    {
        result << new AddForwardDeclForUndefinedIdentifierOp(cppQuickFixInterface, 0,
                                                             m_className, m_symbolPos);
    }

private:
    const QString m_className;
    const int m_symbolPos;
};

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

    QTest::newRow("MoveDeclarationOutOfIf_ifOnly")
        << CppQuickFixFactoryPtr(new MoveDeclarationOutOfIf) << _(
        "void f()\n"
        "{\n"
        "    if (Foo *@foo = g())\n"
        "        h();\n"
        "}\n"
        ) << _(
        "void f()\n"
        "{\n"
        "    Foo *foo = g();\n"
        "    if (foo)\n"
        "        h();\n"
        "}\n"
    );

    QTest::newRow("MoveDeclarationOutOfIf_ifElse")
        << CppQuickFixFactoryPtr(new MoveDeclarationOutOfIf) << _(
        "void f()\n"
        "{\n"
        "    if (Foo *@foo = g())\n"
        "        h();\n"
        "    else\n"
        "        i();\n"
        "}\n"
        ) << _(
        "void f()\n"
        "{\n"
        "    Foo *foo = g();\n"
        "    if (foo)\n"
        "        h();\n"
        "    else\n"
        "        i();\n"
        "}\n"
    );

    QTest::newRow("MoveDeclarationOutOfIf_ifElseIf")
        << CppQuickFixFactoryPtr(new MoveDeclarationOutOfIf) << _(
        "void f()\n"
        "{\n"
        "    if (Foo *foo = g()) {\n"
        "        if (Bar *@bar = x()) {\n"
        "            h();\n"
        "            j();\n"
        "        }\n"
        "    } else {\n"
        "        i();\n"
        "    }\n"
        "}\n"
        ) << _(
        "void f()\n"
        "{\n"
        "    if (Foo *foo = g()) {\n"
        "        Bar *bar = x();\n"
        "        if (bar) {\n"
        "            h();\n"
        "            j();\n"
        "        }\n"
        "    } else {\n"
        "        i();\n"
        "    }\n"
        "}\n"
    );

    QTest::newRow("MoveDeclarationOutOfWhile_singleWhile")
        << CppQuickFixFactoryPtr(new MoveDeclarationOutOfWhile) << _(
        "void f()\n"
        "{\n"
        "    while (Foo *@foo = g())\n"
        "        j();\n"
        "}\n"
        ) << _(
        "void f()\n"
        "{\n"
        "    Foo *foo;\n"
        "    while ((foo = g()) != 0)\n"
        "        j();\n"
        "}\n"
    );

    QTest::newRow("MoveDeclarationOutOfWhile_whileInWhile")
        << CppQuickFixFactoryPtr(new MoveDeclarationOutOfWhile) << _(
        "void f()\n"
        "{\n"
        "    while (Foo *foo = g()) {\n"
        "        while (Bar *@bar = h()) {\n"
        "            i();\n"
        "            j();\n"
        "        }\n"
        "    }\n"
        "}\n"
        ) << _(
        "void f()\n"
        "{\n"
        "    while (Foo *foo = g()) {\n"
        "        Bar *bar;\n"
        "        while ((bar = h()) != 0) {\n"
        "            i();\n"
        "            j();\n"
        "        }\n"
        "    }\n"
        "}\n"
    );

    // Check: Just a basic test since the main functionality is tested in
    // cpppointerdeclarationformatter_test.cpp
    QTest::newRow("ReformatPointerDeclaration")
        << CppQuickFixFactoryPtr(new ReformatPointerDeclaration)
        << _("char@*s;")
        << _("char *s;");

    // Check: Add local variable for a free function.
    QTest::newRow("AssignToLocalVariable_freeFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "int foo() {return 1;}\n"
        "void bar() {fo@o();}\n"
        ) << _(
        "int foo() {return 1;}\n"
        "void bar() {auto localFoo = foo();}\n"
    );

    // Check: Add local variable for a member function.
    QTest::newRow("AssignToLocalVariable_memberFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: int* fooFunc();}\n"
        "void bar() {\n"
        "    Foo *f = new Foo;\n"
        "    @f->fooFunc();\n"
        "}\n"
        ) << _(
        "class Foo {public: int* fooFunc();}\n"
        "void bar() {\n"
        "    Foo *f = new Foo;\n"
        "    auto localFooFunc = f->fooFunc();\n"
        "}\n"
    );

    // Check: Add local variable for a member function, cursor in the middle (QTCREATORBUG-10355)
    QTest::newRow("AssignToLocalVariable_memberFunction2ndGrade1")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "struct Foo {int* func();};\n"
        "struct Baz {Foo* foo();};\n"
        "void bar() {\n"
        "    Baz *b = new Baz;\n"
        "    b->foo@()->func();\n"
        "}"
        ) << _(
        "struct Foo {int* func();};\n"
        "struct Baz {Foo* foo();};\n"
        "void bar() {\n"
        "    Baz *b = new Baz;\n"
        "    auto localFunc = b->foo()->func();\n"
        "}"
    );

    // Check: Add local variable for a member function, cursor on function call (QTCREATORBUG-10355)
    QTest::newRow("AssignToLocalVariable_memberFunction2ndGrade2")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "struct Foo {int* func();};\n"
        "struct Baz {Foo* foo();};\n"
        "void bar() {\n"
        "    Baz *b = new Baz;\n"
        "    b->foo()->f@unc();\n"
        "}"
        ) << _(
        "struct Foo {int* func();};\n"
        "struct Baz {Foo* foo();};\n"
        "void bar() {\n"
        "    Baz *b = new Baz;\n"
        "    auto localFunc = b->foo()->func();\n"
        "}"
    );

    // Check: Add local variable for a static member function.
    QTest::newRow("AssignToLocalVariable_staticMemberFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: static int* fooFunc();}\n"
        "void bar() {\n"
        "    Foo::fooF@unc();\n"
        "}"
        ) << _(
        "class Foo {public: static int* fooFunc();}\n"
        "void bar() {\n"
        "    auto localFooFunc = Foo::fooFunc();\n"
        "}"
    );

    // Check: Add local variable for a new Expression.
    QTest::newRow("AssignToLocalVariable_newExpression")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {}\n"
        "void bar() {\n"
        "    new Fo@o;\n"
        "}"
        ) << _(
        "class Foo {}\n"
        "void bar() {\n"
        "    auto localFoo = new Foo;\n"
        "}"
    );

    // Check: No trigger for function inside member initialization list.
    QTest::newRow("AssignToLocalVariable_noInitializationList")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo\n"
        "{\n"
        "    public: Foo : m_i(fooF@unc()) {}\n"
        "    int fooFunc() {return 2;}\n"
        "    int m_i;\n"
        "};\n"
        ) << _();

    // Check: No trigger for void functions.
    QTest::newRow("AssignToLocalVariable_noVoidFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "void foo() {}\n"
        "void bar() {fo@o();}"
        ) << _();

    // Check: No trigger for void member functions.
    QTest::newRow("AssignToLocalVariable_noVoidMemberFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: void fooFunc();}\n"
        "void bar() {\n"
        "    Foo *f = new Foo;\n"
        "    @f->fooFunc();\n"
        "}"
        ) << _();

    // Check: No trigger for void static member functions.
    QTest::newRow("AssignToLocalVariable_noVoidStaticMemberFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: static void fooFunc();}\n"
        "void bar() {\n"
        "    Foo::fo@oFunc();\n"
        "}"
        ) << _();

    // Check: No trigger for functions in expressions.
    QTest::newRow("AssignToLocalVariable_noFunctionInExpression")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "int foo(int a) {return a;}\n"
        "int bar() {return 1;}"
        "void baz() {foo(@bar() + bar());}"
        ) << _();

    // Check: No trigger for functions in functions. (QTCREATORBUG-9510)
    QTest::newRow("AssignToLocalVariable_noFunctionInFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "int foo(int a, int b) {return a + b;}\n"
        "int bar(int a) {return a;}\n"
        "void baz() {\n"
        "    int a = foo(ba@r(), bar());\n"
        "}\n"
        ) << _();

    // Check: No trigger for functions in return statements (classes).
    QTest::newRow("AssignToLocalVariable_noReturnClass1")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: static void fooFunc();}\n"
        "Foo* bar() {\n"
        "    return new Fo@o;\n"
        "}"
        ) << _();

    // Check: No trigger for functions in return statements (classes). (QTCREATORBUG-9525)
    QTest::newRow("AssignToLocalVariable_noReturnClass2")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: int fooFunc();}\n"
        "int bar() {\n"
        "    return (new Fo@o)->fooFunc();\n"
        "}"
        ) << _();

    // Check: No trigger for functions in return statements (functions).
    QTest::newRow("AssignToLocalVariable_noReturnFunc1")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "class Foo {public: int fooFunc();}\n"
        "int bar() {\n"
        "    return Foo::fooFu@nc();\n"
        "}"
        ) << _();

    // Check: No trigger for functions in return statements (functions). (QTCREATORBUG-9525)
    QTest::newRow("AssignToLocalVariable_noReturnFunc2")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "int bar() {\n"
        "    return list.firs@t().foo;\n"
        "}\n"
        ) << _();

    // Check: No trigger for functions which does not match in signature.
    QTest::newRow("AssignToLocalVariable_noSignatureMatch")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "int someFunc(int);\n"
        "\n"
        "void f()\n"
        "{\n"
        "    some@Func();\n"
        "}"
        ) << _();

    QTest::newRow("ExtractLiteralAsParameter_freeFunction")
        << CppQuickFixFactoryPtr(new ExtractLiteralAsParameter) << _(
        "void foo(const char *a, long b = 1)\n"
        "{return 1@56 + 123 + 156;}\n"
        ) << _(
        "void foo(const char *a, long b = 1, int newParameter = 156)\n"
        "{return newParameter + 123 + newParameter;}\n"
    );

    QTest::newRow("ExtractLiteralAsParameter_memberFunction")
        << CppQuickFixFactoryPtr(new ExtractLiteralAsParameter) << _(
        "class Narf {\n"
        "public:\n"
        "    int zort();\n"
        "};\n\n"
        "int Narf::zort()\n"
        "{ return 15@5 + 1; }\n"
        ) << _(
        "class Narf {\n"
        "public:\n"
        "    int zort(int newParameter = 155);\n"
        "};\n\n"
        "int Narf::zort(int newParameter)\n"
        "{ return newParameter + 1; }\n"
    );

    QTest::newRow("ExtractLiteralAsParameter_memberFunctionInline")
        << CppQuickFixFactoryPtr(new ExtractLiteralAsParameter) << _(
        "class Narf {\n"
        "public:\n"
        "    int zort()\n"
        "    { return 15@5 + 1; }\n"
        "};\n"
        ) << _(
        "class Narf {\n"
        "public:\n"
        "    int zort(int newParameter = 155)\n"
        "    { return newParameter + 1; }\n"
        "};\n"
    );

    // Check: optimize postcrement
    QTest::newRow("OptimizeForLoop_postcrement")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {f@or (int i = 0; i < 3; i++) {}}\n")
        << _("void foo() {for (int i = 0; i < 3; ++i) {}}\n");

    // Check: optimize condition
    QTest::newRow("OptimizeForLoop_condition")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {f@or (int i = 0; i < 3 + 5; ++i) {}}\n")
        << _("void foo() {for (int i = 0, total = 3 + 5; i < total; ++i) {}}\n");

    // Check: optimize fliped condition
    QTest::newRow("OptimizeForLoop_flipedCondition")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {f@or (int i = 0; 3 + 5 > i; ++i) {}}\n")
        << _("void foo() {for (int i = 0, total = 3 + 5; total > i; ++i) {}}\n");

    // Check: if "total" used, create other name.
    QTest::newRow("OptimizeForLoop_alterVariableName")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {f@or (int i = 0, total = 0; i < 3 + 5; ++i) {}}\n")
        << _("void foo() {for (int i = 0, total = 0, totalX = 3 + 5; i < totalX; ++i) {}}\n");

    // Check: optimize postcrement and condition
    QTest::newRow("OptimizeForLoop_optimizeBoth")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {f@or (int i = 0; i < 3 + 5; i++) {}}\n")
        << _("void foo() {for (int i = 0, total = 3 + 5; i < total; ++i) {}}\n");

    // Check: empty initializier
    QTest::newRow("OptimizeForLoop_emptyInitializer")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("int i; void foo() {f@or (; i < 3 + 5; ++i) {}}\n")
        << _("int i; void foo() {for (int total = 3 + 5; i < total; ++i) {}}\n");

    // Check: wrong initializier type -> no trigger
    QTest::newRow("OptimizeForLoop_wrongInitializer")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("int i; void foo() {f@or (double a = 0; i < 3 + 5; ++i) {}}\n")
        << _();

    // Check: No trigger when numeric
    QTest::newRow("OptimizeForLoop_noTriggerNumeric1")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {fo@r (int i = 0; i < 3; ++i) {}}\n")
        << _();

    // Check: No trigger when numeric
    QTest::newRow("OptimizeForLoop_noTriggerNumeric2")
        << CppQuickFixFactoryPtr(new OptimizeForLoop)
        << _("void foo() {fo@r (int i = 0; i < -3; ++i) {}}\n")
        << _();

    // Escape String Literal as UTF-8 (no-trigger)
    QTest::newRow("EscapeStringLiteral_notrigger")
            << CppQuickFixFactoryPtr(new EscapeStringLiteral)
            << _("const char *notrigger = \"@abcdef \\a\\n\\\\\";\n")
            << _();

    // Escape String Literal as UTF-8
    QTest::newRow("EscapeStringLiteral")
            << CppQuickFixFactoryPtr(new EscapeStringLiteral)
            << _("const char *utf8 = \"@\xe3\x81\x82\xe3\x81\x84\";\n")
            << _("const char *utf8 = \"\\xe3\\x81\\x82\\xe3\\x81\\x84\";\n");

    // Unescape String Literal as UTF-8 (from hexdecimal escape sequences)
    QTest::newRow("UnescapeStringLiteral_hex")
            << CppQuickFixFactoryPtr(new EscapeStringLiteral)
            << _("const char *hex_escaped = \"@\\xe3\\x81\\x82\\xe3\\x81\\x84\";\n")
            << _("const char *hex_escaped = \"\xe3\x81\x82\xe3\x81\x84\";\n");

    // Unescape String Literal as UTF-8 (from octal escape sequences)
    QTest::newRow("UnescapeStringLiteral_oct")
            << CppQuickFixFactoryPtr(new EscapeStringLiteral)
            << _("const char *oct_escaped = \"@\\343\\201\\202\\343\\201\\204\";\n")
            << _("const char *oct_escaped = \"\xe3\x81\x82\xe3\x81\x84\";\n");

    // Unescape String Literal as UTF-8 (triggered but no change)
    QTest::newRow("UnescapeStringLiteral_noconv")
            << CppQuickFixFactoryPtr(new EscapeStringLiteral)
            << _("const char *escaped_ascii = \"@\\x1b\";\n")
            << _("const char *escaped_ascii = \"\\x1b\";\n");

    // Unescape String Literal as UTF-8 (no conversion because of invalid utf-8)
    QTest::newRow("UnescapeStringLiteral_invalid")
            << CppQuickFixFactoryPtr(new EscapeStringLiteral)
            << _("const char *escaped = \"@\\xe3\\x81\";\n")
            << _("const char *escaped = \"\\xe3\\x81\";\n");

    QTest::newRow("ConvertFromPointer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString *@str;\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "    f1(*str);\n"
             "    f2(str);\n"
             "}\n")
        << _("void foo() {\n"
             "    QString str;\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "    f1(str);\n"
             "    f2(&str);\n"
             "}\n");

    QTest::newRow("ConvertToPointer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString @str;\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "    f1(str);\n"
             "    f2(&str);\n"
             "}\n")
        << _("void foo() {\n"
             "    QString *str = new QString;\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "    f1(*str);\n"
             "    f2(str);\n"
             "}\n");

    QTest::newRow("ConvertReferenceToPointer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString narf;"
             "    QString &@str = narf;\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "    f1(str);\n"
             "    f2(&str);\n"
             "}\n")
        << _("void foo() {\n"
             "    QString narf;"
             "    QString *str = &narf;\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "    f1(*str);\n"
             "    f2(str);\n"
             "}\n");

    QTest::newRow("ConvertFromPointer_withInitializer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString *@str = new QString(QLatin1String(\"schnurz\"));\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "}\n")
        << _("void foo() {\n"
             "    QString str(QLatin1String(\"schnurz\"));\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "}\n");

    QTest::newRow("ConvertFromPointer_withBareInitializer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString *@str = new QString;\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "}\n")
        << _("void foo() {\n"
             "    QString str;\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "}\n");

    QTest::newRow("ConvertFromPointer_withEmptyInitializer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString *@str = new QString();\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "}\n")
        << _("void foo() {\n"
             "    QString str;\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "}\n");

    QTest::newRow("ConvertFromPointer_structWithPointer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("struct Bar{ QString *str; };\n"
             "void foo() {\n"
             "    Bar *@bar = new Bar;\n"
             "    bar->str = new QString;\n"
             "    delete bar->str;\n"
             "    delete bar;\n"
             "}\n")
        << _("struct Bar{ QString *str; };\n"
             "void foo() {\n"
             "    Bar bar;\n"
             "    bar.str = new QString;\n"
             "    delete bar.str;\n"
             "    // delete bar;\n"
             "}\n");

    QTest::newRow("ConvertToPointer_withInitializer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString @str = QLatin1String(\"narf\");\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "}\n")
        << _("void foo() {\n"
             "    QString *str = new QString(QLatin1String(\"narf\"));\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "}\n");

    QTest::newRow("ConvertToPointer_withParenInitializer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString @str(QLatin1String(\"narf\"));\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "}\n")
        << _("void foo() {\n"
             "    QString *str = new QString(QLatin1String(\"narf\"));\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "}\n");

    QTest::newRow("ConvertToPointer_noTriggerRValueRefs")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo(Narf &&@narf) {}\n")
        << _();

    QTest::newRow("ConvertToPointer_noTriggerGlobal")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("int @global;\n")
        << _();

    QTest::newRow("ConvertToPointer_noTriggerClassMember")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("struct C { int @member; };\n")
        << _();

    QTest::newRow("ConvertToPointer_noTriggerClassMember2")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void f() { struct C { int @member; }; }\n")
        << _();

    QTest::newRow("ConvertToPointer_functionOfFunctionLocalClass")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void f() {\n"
             "    struct C {\n"
             "        void g() { int @member; }\n"
             "    };\n"
             "}\n")
        << _("void f() {\n"
             "    struct C {\n"
             "        void g() { int *member; }\n"
             "    };\n"
             "}\n");

    QTest::newRow("ConvertToPointer_redeclaredVariable_block")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    QString @str;\n"
             "    str.clear();\n"
             "    {\n"
             "        QString str;\n"
             "        str.clear();\n"
             "    }\n"
             "    f1(str);\n"
             "}\n")
        << _("void foo() {\n"
             "    QString *str = new QString;\n"
             "    str->clear();\n"
             "    {\n"
             "        QString str;\n"
             "        str.clear();\n"
             "    }\n"
             "    f1(*str);\n"
             "}\n");

    QTest::newRow("ConvertAutoFromPointer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    auto @str = new QString(QLatin1String(\"foo\"));\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "    f1(*str);\n"
             "    f2(str);\n"
             "}\n")
        << _("void foo() {\n"
             "    auto str = QString(QLatin1String(\"foo\"));\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "    f1(str);\n"
             "    f2(&str);\n"
             "}\n");

    QTest::newRow("ConvertAutoFromPointer2")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    auto *@str = new QString;\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "    f1(*str);\n"
             "    f2(str);\n"
             "}\n")
        << _("void foo() {\n"
             "    auto str = QString();\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "    f1(str);\n"
             "    f2(&str);\n"
             "}\n");

    QTest::newRow("ConvertAutoToPointer")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("void foo() {\n"
             "    auto @str = QString(QLatin1String(\"foo\"));\n"
             "    if (!str.isEmpty())\n"
             "        str.clear();\n"
             "    f1(str);\n"
             "    f2(&str);\n"
             "}\n")
        << _("void foo() {\n"
             "    auto @str = new QString(QLatin1String(\"foo\"));\n"
             "    if (!str->isEmpty())\n"
             "        str->clear();\n"
             "    f1(*str);\n"
             "    f2(str);\n"
             "}\n");

    QTest::newRow("ConvertToPointerWithMacro")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _("#define BAR bar\n"
             "void func()\n"
             "{\n"
             "    int @foo = 42;\n"
             "    int bar;\n"
             "    BAR = foo;\n"
             "}\n")
        << _("#define BAR bar\n"
             "void func()\n"
             "{\n"
             "    int *foo = 42;\n"
             "    int bar;\n"
             "    BAR = *foo;\n"
             "}\n");

    QString testObjAndFunc = "struct Object\n"
                             "{\n"
                             "    Object(%1){}\n"
                             "};\n"
                             "void func()\n"
                             "{\n"
                             "    %2\n"
                             "}\n";

    QTest::newRow("ConvertToStack1_QTCREATORBUG23181")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _(testObjAndFunc.arg("int").arg("Object *@obj = new Object(0);").toUtf8())
        << _(testObjAndFunc.arg("int").arg("Object obj(0);").toUtf8());

    QTest::newRow("ConvertToStack2_QTCREATORBUG23181")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _(testObjAndFunc.arg("int").arg("Object *@obj = new Object{0};").toUtf8())
        << _(testObjAndFunc.arg("int").arg("Object obj{0};").toUtf8());

    QTest::newRow("ConvertToPointer1_QTCREATORBUG23181")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _(testObjAndFunc.arg("").arg("Object @obj;").toUtf8())
        << _(testObjAndFunc.arg("").arg("Object *obj = new Object;").toUtf8());

    QTest::newRow("ConvertToPointer2_QTCREATORBUG23181")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _(testObjAndFunc.arg("").arg("Object @obj();").toUtf8())
        << _(testObjAndFunc.arg("").arg("Object *obj = new Object();").toUtf8());

    QTest::newRow("ConvertToPointer3_QTCREATORBUG23181")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _(testObjAndFunc.arg("").arg("Object @obj{};").toUtf8())
        << _(testObjAndFunc.arg("").arg("Object *obj = new Object{};").toUtf8());

    QTest::newRow("ConvertToPointer4_QTCREATORBUG23181")
        << CppQuickFixFactoryPtr(new ConvertFromAndToPointer)
        << _(testObjAndFunc.arg("int").arg("Object @obj(0);").toUtf8())
        << _(testObjAndFunc.arg("int").arg("Object *obj = new Object(0);").toUtf8());

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
    QTest::newRow("escape string literal: simple case")
        << CppQuickFixFactoryPtr(new EscapeStringLiteral)
        << _(R"(const char *str = @"àxyz";)")
        << _(R"(const char *str = "\xc3\xa0xyz";)");
    QTest::newRow("escape string literal: simple case reverse")
        << CppQuickFixFactoryPtr(new EscapeStringLiteral)
        << _(R"(const char *str = @"\xc3\xa0xyz";)")
        << _(R"(const char *str = "àxyz";)");
    QTest::newRow("escape string literal: raw string literal")
        << CppQuickFixFactoryPtr(new EscapeStringLiteral)
        << _(R"x(const char *str = @R"(àxyz)";)x")
        << _(R"x(const char *str = R"(\xc3\xa0xyz)";)x");
    QTest::newRow("escape string literal: splitting required")
        << CppQuickFixFactoryPtr(new EscapeStringLiteral)
        << _(R"(const char *str = @"àf23бgб1";)")
        << _(R"(const char *str = "\xc3\xa0""f23\xd0\xb1g\xd0\xb1""1";)");
    QTest::newRow("escape string literal: unescape adjacent literals")
        << CppQuickFixFactoryPtr(new EscapeStringLiteral)
        << _(R"(const char *str = @"\xc3\xa0""f23\xd0\xb1g\xd0\xb1""1";)")
        << _(R"(const char *str = "àf23бgб1";)");
    QTest::newRow("AddLocalDeclaration_QTCREATORBUG-26004")
        << CppQuickFixFactoryPtr(new AddDeclarationForUndeclaredIdentifier)
        << _("void func() {\n"
             "    QStringList list;\n"
             "    @it = list.cbegin();\n"
             "}\n")
        << _("void func() {\n"
             "    QStringList list;\n"
             "    auto it = list.cbegin();\n"
             "}\n");
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

void QuickfixTest::testInsertMemberFromUse_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    QByteArray original;
    QByteArray expected;

    original =
            "class C {\n"
            "public:\n"
            "    C(int x) : @m_x(x) {}\n"
            "private:\n"
            "    int m_y;\n"
            "};\n";
    expected =
            "class C {\n"
            "public:\n"
            "    C(int x) : m_x(x) {}\n"
            "private:\n"
            "    int m_y;\n"
            "    int m_x;\n"
            "};\n";
    QTest::addRow("inline constructor") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    C(int x, double d);\n"
        "private:\n"
        "    int m_x;\n"
        "};\n"
        "C::C(int x, double d) : m_x(x), @m_d(d)\n";
    expected =
        "class C {\n"
        "public:\n"
        "    C(int x, double d);\n"
        "private:\n"
        "    int m_x;\n"
        "    double m_d;\n"
        "};\n"
        "C::C(int x, double d) : m_x(x), m_d(d)\n";
    QTest::addRow("out-of-line constructor") << original << expected;

    original =
            "class C {\n"
            "public:\n"
            "    C(int x) : @m_x(x) {}\n"
            "private:\n"
            "    int m_x;\n"
            "};\n";
    expected = "";
    QTest::addRow("member already present") << original << expected;

    original =
            "int func() { return 0; }\n"
            "class C {\n"
            "public:\n"
            "    C() : @m_x(func()) {}\n"
            "private:\n"
            "    int m_y;\n"
            "};\n";
    expected =
            "int func() { return 0; }\n"
            "class C {\n"
            "public:\n"
            "    C() : m_x(func()) {}\n"
            "private:\n"
            "    int m_y;\n"
            "    int m_x;\n"
            "};\n";
    QTest::addRow("initialization via function call") << original << expected;

    original =
        "struct S {\n\n};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.@value = v; }\n"
        "private:\n"
        "    S m_s;\n"
        "};\n";
    expected =
        "struct S {\n\n"
        "    int value;\n"
        "};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.value = v; }\n"
        "private:\n"
        "    S m_s;\n"
        "};\n";
    QTest::addRow("add member to other struct") << original << expected;

    original =
        "struct S {\n\n};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { S::@value = v; }\n"
        "};\n";
    expected =
        "struct S {\n\n"
        "    static int value;\n"
        "};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { S::value = v; }\n"
        "};\n";
    QTest::addRow("add static member to other struct (explicit)") << original << expected;

    original =
        "struct S {\n\n};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.@value = v; }\n"
        "private:\n"
        "    static S m_s;\n"
        "};\n";
    expected =
        "struct S {\n\n"
        "    static int value;\n"
        "};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.value = v; }\n"
        "private:\n"
        "    static S m_s;\n"
        "};\n";
    QTest::addRow("add static member to other struct (implicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    void setValue(int v);\n"
        "};\n"
        "void C::setValue(int v) { this->@m_value = v; }\n";
    expected =
        "class C {\n"
        "public:\n"
        "    void setValue(int v);\n"
        "private:\n"
        "    int m_value;\n"
        "};\n"
        "void C::setValue(int v) { this->@m_value = v; }\n";
    QTest::addRow("add member to this (explicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { @m_value = v; }\n"
        "};\n";
    expected =
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_value = v; }\n"
        "private:\n"
        "    int m_value;\n"
        "};\n";
    QTest::addRow("add member to this (implicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    static void setValue(int v) { @m_value = v; }\n"
        "};\n";
    expected =
        "class C {\n"
        "public:\n"
        "    static void setValue(int v) { m_value = v; }\n"
        "private:\n"
        "    static int m_value;\n"
        "};\n";
    QTest::addRow("add static member to this (inline)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    static void setValue(int v);\n"
        "};\n"
        "void C::setValue(int v) { @m_value = v; }\n";
    expected =
        "class C {\n"
        "public:\n"
        "    static void setValue(int v);\n"
        "private:\n"
        "    static int m_value;\n"
        "};\n"
        "void C::setValue(int v) { @m_value = v; }\n";
    QTest::addRow("add static member to this (non-inline)") << original << expected;

    original =
        "struct S {\n\n};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.@setValue(v); }\n"
        "private:\n"
        "    S m_s;\n"
        "};\n";
    expected =
        "struct S {\n\n"
        "    void setValue(int);\n"
        "};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.setValue(v); }\n"
        "private:\n"
        "    S m_s;\n"
        "};\n";
    QTest::addRow("add member function to other struct") << original << expected;

    original =
        "struct S {\n\n};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { S::@setValue(v); }\n"
        "};\n";
    expected =
        "struct S {\n\n"
        "    static void setValue(int);\n"
        "};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { S::setValue(v); }\n"
        "};\n";
    QTest::addRow("add static member function to other struct (explicit)") << original << expected;

    original =
        "struct S {\n\n};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.@setValue(v); }\n"
        "private:\n"
        "    static S m_s;\n"
        "};\n";
    expected =
        "struct S {\n\n"
        "    static void setValue(int);\n"
        "};\n"
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { m_s.setValue(v); }\n"
        "private:\n"
        "    static S m_s;\n"
        "};\n";
    QTest::addRow("add static member function to other struct (implicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    void setValue(int v);\n"
        "};\n"
        "void C::setValue(int v) { this->@setValueInternal(v); }\n";
    expected =
        "class C {\n"
        "public:\n"
        "    void setValue(int v);\n"
        "private:\n"
        "    void setValueInternal(int);\n"
        "};\n"
        "void C::setValue(int v) { this->setValueInternal(v); }\n";
    QTest::addRow("add member function to this (explicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { @setValueInternal(v); }\n"
        "};\n";
    expected =
        "class C {\n"
        "public:\n"
        "    void setValue(int v) { setValueInternal(v); }\n"
        "private:\n"
        "    void setValueInternal(int);\n"
        "};\n";
    QTest::addRow("add member function to this (implicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    int value() const { return @valueInternal(); }\n"
        "};\n";
    expected =
        "class C {\n"
        "public:\n"
        "    int value() const { return valueInternal(); }\n"
        "private:\n"
        "    int valueInternal() const;\n"
        "};\n";
    QTest::addRow("add const member function to this (implicit)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    static int value() { int i = @valueInternal(); return i; }\n"
        "};\n";
    expected =
        "class C {\n"
        "public:\n"
        "    static int value() { int i = @valueInternal(); return i; }\n"
        "private:\n"
        "    static int valueInternal();\n"
        "};\n";
    QTest::addRow("add static member function to this (inline)") << original << expected;

    original =
        "class C {\n"
        "public:\n"
        "    static int value();\n"
        "};\n"
        "int C::value() { return @valueInternal(); }\n";
    expected =
        "class C {\n"
        "public:\n"
        "    static int value();\n"
        "private:\n"
        "    static int valueInternal();\n"
        "};\n"
        "int C::value() { return valueInternal(); }\n";
    QTest::addRow("add static member function to this (non-inline)") << original << expected;
}

void QuickfixTest::testInsertMemberFromUse()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    QList<TestDocumentPtr> testDocuments({
        CppTestDocument::create("file.h", original, expected)
    });

    AddDeclarationForUndeclaredIdentifier factory;
    factory.setMembersOnly();
    QuickFixOperationTest(testDocuments, &factory);
}

// Function for one of InsertDeclDef section cases
void insertToSectionDeclFromDef(const QByteArray &section, int sectionIndex)
{
    QList<TestDocumentPtr> testDocuments;

    QByteArray original;
    QByteArray expected;
    QByteArray sectionString = section + ":\n";
    if (sectionIndex == 4)
        sectionString.clear();

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        + sectionString +
        "    Foo();\n"
        "@};\n";
    testDocuments << CppTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo@()\n"
        "{\n"
        "}\n"
        ;
    expected = original;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);

    InsertDeclFromDef factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectExplorer::HeaderPaths(), sectionIndex);
}

/// Check from source file: Insert in header file.
void QuickfixTest::testInsertDeclFromDef()
{
    insertToSectionDeclFromDef("public", 0);
    insertToSectionDeclFromDef("public slots", 1);
    insertToSectionDeclFromDef("protected", 2);
    insertToSectionDeclFromDef("protected slots", 3);
    insertToSectionDeclFromDef("private", 4);
    insertToSectionDeclFromDef("private slots", 5);
}

void QuickfixTest::testInsertDeclFromDefTemplateFuncTypename()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "};\n"
        "\n"
        "template<class T>\n"
        "void Foo::fu@nc() {}\n";

    QByteArray expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    template<class T>\n"
        "    void func();\n"
        "};\n"
        "\n"
        "template<class T>\n"
        "void Foo::fu@nc() {}\n";

    InsertDeclFromDef factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory, {}, 0);
}

void QuickfixTest::testInsertDeclFromDefTemplateFuncInt()
{
    QByteArray original =
            "class Foo\n"
            "{\n"
            "};\n"
            "\n"
            "template<int N>\n"
            "void Foo::fu@nc() {}\n";

    QByteArray expected =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    template<int N>\n"
            "    void func();\n"
            "};\n"
            "\n"
            "template<int N>\n"
            "void Foo::fu@nc() {}\n";

    InsertDeclFromDef factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory, {}, 0);
}

void QuickfixTest::testInsertDeclFromDefTemplateReturnType()
{
    QByteArray original =
            "class Foo\n"
            "{\n"
            "};\n"
            "\n"
            "std::vector<int> Foo::fu@nc() const {}\n";

    QByteArray expected =
            "class Foo\n"
            "{\n"
            "public:\n"
            "    std::vector<int> func() const;\n"
            "};\n"
            "\n"
            "std::vector<int> Foo::func() const {}\n";

    InsertDeclFromDef factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory, {}, 0);
}

void QuickfixTest::testInsertDeclFromDefNotTriggeredForTemplateFunc()
{
    QByteArray contents =
        "class Foo\n"
        "{\n"
        "    template<class T>\n"
        "    void func();\n"
        "};\n"
        "\n"
        "template<class T>\n"
        "void Foo::fu@nc() {}\n";

    InsertDeclFromDef factory;
    QuickFixOperationTest(singleDocument(contents, ""), &factory);
}

void QuickfixTest::testAddIncludeForUndefinedIdentifier_data()
{
    QTest::addColumn<QString>("headerPath");
    QTest::addColumn<QuickFixTestDocuments>("testDocuments");
    QTest::addColumn<int>("refactoringOperationIndex");
    QTest::addColumn<QString>("includeForTestFactory");

    const int firstRefactoringOperation = 0;
    const int secondRefactoringOperation = 1;

    QList<TestDocumentPtr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "class Foo {};\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

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
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onSimpleName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { class Foo {}; }\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    N::Fo@o foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    N::Foo foo;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onNameOfQualifiedName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { class Foo {}; }\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @N::Foo foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    N::Foo foo;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onBaseOfQualifiedName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "class Foo { static void bar() {} };\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @Foo::bar();\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo::bar();\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onBaseOfQualifiedClassName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "template <typename T> class Foo { static void bar() {} };\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @Foo<int>::bar();\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo<int>::bar();\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onBaseOfQualifiedTemplateClassName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { template <typename T> class Foo {}; }\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @N::Foo<Bar> foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    N::Foo<Bar> foo;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onTemplateName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { template <typename T> class Foo {}; }\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    N::Bar<@Foo> foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    N::Bar<Foo> foo;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onTemplateNameInsideArguments")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "class Foo {};\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "class Foo;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @Foo foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "class Foo;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("withForwardDeclaration")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "template<class T> class Foo {};\n";
    expected = original;
    testDocuments << CppTestDocument::create("afile.h", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "template<class T> class Foo;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @Foo foo;\n"
        "}\n"
        ;
    expected =
        "#include \"afile.h\"\n"
        "#include \"header.h\"\n"
        "\n"
        "template<class T> class Foo;\n"
        "\n"
        "void f()\n"
        "{\n"
        "    Foo foo;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("withForwardDeclaration2")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "template<class T> class QMyClass {};\n";
    expected = original;
    testDocuments << CppTestDocument::create("qmyclass.h", original, expected);

    // Forward Header File
    original = "#include \"qmyclass.h\"\n";
    expected = original;
    testDocuments << CppTestDocument::create("QMyClass", original, expected);

    // Source File
    original =
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    @QMyClass c;\n"
        "}\n"
        ;
    expected =
        "#include \"QMyClass\"\n"
        "#include \"header.h\"\n"
        "\n"
        "void f()\n"
        "{\n"
        "    QMyClass c;\n"
        "}\n"
        ;
    testDocuments << CppTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("withForwardHeader")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << secondRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "void @f();\n"
        "#include \"file.moc\";\n"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "void f();\n"
        "#include \"file.moc\";\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("insertingIgnoreMoc")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"y.h\"\n"
        "#include \"z.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"file.h\"\n"
        "#include \"y.h\"\n"
        "#include \"z.h\"\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("insertingSortingTop")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"a.h\"\n"
        "#include \"z.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"file.h\"\n"
        "#include \"z.h\"\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("insertingSortingMiddle")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"a.h\"\n"
        "#include \"b.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"b.h\"\n"
        "#include \"file.h\"\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("insertingSortingBottom")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"b.h\"\n"
        "#include \"a.h\"\n"
        "\n@"
        ;
    expected =
        "#include \"b.h\"\n"
        "#include \"a.h\"\n"
        "#include \"file.h\"\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_appendToUnsorted")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_firstLocalIncludeAtFront")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("firstGlobalIncludeAtBack")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<file.h>";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_preferGroupWithLongerMatchingPrefix")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"prefixc.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_newGroupIfOnlyDifferentIncludeDirs")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_mixedDirsSorted")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<firstlib/file.h>";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_mixedDirsUnsorted")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<lastlib/file.h>";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"a.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_mixedIncludeTypes1")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"z.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"a.h\"\n"
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_mixedIncludeTypes2")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"a.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"z.h\"\n"
        "#include \"lib/file.h\"\n"
        "#include <global.h>\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_mixedIncludeTypes3")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"lib/file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "\n@"
        ;
    expected =
        "#include \"z.h\"\n"
        "#include <global.h>\n"
        "#include <lib/file.h>\n"
        "\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_mixedIncludeTypes4")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "<lib/file.h>";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "void @f();\n"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "void f();\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_noinclude")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "#ifndef FOO_H\n"
        "#define FOO_H\n"
        "void @f();\n"
        "#endif\n"
        ;
    expected =
        "#ifndef FOO_H\n"
        "#define FOO_H\n"
        "\n"
        "#include \"file.h\"\n"
        "\n"
        "void f();\n"
        "#endif\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_onlyIncludeGuard")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_veryFirstIncludeCppStyleCommentOnTop")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

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
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_veryFirstIncludeCStyleCommentOnTop")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "\"file.h\"";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    original =
        "@QDir dir;\n"
        ;
    expected =
        "#include <QDir>\n"
        "\n"
        "QDir dir;\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_checkQSomethingInQtIncludePaths")
            << TestIncludePaths::globalQtCoreIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    original =
        "std::s@tring s;\n"
        ;
    expected =
        "#include <string>\n"
        "\n"
        "std::string s;\n"
        ;
    testDocuments << CppTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_std::string")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    original = "class A{};";
    testDocuments << CppTestDocument::create("a.h", original, original);
    original = "class B{};";
    testDocuments << CppTestDocument::create("b.h", original, original);
    original =
            "#include \"b.h\"\n"
            "@A a;\n"
            "B b;";
    expected =
            "#include \"b.h\"\n\n"
            "#include \"a.h\"\n"
            "A a;\n"
            "B b;";
    testDocuments << CppTestDocument::create("b.cpp", original, expected);
    QTest::newRow("preserve first header")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    original = "class C{};";
    testDocuments << CppTestDocument::create("c.h", original, original);
    original = "class B{};";
    testDocuments << CppTestDocument::create("b.h", original, original);
    original =
            "#include \"c.h\"\n"
            "C c;\n"
            "@B b;";
    expected =
            "#include \"b.h\"\n"
            "#include \"c.h\"\n"
            "C c;\n"
            "B b;";
    testDocuments << CppTestDocument::create("x.cpp", original, expected);
    QTest::newRow("do not preserve first header")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();
}

void QuickfixTest::testAddIncludeForUndefinedIdentifier()
{
    QFETCH(QString, headerPath);
    QFETCH(QuickFixTestDocuments, testDocuments);
    QFETCH(int, refactoringOperationIndex);
    QFETCH(QString, includeForTestFactory);

    TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    for (const TestDocumentPtr &testDocument : std::as_const(testDocuments))
        testDocument->setBaseDirectory(temporaryDir.path());

    QScopedPointer<CppQuickFixFactory> factory;
    if (includeForTestFactory.isEmpty())
        factory.reset(new AddIncludeForUndefinedIdentifier);
    else
        factory.reset(new AddIncludeForUndefinedIdentifierTestFactory(includeForTestFactory));

    QuickFixOperationTest::run(testDocuments, factory.data(), headerPath,
                               refactoringOperationIndex);
}

void QuickfixTest::testAddIncludeForUndefinedIdentifierNoDoubleQtHeaderInclude()
{
    TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QList<TestDocumentPtr> testDocuments;
    QByteArray original;
    QByteArray expected;

    const QByteArray base = temporaryDir.path().toUtf8();

    // This file makes the QDir definition available so that locator finds it.
    original = expected = "#include <QDir>\n"
                          "void avoidBeingRecognizedAsForwardingHeader();";
    testDocuments << CppTestDocument::create(base + "/fileUsingQDir.cpp", original, expected);

    original = expected = "@QDir dir;\n";
    testDocuments << CppTestDocument::create(base + "/fileWantsToUseQDir.cpp", original, expected);

    AddIncludeForUndefinedIdentifier factory;
    const QStringList expectedOperations = QStringList("Add #include <QDir>");
    QuickFixOfferedOperationsTest(testDocuments, &factory, ProjectExplorer::toUserHeaderPaths(
            QStringList{TestIncludePaths::globalQtCoreIncludePath()}), expectedOperations);
}

void QuickfixTest::testAddForwardDeclForUndefinedIdentifier_data()
{
    QTest::addColumn<QuickFixTestDocuments>("testDocuments");
    QTest::addColumn<QString>("symbol");
    QTest::addColumn<int>("symbolPos");

    QByteArray original;
    QByteArray expected;

    original =
        "#pragma once\n"
        "\n"
        "void f(const Blu@bb &b)\n"
        "{\n"
        "}\n"
        ;
    expected =
        "#pragma once\n"
        "\n"
        "\n"
        "class Blubb;\n"
        "void f(const Blubb &b)\n"
        "{\n"
        "}\n"
        ;
    QTest::newRow("unqualified symbol")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "Blubb" << original.indexOf('@');

    original =
        "#pragma once\n"
        "\n"
        "namespace NS {\n"
        "class C;\n"
        "}\n"
        "void f(const NS::Blu@bb &b)\n"
        "{\n"
        "}\n"
        ;
    expected =
        "#pragma once\n"
        "\n"
        "namespace NS {\n"
        "\n"
        "class Blubb;\n"
        "class C;\n"
        "}\n"
        "void f(const NS::Blubb &b)\n"
        "{\n"
        "}\n"
        ;
    QTest::newRow("qualified symbol, full namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS::Blubb" << original.indexOf('@');

    original =
        "#pragma once\n"
        "\n"
        "namespace NS {\n"
        "class C;\n"
        "}\n"
        "void f(const NS::NS2::Blu@bb &b)\n"
        "{\n"
        "}\n"
        ;
    expected =
        "#pragma once\n"
        "\n"
        "namespace NS {\n"
        "\n"
        "namespace NS2 { class Blubb; }\n"
        "class C;\n"
        "}\n"
        "void f(const NS::NS2::Blubb &b)\n"
        "{\n"
        "}\n"
        ;
    QTest::newRow("qualified symbol, partial namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS::NS2::Blubb" << original.indexOf('@');

    original =
        "#pragma once\n"
        "\n"
        "namespace NS {\n"
        "class C;\n"
        "}\n"
        "void f(const NS2::Blu@bb &b)\n"
        "{\n"
        "}\n"
        ;
    expected =
        "#pragma once\n"
        "\n"
        "\n"
        "namespace NS2 { class Blubb; }\n"
        "namespace NS {\n"
        "class C;\n"
        "}\n"
        "void f(const NS2::Blubb &b)\n"
        "{\n"
        "}\n"
        ;
    QTest::newRow("qualified symbol, other namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS2::Blubb" << original.indexOf('@');

    original =
        "#pragma once\n"
        "\n"
        "void f(const NS2::Blu@bb &b)\n"
        "{\n"
        "}\n"
        ;
    expected =
        "#pragma once\n"
        "\n"
        "\n"
        "namespace NS2 { class Blubb; }\n"
        "void f(const NS2::Blubb &b)\n"
        "{\n"
        "}\n"
        ;
    QTest::newRow("qualified symbol, no namespace present")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS2::Blubb" << original.indexOf('@');

    original =
        "#pragma once\n"
        "\n"
        "void f(const NS2::Blu@bb &b)\n"
        "{\n"
        "}\n"
        "namespace NS2 {}\n"
        ;
    expected =
        "#pragma once\n"
        "\n"
        "\n"
        "namespace NS2 { class Blubb; }\n"
        "void f(const NS2::Blubb &b)\n"
        "{\n"
        "}\n"
        "namespace NS2 {}\n"
        ;
    QTest::newRow("qualified symbol, existing namespace after symbol")
            << QuickFixTestDocuments{CppTestDocument::create("theheader.h", original, expected)}
            << "NS2::Blubb" << original.indexOf('@');
}

void QuickfixTest::testAddForwardDeclForUndefinedIdentifier()
{
    QFETCH(QuickFixTestDocuments, testDocuments);
    QFETCH(QString, symbol);
    QFETCH(int, symbolPos);

    TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    testDocuments.first()->setBaseDirectory(temporaryDir.path());

    QScopedPointer<CppQuickFixFactory> factory(
                new AddForwardDeclForUndefinedIdentifierTestFactory(symbol, symbolPos));
    QuickFixOperationTest::run({testDocuments}, factory.data(), ".", 0);
}

void QuickfixTest::testAssignToLocalVariableTemplates()
{

    QList<TestDocumentPtr> testDocuments;
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
    expected = original;
    testDocuments << CppTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "void foo() {\n"
        "    List<int> list;\n"
        "    li@st.first();\n"
        "}\n";
    expected =
        "#include \"file.h\"\n"
        "void foo() {\n"
        "    List<int> list;\n"
        "    auto localFirst = list.first();\n"
        "}\n";
    testDocuments << CppTestDocument::create("file.cpp", original, expected);

    AssignToLocalVariable factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void QuickfixTest::testExtractFunction_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("basic")
        << _("// Documentation for f\n"
             "void f()\n"
             "{\n"
             "    @{start}g();@{end}\n"
             "}\n")
        << _("inline void extracted()\n"
             "{\n"
             "    g();\n"
             "}\n"
             "\n"
             "// Documentation for f\n"
             "void f()\n"
             "{\n"
             "    extracted();\n"
             "}\n");

    QTest::newRow("class function")
        << _("class Foo\n"
             "{\n"
             "private:\n"
             "    void bar();\n"
             "};\n\n"
             "void Foo::bar()\n"
             "{\n"
             "    @{start}g();@{end}\n"
             "}\n")
        << _("class Foo\n"
             "{\n"
             "public:\n"
             "    void extracted();\n\n"
             "private:\n"
             "    void bar();\n"
             "};\n\n"
             "inline void Foo::extracted()\n"
             "{\n"
             "    g();\n"
             "}\n\n"
             "void Foo::bar()\n"
             "{\n"
             "    extracted();\n"
             "}\n");

    QTest::newRow("class in namespace")
        << _("namespace NS {\n"
             "class C {\n"
             "    void f(C &c);\n"
             "};\n"
             "}\n"
             "void NS::C::f(NS::C &c)\n"
             "{\n"
             "    @{start}C *c2 = &c;@{end}\n"
             "}\n")
        << _("namespace NS {\n"
             "class C {\n"
             "    void f(C &c);\n"
             "\n"
             "public:\n"
             "    void extracted(NS::C &c);\n" // TODO: Remove non-required qualification
             "};\n"
             "}\n"
             "inline void NS::C::extracted(NS::C &c)\n"
             "{\n"
             "    C *c2 = &c;\n"
             "}\n"
             "\n"
             "void NS::C::f(NS::C &c)\n"
             "{\n"
             "    extracted(c);\n"
             "}\n");

    QTest::newRow("if-block")
            << _("inline void func()\n"
                 "{\n"
                 "    int dummy = 0;\n"
                 "    @{start}if@{end} (dummy < 10) {\n"
                 "        ++dummy;\n"
                 "    }\n"
                 "}\n")
            << _("inline void extracted(int dummy)\n"
                 "{\n"
                 "    if (dummy < 10) {\n"
                 "        ++dummy;\n"
                 "    }\n"
                 "}\n\n"
                 "inline void func()\n"
                 "{\n"
                 "    int dummy = 0;\n"
                 "    extracted(dummy);\n"
                 "}\n");
}

void QuickfixTest::testExtractFunction()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    QList<TestDocumentPtr> testDocuments;
    testDocuments << CppTestDocument::create("file.h", original, expected);

    ExtractFunction factory([]() { return QLatin1String("extracted"); });
    QuickFixOperationTest(testDocuments, &factory);
}

void QuickfixTest::testExtractLiteralAsParameterTypeDeduction_data()
{
    QTest::addColumn<QByteArray>("typeString");
    QTest::addColumn<QByteArray>("literal");
    QTest::newRow("int")
            << QByteArray("int ") << QByteArray("156");
    QTest::newRow("unsigned int")
            << QByteArray("unsigned int ") << QByteArray("156u");
    QTest::newRow("long")
            << QByteArray("long ") << QByteArray("156l");
    QTest::newRow("unsigned long")
            << QByteArray("unsigned long ") << QByteArray("156ul");
    QTest::newRow("long long")
            << QByteArray("long long ") << QByteArray("156ll");
    QTest::newRow("unsigned long long")
            << QByteArray("unsigned long long ") << QByteArray("156ull");
    QTest::newRow("float")
            << QByteArray("float ") << QByteArray("3.14159f");
    QTest::newRow("double")
            << QByteArray("double ") << QByteArray("3.14159");
    QTest::newRow("long double")
            << QByteArray("long double ") << QByteArray("3.14159L");
    QTest::newRow("bool")
            << QByteArray("bool ") << QByteArray("true");
    QTest::newRow("bool")
            << QByteArray("bool ") << QByteArray("false");
    QTest::newRow("char")
            << QByteArray("char ") << QByteArray("'X'");
    QTest::newRow("wchar_t")
            << QByteArray("wchar_t ") << QByteArray("L'X'");
    QTest::newRow("char16_t")
            << QByteArray("char16_t ") << QByteArray("u'X'");
    QTest::newRow("char32_t")
            << QByteArray("char32_t ") << QByteArray("U'X'");
    QTest::newRow("const char *")
            << QByteArray("const char *") << QByteArray("\"narf\"");
    QTest::newRow("const wchar_t *")
            << QByteArray("const wchar_t *") << QByteArray("L\"narf\"");
    QTest::newRow("const char16_t *")
            << QByteArray("const char16_t *") << QByteArray("u\"narf\"");
    QTest::newRow("const char32_t *")
            << QByteArray("const char32_t *") << QByteArray("U\"narf\"");
}

void QuickfixTest::testExtractLiteralAsParameterTypeDeduction()
{
    QFETCH(QByteArray, typeString);
    QFETCH(QByteArray, literal);
    const QByteArray original = QByteArray("void foo() {return @") + literal + QByteArray(";}\n");
    const QByteArray expected = QByteArray("void foo(") + typeString + QByteArray("newParameter = ")
            + literal + QByteArray(") {return newParameter;}\n");

    if (literal == "3.14159") {
        qWarning("Literal 3.14159 is wrongly reported as int. Skipping.");
        return;
    } else if (literal == "3.14159L") {
        qWarning("Literal 3.14159L is wrongly reported as long. Skipping.");
        return;
    }

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void QuickfixTest::testExtractLiteralAsParameterFreeFunctionSeparateFiles()
{
    QList<TestDocumentPtr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "void foo(const char *a, long b = 1);\n";
    expected =
        "void foo(const char *a, long b = 1, int newParameter = 156);\n";
    testDocuments << CppTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "void foo(const char *a, long b)\n"
        "{return 1@56 + 123 + 156;}\n";
    expected =
        "void foo(const char *a, long b, int newParameter)\n"
        "{return newParameter + 123 + newParameter;}\n";
    testDocuments << CppTestDocument::create("file.cpp", original, expected);

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void QuickfixTest::testExtractLiteralAsParameterMemberFunctionSeparateFiles()
{
    QList<TestDocumentPtr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Narf {\n"
        "public:\n"
        "    int zort();\n"
        "};\n";
    expected =
        "class Narf {\n"
        "public:\n"
        "    int zort(int newParameter = 155);\n"
        "};\n";
    testDocuments << CppTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n\n"
        "int Narf::zort()\n"
        "{ return 15@5 + 1; }\n";
    expected =
        "#include \"file.h\"\n\n"
        "int Narf::zort(int newParameter)\n"
        "{ return newParameter + 1; }\n";
    testDocuments << CppTestDocument::create("file.cpp", original, expected);

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void QuickfixTest::testExtractLiteralAsParameterNotTriggeringForInvalidCode()
{
    QList<TestDocumentPtr> testDocuments;
    QByteArray original;
    original =
        "T(\"test\")\n"
        "{\n"
        "    const int i = @14;\n"
        "}\n";
    testDocuments << CppTestDocument::create("file.cpp", original, "");

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void QuickfixTest::testAddCurlyBraces_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    QByteArray original = R"delim(
void MyObject::f()
{
    @if (true)
        emit mySig();
})delim";
    QByteArray expected = R"delim(
void MyObject::f()
{
    if (true) {
        emit mySig();
    }
})delim";
    QTest::newRow("if") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (true)
        emit mySig();
    else
        emit otherSig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if with one else, unbraced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else
        emit otherSig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if with one else, if braced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (true)
        emit mySig();
    else {
        emit otherSig();
    }
})delim";
    expected = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if with one else, else braced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (true) {
        emit mySig();
    } else {
        emit otherSig();
    }
})delim";
    expected.clear();
    QTest::newRow("if with one else, both braced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
    QTest::newRow("if-else chain without final else, unbraced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2)
        emit sig2();
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
    QTest::newRow("if-else chain without final else, partially braced 1") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2) {
        emit sig2();
    }
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
    QTest::newRow("if-else chain without final else, partially braced 2") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    }
})delim";
    expected.clear();
    QTest::newRow("if-else chain without final else, fully braced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else
        emit otherSig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if-else chain, unbraced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else
        emit otherSig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if-else chain, partially braced 1") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2) {
        emit sig2();
    } else if (x == 3)
        emit sig3();
    else
        emit otherSig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if-else chain, partially braced 2") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3) {
        emit sig3();
    } else
        emit otherSig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if-else chain, partially braced 3") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1)
        emit sig1();
    else if (x == 2)
        emit sig2();
    else if (x == 3)
        emit sig3();
    else {
        emit otherSig();
    }
})delim";
    expected = R"delim(
void MyObject::f()
{
    if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
    QTest::newRow("if-else chain, partially braced 4") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @if (x == 1) {
        emit sig1();
    } else if (x == 2) {
        emit sig2();
    } else if (x == 3) {
        emit sig3();
    } else {
        emit otherSig();
    }
})delim";
    expected.clear();
    QTest::newRow("if-else chain, fully braced") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @while (true)
        emit mySig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    while (true) {
        emit mySig();
    }
})delim";
    QTest::newRow("while") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @for (int i = 0; i < 10; ++i)
        emit mySig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    for (int i = 0; i < 10; ++i) {
        emit mySig();
    }
})delim";
    QTest::newRow("for") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @for (int i : list)
        emit mySig();
})delim";
    expected = R"delim(
void MyObject::f()
{
    for (int i : list) {
        emit mySig();
    }
})delim";
    QTest::newRow("range-based for") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @do
        emit mySig();
    while (true);
})delim";
    expected = R"delim(
void MyObject::f()
{
    do {
        emit mySig();
    } while (true);
})delim";
    QTest::newRow("do") << original << expected;

    original = R"delim(
void MyObject::f()
{
    @do {
        emit mySig();
    } while (true);
})delim";
    expected.clear();
    QTest::newRow("already has braces") << original << expected;
}

void QuickfixTest::testAddCurlyBraces()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    AddBracesToControlStatement factory;
    QuickFixOperationTest({CppTestDocument::create("file.cpp", original, expected)}, &factory);
}

void QuickfixTest::testChangeCommentType_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expectedOutput");

    QTest::newRow("C -> C++ / no selection / single line") << R"(
int var1;
/* Other comment, unaffected */
/* Our @comment */
/* Another unaffected comment */
int var2;)" << R"(
int var1;
/* Other comment, unaffected */
// Our comment
/* Another unaffected comment */
int var2;)";

    QTest::newRow("C -> C++ / no selection / multi-line / preserved header and footer") << R"(
/****************************************************
 * some info
 * more @info
 ***************************************************/)" << R"(
/////////////////////////////////////////////////////
// some info
// more info
/////////////////////////////////////////////////////)";

    QTest::newRow("C -> C++ / no selection / multi-line / non-preserved header and footer") << R"(
/*
 * some info
 * more @info
 */)" << R"(
// some info
// more info
)";

    QTest::newRow("C -> C++ / no selection / qdoc") << R"(
/*!
    \qmlproperty string Type::element.name
    \qmlproperty int Type::element.id

    \brief Holds the @element name and id.
*/)" << R"(
//! \qmlproperty string Type::element.name
//! \qmlproperty @int Type::element.id
//!
//! \brief Holds the element name and id.
)";

    QTest::newRow("C -> C++ / no selection / doxygen") << R"(
/*! \class Test
    \brief A test class.

    A more detailed @class description.
*/)" << R"(
//! \class Test
//! \brief A test class.
//!
//! A more detailed class description.
)";

    QTest::newRow("C -> C++ / selection / single line") << R"(
int var1;
/* Other comment, unaffected */
@{start}/* Our comment */@{end}
/* Another unaffected comment */
int var2;)" << R"(
int var1;
/* Other comment, unaffected */
// Our comment
/* Another unaffected comment */
int var2;)";

    QTest::newRow("C -> C++ / selection / multi-line / preserved header and footer") << R"(
/****************************************************
 * @{start}some info
 * more info@{end}
 ***************************************************/)" << R"(
/////////////////////////////////////////////////////
// some info
// more info
/////////////////////////////////////////////////////)";

    QTest::newRow("C -> C++ / selection / multi-line / non-preserved header and footer") << R"(
/*@{start}
 * some in@{end}fo
 * more info
 */)" << R"(
// some info
// more info
)";

    QTest::newRow("C -> C++ / selection / qdoc") << R"(
/*!@{start}
    \qmlproperty string Type::element.name
    \qmlproperty int Type::element.id

    \brief Holds the element name and id.
*/@{end})" << R"(
//! \qmlproperty string Type::element.name
//! \qmlproperty int Type::element.id
//!
//! \brief Holds the element name and id.
)";

    QTest::newRow("C -> C++ / selection / doxygen") << R"(
/** Expand envi@{start}ronment variables in a string.
 *
 * Environment variables are accepted in the @{end}following forms:
 * $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
 * No escapes and quoting are supported.
 * If a variable is not found, it is not substituted.
 */)" << R"(
//! Expand environment variables in a string.
//!
//! Environment variables are accepted in the following forms:
//! $SOMEVAR, ${SOMEVAR} on Unix and %SOMEVAR% on Windows.
//! No escapes and quoting are supported.
//! If a variable is not found, it is not substituted.
)";

    QTest::newRow("C -> C++ / selection / multiple comments") << R"(
@{start}/* Affected comment */
/* Another affected comment */
/* A third affected comment */@{end}
/* An unaffected comment */)" << R"(
// Affected comment
// Another affected comment
// A third affected comment
/* An unaffected comment */)";

    // FIXME: Remove adjacent newline along with last block
    // FIXME: Use CppRefactoringFile to auto-indent continuation lines?
    QTest::newRow("C -> C++, indented") << R"(
struct S {
    /*
     * @This is an
     * indented comment.
     */
    void func();
)" << R"(
struct S {
    // This is an
// indented comment.

    void func();
)";

    QTest::newRow("C++ -> C / no selection / single line") << R"(
// Other comment, unaffected
// Our @comment
// Another unaffected comment)" << R"(
// Other comment, unaffected
/* Our comment */
// Another unaffected comment)";

    QTest::newRow("C++ -> C / selection / single line") << R"(
// Other comment, unaffected
@{start}// Our comment@{end}
// Another unaffected comment)" << R"(
// Other comment, unaffected
/* Our comment */
// Another unaffected comment)";

    QTest::newRow("C++ -> C / selection / multi-line / preserved header and footer") << R"(
@{start}/////////////////////////////////////////////////////
// some info
// more info
/////////////////////////////////////////////////////@{end})" << R"(
/****************************************************/
/* some info                                        */
/* more info                                        */
/****************************************************/)";

    QTest::newRow("C++ -> C / selection / qdoc") << R"(
@{start}//! \qmlproperty string Type::element.name
//! \qmlproperty int Type::element.id
//!
//! \brief Holds the element name and id.@{end}
)" << R"(
/*!
    \qmlproperty string Type::element.name
    \qmlproperty int Type::element.id

    \brief Holds the element name and id.
*/
)";

    QTest::newRow("C++ -> C / selection / doxygen") << R"(
@{start}//! \class Test
//! \brief A test class.
//!
//! A more detailed class description.@{end}
)" << R"(
/*!
    \class Test
    \brief A test class.

    A more detailed class description.
*/
)";
}

void QuickfixTest::testChangeCommentType()
{
    QFETCH(QString, input);
    QFETCH(QString, expectedOutput);

    ConvertCommentStyle factory;
    QuickFixOperationTest(
        {CppTestDocument::create("file.h", input.toUtf8(), expectedOutput.toUtf8())},
        &factory);
}

void QuickfixTest::testMoveComments_data()
{
    QTest::addColumn<QByteArrayList>("headers");
    QTest::addColumn<QByteArrayList>("sources");

    const QByteArrayList headersFuncDecl2Def{R"(
// Function comment
void @aFunction();
)", R"(
void aFunction();
)"};
    const QByteArrayList sourcesFuncDecl2Def{R"(
#include "file.h"

void aFunction() {}
)", R"(
#include "file.h"

// Function comment
void aFunction() {}
)"};
    QTest::newRow("function: from decl to def") << headersFuncDecl2Def << sourcesFuncDecl2Def;

    const QByteArrayList headersFuncDef2Decl{R"(
void aFunction();
)", R"(
/* function */
/* comment */
void aFunction();
)"};
    const QByteArrayList sourcesFuncDef2Decl{R"(
#include "file.h"

/* function */
/* comment */
void a@Function() {}
)", R"(
#include "file.h"

void aFunction() {}
)"};
    QTest::newRow("function: from def to decl") << headersFuncDef2Decl << sourcesFuncDef2Decl;

    const QByteArrayList headersFuncNoDef{R"(
// Function comment
void @aFunction();
)", R"(
// Function comment
void aFunction();
)"};
    QTest::newRow("function: no def") << headersFuncNoDef << QByteArrayList();

    const QByteArrayList headersFuncNoDecl{R"(
// Function comment
inline void @aFunction() {}
)", R"(
// Function comment
inline void aFunction() {}
)"};
    QTest::newRow("function: no decl") << headersFuncNoDecl << QByteArrayList();

    const QByteArrayList headersFuncTemplateDecl2Def{R"(
// Function comment
template<typename T> T @aFunction();

template<typename T> inline T aFunction() { return T(); }
)", R"(
template<typename T> T aFunction();

// Function comment
template<typename T> inline T aFunction() { return T(); }
)"};
    QTest::newRow("function template: from decl to def") << headersFuncTemplateDecl2Def
                                                         << QByteArrayList();

    const QByteArrayList headersFuncTemplateDef2Decl{R"(
template<typename T> T aFunction();

// Function comment
template<typename T> inline T @aFunction() { return T(); }
)", R"(
// Function comment
template<typename T> T aFunction();

template<typename T> inline T aFunction() { return T(); }
)"};
    QTest::newRow("function template: from def to decl") << headersFuncTemplateDef2Decl
                                                         << QByteArrayList();

    const QByteArrayList headersMemberDecl2Def{R"(
class C {
    /**
      * \brief Foo::aMember
      */
    void @aMember();
)", R"(
class C {
    void aMember();
)"};
    const QByteArrayList sourcesMemberDecl2Def{R"(
#include "file.h"

void C::aMember() {}
)", R"(
#include "file.h"

/**
  * \brief Foo::aMember
  */
void C::aMember() {}
)"};
    QTest::newRow("member function: from decl to def") << headersMemberDecl2Def
                                                       << sourcesMemberDecl2Def;

    const QByteArrayList headersMemberDef2Decl{R"(
class C {
    void aMember();
)", R"(
class C {
    /**
      * \brief Foo::aMember
      */
    void aMember();
)"};
    const QByteArrayList sourcesMemberDef2Decl{R"(
#include "file.h"

/**
  * \brief Foo::aMember
  */
void C::aMember() {@}
)", R"(
#include "file.h"

void C::aMember() {}
)"};
    QTest::newRow("member function: from def to decl") << headersMemberDef2Decl
                                                       << sourcesMemberDef2Decl;
}

void QuickfixTest::testMoveComments()
{
    QFETCH(QByteArrayList, headers);
    QFETCH(QByteArrayList, sources);

    QList<TestDocumentPtr> documents;
    QCOMPARE(headers.size(), 2);
    documents << CppTestDocument::create("file.h", headers.at(0), headers.at(1));
    if (!sources.isEmpty()) {
        QCOMPARE(sources.size(), 2);
        documents << CppTestDocument::create("file.cpp", sources.at(0), sources.at(1));
    }
    MoveFunctionComments factory;
    QByteArray failMessage;
    if (QByteArray(QTest::currentDataTag()) == "function template: from def to decl")
        failMessage = "decl/def switch doesn't work for templates";
    QuickFixOperationTest(documents, &factory, {}, {}, failMessage);
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
