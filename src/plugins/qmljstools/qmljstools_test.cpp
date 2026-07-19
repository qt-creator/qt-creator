// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "qmljstools_test.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/ieditor.h>
#include <coreplugin/icore.h>

#include <qmljs/qmljsdocument.h>
#include <qmljs/qmljslink.h>
#include <qmljs/qmljsmodelmanagerinterface.h>
#include <qmljs/qmljsvalueowner.h>
#include <qmljstools/qmljsindenter.h>

#include <texteditor/tabsettings.h>
#include <texteditor/texteditor.h>
#include <texteditor/textdocument.h>

#include <utils/temporarydirectory.h>

#include <QTest>
#include <QTextCursor>
#include <QTextDocument>

using namespace QmlJS;

namespace QmlJSTools::Internal {

class QmlJSToolsTest final : public QObject
{
    Q_OBJECT

private slots:
    void test_basic();
    void test_qmlReindent_data();
    void test_qmlReindent();
    void test_qmlAutoIndentOnNewLine();
    void test_qmlAutoIndentWhileTyping();
    void test_qmlSyntaxErrorDiagnostic();
};

void QmlJSToolsTest::test_basic()
{
    ModelManagerInterface *modelManager = ModelManagerInterface::instance();

    const Utils::FilePath qmlFilePath = Core::ICore::resourcePath(
                                        "qmldesigner/itemLibraryQmlSources/ItemDelegate.qml");
    modelManager->updateSourceFiles(Utils::FilePaths({qmlFilePath}), false);
    modelManager->test_joinAllThreads();

    Snapshot snapshot = modelManager->snapshot();
    Document::Ptr doc = snapshot.document(qmlFilePath);
    QVERIFY(doc && doc->isQmlDocument());

    ContextPtr context = Link(snapshot, ViewerContext(), LibraryInfo())();
    QVERIFY(context);

    const CppComponentValue *rectangleValue = context->valueOwner()->cppQmlTypes().objectByQualifiedName(
                QLatin1String("QtQuick"), QLatin1String("QDeclarativeRectangle"), LanguageUtils::ComponentVersion());
    QVERIFY(rectangleValue);
    QVERIFY(!rectangleValue->isWritable(QLatin1String("border")));
    QVERIFY(rectangleValue->hasProperty(QLatin1String("border")));
    QVERIFY(rectangleValue->isPointer(QLatin1String("border")));
    QVERIFY(rectangleValue->isWritable(QLatin1String("color")));
    QVERIFY(!rectangleValue->isPointer(QLatin1String("color")));

    const ObjectValue *ovItem = context->lookupType(doc.data(), QStringList(QLatin1String("Item")));
    QVERIFY(ovItem);
    QCOMPARE(ovItem->className(), QLatin1String("Item"));
    QCOMPARE(context->imports(doc.data())->info(QLatin1String("Item"), context.data()).name(), QLatin1String("QtQuick"));

    const ObjectValue *ovProperty = context->lookupType(doc.data(), QStringList() << QLatin1String("Item") << QLatin1String("states"));
    QVERIFY(ovProperty);
    QCOMPARE(ovProperty->className(), QLatin1String("State"));

    const CppComponentValue *qmlItemValue = value_cast<CppComponentValue>(ovItem);
    QVERIFY(qmlItemValue);
    QCOMPARE(qmlItemValue->defaultPropertyName(), QLatin1String("data"));
    QCOMPARE(qmlItemValue->propertyType(QLatin1String("state")), QLatin1String("string"));

    const ObjectValue *ovState = context->lookupType(doc.data(), QStringList(QLatin1String("State")));
    const CppComponentValue *qmlState2Value = value_cast<CppComponentValue>(ovState);
    QCOMPARE(qmlState2Value->className(), QLatin1String("State"));

    const ObjectValue *ovImage = context->lookupType(doc.data(), QStringList(QLatin1String("Image")));
    const CppComponentValue *qmlImageValue = value_cast<CppComponentValue>(ovImage);
    QCOMPARE(qmlImageValue->className(), QLatin1String("Image"));
    QCOMPARE(qmlImageValue->propertyType(QLatin1String("source")), QLatin1String("QUrl"));
}

// Editor-level QML auto-indentation: the Ctrl+I action and auto-indent on typing.
static TextEditor::TabSettingsData fourSpaceTabSettings()
{
    TextEditor::TabSettingsData settings(TextEditor::TabSettingsData::SpacesOnlyTabPolicy,
                                         4, 4,
                                         TextEditor::TabSettingsData::ContinuationAlignWithSpaces);
    settings.m_autoDetect = false;
    return settings;
}

static QString stripLeadingWhitespace(const QString &text)
{
    const QStringList lines = text.split('\n');
    QStringList result;
    for (const QString &line : lines) {
        int i = 0;
        while (i < line.size() && (line.at(i) == ' ' || line.at(i) == '\t'))
            ++i;
        result.append(line.mid(i));
    }
    return result.join('\n');
}

void QmlJSToolsTest::test_qmlReindent_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<QString>("expected");

    // A block echoing the structure of the former tst_QMLS08: an object with a
    // property, a nested object, and a JS function with a braceless for loop.
    const QString expected =
        "import QtQuick\n"
        "Window {\n"
        "    id: wdw\n"
        "    property bool random: true\n"
        "    Text {\n"
        "        anchors.bottom: parent.bottom\n"
        "        text: wdw.random ? getRandom() : \"fixed\"\n"
        "    }\n"
        "    function getRandom() {\n"
        "        var result = \"random: \";\n"
        "        for (var i = 0; i < 8; ++i)\n"
        "            result += i;\n"
        "        return result;\n"
        "    }\n"
        "}\n";

