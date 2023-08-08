// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppdoxygen_test.h"

#include "cppeditorwidget.h"
#include "cpptoolssettings.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <texteditor/commentssettings.h>

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <QKeyEvent>
#include <QtTest>

namespace { typedef QByteArray _; }

using CppEditor::Tests::TemporaryDir;
using CppEditor::Tests::TestCase;
using CppEditor::Internal::Tests::VerifyCleanCppModelManager;
using namespace TextEditor;

using namespace Utils;

namespace CppEditor {
namespace Internal {
namespace Tests {

class SettingsInjector
{
public:
    SettingsInjector(const CommentsSettings::Data &tempSettings)
    {
        CommentsSettings::setData(tempSettings);
    }
    ~SettingsInjector() { CommentsSettings::setData(m_oldSettings); }

private:
    const CommentsSettings::Data m_oldSettings = CommentsSettings::data();
};

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
    QVERIFY(Core::EditorManager::closeAllEditors(false));
    QVERIFY(TestCase::garbageCollectGlobalSnapshot());
}

void DoxygenTest::testBasic_data()
{
    QTest::addColumn<QByteArray>("given");
    QTest::addColumn<QByteArray>("expected");
    QTest::addColumn<int>("commandPrefix");

    using CommandPrefix = CommentsSettings::CommandPrefix;
    QTest::newRow("qt_style") << _(
        "bool preventFolding;\n"
        "/*!|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("qt_style_settings_override") << _(
        "bool preventFolding;\n"
        "/*!|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * @brief a\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::At);

    QTest::newRow("qt_style_settings_override_redundant") << _(
        "bool preventFolding;\n"
        "/*!|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * \\brief a\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Backslash);

    QTest::newRow("qt_style_cursor_before_existing_comment") << _(
        "bool preventFolding;\n"
        "/*!|\n"
        " * \\brief something\n"
        " */\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*!\n"
        " * \n"
        " * \\brief something\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Auto);

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
        "int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("java_style") << _(
        "bool preventFolding;\n"
        "/**|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("java_style_settings_override") << _(
        "bool preventFolding;\n"
        "/**|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/**\n"
        " * \\brief a\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Backslash);

    QTest::newRow("java_style_settings_override_redundant") << _(
        "bool preventFolding;\n"
        "/**|\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/**\n"
        " * @brief a\n"
        " */\n"
        "int a;\n") << int(CommandPrefix::At);

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
        "int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("cpp_styleA") << _(
         "bool preventFolding;\n"
         "///|\n"
         "int a;\n"
        ) << _(
         "bool preventFolding;\n"
         "///\n"
         "/// \\brief a\n"
         "///\n"
         "int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("cpp_styleB") << _(
         "bool preventFolding;\n"
         "//!|\n"
         "int a;\n"
        ) << _(
         "bool preventFolding;\n"
         "//!\n"
         "//! \\brief a\n"
         "//!\n"
         "int a;\n") << int(CommandPrefix::Auto);

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
         "int a;\n") << int(CommandPrefix::Auto);

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
         "int a;\n") << int(CommandPrefix::Auto);

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
         "    int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("cpp_styleB_indented") << _(
         "    bool preventFolding;\n"
         "    //!|\n"
         "    int a;\n"
        ) << _(
         "    bool preventFolding;\n"
         "    //!\n"
         "    //! \\brief a\n"
         "    //!\n"
         "    int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("cpp_styleA_indented_preserve_mixed_indention_continuation") << _(
         "\t bool preventFolding;\n"
         "\t /// \\brief a|\n"
         "\t int a;\n"
        ) << _(
         "\t bool preventFolding;\n"
         "\t /// \\brief a\n"
         "\t /// \n"
         "\t int a;\n") << int(CommandPrefix::Auto);

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
         "    int a;\n") << int(CommandPrefix::Auto);

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
         "    int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("cpp_styleA_corner_case") << _(
          "bool preventFolding;\n"
          "///\n"
          "void d(); ///|\n"
        ) << _(
            "bool preventFolding;\n"
            "///\n"
            "void d(); ///\n"
            "\n") << int(CommandPrefix::Auto);

    QTest::newRow("cpp_styleB_corner_case") << _(
          "bool preventFolding;\n"
          "//!\n"
          "void d(); //!|\n"
        ) << _(
          "bool preventFolding;\n"
          "//!\n"
          "void d(); //!\n"
          "\n") << int(CommandPrefix::Auto);

    QTest::newRow("noContinuationForExpressionAndComment1") << _(
          "bool preventFolding;\n"
          "*foo //|\n"
        ) << _(
          "bool preventFolding;\n"
          "*foo //\n"
          "\n") << int(CommandPrefix::Auto);

    QTest::newRow("noContinuationForExpressionAndComment2") << _(
          "bool preventFolding;\n"
          "*foo /*|\n"
        ) << _(
          "bool preventFolding;\n"
          "*foo /*\n"
          "       \n") << int(CommandPrefix::Auto);

    QTest::newRow("withMacroFromDocumentBeforeFunction") << _(
          "#define API\n"
          "/**|\n"
          "API void f();\n"
        ) << _(
          "#define API\n"
          "/**\n"
          " * @brief f\n"
          " */\n"
          "API void f();\n") << int(CommandPrefix::Auto);

    QTest::newRow("withAccessSpecifierBeforeFunction") << _(
          "class C {\n"
          "    /**|\n"
          "    public: void f();\n"
          "};\n"
        ) << _(
          "class C {\n"
          "    /**\n"
          "     * @brief f\n"
          "     */\n"
          "    public: void f();\n"
          "};\n") << int(CommandPrefix::Auto);

    QTest::newRow("classTemplate") << _(
          "bool preventFolding;\n"
          "/**|\n"
          "template<typename T> class C {\n"
          "};\n"
        ) << _(
          "bool preventFolding;\n"
          "/**\n"
          " * @brief The C class\n"
          " */\n"
          "template<typename T> class C {\n"
          "};\n") << int(CommandPrefix::Auto);

    QTest::newRow("continuation_after_text_in_first_line") << _(
        "bool preventFolding;\n"
        "/*! leading comment|\n"
        " */\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*! leading comment\n"
        " *  \n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Auto);

    QTest::newRow("continuation_after_extra_indent") << _(
        "bool preventFolding;\n"
        "/*! leading comment\n"
        " *  cont|\n"
        " */\n"
        "int a;\n"
        ) << _(
        "bool preventFolding;\n"
        "/*! leading comment\n"
        " *  cont\n"
        " *  \n"
        " */\n"
        "int a;\n") << int(CommandPrefix::Auto);
}

