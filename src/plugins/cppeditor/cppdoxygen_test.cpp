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

#include <coreplugin/editormanager/editormanager.h>
#include <cplusplus/CppDocument.h>
#include <cppeditor/cppeditor.h>
#include <cppeditor/cppeditorplugin.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <utils/fileutils.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QString>
#include <QtTest>

/*!
    Tests for inserting doxygen comments.
 */
using namespace Core;
using namespace CPlusPlus;
using namespace CppEditor::Internal;

namespace {
/**
 * Encapsulates the whole process of setting up an editor,
 * pressing ENTER and checking the result.
 */
struct TestCase
{
    QByteArray originalText;
    int pos;
    CPPEditor *editor;
    CPPEditorWidget *editorWidget;

    TestCase(const QByteArray &input);
    ~TestCase();

    void run(const QByteArray &expected, int undoCount = 1);

private:
    TestCase(const TestCase &);
    TestCase &operator=(const TestCase &);
};

/// The '|' in the input denotes the cursor position.
TestCase::TestCase(const QByteArray &input)
    : originalText(input)
{
    pos = originalText.indexOf('|');
    QVERIFY(pos != -1);
    originalText.remove(pos, 1);
    QString fileName(QDir::tempPath() + QLatin1String("/file.cpp"));
    Utils::FileSaver srcSaver(fileName);
    srcSaver.write(originalText);
    srcSaver.finalize();
    CppTools::CppModelManagerInterface::instance()->updateSourceFiles(QStringList()<<fileName);

    // Wait for the parser in the future to give us the document
    while (true) {
        Snapshot s = CppTools::CppModelManagerInterface::instance()->snapshot();
        if (s.contains(fileName))
            break;
        QCoreApplication::processEvents();
    }

    editor = dynamic_cast<CPPEditor *>(EditorManager::openEditor(fileName));
    QVERIFY(editor);
    editorWidget = dynamic_cast<CPPEditorWidget *>(editor->editorWidget());
    QVERIFY(editorWidget);

    // We want to test documents that start with a comment. By default, the
    // editor will fold the very first comment it encounters, assuming
    // it is a license header. Currently unfoldAll() does not work as
    // expected (some blocks are still hidden in some test cases, so the
    // cursor movements are not as expected). For the time being, we just
    // prepend a declaration before the initial test comment.
//    editorWidget->unfoldAll();
    editor->setCursorPosition(pos);

    editorWidget->semanticRehighlight(true);
    // Wait for the semantic info from the future:
    while (editorWidget->semanticInfo().doc.isNull())
        QCoreApplication::processEvents();
}

TestCase::~TestCase()
{
    EditorManager::instance()->closeEditors(QList<Core::IEditor *>() << editor, false);
    QCoreApplication::processEvents(); // process any pending events

    // Remove the test file from the code-model
    CppTools::CppModelManagerInterface *mmi = CppTools::CppModelManagerInterface::instance();
    mmi->GC();
    QCOMPARE(mmi->snapshot().size(), 0);
}

void TestCase::run(const QByteArray &expected, int undoCount)
{
    // Send 'ENTER' key press
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    QCoreApplication::sendEvent(editorWidget, &event);
    const QByteArray result = editorWidget->document()->toPlainText().toUtf8();

    QCOMPARE(QLatin1String(result), QLatin1String(expected));

    for (int i = 0; i < undoCount; ++i)
        editorWidget->undo();
    const QByteArray contentsAfterUndo = editorWidget->document()->toPlainText().toUtf8();
    QCOMPARE(contentsAfterUndo, originalText);
}
} // anonymous namespace

void CppEditorPlugin::test_doxygen_comments_qt_style()
{
    const QByteArray given =
        "bool preventFolding;\n"
        "/*!|\n"
        "int a;\n"
        ;
    const QByteArray expected =
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a\n"
        " */\n"
        "int a;\n"
        ;

    TestCase data(given);
    data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_qt_style_continuation()
{
    const QByteArray given =
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a|\n"
        " */\n"
        "int a;\n"
        ;
    const QByteArray expected =
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a\n"
        " *\n"
        " */\n"
        "int a;\n"
        ;

    TestCase data(given);
    data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_java_style()
{
    const QByteArray given =
        "bool preventFolding;\n"
        "/**|\n"
        "int a;\n"
        ;
    const QByteArray expected =
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a\n"
        " */\n"
        "int a;\n"
        ;

    TestCase data(given);
    data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_java_style_continuation()
{
    const QByteArray given =
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a|\n"
        " */\n"
        "int a;\n"
        ;
    const QByteArray expected =
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a\n"
        " *\n"
        " */\n"
        "int a;\n"
        ;

    TestCase data(given);
    data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_cpp_styleA()
{
   const QByteArray given =
         "bool preventFolding;\n"
         "///|\n"
         "int a;\n"
         ;

   const QByteArray expected =
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a\n"
         "///\n"
         "int a;\n"
         ;
   TestCase data(given);
   data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_cpp_styleB()
{
   const QByteArray given =
         "bool preventFolding;\n"
         "//!|\n"
         "int a;\n"
         ;

   const QByteArray expected =
         "bool preventFolding;\n"
         "//!\n"
         "//! \\brief a\n"
         "//!\n"
         "int a;\n"
         ;
   TestCase data(given);
   data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_cpp_styleA_continuation()
{
   const QByteArray given =
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a|\n"
         "///\n"
         "int a;\n"
         ;
   const QByteArray expected =
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a\n"
         "///\n"
         "///\n"
         "int a;\n"
         ;

   TestCase data(given);
   data.run(expected);
}

/// test cpp style doxygen comment when inside a indented scope
void CppEditorPlugin::test_doxygen_comments_cpp_styleA_indented()
{
   const QByteArray given =
         "    bool preventFolding;\n"
         "    ///|\n"
         "    int a;\n"
         ;

   const QByteArray expected =
         "    bool preventFolding;\n"
         "    ///\n"
         "    /// \\brief a\n"
         "    ///\n"
         "    int a;\n"
         ;
   TestCase data(given);
   data.run(expected);
}

/// test cpp style doxygen comment continuation when inside a indented scope
void CppEditorPlugin::test_doxygen_comments_cpp_styleA_indented_continuation()
{
   const QByteArray given =
         "    bool preventFolding;\n"
         "    ///\n"
         "    /// \\brief a|\n"
         "    ///\n"
         "    int a;\n"
         ;
   const QByteArray expected =
         "    bool preventFolding;\n"
         "    ///\n"
         "    /// \\brief a\n"
         "    ///\n"
         "    ///\n"
         "    int a;\n"
         ;

   TestCase data(given);
   data.run(expected);
}

void CppEditorPlugin::test_doxygen_comments_cpp_styleA_corner_case()
{
    const QByteArray given =
          "bool preventFolding;\n"
          "///\n"
          "void d(); ///|\n"
          ;
    const QByteArray expected =
            "bool preventFolding;\n"
            "///\n"
            "void d(); ///\n"
            "\n"
          ;

    TestCase data(given);
    data.run(expected);
}
