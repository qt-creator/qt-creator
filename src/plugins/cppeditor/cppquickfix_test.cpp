/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppeditorplugin.h"
#include "cppeditortestcase.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"
#include "cppquickfix_test.h"

#include <cpptools/cppcodestylepreferences.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppsourceprocessertesthelper.h>
#include <cpptools/cpptoolssettings.h>

#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QtTest>

/*!
    Tests for quick-fixes.
 */
using namespace Core;
using namespace CPlusPlus;
using namespace CppEditor::Internal::Tests;
using namespace TextEditor;

using CppTools::Tests::TestIncludePaths;

namespace CppEditor {
namespace Internal {
namespace Tests {

QuickFixTestDocument::Ptr QuickFixTestDocument::create(const QByteArray &fileName,
                                                       const QByteArray &source,
                                                       const QByteArray &expectedSource)
{
    return Ptr(new QuickFixTestDocument(fileName, source, expectedSource));
}


QuickFixTestDocument::QuickFixTestDocument(const QByteArray &fileName,
                                           const QByteArray &source,
                                           const QByteArray &expectedSource)
    : TestDocument(fileName, source)
    , m_expectedSource(QString::fromUtf8(expectedSource))
{
    removeMarkers();
}

void QuickFixTestDocument::removeMarkers()
{
    // Remove selection markers
    if (m_anchorPosition != -1) {
        if (m_anchorPosition < m_cursorPosition) {
            m_source.remove(m_anchorPosition, m_selectionStartMarker.size());
            m_cursorPosition -= m_selectionStartMarker.size();
            m_source.remove(m_cursorPosition, m_selectionEndMarker.size());
        } else {
            m_source.remove(m_cursorPosition, m_selectionEndMarker.size());
            m_anchorPosition -= m_selectionEndMarker.size();
            m_source.remove(m_anchorPosition, m_selectionStartMarker.size());
        }

    // Remove simple cursor marker
    } else if (m_cursorPosition != -1) {
        m_source.remove(m_cursorPosition, 1);
    }

    const int cursorPositionInExpectedSource
        = m_expectedSource.indexOf(QLatin1Char(m_cursorMarker));
    if (cursorPositionInExpectedSource > -1)
        m_expectedSource.remove(cursorPositionInExpectedSource, 1);
}

QList<QuickFixTestDocument::Ptr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected)
{
    return QList<QuickFixTestDocument::Ptr>()
            << QuickFixTestDocument::create("file.cpp", original, expected);
}

BaseQuickFixTestCase::BaseQuickFixTestCase(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                                           const ProjectPartHeaderPaths &headerPaths)
    : m_testDocuments(testDocuments)
    , m_cppCodeStylePreferences(0)
    , m_restoreHeaderPaths(false)
{
    QVERIFY(succeededSoFar());
    m_succeededSoFar = false;

    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    foreach (const QuickFixTestDocument::Ptr document, m_testDocuments) {
        if (document->hasCursorMarker())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write documents to disk
    m_temporaryDirectory.reset(new CppTools::Tests::TemporaryDir);
    QVERIFY(m_temporaryDirectory->isValid());
    foreach (QuickFixTestDocument::Ptr document, m_testDocuments) {
        if (QFileInfo(document->m_fileName).isRelative())
            document->setBaseDirectory(m_temporaryDirectory->path());
        document->writeToDisk();
    }

    // Set appropriate include paths
    if (!headerPaths.isEmpty()) {
        m_restoreHeaderPaths = true;
        m_headerPathsToRestore = m_modelManager->headerPaths();
        m_modelManager->setHeaderPaths(headerPaths);
    }

    // Update Code Model
    QSet<QString> filePaths;
    foreach (const QuickFixTestDocument::Ptr &document, m_testDocuments)
        filePaths << document->filePath();
    QVERIFY(parseFiles(filePaths));

    // Open Files
    foreach (QuickFixTestDocument::Ptr document, m_testDocuments) {
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
    m_cppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
    QVERIFY(m_cppCodeStylePreferences);
    m_cppCodeStylePreferencesOriginalDelegateId = m_cppCodeStylePreferences->currentDelegateId();
    m_cppCodeStylePreferences->setCurrentDelegate("qt");

    // Find the document having the cursor marker
    foreach (const QuickFixTestDocument::Ptr document, m_testDocuments) {
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
        m_modelManager->setHeaderPaths(m_headerPathsToRestore);

    // Remove created files from file system
    foreach (const QuickFixTestDocument::Ptr &testDocument, m_testDocuments)
        QVERIFY(QFile::remove(testDocument->filePath()));
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

QuickFixOperationTest::QuickFixOperationTest(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                                             CppQuickFixFactory *factory,
                                             const ProjectPartHeaderPaths &headerPaths,
                                             int operationIndex,
                                             const QByteArray &expectedFailMessage)
    : BaseQuickFixTestCase(testDocuments, headerPaths)
{
    QVERIFY(succeededSoFar());

    // Perform operation if there is one
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations operations;
    factory->match(quickFixInterface, operations);
    if (operations.isEmpty()) {
        QEXPECT_FAIL("onBaseOfQualifiedClassName", "QTCREATORBUG-14499", Continue);
        QVERIFY(testDocuments.first()->m_expectedSource.isEmpty());
        return;
    }

    QVERIFY(operationIndex < operations.size());
    const QuickFixOperation::Ptr operation = operations.at(operationIndex);
    operation->perform();

    // Compare all files
    foreach (const QuickFixTestDocument::Ptr testDocument, m_testDocuments) {
        // Check
        QString result = testDocument->m_editorWidget->document()->toPlainText();
        removeTrailingWhitespace(result);
        if (!expectedFailMessage.isEmpty())
            QEXPECT_FAIL("", expectedFailMessage.data(), Continue);
        QCOMPARE(result, testDocument->m_expectedSource);

        // Undo the change
        for (int i = 0; i < 100; ++i)
            testDocument->m_editorWidget->undo();
        result = testDocument->m_editorWidget->document()->toPlainText();
        QCOMPARE(result, testDocument->m_source);
    }
}

void QuickFixOperationTest::run(const QList<QuickFixTestDocument::Ptr> &testDocuments,
                                CppQuickFixFactory *factory,
                                const QString &headerPath,
                                int operationIndex)
{
    ProjectPartHeaderPaths headerPaths;
    headerPaths += ProjectPartHeaderPath(headerPath, ProjectPartHeaderPath::IncludePath);
    QuickFixOperationTest(testDocuments, factory, headerPaths, operationIndex);
}

QuickFixOfferedOperationsTest::QuickFixOfferedOperationsTest(
        const QList<QuickFixTestDocument::Ptr> &testDocuments,
        CppQuickFixFactory *factory,
        const ProjectPartHeaderPaths &headerPaths,
        const QStringList &expectedOperations)
    : BaseQuickFixTestCase(testDocuments, headerPaths)
{
    // Get operations
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations actualOperations;
    factory->match(quickFixInterface, actualOperations);

    // Convert to QStringList
    QStringList actualOperationsAsStringList;
    foreach (const QuickFixOperation::Ptr &operation, actualOperations)
        actualOperationsAsStringList << operation->description();

    QCOMPARE(actualOperationsAsStringList, expectedOperations);
}

/// Delegates directly to AddIncludeForUndefinedIdentifierOp for easier testing.
class AddIncludeForUndefinedIdentifierTestFactory : public CppQuickFixFactory
{
public:
    AddIncludeForUndefinedIdentifierTestFactory(const QString &include)
        : m_include(include) {}

    void match(const CppQuickFixInterface &cppQuickFixInterface, QuickFixOperations &result)
    {
        result << new AddIncludeForUndefinedIdentifierOp(cppQuickFixInterface, 0, m_include);
    }

private:
    const QString m_include;
};

} // namespace Tests
} // namespace Internal

typedef QSharedPointer<CppQuickFixFactory> CppQuickFixFactoryPtr;

} // namespace CppEditor

namespace CppEditor {
namespace Internal {

void CppEditorPlugin::test_quickfix_data()
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

    // Checks:
    // 1. If the name does not start with ("m_" or "_") and does not
    //    end with "_", we are forced to prefix the getter with "get".
    // 2. Setter: Use pass by value on integer/float and pointer types.
    QTest::newRow("GenerateGetterSetter_basicGetterWithPrefix")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int @it;\n"
        "};\n"
        ) << _(
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
    );

    // Checks: In addition to test_quickfix_GenerateGetterSetter_basicGetterWithPrefix
    // generated definitions should fit in the namespace.
    QTest::newRow("GenerateGetterSetter_basicGetterWithPrefixAndNamespace")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "namespace SomeNamespace {\n"
        "class Something\n"
        "{\n"
        "    int @it;\n"
        "};\n"
        "}\n"
        ) << _(
        "namespace SomeNamespace {\n"
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
        "}\n"
    );

    // Checks:
    // 1. Getter: "get" prefix is not necessary.
    // 2. Setter: Parameter name is base name.
    QTest::newRow("GenerateGetterSetter_basicGetterWithoutPrefix")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int @m_it;\n"
        "};\n"
        ) << _(
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
    );

