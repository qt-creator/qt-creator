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

#include <AST.h>
#include <Bind.h>
#include <Control.h>
#include <CppDocument.h>
#include <DiagnosticClient.h>
#include <Literals.h>
#include <Scope.h>
#include <Symbols.h>
#include <TranslationUnit.h>

#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditor.h>
#include <cppeditor/cppinsertdecldef.h>
#include <cppeditor/cppplugin.h>
#include <cppeditor/cppquickfixassistant.h>
#include <cppeditor/cppquickfix.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/iassistproposalmodel.h>
#include <texteditor/plaintexteditor.h>
#include <utils/changeset.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QTextDocument>
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
/**
 * Encapsulates the whole process of setting up an editor, getting the
 * quick-fix, applying it, and checking the result.
 */
struct TestCase
{
    QByteArray originalText;
    int pos;
    CPPEditor *editor;
    CPPEditorWidget *editorWidget;

    TestCase(const QByteArray &input);
    ~TestCase();

    QuickFixOperation::Ptr getFix(CppQuickFixFactory *factory);

    void run(CppQuickFixFactory *factory, const QByteArray &expected, bool changesExpected = true,
             int undoCount = 1);

private:
    TestCase(const TestCase &);
    TestCase &operator=(const TestCase &);
};

/// Apply the factory on the source and get back the first result or a null pointer.
QuickFixOperation::Ptr TestCase::getFix(CppQuickFixFactory *factory)
{
    CppQuickFixInterface qfi(new CppQuickFixAssistInterface(editorWidget, ExplicitlyInvoked));
    TextEditor::QuickFixOperations results;
    factory->match(qfi, results);
    return results.isEmpty() ? QuickFixOperation::Ptr() : results.first();
}

/// The '@' in the input is the position from where the quick-fix discovery is triggered.
TestCase::TestCase(const QByteArray &input)
    : originalText(input)
{
    pos = originalText.indexOf('@');
    QVERIFY(pos != -1);
    originalText.remove(pos, 1);
    QString fileName(QDir::tempPath() + QLatin1String("/file.cpp"));
    Utils::FileSaver srcSaver(fileName);
    srcSaver.write(originalText);
    srcSaver.finalize();
    CPlusPlus::CppModelManagerInterface::instance()->updateSourceFiles(QStringList()<<fileName);

    // wait for the parser in the future to give us the document:
    while (true) {
        Snapshot s = CPlusPlus::CppModelManagerInterface::instance()->snapshot();
        if (s.contains(fileName))
            break;
        QCoreApplication::processEvents();
    }

    editor = dynamic_cast<CPPEditor *>(EditorManager::openEditor(fileName));
    QVERIFY(editor);
    editor->setCursorPosition(pos);
    editorWidget = dynamic_cast<CPPEditorWidget *>(editor->editorWidget());
    QVERIFY(editorWidget);
    editorWidget->semanticRehighlight(true);

    // wait for the semantic info from the future:
    while (editorWidget->semanticInfo().doc.isNull())
        QCoreApplication::processEvents();
}

TestCase::~TestCase()
{
    EditorManager::instance()->closeEditors(QList<Core::IEditor *>() << editor,
                                            false);
    QCoreApplication::processEvents(); // process any pending events

    // Remove the test file from the code-model:
    CppModelManagerInterface *mmi = CPlusPlus::CppModelManagerInterface::instance();
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

void TestCase::run(CppQuickFixFactory *factory, const QByteArray &expected,
                   bool changesExpected, int undoCount)
{
    QuickFixOperation::Ptr fix = getFix(factory);
    if (!fix) {
        QVERIFY2(!changesExpected, "No QuickFixOperation");
        return;
    }

    fix->perform();
    QByteArray result = editorWidget->document()->toPlainText().toUtf8();
    removeTrailingWhitespace(result);

    QCOMPARE(QLatin1String(result), QLatin1String(expected));

    for (int i = 0; i < undoCount; ++i)
        editorWidget->undo();

    result = editorWidget->document()->toPlainText().toUtf8();
    QCOMPARE(result, originalText);
}
} // anonymous namespace

/// Checks:
/// 1. If the name does not start with ("m_" or "_") and does not
///    end with "_", we are forced to prefix the getter with "get".
/// 2. Setter: Use pass by value on integer/float and pointer types.
void CppPlugin::test_quickfix_GetterSetter_basicGetterWithPrefix()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    int @it;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Checks:
/// 1. Getter: "get" prefix is not necessary.
/// 2. Setter: Parameter name is base name.
void CppPlugin::test_quickfix_GetterSetter_basicGetterWithoutPrefix()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    int @m_it;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Check: Setter: Use pass by reference for parameters which
/// are not integer, float or pointers.
void CppPlugin::test_quickfix_GetterSetter_customType()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    MyType @it;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Checks:
/// 1. Setter: No setter is generated for const members.
/// 2. Getter: Return a non-const type since it pass by value anyway.
void CppPlugin::test_quickfix_GetterSetter_constMember()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    const int @it;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Checks: No special treatment for pointer to non const.
void CppPlugin::test_quickfix_GetterSetter_pointerToNonConst()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    int *it@;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Checks: No special treatment for pointer to const.
void CppPlugin::test_quickfix_GetterSetter_pointerToConst()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    const int *it@;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Checks:
/// 1. Setter: Setter is a static function.
/// 2. Getter: Getter is a static, non const function.
void CppPlugin::test_quickfix_GetterSetter_staticMember()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    static int @m_member;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Check: Check if it works on the second declarator
void CppPlugin::test_quickfix_GetterSetter_secondDeclarator()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    int *foo, @it;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Check: Quick fix is offered for "int *@it;" ('@' denotes the text cursor position)
void CppPlugin::test_quickfix_GetterSetter_triggeringRightAfterPointerSign()
{
    TestCase data("\n"
                  "class Something\n"
                  "{\n"
                  "    int *@it;\n"
                  "};\n"
                  );
    QByteArray expected = "\n"
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

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected);
}

/// Check: Quick fix is not triggered on a member function.
void CppPlugin::test_quickfix_GetterSetter_notTriggeringOnMemberFunction()
{
    TestCase data("class Something { void @f(); };");
    QByteArray expected = data.originalText;

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected, /*changesExpected=*/ false);
}

/// Check: Quick fix is not triggered on an member array;
void CppPlugin::test_quickfix_GetterSetter_notTriggeringOnMemberArray()
{
    TestCase data("class Something { void @a[10]; };");
    QByteArray expected = data.originalText;

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected, /*changesExpected=*/ false);
}

/// Check: Do not offer the quick fix if there is already a member with the
/// getter or setter name we would generate.
void CppPlugin::test_quickfix_GetterSetter_notTriggeringWhenGetterOrSetterExist()
{
    TestCase data("\n"
                  "class Something {\n"
                  "     int @it;\n"
                  "     void setIt();\n"
                  "};\n");
    QByteArray expected = data.originalText;

    GetterSetter factory(/*testMode=*/ true);
    data.run(&factory, expected, /*changesExpected=*/ false);
}
