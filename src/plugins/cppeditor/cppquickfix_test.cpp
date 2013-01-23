/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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
#include <Control.h>
#include <CppDocument.h>
#include <DiagnosticClient.h>
#include <Scope.h>
#include <TranslationUnit.h>
#include <Literals.h>
#include <Bind.h>
#include <Symbols.h>
#include <utils/changeset.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/plaintexteditor.h>
#include <texteditor/codeassist/iassistproposal.h>
#include <texteditor/codeassist/iassistproposalmodel.h>
#include <texteditor/codeassist/basicproposalitemlistmodel.h>
#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditor.h>
#include <cppeditor/cppplugin.h>
#include <cppeditor/cppquickfix.h>
#include <cppeditor/cppquickfixassistant.h>
#include <cppeditor/cppinsertdecldef.h>
#include <extensionsystem/pluginmanager.h>
#include <utils/fileutils.h>

#include <QtTest>
#include <QDebug>
#include <QTextDocument>
#include <QDir>

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

    void run(CppQuickFixFactory *factory, const QByteArray &expected, int undoCount = 1);

private:
    TestCase(const TestCase &);
    TestCase &operator=(const TestCase &);
};

/// apply the factory on the source, and get back the first result.
QuickFixOperation::Ptr TestCase::getFix(CppQuickFixFactory *factory)
{
    CppQuickFixInterface qfi(new CppQuickFixAssistInterface(editorWidget, ExplicitlyInvoked));
    TextEditor::QuickFixOperations results;
    factory->match(qfi, results);
    Q_ASSERT(!results.isEmpty());
    return results.first();
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

void TestCase::run(CppQuickFixFactory *factory, const QByteArray &expected, int undoCount)
{
    QuickFixOperation::Ptr fix = getFix(factory);
    fix->perform();
    QByteArray result = editorWidget->document()->toPlainText().toUtf8();
    removeTrailingWhitespace(result);

    QCOMPARE(result, expected);

    for (int i = 0; i < undoCount; ++i)
        editorWidget->undo();

    result = editorWidget->document()->toPlainText().toUtf8();
    QCOMPARE(result, originalText);
}
} // anonymous namespace

void CppPlugin::test_quickfix_GetterSetter()
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
            "    int it() const;\n"
            "    void setIt(int value);\n"
            "};\n"
            "\n"
            "int Something::it() const\n"
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

    GetterSetter factory;
    data.run(&factory, expected);
}