    // Checks if getter uses 'get' prefix if member function with such a prefix is found
    QTest::newRow("GenerateGetterSetter_getterWithGetPrefix")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int getFoo();\n"
        "    int @m_it;\n"
        "};\n"
        ) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int getFoo();\n"
        "    int m_it;\n"
        "\n"
        "public:\n"
        "    int getIt() const;\n"
        "    void setIt(int it);\n"
        "};\n"
        "\n"
        "int Something::getIt() const\n"
        "{\n"
        "    return m_it;\n"
        "}\n"
        "\n"
        "void Something::setIt(int it)\n"
        "{\n"
        "    m_it = it;\n"
        "}\n"
    );

    // Check: Setter: Use pass by reference for parameters which
    // are not integer, float or pointers.
    QTest::newRow("GenerateGetterSetter_customType")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    MyType @it;\n"
        "};\n"
        ) << _(
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
    );

    // Checks:
    // 1. Setter: No setter is generated for const members.
    // 2. Getter: Return a non-const type since it pass by value anyway.
    QTest::newRow("GenerateGetterSetter_constMember")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    const int @it;\n"
        "};\n"
        ) << _(
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
    );

    // Checks: No special treatment for pointer to non const.
    QTest::newRow("GenerateGetterSetter_pointerToNonConst")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int *it@;\n"
        "};\n"
        ) << _(
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
    );

    // Checks: No special treatment for pointer to const.
    QTest::newRow("GenerateGetterSetter_pointerToConst")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    const int *it@;\n"
        "};\n"
        ) << _(
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
    );

    // Checks: No special treatment for reference to non const.
    QTest::newRow("GenerateGetterSetter_referenceToNonConst")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int &it@;\n"
        "};\n"
        ) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int &it;\n"
        "\n"
        "public:\n"
        "    int &getIt() const;\n"
        "    void setIt(const int &value);\n"
        "};\n"
        "\n"
        "int &Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(const int &value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
    );

    // Checks: No special treatment for reference to const.
    QTest::newRow("GenerateGetterSetter_referenceToConst")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    const int &it@;\n"
        "};\n"
        ) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    const int &it;\n"
        "\n"
        "public:\n"
        "    const int &getIt() const;\n"
        "    void setIt(const int &value);\n"
        "};\n"
        "\n"
        "const int &Something::getIt() const\n"
        "{\n"
        "    return it;\n"
        "}\n"
        "\n"
        "void Something::setIt(const int &value)\n"
        "{\n"
        "    it = value;\n"
        "}\n"
    );

    // Checks:
    // 1. Setter: Setter is a static function.
    // 2. Getter: Getter is a static, non const function.
    QTest::newRow("GenerateGetterSetter_staticMember")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    static int @m_member;\n"
        "};\n"
        ) << _(
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
    );

    // Check: Check if it works on the second declarator
    QTest::newRow("GenerateGetterSetter_secondDeclarator")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int *foo, @it;\n"
        "};\n"
        ) << _(
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
    );

    // Check: Quick fix is offered for "int *@it;" ('@' denotes the text cursor position)
    QTest::newRow("GenerateGetterSetter_triggeringRightAfterPointerSign")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int *@it;\n"
        "};\n"
        ) << _(
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
    );

    // Check: Quick fix is not triggered on a member function.
    QTest::newRow("GenerateGetterSetter_notTriggeringOnMemberFunction")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter)
        << _("class Something { void @f(); };\n") << _();

    // Check: Quick fix is not triggered on an member array;
    QTest::newRow("GenerateGetterSetter_notTriggeringOnMemberArray")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter)
        << _("class Something { void @a[10]; };\n") << _();

    // Check: Do not offer the quick fix if there is a getter and the variable is const
    QTest::newRow("GenerateGetterSetter_notTriggeringWhenGetterAndConstVariable")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "class Foo\n"
        "{\n"
        "public:\n"
        "    const int bar@;\n"
        "    int getBar() const;\n"
        "};\n"
        ) << _();

    // Check: Do not offer the quick fix if there is a getter and a setter
    QTest::newRow("GenerateGetterSetter_notTriggeringWhenGetterAndConstVariable")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "class Foo\n"
        "{\n"
        "public:\n"
        "    const int bar@;\n"
        "    int getBar() const;\n"
        "    void setBar(int value);\n"
        "};\n"
        ) << _();

    // Checks if "m_" is recognized as "m" with the postfix "_" and not simply as "m_" prefix.
    QTest::newRow("GenerateGetterSetter_recognizeMasVariableName")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int @m_;\n"
        "};\n"
        ) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int m_;\n"
        "\n"
        "public:\n"
        "    int m() const;\n"
        "    void setM(int m);\n"
        "};\n"
        "\n"
        "int Something::m() const\n"
        "{\n"
        "    return m_;\n"
        "}\n"
        "\n"
        "void Something::setM(int m)\n"
        "{\n"
        "    m_ = m;\n"
        "}\n"
    );

    // Checks if "m" followed by an upper character is recognized as a prefix
    QTest::newRow("GenerateGetterSetter_recognizeMFollowedByCapital")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int @mFoo;\n"
        "};\n"
        ) << _(
        "\n"
        "class Something\n"
        "{\n"
        "    int mFoo;\n"
        "\n"
        "public:\n"
        "    int foo() const;\n"
        "    void setFoo(int foo);\n"
        "};\n"
        "\n"
        "int Something::foo() const\n"
        "{\n"
        "    return mFoo;\n"
        "}\n"
        "\n"
        "void Something::setFoo(int foo)\n"
        "{\n"
        "    mFoo = foo;\n"
        "}\n"
    );

    // Checks if the declaration inside Q_PROPERTY macro is ignored and a getter created
    QTest::newRow("GenerateGetterSetter_ignoreQPropertiesMacro")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "class Something\n"
        "{\n"
        "    Q_PROPERTY(int foo)\n"
        "    int @m_foo;\n"
        "};\n"
        ) << _(
        "class Something\n"
        "{\n"
        "    Q_PROPERTY(int foo)\n"
        "    int m_foo;\n"
        "\n"
        "public:\n"
        "    int foo() const;\n"
        "    void setFoo(int foo);\n"
        "};\n"
        "\n"
        "int Something::foo() const\n"
        "{\n"
        "    return m_foo;\n"
        "}\n"
        "\n"
        "void Something::setFoo(int foo)\n"
        "{\n"
        "    m_foo = foo;\n"
        "}\n"
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

    // Check from source file: If there is no header file, insert the definition after the class.
    QByteArray original =
        "struct Foo\n"
        "{\n"
        "    Foo();@\n"
        "};\n";

    QTest::newRow("InsertDefFromDecl_basic")
        << CppQuickFixFactoryPtr(new InsertDefFromDecl) << original
           << original + _(
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n"
    );

    QTest::newRow("InsertDefFromDecl_freeFunction")
        << CppQuickFixFactoryPtr(new InsertDefFromDecl)
        << _("void free()@;\n")
        << _(
        "void free()\n"
        "{\n\n"
        "}\n"
    );

    // Check not triggering when it is a statement
    QTest::newRow("InsertDefFromDecl_notTriggeringStatement")
        << CppQuickFixFactoryPtr(new InsertDefFromDecl) << _(
            "class Foo {\n"
            "public:\n"
            "    Foo() {}\n"
            "};\n"
            "void freeFunc() {\n"
            "    Foo @f();"
            "}\n"
        ) << _();

    // Check: Add local variable for a free function.
    QTest::newRow("AssignToLocalVariable_freeFunction")
        << CppQuickFixFactoryPtr(new AssignToLocalVariable) << _(
        "int foo() {return 1;}\n"
        "void bar() {fo@o();}\n"
        ) << _(
        "int foo() {return 1;}\n"
        "void bar() {int localFoo = foo();}\n"
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
        "    int *localFooFunc = f->fooFunc();\n"
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
        "    int *localFunc = b->foo()->func();\n"
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
        "    int *localFunc = b->foo()->func();\n"
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
        "    int *localFooFunc = Foo::fooFunc();\n"
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
        "    Foo *localFoo = new Foo;\n"
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

    QTest::newRow("InsertQtPropertyMembers")
            << CppQuickFixFactoryPtr(new InsertQtPropertyMembers)
            << _("struct XmarksTheSpot {\n"
                 "    @Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)\n"
                 "};\n"
                 )
            << _("struct XmarksTheSpot {\n"
                 "    Q_PROPERTY(int it READ getIt WRITE setIt NOTIFY itChanged)\n"
                 "\n"
                 "public:\n"
                 "    int getIt() const\n"
                 "    {\n"
                 "        return m_it;\n"
                 "    }\n"
                 "\n"
                 "public slots:\n"
                 "    void setIt(int it)\n"
                 "    {\n"
                 "        if (m_it == it)\n"
                 "            return;\n"
                 "\n"
                 "        m_it = it;\n"
                 "        emit itChanged(m_it);\n"
                 "    }\n"
                 "\n"
                 "signals:\n"
                 "    void itChanged(int it);\n"
                 "\n"
                 "private:\n"
                 "    int m_it;\n"
                 "};\n"
                 );

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
             "    QString *str;\n"
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
             "    QString str = QLatin1String(\"schnurz\");\n"
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
             "    QString *str;\n"
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

    QTest::newRow("InsertQtPropertyMembers_noTriggerInvalidCode")
        << CppQuickFixFactoryPtr(new InsertQtPropertyMembers)
        << _("class C { @Q_PROPERTY(typeid foo READ foo) };\n")
        << _();
}

void CppEditorPlugin::test_quickfix()
{
    QFETCH(CppQuickFixFactoryPtr, factory);
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    QuickFixOperationTest(singleDocument(original, expected), factory.data());
}

/// Checks: In addition to test_quickfix_GenerateGetterSetter_basicGetterWithPrefix
/// generated definitions should fit in the namespace.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespaceToCpp()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "namespace SomeNamespace {\n"
        "}\n";
    expected =
        "#include \"file.h\"\n"
        "namespace SomeNamespace {\n"
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Checks: Only generate getter
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_onlyGetter()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar@;\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar@;\n"
        "    int getBar() const;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "int Foo::getBar() const\n"
        "{\n"
        "    return bar;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), 1);
}