void DoxygenTest::testBasic()
{
    QFETCH(QByteArray, given);
    QFETCH(QByteArray, expected);
    QFETCH(int, commandPrefix);

    CommentsSettings::Data settings = CommentsSettings::data();
    settings.commandPrefix = static_cast<CommentsSettings::CommandPrefix>(commandPrefix);
    const SettingsInjector injector(settings);

    runTest(given, expected);
}

void DoxygenTest::testWithMacroFromHeaderBeforeFunction()
{
    const QByteArray given =
        "#include \"header.h\"\n"
        "/**|\n"
        "API void f();\n";

    const QByteArray expected =
        "#include \"header.h\"\n"
        "/**\n"
        " * @brief f\n"
        " */\n"
        "API void f();\n";

    const CppTestDocument headerDocumentDefiningMacro("header.h", "#define API\n");

    runTest(given, expected, {headerDocumentDefiningMacro});
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

    TextEditor::CommentsSettings::Data injection;
    injection.enableDoxygen = true;
    injection.leadingAsterisks = false;
    const SettingsInjector injector(injection);

    runTest(given, expected);
}

void DoxygenTest::verifyCleanState() const
{
    QVERIFY(VerifyCleanCppModelManager::isClean());
    QVERIFY(Core::DocumentModel::openedDocuments().isEmpty());
    QVERIFY(Core::EditorManager::visibleEditors().isEmpty());
}

/// The '|' in the input denotes the cursor position.
void DoxygenTest::runTest(const QByteArray &original,
                          const QByteArray &expected,
                          const TestDocuments &includedHeaderDocuments)
{
    // Write files to disk
    TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    CppTestDocument testDocument("file.cpp", original, '|');
    QVERIFY(testDocument.hasCursorMarker());
    testDocument.m_source.remove(testDocument.m_cursorPosition, 1);
    testDocument.setBaseDirectory(temporaryDir.path());
    QVERIFY(testDocument.writeToDisk());
    for (CppTestDocument testDocument : includedHeaderDocuments) {
        testDocument.setBaseDirectory(temporaryDir.path());
        QVERIFY(testDocument.writeToDisk());
    }

    // Update Code Model
    QVERIFY(TestCase::parseFiles(testDocument.filePath().toString()));

    // Open Editor
    QVERIFY(TestCase::openCppEditor(testDocument.filePath(), &testDocument.m_editor,
                                    &testDocument.m_editorWidget));

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
