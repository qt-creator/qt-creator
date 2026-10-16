// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "qmljsscopeend_test.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>

#include <texteditor/displaysettings.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditor.h>

#include <utils/temporarydirectory.h>

#include <QTest>
#include <QTextBlock>

using namespace Core;
using namespace Utils;

namespace QmlJSEditor::Internal {

class QmlJSScopeEndTest final : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();
    void testScopesNameTheirObjects();
    void testRunOfClosingBraces();
};

void QmlJSScopeEndTest::cleanup()
{
    EditorManager::closeAllEditors(false);
}

void QmlJSScopeEndTest::testScopesNameTheirObjects()
{
    TemporaryDirectory tempDir("qtc-qmljs-scopeend-XXXXXX");
    QVERIFY(tempDir.isValid());
    const FilePath filePath = tempDir.filePath("Scopes.qml");
    QString text = "import QtQuick\n"
                   "Item {\n"
                   "    id: screen\n"
                   "    Column {\n"
                   "        id: column\n";
    for (int i = 0; i < 6; ++i)
        text += QString("        Text { text: \"%1\" }\n").arg(i);
    text += "    }\n"
            "    transitions: [\n"
            "        Transition {\n"
            "        },\n"
            "        Transition {\n";
    for (int i = 0; i < 6; ++i)
        text += QString("            PauseAnimation { duration: %1 }\n").arg(i);
    text += "        }\n"
            "    ]\n"
            "    Rectangle {\n"
            "        // Draws the {frame} of the screen.\n"
            "        id: frame\n";
    for (int i = 0; i < 6; ++i)
        text += QString("        property int p%1: %1\n").arg(i);
    text += "    }\n"
            "    function format(value,\n"
            "                    suffix = \")\") {\n";
    for (int i = 0; i < 6; ++i)
        text += QString("        console.log(%1)\n").arg(i);
    text += "    }\n"
            "}\n";
    QVERIFY(filePath.writeFileContents(text.toUtf8()));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditor::TextEditorWidget *widget = baseEditor->editorWidget();
    TextEditor::DisplaySettingsData settings = TextEditor::displaySettings().data();
    settings.m_displayScopeEnds = true;
    settings.m_scopeEndMinimumLines = 5;
    widget->setDisplaySettings(settings);

    const auto labels = [widget] {
        QMap<int, QString> result;
        for (QTextBlock block = widget->document()->begin(); block.isValid();
             block = block.next()) {
            const QString label = widget->scopeEndLabel(block);
            if (!label.isEmpty())
                result.insert(block.blockNumber() + 1, label);
        }
        return result;
    };

    // The braces are known once the highlighter has been through the document.
    QTRY_COMPARE(labels().size(), 5);
    const QMap<int, QString> found = labels();
    QCOMPARE(found.value(12), QString("Column id: column"));
    QCOMPARE(found.value(23), QString("Transition"));
    QCOMPARE(found.value(34), QString("Rectangle id: frame"));
    QCOMPARE(found.value(43), QString("function format(value, suffix = \")\")"));
    QCOMPARE(found.value(44), QString("Item id: screen"));
}

void QmlJSScopeEndTest::testRunOfClosingBraces()
{
    TemporaryDirectory tempDir("qtc-qmljs-scopeend-XXXXXX");
    QVERIFY(tempDir.isValid());
    const FilePath filePath = tempDir.filePath("Run.qml");
    QString text = "import QtQuick\n"
                   "Item {\n"
                   "    id: outer\n"
                   "    Item {\n"
                   "        id: inner\n";
    for (int i = 0; i < 6; ++i)
        text += QString("        property int p%1: %1\n").arg(i);
    text += "}}\n"; // 12
    QVERIFY(filePath.writeFileContents(text.toUtf8()));

    IEditor *editor = EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditor::TextEditorWidget *widget = baseEditor->editorWidget();
    TextEditor::DisplaySettingsData settings = TextEditor::displaySettings().data();
    settings.m_displayScopeEnds = true;
    settings.m_scopeEndMinimumLines = 5;
    widget->setDisplaySettings(settings);

    const QTextBlock closingLine = widget->document()->findBlockByNumber(11);
    QCOMPARE(closingLine.text(), QString("}}"));
    // The braces are known once the highlighter has been through the document.
    QTRY_COMPARE(widget->scopeEndLabel(closingLine), QString("Item id: outer"));
    QCOMPARE(widget->scopeEndLabel(closingLine, 0), QString("Item id: inner"));
    QCOMPARE(widget->scopeEndLabel(closingLine, 1), QString("Item id: outer"));
}

QObject *createQmlJSScopeEndTest()
{
    return new QmlJSScopeEndTest;
}

} // namespace QmlJSEditor::Internal

#include "qmljsscopeend_test.moc"

#endif // WITH_TESTS