/// Checks: Only generate setter
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_onlySetter()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar@;\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar@;\n"
        "    void setBar(int value);\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "void Foo::setBar(int value)\n"
        "{\n"
        "    bar = value;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), 2);
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

    CppCodeStylePreferences *preferences = CppToolsSettings::instance()->cppCodeStyle();
    preferences->currentDelegate()->setValue(variant);
}

CppCodeStyleSettings CppCodeStyleSettingsChanger::currentSettings()
{
    CppCodeStylePreferences *preferences = CppToolsSettings::instance()->cppCodeStyle();
    return preferences->currentDelegate()->value().value<CppCodeStyleSettings>();
}

void CppEditorPlugin::test_quickfix_GenerateGetterSetter_onlyGetter_DontPreferGetterWithGet()
{
    CppCodeStyleSettings modifiedSettings = CppCodeStyleSettingsChanger::currentSettings();
    modifiedSettings.preferGetterNameWithoutGetPrefix = false;
    CppCodeStyleSettingsChanger changer(modifiedSettings);

    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int m_bar@;\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int m_bar@;\n"
        "    int getBar() const;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "int Foo::getBar() const\n"
        "{\n"
        "    return m_bar;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), 1);
}

/// Checks: Offer a "generate getter" quick fix if there is a setter
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_offerGetterWhenSetterPresent()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar@;\n"
        "    void setBar(int value);\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar;\n"
        "    void setBar(int value);\n"
        "    int getBar() const;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "int Foo::getBar() const\n"
        "{\n"
        "    return bar;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Checks: Offer a "generate setter" quick fix if there is a getter
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_offerSetterWhenGetterPresent()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar@;\n"
        "    int getBar() const;\n"
        "};\n";
    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    int bar;\n"
        "    int getBar() const;\n"
        "    void setBar(int value);\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "void Foo::setBar(int value)\n"
        "{\n"
        "    bar = value;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check if definition is inserted right after class for insert definition outside
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_afterClass()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "class Bar {};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), 1);
}

