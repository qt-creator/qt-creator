/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
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
#include "cppeditortestcase.h"
#include "cppquickfixassistant.h"
#include "cppquickfixes.h"
#include "cppquickfix_test.h"

#include <cpptools/cppcodestylepreferences.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpppreprocessertesthelper.h>
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
using namespace CppEditor::Internal::Tests;
using namespace CppTools;
using namespace IncludeUtils;
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
    , m_expectedSource(expectedSource)
{
    if (m_cursorPosition > -1)
        m_source.remove(m_cursorPosition, 1);

    const int cursorPositionInExpectedSource = m_expectedSource.indexOf(m_cursorMarker);
    if (cursorPositionInExpectedSource > -1)
        m_expectedSource.remove(cursorPositionInExpectedSource, 1);
}

QList<QuickFixTestDocument::Ptr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected)
{
    return QList<QuickFixTestDocument::Ptr>()
            << QuickFixTestDocument::create("file.cpp", original, expected);
}

/// Leading whitespace is not removed, so we can check if the indetation ranges
/// have been set correctly by the quick-fix.
static QByteArray &removeTrailingWhitespace(QByteArray &input)
{
    QList<QByteArray> lines = input.split('\n');
    input.resize(0);
    for (int i = 0, total = lines.size(); i < total; ++i) {
        QByteArray line = lines.at(i);
        while (line.length() > 0) {
            char lastChar = line[line.length() - 1];
            if (lastChar == ' ' || lastChar == '\t')
                line.chop(1);
            else
                break;
        }
        input.append(line);

        const bool isLastLine = i == lines.size() - 1;
        if (!isLastLine)
            input.append('\n');
    }
    return input;
}

} // namespace Tests
} // namespace Internal
} // namespace CppEditor