    QTest::newRow("nested object and function")
        << stripLeadingWhitespace(expected) << expected;

    // A nested object with a signal handler block, as in tst_qml_indent.
    const QString handler =
        "import QtQuick\n"
        "Window {\n"
        "    title: qsTr(\"Hello World\")\n"
        "    MouseArea {\n"
        "        anchors.fill: parent\n"
        "        onClicked: {\n"
        "            console.log(parent.title)\n"
        "        }\n"
        "    }\n"
        "}\n";

    QTest::newRow("nested object with signal handler")
        << stripLeadingWhitespace(handler) << handler;
}

void QmlJSToolsTest::test_qmlReindent()
{
    QFETCH(QString, input);
    QFETCH(QString, expected);

    TextEditor::TextDocument doc;
    doc.setIndenter(QmlJSEditor::createQmlJsIndenter(doc.document()));
    doc.setTabSettings(fourSpaceTabSettings());
    doc.setPlainText(input);

    // Reindent the whole document, as the "Auto-indent Selection" (Ctrl+I)
    // action does (TextEditorWidget::autoIndent() -> autoFormatOrIndent()).
    QTextCursor cursor(doc.document());
    cursor.select(QTextCursor::Document);
    doc.autoFormatOrIndent(cursor);

    QCOMPARE(doc.plainText(), expected);
}

void QmlJSToolsTest::test_qmlAutoIndentOnNewLine()
{
    TextEditor::TextDocument doc;
    doc.setIndenter(QmlJSEditor::createQmlJsIndenter(doc.document()));
    doc.setTabSettings(fourSpaceTabSettings());
    doc.setPlainText("Window {\n}\n");

    // Simulate pressing Return right after the opening brace: the newly created
    // line must be auto-indented one level deeper.
    QTextCursor cursor(doc.document());
    cursor.movePosition(QTextCursor::EndOfLine);
    cursor.insertText("\n");
    doc.autoIndent(cursor);

    QCOMPARE(doc.document()->findBlockByNumber(1).text(), QString("    "));
}

void QmlJSToolsTest::test_qmlAutoIndentWhileTyping()
{
    // The former tst_QMLS08 typed the lines out (without leading indentation)
    // and checked the indentation applied *while typing*. Reproduce that by
    // posting key events to a real editor, exercising the indent-on-typing path
    // in TextEditorWidget::keyPressEvent.
    Utils::TemporaryDirectory tempDir("qtc-qmljs-typing-XXXXXX");
    QVERIFY(tempDir.isValid());
    const Utils::FilePath filePath = tempDir.filePath("Typed.qml");
    QVERIFY(filePath.writeFileContents("import QtQuick\nItem {\n}\n"));

    Core::IEditor *editor = Core::EditorManager::openEditor(filePath);
    QVERIFY(editor);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    QVERIFY(baseEditor);
    TextEditor::TextEditorWidget *widget = baseEditor->editorWidget();
    QVERIFY(widget);
    widget->textDocument()->setIndenter(QmlJSEditor::createQmlJsIndenter(widget->document()));
    widget->textDocument()->setTabSettings(fourSpaceTabSettings());

    // Put the cursor at the end of the "Item {" line.
    QTextCursor cursor(widget->document());
    cursor.movePosition(QTextCursor::Start);
    cursor.movePosition(QTextCursor::Down);
    cursor.movePosition(QTextCursor::EndOfLine);
    widget->setTextCursor(cursor);

    // Type two properties without any leading indentation; the editor must
    // indent each new line to its enclosing level while typing.
    QTest::keyClick(widget, Qt::Key_Return);
    QTest::keyClicks(widget, "property int x: 10");
    QTest::keyClick(widget, Qt::Key_Return);
    QTest::keyClicks(widget, "property int y: 20");

    QCOMPARE(widget->textDocument()->plainText(), QString(
        "import QtQuick\n"
        "Item {\n"
        "    property int x: 10\n"
        "    property int y: 20\n"
        "}\n"));

    Core::EditorManager::closeAllEditors(false);
}

void QmlJSToolsTest::test_qmlSyntaxErrorDiagnostic()
{
    // A malformed QML document (unclosed object) yields a parse diagnostic.
    Document::MutablePtr broken = Document::create(Utils::FilePath::fromString("broken.qml"),
                                                   Dialect::Qml);
    broken->setSource("import QtQuick\nItem {\n");
    broken->parse();
    QVERIFY(!broken->diagnosticMessages().isEmpty());

    // A stray identifier inside an object is also reported.
    Document::MutablePtr strayToken = Document::create(Utils::FilePath::fromString("stray.qml"),
                                                       Dialect::Qml);
    strayToken->setSource("import QtQuick\nItem {\n    SyntaxError\n}\n");
    strayToken->parse();
    QVERIFY(!strayToken->diagnosticMessages().isEmpty());

    // A well-formed document has none.
    Document::MutablePtr valid = Document::create(Utils::FilePath::fromString("valid.qml"),
                                                  Dialect::Qml);
    valid->setSource("import QtQuick\nItem {\n}\n");
    valid->parse();
    QVERIFY(valid->diagnosticMessages().isEmpty());
}

QObject *createQmlJSToolsTest()
{
    return new QmlJSToolsTest;
}

} // QmlJSTools::Internal

#include "qmljstools_test.moc"