/// Check from header file: If there is a source file, insert the definition in the source file.
/// Case: Source file is empty.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic1()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n"
        ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check from header file: If there is a source file, insert the definition in the source file.
/// Case: Source file is not empty.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic2()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            "\n"
            "int x;\n"
            ;
    expected =
            "#include \"file.h\"\n"
            "\n"
            "int x;\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check from source file: Insert in source file, not header file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic3()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Empty Header File
    testDocuments << QuickFixTestDocument::create("file.h", "", "");

    // Source File
    original =
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n";
    expected = original +
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n"
        ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check from header file: If the class is in a namespace, the added function definition
/// name must be qualified accordingly.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace1()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

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
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "N::Foo::Foo()\n"
        "{\n\n"
        "}\n"
        ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check from header file: If the class is in namespace N and the source file has a
/// "using namespace N" line, the function definition name must be qualified accordingly.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace2()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

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
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            "using namespace N;\n"
            ;
    expected = original +
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
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
        "};";

    InsertDefFromDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory, ProjectPartHeaderPaths(),
                          1);
}

/// Check not triggering when definition exists
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_notTriggeringWhenDefinitionExists()
{
    const QByteArray original =
            "class Foo {\n"
            "    void b@ar();\n"
            "};\n"
            "void Foo::bar() {}\n";

    InsertDefFromDecl factory;
    QuickFixOperationTest(singleDocument(original, ""), &factory, ProjectPartHeaderPaths(), 1);
}