/// The '@' in the originalSource is the position from where the quick-fix discovery is triggered.
/// Exactly one TestFile must contain the cursor position marker '@' in the originalSource.
QuickFixTestCase::QuickFixTestCase(const QList<QuickFixTestDocument::Ptr> &theTestFiles,
                                   CppQuickFixFactory *factory,
                                   const QStringList &includePaths,
                                   int resultIndex)
    : m_testFiles(theTestFiles)
    , m_cppCodeStylePreferences(0)
    , m_restoreIncludePaths(false)
{
    QVERIFY(succeededSoFar());

    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    foreach (const QuickFixTestDocument::Ptr testFile, m_testFiles) {
        if (testFile->hasCursorMarker())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write files to disk
    foreach (QuickFixTestDocument::Ptr testFile, m_testFiles)
        testFile->writeToDisk();

    // Set appropriate include paths
    if (!includePaths.isEmpty()) {
        m_restoreIncludePaths = true;
        m_includePathsToRestore = m_modelManager->includePaths();
        m_modelManager->setIncludePaths(includePaths);
    }

    // Update Code Model
    QStringList filePaths;
    foreach (const QuickFixTestDocument::Ptr &testFile, m_testFiles)
        filePaths << testFile->filePath();
    QVERIFY(parseFiles(filePaths));

    // Open Files
    foreach (QuickFixTestDocument::Ptr testFile, m_testFiles) {
        QVERIFY(openCppEditor(testFile->filePath(), &testFile->m_editor,
                              &testFile->m_editorWidget));
        closeEditorAtEndOfTestCase(testFile->m_editor);

        // Set cursor position
        const int cursorPosition = testFile->hasCursorMarker()
                ? testFile->m_cursorPosition : 0;
        testFile->m_editor->setCursorPosition(cursorPosition);

        // Rehighlight
        waitForRehighlightedSemanticDocument(testFile->m_editorWidget);
    }

    // Enforce the default cpp code style, so we are independent of config file settings.
    // This is needed by e.g. the GenerateGetterSetter quick fix.
    m_cppCodeStylePreferences = CppToolsSettings::instance()->cppCodeStyle();
    QVERIFY(m_cppCodeStylePreferences);
    m_cppCodeStylePreferencesOriginalDelegateId = m_cppCodeStylePreferences->currentDelegateId();
    m_cppCodeStylePreferences->setCurrentDelegate("qt");

    // Run the fix in the file having the cursor marker
    QuickFixTestDocument::Ptr testFile;
    foreach (const QuickFixTestDocument::Ptr file, m_testFiles) {
        if (file->hasCursorMarker())
            testFile = file;
    }
    QVERIFY2(testFile, "No test file with cursor marker found");

    if (QuickFixOperation::Ptr fix = getFix(factory, testFile->m_editorWidget, resultIndex))
        fix->perform();
    else
        qDebug() << "Quickfix was not triggered";

    // Compare all files
    foreach (const QuickFixTestDocument::Ptr testFile, m_testFiles) {
        // Check
        QByteArray result = testFile->m_editorWidget->document()->toPlainText().toUtf8();
        removeTrailingWhitespace(result);
        QCOMPARE(QLatin1String(result), QLatin1String(testFile->m_expectedSource));

        // Undo the change
        for (int i = 0; i < 100; ++i)
            testFile->m_editorWidget->undo();
        result = testFile->m_editorWidget->document()->toPlainText().toUtf8();
        QCOMPARE(result, testFile->m_source);
    }
}

QuickFixTestCase::~QuickFixTestCase()
{
    // Restore default cpp code style
    if (m_cppCodeStylePreferences)
        m_cppCodeStylePreferences->setCurrentDelegate(m_cppCodeStylePreferencesOriginalDelegateId);

    // Restore include paths
    if (m_restoreIncludePaths)
        m_modelManager->setIncludePaths(m_includePathsToRestore);

    // Remove created files from file system
    foreach (const QuickFixTestDocument::Ptr &testDocument, m_testFiles)
        QVERIFY(QFile::remove(testDocument->filePath()));
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

/// Apply the factory on the source and get back the resultIndex'th result or a null pointer.
QSharedPointer<TextEditor::QuickFixOperation> QuickFixTestCase::getFix(
        CppQuickFixFactory *factory, CPPEditorWidget *editorWidget, int resultIndex)
{
    CppQuickFixInterface qfi(new CppQuickFixAssistInterface(editorWidget, ExplicitlyInvoked));
    TextEditor::QuickFixOperations results;
    factory->match(qfi, results);
    return results.isEmpty() ? QuickFixOperation::Ptr() : results.at(resultIndex);
}

typedef QSharedPointer<CppQuickFixFactory> CppQuickFixFactoryPtr;

Q_DECLARE_METATYPE(CppQuickFixFactoryPtr)

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

    // Check: Do not offer the quick fix if there is already a member with the
    // getter or setter name we would generate.
    QTest::newRow("GenerateGetterSetter_notTriggeringWhenGetterOrSetterExist")
        << CppQuickFixFactoryPtr(new GenerateGetterSetter) << _(
        "class Something {\n"
        "     int @it;\n"
        "     void setIt();\n"
        "};\n"
        ) << _();

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
        << _("int i; void foo() {f@or (double a = 0; i < 3 + 5; ++i) {}}\n");

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
}

void CppEditorPlugin::test_quickfix()
{
    QFETCH(CppQuickFixFactoryPtr, factory);
    QFETCH(QByteArray, original);
    QFETCH(QByteArray, expected);

    if (expected.isEmpty())
        expected = original;
    QuickFixTestCase(singleDocument(original, expected), factory.data());
}

/// Checks: In addition to test_quickfix_GenerateGetterSetter_basicGetterWithPrefix
/// generated definitions should fit in the namespace.
void CppEditorPlugin::test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespaceToCpp()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
        "}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    GenerateGetterSetter factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check if definition is inserted right after class for insert definition outside
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_afterClass()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory, QStringList(), 1);
}

/// Check from header file: If there is a source file, insert the definition in the source file.
/// Case: Source file is empty.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic1()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "struct Foo\n"
        "{\n"
        "    Foo()@;\n"
        "};\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n"
        ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check from header file: If there is a source file, insert the definition in the source file.
/// Case: Source file is not empty.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic2()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
            "\n"
            "void f()\n"
            "{\n"
            "\n"
            "}\n"
            ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check from source file: Insert in source file, not header file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_basic3()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Empty Header File
    testFiles << QuickFixTestDocument::create("file.h", "", "");

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
        ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check from header file: If the class is in a namespace, the added function definition
/// name must be qualified accordingly.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace1()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original.resize(0);
    expected =
        "\n"
        "N::Foo::Foo()\n"
        "{\n\n"
        "}\n"
        ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check from header file: If the class is in namespace N and the source file has a
/// "using namespace N" line, the function definition name must be qualified accordingly.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_headerSource_namespace2()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
            ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory, QStringList(), 1);
}

/// Check not triggering when definition exists
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_notTriggeringWhenDefinitionExists()
{
    const QByteArray original =
            "class Foo {\n"
            "    void b@ar();\n"
            "};\n"
            "void Foo::bar() {}\n";
    const QByteArray expected = original;

    InsertDefFromDecl factory;
    QuickFixTestCase test(singleDocument(original, expected), &factory, QStringList(), 1);
}

