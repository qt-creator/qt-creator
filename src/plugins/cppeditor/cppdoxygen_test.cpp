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

#include "cppeditorplugin.h"
#include "cppeditortestcase.h"

#include <coreplugin/editormanager/editormanager.h>
#include <cpptools/cppmodelmanagerinterface.h>

#include <cplusplus/CppDocument.h>
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

namespace { typedef QByteArray _; }

/**
 * Encapsulates the whole process of setting up an editor,
 * pressing ENTER and checking the result.
 */

namespace CppEditor {
namespace Internal {
namespace Tests {

class DoxygenTestCase : public Internal::Tests::TestCase
{
public:
    /// The '|' in the input denotes the cursor position.
    DoxygenTestCase(const QByteArray &original, const QByteArray &expected)
    {
        QVERIFY(succeededSoFar());

        TestDocument testDocument("file.cpp", original, '|');
        QVERIFY(testDocument.hasCursorMarker());
        testDocument.m_source.remove(testDocument.m_cursorPosition, 1);
        QVERIFY(testDocument.writeToDisk());

        // Update Code Model
        QVERIFY(parseFiles(testDocument.filePath()));

        // Open Editor
        QVERIFY(openCppEditor(testDocument.filePath(), &testDocument.m_editor,
                              &testDocument.m_editorWidget));
        closeEditorAtEndOfTestCase(testDocument.m_editor);

        // We want to test documents that start with a comment. By default, the
        // editor will fold the very first comment it encounters, assuming
        // it is a license header. Currently unfoldAll() does not work as
        // expected (some blocks are still hidden in some test cases, so the
        // cursor movements are not as expected). For the time being, we just
        // prepend a declaration before the initial test comment.
        //    testDocument.m_editorWidget->unfoldAll();
        testDocument.m_editor->setCursorPosition(testDocument.m_cursorPosition);

        waitForRehighlightedSemanticDocument(testDocument.m_editorWidget);

        // Send 'ENTER' key press
        QKeyEvent event(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
        QCoreApplication::sendEvent(testDocument.m_editorWidget, &event);
        const QByteArray result = testDocument.m_editorWidget->document()->toPlainText().toUtf8();

        QCOMPARE(QLatin1String(result), QLatin1String(expected));

        testDocument.m_editorWidget->undo();
        const QString contentsAfterUndo = testDocument.m_editorWidget->document()->toPlainText();
        QCOMPARE(contentsAfterUndo, testDocument.m_source);
    }
};

} // namespace Tests

void CppEditorPlugin::test_doxygen_comments_data()
{
    QTest::addColumn<QByteArray>("given");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("qt_style") << _(
        "bool preventFolding;\n"
        "/*!|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a\n"
        " */\n"
        "int a;\n"
    );

    QTest::newRow("qt_style_continuation") << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a|\n"
        " */\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a\n"
        " * \n"
        " */\n"
        "int a;\n"
    );

    QTest::newRow("java_style") << _(
        "bool preventFolding;\n"
        "/**|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a\n"
        " */\n"
        "int a;\n"
    );

    QTest::newRow("java_style_continuation") << _(
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a|\n"
        " */\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a\n"
        " * \n"
        " */\n"
        "int a;\n"
    );

    QTest::newRow("cpp_styleA") << _(
         "bool preventFolding;\n"
         "///|\n"
         "int a;\n"
        ) << _(
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a\n"
         "///\n"
         "int a;\n"
    );

    QTest::newRow("cpp_styleB") << _(
         "bool preventFolding;\n"
         "//!|\n"
         "int a;\n"
        ) << _(
         "bool preventFolding;\n"
         "//!\n"
         "//! \\brief a\n"
         "//!\n"
         "int a;\n"
    );

    QTest::newRow("cpp_styleA_continuation") << _(
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a|\n"
         "///\n"
         "int a;\n"
        ) << _(
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a\n"
         "/// \n"
         "///\n"
         "int a;\n"
     );

    /// test cpp style doxygen comment when inside a indented scope
    QTest::newRow("cpp_styleA_indented") << _(
         "    bool preventFolding;\n"
         "    ///|\n"
         "    int a;\n"
        ) << _(
         "    bool preventFolding;\n"
         "    ///\n"
         "    /// \\brief a\n"
         "    ///\n"
         "    int a;\n"
    );

    /// test cpp style doxygen comment continuation when inside a indented scope
    QTest::newRow("cpp_styleA_indented_continuation") << _(
         "    bool preventFolding;\n"
         "    ///\n"
         "    /// \\brief a|\n"
         "    ///\n"
         "    int a;\n"
        ) << _(
         "    bool preventFolding;\n"
         "    ///\n"
         "    /// \\brief a\n"
         "    /// \n"
         "    ///\n"
         "    int a;\n"
    );

    QTest::newRow("cpp_styleA_corner_case") << _(
          "bool preventFolding;\n"
          "///\n"
          "void d(); ///|\n"
        ) << _(
            "bool preventFolding;\n"
            "///\n"
            "void d(); ///\n"
            "\n"
    );
}

void CppEditorPlugin::test_doxygen_comments()
{
    QFETCH(QByteArray, given);
    QFETCH(QByteArray, expected);
    Tests::DoxygenTestCase(given, expected);
}

} // namespace Internal
} // namespace CppEditor