/// Find right implementation file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_findRightImplementationFile()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

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
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File #1
    original =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);


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
            "}\n";
    testDocuments << QuickFixTestDocument::create("file2.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Ignore generated functions declarations when looking at the surrounding
/// functions declarations in order to find the right implementation file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_ignoreSurroundingGeneratedDeclarations()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

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
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

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
            "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    // Source File #2
    original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::hidden()\n"
            "{\n\n"
            "}\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file2.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
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
        "Foo &Foo::operator =()\n"
        "{\n"
        "\n"
        "}\n";

    InsertDefFromDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
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
        "Foo &Foo::operator=()\n"
        "{\n"
        "\n"
        "}\n";

    InsertDefFromDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check if a function like macro use is not separated by the function to insert
/// Case: Macro preceded by preproceesor directives and declaration.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile1()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "int lala;\n"
            "\n"
            "MACRO(int)\n"
            ;
    expected =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "int lala;\n"
            "\n"
            "\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "MACRO(int)\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check if a function like macro use is not separated by the function to insert
/// Case: Marco preceded only by preprocessor directives.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile2()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "\n"
            "MACRO(int)\n"
            ;
    expected =
            "#include \"file.h\"\n"
            "#define MACRO(X) X x;\n"
            "\n"
            "\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "MACRO(int)\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check if insertion happens before syntactically erroneous statements at end of file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_erroneousStatementAtEndOfFile()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            "\n"
            "MissingSemicolon(int)\n"
            ;
    expected =
            "#include \"file.h\"\n"
            "\n"
            "\n"
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            "\n"
            "MissingSemicolon(int)\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Respect rvalue references
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_rvalueReference()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f(Foo &&)@;\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original = "";
    expected =
            "\n"
            "void f(Foo &&)\n"
            "{\n"
            "\n"
            "}\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Find right implementation file. (QTCREATORBUG-10728)
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_findImplementationFile()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
            "class Foo {\n"
            "    void bar();\n"
            "    void ba@z();\n"
            "};\n"
            "\n"
            "void Foo::bar()\n"
            "{}\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            ;
    expected =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::baz()\n"
            "{\n"
            "\n"
            "}\n"
            ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_InsertDefFromDecl_unicodeIdentifier()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

    QByteArray original;
    QByteArray expected;

    //
    // The following "non-latin1" code points are used in the tests:
    //
    //   U+00FC  - 2 code units in UTF8, 1 in UTF16 - LATIN SMALL LETTER U WITH DIAERESIS
    //   U+4E8C  - 3 code units in UTF8, 1 in UTF16 - CJK UNIFIED IDEOGRAPH-4E8C
    //   U+10302 - 4 code units in UTF8, 2 in UTF16 - OLD ITALIC LETTER KE
    //

#define UNICODE_U00FC "\xc3\xbc"
#define UNICODE_U4E8C "\xe4\xba\x8c"
#define UNICODE_U10302 "\xf0\x90\x8c\x82"
#define TEST_UNICODE_IDENTIFIER UNICODE_U00FC UNICODE_U4E8C UNICODE_U10302

    original =
            "class Foo {\n"
            "    void @" TEST_UNICODE_IDENTIFIER "();\n"
            "};\n";
            ;
    expected = original;
    expected +=
            "\n"
            "void Foo::" TEST_UNICODE_IDENTIFIER "()\n"
            "{\n"
            "\n"
            "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

#undef UNICODE_U00FC
#undef UNICODE_U4E8C
#undef UNICODE_U10302
#undef TEST_UNICODE_IDENTIFIER

    InsertDefFromDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_InsertDefFromDecl_templateClass()
{
    QByteArray original =
        "template<class T>\n"
        "class Foo\n"
        "{\n"
        "    void fun@c();\n"
        "};\n";
    QByteArray expected =
        "template<class T>\n"
        "class Foo\n"
        "{\n"
        "    void fun@c();\n"
        "};\n"
        "\n"
        "template<class T>\n"
        "void Foo::func()\n" // Should really be Foo<T>::func()
        "{\n"
        "\n"
        "}\n";

    InsertDefFromDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_InsertDefFromDecl_templateFunction()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "    template<class T>\n"
        "    void fun@c();\n"
        "};\n";
    QByteArray expected =
        "class Foo\n"
        "{\n"
        "    template<class T>\n"
        "    void fun@c();\n"
        "};\n"
        "\n"
        "template<class T>\n"
        "void Foo::func()\n"
        "{\n"
        "\n"
        "}\n";

    InsertDefFromDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

// Function for one of InsertDeclDef section cases
void insertToSectionDeclFromDef(const QByteArray &section, int sectionIndex)
{
    QList<QuickFixTestDocument::Ptr> testDocuments;

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
        "@};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo@()\n"
        "{\n"
        "}\n"
        ;
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDeclFromDef factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), sectionIndex);
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

void CppEditorPlugin::test_quickfix_InsertDeclFromDef_templateFuncTypename()
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

void CppEditorPlugin::test_quickfix_InsertDeclFromDef_templateFuncInt()
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

void CppEditorPlugin::test_quickfix_InsertDeclFromDef_notTriggeredForTemplateFunc()
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

void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_data()
{
    QTest::addColumn<QString>("headerPath");
    QTest::addColumn<QuickFixTestDocuments>("testDocuments");
    QTest::addColumn<int>("refactoringOperationIndex");
    QTest::addColumn<QString>("includeForTestFactory");

    const int firstRefactoringOperation = 0;
    const int secondRefactoringOperation = 1;

    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "class Foo {};\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onSimpleName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { class Foo {}; }\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onNameOfQualifiedName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { class Foo {}; }\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onBaseOfQualifiedName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "class Foo { static void bar() {} };\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onBaseOfQualifiedClassName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { template <typename T> class Foo {}; }\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onTemplateName")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "namespace N { template <typename T> class Foo {}; }\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("onTemplateNameInsideArguments")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "class Foo {};\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("withForwardDeclaration")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "template<class T> class Foo {};\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("afile.h", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
    QTest::newRow("withForwardDeclaration2")
            << TestIncludePaths::globalIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();

    // -------------------------------------------------------------------------------------------

    // Header File
    original = "template<class T> class QMyClass {};\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("qmyclass.h", original, expected);

    // Forward Header File
    original = "#include \"qmyclass.h\"\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("QMyClass", original, expected);

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
    testDocuments << QuickFixTestDocument::create("afile.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
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
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);
    QTest::newRow("inserting_checkQSomethingInQtIncludePaths")
            << TestIncludePaths::globalQtCoreIncludePath()
            << testDocuments << firstRefactoringOperation << "";
    testDocuments.clear();
}

void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier()
{
    QFETCH(QString, headerPath);
    QFETCH(QuickFixTestDocuments, testDocuments);
    QFETCH(int, refactoringOperationIndex);
    QFETCH(QString, includeForTestFactory);

    CppTools::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    foreach (QuickFixTestDocument::Ptr testDocument, testDocuments)
        testDocument->setBaseDirectory(temporaryDir.path());

    QScopedPointer<CppQuickFixFactory> factory;
    if (includeForTestFactory.isEmpty())
        factory.reset(new AddIncludeForUndefinedIdentifier);
    else
        factory.reset(new AddIncludeForUndefinedIdentifierTestFactory(includeForTestFactory));

    QuickFixOperationTest::run(testDocuments, factory.data(), headerPath,
                               refactoringOperationIndex);
}

void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_noDoubleQtHeaderInclude()
{
    CppTools::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());

    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    const QByteArray base = temporaryDir.path().toUtf8();

    // This file makes the QDir definition available so that locator finds it.
    original = expected = "#include <QDir>\n"
                          "void avoidBeingRecognizedAsForwardingHeader();";
    testDocuments << QuickFixTestDocument::create(base + "/fileUsingQDir.cpp", original, expected);

    original = expected = "@QDir dir;\n";
    testDocuments << QuickFixTestDocument::create(base + "/fileWantsToUseQDir.cpp", original, expected);

    ProjectPartHeaderPaths headerPaths;
    headerPaths += ProjectPartHeaderPath(TestIncludePaths::globalQtCoreIncludePath(),
                                         ProjectPartHeaderPath::IncludePath);

    AddIncludeForUndefinedIdentifier factory;
    const QStringList expectedOperations = QStringList("Add #include <QDir>");
    QuickFixOfferedOperationsTest(testDocuments, &factory, headerPaths, expectedOperations);
}

/// Check: Move definition from header to cpp.
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "};\n";
    expected =
        "class Foo {\n"
        "  inline int number() const;\n"
        "\n"
        "    void bar();\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "int Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppInsideNS()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
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
        "void Foo::f4() {}\n";

    MoveFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: Move definition outside class
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncOutside2()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "void Foo::f1() {}\n"
        "void Foo::f3() {}\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), 1);
}