/// Find right implementation file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_findRightImplementationFile()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File #1
    original =
            "#include \"file.h\"\n"
            "\n"
            "Foo::Foo()\n"
            "{\n\n"
            "}\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);


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
    testFiles << QuickFixTestDocument::create("file2.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Ignore generated functions declarations when looking at the surrounding
/// functions declarations in order to find the right implementation file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_ignoreSurroundingGeneratedDeclarations()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    // Source File #2
    original =
            "#include \"file.h\"\n"
            "\n"
            "void Foo::hidden()\n"
            "{\n\n"
            "}\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file2.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
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
        "}\n";

    InsertDefFromDecl factory;
    QuickFixTestCase(singleDocument(original, expected), &factory);
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
        "}\n";

    InsertDefFromDecl factory;
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check if a function like macro use is not separated by the function to insert
/// Case: Macro preceded by preproceesor directives and declaration.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile1()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check if a function like macro use is not separated by the function to insert
/// Case: Marco preceded only by preprocessor directives.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile2()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check if insertion happens before syntactically erroneous statements at end of file.
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_erroneousStatementAtEndOfFile()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f()@;\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: Respect rvalue references
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_rvalueReference()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "void f(Foo &&)@;\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original = "";
    expected =
            "\n"
            "void f(Foo &&)\n"
            "{\n"
            "\n"
            "}\n"
            ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Find right implementation file. (QTCREATORBUG-10728)
void CppEditorPlugin::test_quickfix_InsertDefFromDecl_findImplementationFile()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
            "#include \"file.h\"\n"
            ;
    expected =
            "#include \"file.h\"\n"
            "\n"
            "\n"
            "void Foo::baz()\n"
            "{\n"
            "\n"
            "}\n"
            ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDefFromDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

// Function for one of InsertDeclDef section cases
void insertToSectionDeclFromDef(const QByteArray &section, int sectionIndex)
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo@()\n"
        "{\n"
        "}\n"
        ;
    expected = original;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    InsertDeclFromDef factory;
    QuickFixTestCase(testFiles, &factory, QStringList(), sectionIndex);
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
    const QString fileName = TestIncludePaths::testFilePath();
    CppTools::Tests::TestCase::writeFile(fileName, source);

    using namespace CppTools::Internal;

    CppModelManager *cmm = CppModelManager::instance();
    cmm->GC();
    CppPreprocessor pp((QPointer<CppModelManager>(cmm)));
    pp.setIncludePaths(QStringList(TestIncludePaths::globalIncludePath()));
    pp.run(fileName);

    Document::Ptr document = cmm->snapshot().document(fileName);
    return document->resolvedIncludes();
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
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    // Header File
    original = "class Foo {};\n";
    expected = original;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8() + "/afile.h",
                                      original, expected);

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
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/afile.cpp", original, expected);

    // Do not use the test factory, at least once we want to go through the "full stack".
    AddIncludeForUndefinedIdentifier factory;
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Ignore *.moc includes
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_ignoremoc()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert include at top for a sorted group
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_sortingTop()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert include in the middle for a sorted group
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_sortingMiddle()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert include at bottom for a sorted group
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_sortingBottom()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: For an unsorted group the new include is appended
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_appendToUnsorted()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8() +
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert a local include at front if there are only global includes
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_firstLocalIncludeAtFront()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert a global include at back if there are only local includes
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_firstGlobalIncludeAtBack()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<file.h>"));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Prefer group with longest matching prefix
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_preferGroupWithLongerMatchingPrefix()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"prefixc.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Create a new include group if there are only include groups with a different include dir
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_newGroupIfOnlyDifferentIncludeDirs()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Include group with mixed include dirs, sorted --> insert properly
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedDirsSorted()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<firstlib/file.h>"));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Include group with mixed include dirs, unsorted --> append
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedDirsUnsorted()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<lastlib/file.h>"));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes1()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"z.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes2()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"a.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes3()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"lib/file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Include group with mixed include types
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes4()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        "\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("<lib/file.h>"));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert very first include
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_noinclude()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "void @f();\n"
        ;
    expected =
        "#include \"file.h\"\n"
        "\n"
        "void f();\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert very first include if there is a c++ style comment on top
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_veryFirstIncludeCppStyleCommentOnTop()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: Insert very first include if there is a c style comment on top
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_veryFirstIncludeCStyleCommentOnTop()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

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
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifierTestFactory factory(QLatin1String("\"file.h\""));
    QuickFixTestCase(testFiles, &factory, QStringList(TestIncludePaths::globalIncludePath()));
}

