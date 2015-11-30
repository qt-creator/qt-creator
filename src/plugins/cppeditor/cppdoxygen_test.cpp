/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cppdoxygen_test.h"

#include "cppeditortestcase.h"

#include <cpptools/cpptoolssettings.h>

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QtTest>

namespace { typedef QByteArray _; }

namespace CppEditor {
namespace Internal {
namespace Tests {

void DoxygenTest::initTestCase()
{
    verifyCleanState();
}

void DoxygenTest::cleanTestCase()
{
    verifyCleanState();
}

void DoxygenTest::cleanup()
{
    if (oldSettings)
        CppTools::CppToolsSettings::instance()->setCommentsSettings(*oldSettings);
    QVERIFY(Core::EditorManager::closeAllEditors(false));
    QVERIFY(TestCase::garbageCollectGlobalSnapshot());
}

void DoxygenTest::testBasic_data()
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

    QTest::newRow("cpp_styleB_continuation") << _(
         "bool preventFolding;\n"
         "//!\n"
         "//! \\brief a|\n"
         "//!\n"
         "int a;\n"
        ) << _(
         "bool preventFolding;\n"
         "//!\n"
         "//! \\brief a\n"
         "//! \n"
         "//!\n"
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

    QTest::newRow("cpp_styleB_indented") << _(
         "    bool preventFolding;\n"
         "    //!|\n"
         "    int a;\n"
        ) << _(
         "    bool preventFolding;\n"
         "    //!\n"
         "    //! \\brief a\n"
         "    //!\n"
         "    int a;\n"
    );

    QTest::newRow("cpp_styleA_indented_preserve_mixed_indention_continuation") << _(
         "\t bool preventFolding;\n"
         "\t /// \brief a|\n"
         "\t int a;\n"
        ) << _(
         "\t bool preventFolding;\n"
         "\t /// \brief a\n"
         "\t /// \n"
         "\t int a;\n"
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

    QTest::newRow("cpp_styleB_indented_continuation") << _(
         "    bool preventFolding;\n"
         "    //!\n"
         "    //! \\brief a|\n"
         "    //!\n"
         "    int a;\n"
        ) << _(
         "    bool preventFolding;\n"
         "    //!\n"
         "    //! \\brief a\n"
         "    //! \n"
         "    //!\n"
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

    QTest::newRow("cpp_styleB_corner_case") << _(
          "bool preventFolding;\n"
          "//!\n"
          "void d(); //!|\n"
        ) << _(
          "bool preventFolding;\n"
          "//!\n"
          "void d(); //!\n"
          "\n"
    );

    QTest::newRow("noContinuationForExpressionAndComment1") << _(
          "bool preventFolding;\n"
          "*foo //|\n"
        ) << _(
          "bool preventFolding;\n"
          "*foo //\n"
          "\n"
    );

    QTest::newRow("noContinuationForExpressionAndComment2") << _(
          "bool preventFolding;\n"
          "*foo /*|\n"
        ) << _(
          "bool preventFolding;\n"
          "*foo /*\n"
          "       \n"
    );
}

void DoxygenTest::testBasic()
{
    QFETCH(QByteArray, given);
    QFETCH(QByteArray, expected);
    runTest(given, expected);
}

void DoxygenTest::testNoLeadingAsterisks_data()
{
    QTest::addColumn<QByteArray>("given");
    QTest::addColumn<QByteArray>("expected");

    QTest::newRow("cpp_style_no_astrix") << _(
            "/* asdf asdf|\n"
        ) << _(
            "/* asdf asdf\n"
            "   \n"
    );
}

void DoxygenTest::testNoLeadingAsterisks()
{
    QFETCH(QByteArray, given);
    QFETCH(QByteArray, expected);

    CppTools::CommentsSettings injection;
    injection.m_enableDoxygen = true;
    injection.m_leadingAsterisks = false;

    runTest(given, expected, &injection);
}

void DoxygenTest::verifyCleanState() const
{
    QVERIFY(CppTools::Tests::VerifyCleanCppModelManager::isClean());
    QVERIFY(Core::DocumentModel::openedDocuments().isEmpty());
    QVERIFY(Core::EditorManager::visibleEditors().isEmpty());
}

/// The '|' in the input denotes the cursor position.
void DoxygenTest::runTest(const QByteArray &original, const QByteArray &expected,
                              CppTools::CommentsSettings *settings)
{
    // Write files to disk
    CppTools::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    TestDocument testDocument("file.cpp", original, '|');
    QVERIFY(testDocument.hasCursorMarker());
    testDocument.m_source.remove(testDocument.m_cursorPosition, 1);
    testDocument.setBaseDirectory(temporaryDir.path());
    QVERIFY(testDocument.writeToDisk());

    // Update Code Model
    QVERIFY(TestCase::parseFiles(testDocument.filePath()));

    // Open Editor
    QVERIFY(TestCase::openCppEditor(testDocument.filePath(), &testDocument.m_editor,
                                    &testDocument.m_editorWidget));

    if (settings) {
        auto *cts = CppTools::CppToolsSettings::instance();
        oldSettings.reset(new CppTools::CommentsSettings(cts->commentsSettings()));
        cts->setCommentsSettings(*settings);
    }

    // We want to test documents that start with a comment. By default, the
    // editor will fold the very first comment it encounters, assuming
    // it is a license header. Currently unfoldAll() does not work as
    // expected (some blocks are still hidden in some test cases, so the
    // cursor movements are not as expected). For the time being, we just
    // prepend a declaration before the initial test comment.
    //    testDocument.m_editorWidget->unfoldAll();
    testDocument.m_editor->setCursorPosition(testDocument.m_cursorPosition);

    TestCase::waitForRehighlightedSemanticDocument(testDocument.m_editorWidget);

    // Send 'ENTER' key press
    QKeyEvent event(QEvent::KeyPress, Qt::Key_Enter, Qt::NoModifier);
    QCoreApplication::sendEvent(testDocument.m_editorWidget, &event);
    const QByteArray result = testDocument.m_editorWidget->document()->toPlainText().toUtf8();

    QCOMPARE(QLatin1String(result), QLatin1String(expected));

    testDocument.m_editorWidget->undo();
    const QString contentsAfterUndo = testDocument.m_editorWidget->document()->toPlainText();
    QCOMPARE(contentsAfterUndo, testDocument.m_source);
}

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