/// Check: Move definition from header to cpp (with namespace).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "int MyNs::Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Move definition from header to cpp (with namespace + using).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    expected =
        "namespace MyNs {\n"
        "class Foo {\n"
        "  inline int number() const;\n"
        "};\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "using namespace MyNs;\n";
    expected =
        "#include \"file.h\"\n"
        "using namespace MyNs;\n"
        "\n"
        "int Foo::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
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
        "};}\n";
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
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: Move free function from header to cpp.
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_FreeFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
         ;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "int number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Move free function from header to cpp (with namespace).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "int MyNamespace::number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Move Ctor with member initialization list (QTCREATORBUG-9157).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_CtorWithInitialization1()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "};\n";
    expected =
        "class Foo {\n"
        "public:\n"
        "    Foo();\n"
        "private:\n"
        "    int a;\n"
        "    float b;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original ="#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo() : a(42), b(3.141) {}\n"
       ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Move Ctor with member initialization list (QTCREATORBUG-9462).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_CtorWithInitialization2()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "};\n";

    expected =
        "class Foo\n"
        "{\n"
        "public:\n"
        "    Foo();\n"
        "\n"
        "    int member;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original ="#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo() : member(2)\n"
        "{\n"
        "}\n"
       ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check if definition is inserted right after class for move definition outside
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_afterClass()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "class Bar {};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n";
    expected = original;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory, ProjectPartHeaderPaths(), 1);
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
        "Foo &Foo::operator =() {}\n"
       ;

    MoveFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
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
        "Foo &Foo::operator=() {}\n"
       ;

    MoveFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_macroUses()
{
    QByteArray original =
        "#define CONST const\n"
        "#define VOLATILE volatile\n"
        "class Foo\n"
        "{\n"
        "    int fu@nc(int a, int b) CONST VOLATILE\n"
        "    {\n"
        "        return 42;\n"
        "    }\n"
        "};\n";
    QByteArray expected =
        "#define CONST const\n"
        "#define VOLATILE volatile\n"
        "class Foo\n"
        "{\n"
        "    int func(int a, int b) CONST VOLATILE;\n"
        "};\n"
        "\n"
        "\n"
        // const volatile become lowercase: QTCREATORBUG-12620
        "int Foo::func(int a, int b) const volatile\n"
        "{\n"
        "    return 42;\n"
        "}\n"
       ;

    MoveFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory,
                          ProjectPartHeaderPaths(), 0, "QTCREATORBUG-12314");
}