/// Check: If a "Qt Class" was not found by the locator, check the header files in the Qt
/// include paths
void CppEditorPlugin::test_quickfix_AddIncludeForUndefinedIdentifier_checkQSomethingInQtIncludePaths()
{
    QList<QuickFixTestDocument::Ptr> testFiles;

    QByteArray original;
    QByteArray expected;

    original =
        "@QDir dir;\n"
        ;
    expected =
        "#include <QDir>\n"
        "\n"
        "QDir dir;\n"
        ;
    testFiles << QuickFixTestDocument::create(TestIncludePaths::directoryOfTestFile().toUtf8()
                                      + "/file.cpp", original, expected);

    AddIncludeForUndefinedIdentifier factory;
    QuickFixTestCase(testFiles,&factory,
                     QStringList(CppTools::Tests::TestIncludePaths::globalQtCoreIncludePath()));
}

/// Check: Move definition from header to cpp.
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
        ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
}

void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppInsideNS()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check: Move definition outside class
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncOutside2()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "void Foo::f1() {}\n"
        "void Foo::f3() {}\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory, QStringList(), 1);
}

/// Check: Move definition from header to cpp (with namespace).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
        "}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: Move definition from header to cpp (with namespace + using).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
        "}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check: Move free function from header to cpp.
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_FreeFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
        "}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: Move free function from header to cpp (with namespace).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
        "}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: Move Ctor with member initialization list (QTCREATORBUG-9157).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_CtorWithInitialization1()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original ="#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "Foo::Foo() : a(42), b(3.141) {}\n"
       ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: Move Ctor with member initialization list (QTCREATORBUG-9462).
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_CtorWithInitialization2()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original ="#include \"file.h\"\n";
    expected =
        "#include \"file.h\"\n"
        "\n"
        "\n"
        "Foo::Foo() : member(2)\n"
        "{\n"
        "}\n"
       ;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check if definition is inserted right after class for move definition outside
void CppEditorPlugin::test_quickfix_MoveFuncDefOutside_afterClass()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::Foo()\n"
        "{\n\n"
        "}\n";
    expected = original;
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefOutside factory;
    QuickFixTestCase(testFiles, &factory, QStringList(), 1);
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
       ;

    MoveFuncDefOutside factory;
    QuickFixTestCase(singleDocument(original, expected), &factory);
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
       ;

    MoveFuncDefOutside factory;
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCpp()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFunc()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int Foo::num@ber() const {return 5;}\n";
    expected =
        "#include \"file.h\"\n"
        "\n\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "int MyNs::Foo::num@ber() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    expected = "#include \"file.h\"\n\n\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNSUsing()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_FreeFuncToCpp()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_FreeFuncToCpp()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original = "int number() const;\n";
    expected =
        "int number() const\n"
        "{\n"
        "    return 5;\n"
        "}\n";
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_FreeFuncToCppNS()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixTestCase(testFiles, &factory);
}

/// Check: revert test_quickfix_MoveFuncDefOutside_CtorWithInitialization()
void CppEditorPlugin::test_quickfix_MoveFuncDefToDecl_CtorWithInitialization()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n"
        "\n"
        "Foo::F@oo() : a(42), b(3.141) {}"
        ;
    expected ="#include \"file.h\"\n\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    MoveFuncDefToDecl factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_AssignToLocalVariable_templates()
{

    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

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
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    AssignToLocalVariable factory;
    QuickFixTestCase(testFiles, &factory);
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
    QuickFixTestCase(singleDocument(original, expected), &factory);
}

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_freeFunction_separateFiles()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
    QByteArray original;
    QByteArray expected;

    // Header File
    original =
        "void foo(const char *a, long b = 1);\n";
    expected =
        "void foo(const char *a, long b = 1, int newParameter = 156);\n";
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "void foo(const char *a, long b)\n"
        "{return 1@56 + 123 + 156;}\n";
    expected =
        "void foo(const char *a, long b, int newParameter)\n"
        "{return newParameter + 123 + newParameter;}\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    ExtractLiteralAsParameter factory;
    QuickFixTestCase(testFiles, &factory);
}

void CppEditorPlugin::test_quickfix_ExtractLiteralAsParameter_memberFunction_separateFiles()
{
    QList<QuickFixTestDocument::Ptr> testFiles;
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
    testFiles << QuickFixTestDocument::create("file.h", original, expected);

    // Source File
    original =
        "#include \"file.h\"\n\n"
        "int Narf::zort()\n"
        "{ return 15@5 + 1; }\n";
    expected =
        "#include \"file.h\"\n\n"
        "int Narf::zort(int newParameter)\n"
        "{ return newParameter + 1; }\n";
    testFiles << QuickFixTestDocument::create("file.cpp", original, expected);

    ExtractLiteralAsParameter factory;
    QuickFixTestCase(testFiles, &factory);
}
