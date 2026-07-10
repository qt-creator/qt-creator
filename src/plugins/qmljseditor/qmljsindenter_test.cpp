// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "qmljsindenter_test.h"

#include "qmljseditor.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <texteditor/tabsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/temporarydirectory.h>

#include <QTest>
#include <QTextCursor>

using namespace Core;
using namespace Utils;

namespace QmlJSEditor::Internal {

// Editor-level QML auto-indentation while typing, as the removed Squish test
// suite_QMLS/tst_QMLS08 exercised: type a block out without leading indentation
// and let the editor indent it. Unlike the QmlJSTools plugin test (which drives
// the indenter directly), this runs against a real QmlJSEditorWidget - with the
// QmlJS auto-completer active - so it also covers the "electric" closing brace
// that dedents, and the full TextEditorWidget::keyPressEvent path.

class QmlJSIndenterTest final : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void testAutoIndentWhileTyping();
};

void QmlJSIndenterTest::cleanup()
{
    EditorManager::closeAllEditors(false);
}

void QmlJSIndenterTest::testAutoIndentWhileTyping()
{
    TemporaryDirectory tempDir("qtc-qmljs-indent-XXXXXX");
    QVERIFY(tempDir.isValid());
    const FilePath filePath = tempDir.filePath("Typed.qml");
    QVERIFY(filePath.writeFileContents("import QtQuick\nItem {\n}\n"));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    auto widget = qobject_cast<QmlJSEditorWidget *>(baseEditor->editorWidget());
    QVERIFY(widget);

    TextEditor::TabSettingsData tabSettings(TextEditor::TabSettingsData::SpacesOnlyTabPolicy,
                                            4, 4,
                                            TextEditor::TabSettingsData::ContinuationAlignWithSpaces);
    tabSettings.m_autoDetect = false;
    widget->textDocument()->setTabSettings(tabSettings);

    // Put the cursor at the end of the "Item {" line.
    QTextCursor cursor(widget->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down);
    cursor.movePosition(QTextCursor::EndOfLine);
    widget->setTextCursor(cursor);

    // Type a nested object out. Typing "{" is an electric character: the editor
    // auto-closes the brace and, on the following Return, expands the block and
    // indents its body and closing brace - all via keyPressEvent's electric-
    // character handling. (The matching "}" is inserted by the editor, so it is
    // not typed here.)
    QTest::keyClick(widget, Qt::Key_Return);
    QTest::keyClicks(widget, "Rectangle {");
    QTest::keyClick(widget, Qt::Key_Return);
    QTest::keyClicks(widget, "width: 10");

    QCOMPARE(widget->textDocument()->plainText(), QString(
        "import QtQuick\n"
        "Item {\n"
        "    Rectangle {\n"
        "        width: 10\n"
        "    }\n"
        "}\n"));
}

QObject *createQmlJSIndenterTest()
{
    return new QmlJSIndenterTest;
}

} // namespace QmlJSEditor::Internal

#include "qmljsindenter_test.moc"

#endif // WITH_TESTS