void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_template()
{
    QByteArray original =
        "template<class T>\n"
        "class Foo { void fu@nc() {} };\n";
    QByteArray expected =
        "template<class T>\n"
        "class Foo { void fu@nc(); };\n"
        "\n"
        "template<class T>\n"
        "void Foo::func() {}\n"; // Should be Foo<T>::func
       ;

    MoveFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCpp()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFunc()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int Foo::num@ber() const {return 5;}\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
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
        "};\n\n\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int MyNs::Foo::num@ber() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected = "#include \"file.h\"\n\n\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNSUsing()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

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
        "\n\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
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
        "};\n\n\n}\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_FreeFuncToCpp()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_FreeFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original = "int number() const;\n";
    expected =
        "int number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "int numb@er() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected = "#include \"file.h\"\n\n\n\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_FreeFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

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
        "\n\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_CtorWithInitialization()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_CtorWithInitialization()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
        "};\n";
    expected =
        "class Foo {\n"
        "public:\n"
        "    Foo() : a(42), b(3.141) {}\n"
        "private:\n"
        "    int a;\n"
        "    float b;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::F@oo() : a(42), b(3.141) {}"
        ;
    expected ="#include \"file.h\"\n\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Definition should not be placed behind the variable. QTCREATORBUG-10303
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_structWithAssignedVariable()
{
    QByteArray original =
        "struct Foo\n"
        "{\n"
        "    void foo();\n"
        "} bar;\n"
        "void Foo::fo@o()\n"
        "{\n"
        "    return;\n"
        "}";

    QByteArray expected =
        "struct Foo\n"
        "{\n"
        "    void foo()\n"
        "    {\n"
        "        return;\n"
        "    }\n"
        "} bar;\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_macroUses()
{
    QByteArray original =
        "#define CONST const\n"
        "#define VOLATILE volatile\n"
        "class Foo\n"
        "{\n"
        "    int func(int a, int b) CONST VOLATILE;\n"
        "};\n"
        "\n"
        "\n"
        "int Foo::fu@nc(int a, int b) CONST VOLATILE"
        "{\n"
        "    return 42;\n"
        "}\n";
    QByteArray expected =
        "#define CONST const\n"
        "#define VOLATILE volatile\n"
        "class Foo\n"
        "{\n"
        "    int func(int a, int b) CONST VOLATILE\n"
        "    {\n"
        "        return 42;\n"
        "    }\n"
        "};\n\n\n\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory,
                          ProjectPartHeaderPaths(), 0, "QTCREATORBUG-12314");
}

void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_override()
{
    QByteArray original =
        "struct Base {\n"
        "    virtual int foo() = 0;\n"
        "};\n"
        "struct Derived : Base {\n"
        "    int foo() override;\n"
        "};\n"
        "\n"
        "int Derived::fo@o()\n"
        "{\n"
        "    return 5;\n"
        "}\n";

    QByteArray expected =
        "struct Base {\n"
        "    virtual int foo() = 0;\n"
        "};\n"
        "struct Derived : Base {\n"
        "    int foo() override\n"
        "    {\n"
        "        return 5;\n"
        "    }\n"
        "};\n\n\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_template()
{
    QByteArray original =
        "template<class T>\n"
        "class Foo { void func(); };\n"
        "\n"
        "template<class T>\n"
        "void Foo<T>::fu@nc() {}\n";

    QByteArray expected =
        "template<class T>\n"
        "class Foo { void fu@nc() {} };\n\n\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_templateFunction()
{
    QByteArray original =
        "class Foo\n"
        "{\n"
        "    template<class T>\n"
        "    void func();\n"
        "};\n"
        "\n"
        "template<class T>\n"
        "void Foo::fu@nc() {}\n";

    QByteArray expected =
        "class Foo\n"
        "{\n"
        "    template<class T>\n"
        "    void func() {}\n"
        "};\n\n\n";

    MoveFuncDefToDecl factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: Move all definitions from header to cpp.
void CppEditorPlugin::test_quickfix_MoveAllFuncDefOutside_MemberFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "class Fo@o {\n"
        "  int numberA() const\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "  int numberB() const\n"
        "  {\n"
        "    return 5;\n"
        "  }\n"
        "};\n";
    expected =
        "class Foo {\n"
        "  int numberA() const;\n"
        "  int numberB() const;\n"
        "};\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "int Foo::numberA() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        "\n"
        "int Foo::numberB() const\n"
        "{\n"
        "    return 5;\n"
        "}\n"
        ;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveAllFuncDefOutside factory;
    QuickFixOperationTest(testDocuments, &factory);
}

/// Check: Move all definition outside class
void CppEditorPlugin::test_quickfix_MoveAllFuncDefOutside_MemberFuncOutside()
{
    QByteArray original =
        "class F@oo {\n"
        "    int f1()\n"
        "    {\n"
        "        return 1;\n"
        "    }\n"
        "    int f2() const\n"
        "    {\n"
        "        return 2;\n"
        "    }\n"
        "};\n";
    QByteArray expected =
        "class Foo {\n"
        "    int f1();\n"
        "    int f2() const;\n"
        "};\n"
        "\n"
        "int Foo::f1()\n"
        "{\n"
        "    return 1;\n"
        "}\n"
        "\n"
        "int Foo::f2() const\n"
        "{\n"
        "    return 2;\n"
        "}\n";

    MoveAllFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: Move all definition outside class
void CppEditorPlugin::test_quickfix_MoveAllFuncDefOutside_DoNotTriggerOnBaseClass()
{
    QByteArray original =
        "class Bar;\n"
        "class Foo : public Ba@r {\n"
        "    int f1()\n"
        "    {\n"
        "        return 1;\n"
        "    }\n"
        "};\n";

    MoveAllFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, ""), &factory);
}

/// Check: Move all definition outside class
void CppEditorPlugin::test_quickfix_MoveAllFuncDefOutside_classWithBaseClass()
{
    QByteArray original =
        "class Bar;\n"
        "class Fo@o : public Bar {\n"
        "    int f1()\n"
        "    {\n"
        "        return 1;\n"
        "    }\n"
        "};\n";
    QByteArray expected =
        "class Bar;\n"
        "class Foo : public Bar {\n"
        "    int f1();\n"
        "};\n"
        "\n"
        "int Foo::f1()\n"
        "{\n"
        "    return 1;\n"
        "}\n";

    MoveAllFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

/// Check: Do not take macro expanded code into account (QTCREATORBUG-13900)
void CppEditorPlugin::test_quickfix_MoveAllFuncDefOutside_ignoreMacroCode()
{
    QByteArray original =
        "#define FAKE_Q_OBJECT int bar() {return 5;}\n"
        "class Fo@o {\n"
        "    FAKE_Q_OBJECT\n"
        "    int f1()\n"
        "    {\n"
        "        return 1;\n"
        "    }\n"
        "};\n";
    QByteArray expected =
        "#define FAKE_Q_OBJECT int bar() {return 5;}\n"
        "class Foo {\n"
        "    FAKE_Q_OBJECT\n"
        "    int f1();\n"
        "};\n"
        "\n"
        "int Foo::f1()\n"
        "{\n"
        "    return 1;\n"
        "}\n";

    MoveAllFuncDefOutside factory;
    QuickFixOperationTest(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_AssignToLocalVariable_templates()
{

    QList<QuickFixTestDocument::Ptr> testDocuments;
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
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

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
        "    int localFirst = list.first();\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    AssignToLocalVariable factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ExtractFunction_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("basic")
        << _("void f()\n"
             "{\n"
             "    @{start}g();@{end}\n"
             "}\n")
        << _("void extracted()\n"
             "{\n"
             "    g();\n"
             "}\n"
             "\n"
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
             "void Foo::extracted()\n"
             "{\n"
             "    g();\n"
             "}\n\n"
             "void Foo::bar()\n"
             "{\n"
             "    extracted();\n"
             "}\n");
}

void CppEditorPlugin::test_quickfix_ExtractFunction()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    QList<QuickFixTestDocument::Ptr> testDocuments;
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    ExtractFunction factory([]() { return QLatin1String("extracted"); });
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_typeDeduction_data()
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

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_typeDeduction()
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

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_freeFunction_separateFiles()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "void foo(const char *a, long b = 1);\n";
    expected =
        "void foo(const char *a, long b = 1, int newParameter = 156);\n";
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "void foo(const char *a, long b)\n"
        "{return 1@56 + 123 + 156;}\n";
    expected =
        "void foo(const char *a, long b, int newParameter)\n"
        "{return newParameter + 123 + newParameter;}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_memberFunction_separateFiles()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
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
    testDocuments << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n\n"
        "int Narf::zort()\n"
        "{ return 15@5 + 1; }\n";
    expected =
        "#include \"file.h\"\n\n"
        "int Narf::zort(int newParameter)\n"
        "{ return newParameter + 1; }\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_notTriggeringForInvalidCode()
{
    QList<QuickFixTestDocument::Ptr> testDocuments;
    QByteArray original;
    original =
        "T(\"test\")\n"
        "{\n"
        "    const int i = @14;\n"
        "}\n";
    testDocuments << QuickFixTestDocument::create("file.cpp", original, "");

    ExtractLiteralAsParameter factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ConvertQt4Connect_connectOutOfClass()
{
    QByteArray prefix =
        "class QObject {};\n"
        "class TestClass : public QObject\n"
        "{\n"
        "public:\n"
        "    void setProp(int) {}\n"
        "    void sigFoo(int) {}\n"
        "};\n"
        "\n"
        "int foo()\n"
        "{\n";

    QByteArray suffix = "\n}\n";

    QByteArray original = prefix
          + "    TestClass obj;\n"
            "    conne@ct(&obj, SIGNAL(sigFoo(int)), &obj, SLOT(setProp(int)));"
          + suffix;

    QByteArray expected = prefix
          + "    TestClass obj;\n"
            "    connect(&obj, &TestClass::sigFoo, &obj, &TestClass::setProp);"
          + suffix;

    QList<QuickFixTestDocument::Ptr> testDocuments;
    testDocuments << QuickFixTestDocument::create("file.cpp", original, expected);

    ConvertQt4Connect factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ConvertQt4Connect_connectWithinClass_data()
{
    QTest::addColumn<QByteArray>("original");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("four-args-connect")
            << QByteArray("conne@ct(this, SIGNAL(sigFoo(int)), this, SLOT(setProp(int)));")
            << QByteArray("connect(this, &TestClass::sigFoo, this, &TestClass::setProp);");

    QTest::newRow("four-args-disconnect")
            << QByteArray("disconne@ct(this, SIGNAL(sigFoo(int)), this, SLOT(setProp(int)));")
            << QByteArray("disconnect(this, &TestClass::sigFoo, this, &TestClass::setProp);");

    QTest::newRow("three-args-connect")
            << QByteArray("conne@ct(this, SIGNAL(sigFoo(int)), SLOT(setProp(int)));")
            << QByteArray("connect(this, &TestClass::sigFoo, this, &TestClass::setProp);");

    QTest::newRow("template-value")
            << QByteArray("Pointer<TestClass> p;\n"
                          "conne@ct(p.t, SIGNAL(sigFoo(int)), p.t, SLOT(setProp(int)));")
            << QByteArray("Pointer<TestClass> p;\n"
                          "connect(p.t, &TestClass::sigFoo, p.t, &TestClass::setProp);");

    QTest::newRow("implicit-pointer")
            << QByteArray("Pointer<TestClass> p;\n"
                          "conne@ct(p, SIGNAL(sigFoo(int)), p, SLOT(setProp(int)));")
            << QByteArray("Pointer<TestClass> p;\n"
                          "connect(p.data(), &TestClass::sigFoo, p.data(), &TestClass::setProp);");
}

void CppEditorPlugin::test_quickfix_ConvertQt4Connect_connectWithinClass()
{
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    QByteArray prefix =
        "template<class T>\n"
        "struct Pointer\n"
        "{\n"
        "    T *t;\n"
        "    operator T*() const { return t; }\n"
        "    T *data() const { return t; }\n"
        "};\n"
        "class QObject {};\n"
        "class TestClass : public QObject\n"
        "{\n"
        "public:\n"
        "    void setProp(int) {}\n"
        "    void sigFoo(int) {}\n"
        "    void setupSignals();\n"
        "};\n"
        "\n"
        "int TestClass::setupSignals()\n"
        "{\n";

    QByteArray suffix = "\n}\n";

    QList<QuickFixTestDocument::Ptr> testDocuments;
    testDocuments << QuickFixTestDocument::create("file.cpp",
                                              prefix + original + suffix,
                                              prefix + expected + suffix);

    ConvertQt4Connect factory;
    QuickFixOperationTest(testDocuments, &factory);
}

void CppEditorPlugin::test_quickfix_ConvertQt4Connect_differentNamespace()
{
    const QByteArray prefix =
        "namespace NsA {\n"
        "class ClassA : public QObject\n"
        "{\n"
        "  static ClassA *instance();\n"
        "signals:\n"
        "  void sig();\n"
        "};\n"
        "}\n"
        "\n"
        "namespace NsB {\n"
        "class ClassB : public QObject\n"
        "{\n"
        "  void slot();\n"
        "  void connector() {\n";

    const QByteArray suffix = "  }\n};\n}";

    const QByteArray original = "co@nnect(NsA::ClassA::instance(), SIGNAL(sig()),\n"
                                "        this, SLOT(slot()));\n";
    const QByteArray expected = "connect(NsA::ClassA::instance(), &NsA::ClassA::sig,\n"
                                "        this, &ClassB::slot);\n";
    QList<QuickFixTestDocument::Ptr> testDocuments;
    testDocuments << QuickFixTestDocument::create("file.cpp",
                                              prefix + original + suffix,
                                              prefix + expected + suffix);

    ConvertQt4Connect factory;
    QuickFixOperationTest(testDocuments, &factory);
}

} // namespace Internal
} // namespace CppEditor
