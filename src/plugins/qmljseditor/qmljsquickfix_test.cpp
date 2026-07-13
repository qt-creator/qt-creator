// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifdef WITH_TESTS

#include "qmljsquickfix_test.h"

#include "qmljseditor.h"
#include "qmljseditordocument.h"
#include "qmljsquickfix.h"
#include "qmljsquickfixassist.h"

#include <coreplugin/editormanager/editormanager.h>

#include <texteditor/texteditor.h>

#include <utils/temporarydirectory.h>

#include <QElapsedTimer>
#include <QTest>
#include <QTextCursor>

using namespace Core;
using namespace Utils;

namespace QmlJSEditor::Internal {

// In-process ports of the Squish system tests suite_QMLS/tst_QMLS05..07, which
// applied the QML "Refactoring" quick-fixes (Split Initializer, Wrap Component
// in Loader, Add a Comment to Suppress This Message) from the editor's context
// menu and checked the resulting text. Here we open a QML editor, wait for its
// semantic info, put the cursor at the relevant position, then collect and
// apply the quick-fix through the real code (findQmlJSQuickFixes ->
// QmlJSQuickFixOperation::perform()) and compare the document text.

static bool waitForSemanticInfo(QmlJSEditorWidget *widget, int timeoutMs = 10000)
{
    QmlJSEditorDocument *document = widget->qmlJsEditorDocument();
    QElapsedTimer timer;
    timer.start();
    while (document->isSemanticInfoOutdated() || document->semanticInfo().document.isNull()) {
        if (timer.elapsed() > timeoutMs)
            return false;
        QCoreApplication::processEvents(QEventLoop::AllEvents, 20);
    }
    return true;
}

class QmlJSQuickFixTest final : public QObject
{
    Q_OBJECT

private:
    // Opens a QML editor for the given source, waits for its semantic info,
    // places the cursor at cursorPos, then applies the quick-fix whose
    // description equals opDescription. Returns the resulting document text.
    QString applyQuickFix(const QString &source, int cursorPos, const QString &opDescription);

private slots:
    void cleanup();
    void testSplitInitializer();
    void testWrapInLoader();
    void testSuppressMessage_data();
    void testSuppressMessage();

private:
    TemporaryDirectory *m_tempDir = nullptr;
    QString m_errorString;
};

QString QmlJSQuickFixTest::applyQuickFix(const QString &source, int cursorPos,
                                         const QString &opDescription)
{
    m_tempDir = new TemporaryDirectory("qtc-qmljs-quickfix-XXXXXX");
    const FilePath filePath = m_tempDir->filePath("Test.qml");
    if (!filePath.writeFileContents(source.toUtf8())) {
        m_errorString = "Could not write test file";
        return {};
    }

    IEditor *editor = EditorManager::openEditor(filePath);
    auto baseEditor = qobject_cast<TextEditor::BaseTextEditor *>(editor);
    if (!baseEditor) {
        m_errorString = "Could not open QML editor";
        return {};
    }
    auto widget = qobject_cast<QmlJSEditorWidget *>(baseEditor->editorWidget());
    if (!widget) {
        m_errorString = "Editor is not a QmlJSEditorWidget";
        return {};
    }
    if (!waitForSemanticInfo(widget)) {
        m_errorString = "Timed out waiting for semantic info";
        return {};
    }

    QTextCursor cursor = widget->textCursor();
    cursor.setPosition(cursorPos);
    widget->setTextCursor(cursor);

    QmlJSQuickFixAssistInterface interface(widget, TextEditor::ExplicitlyInvoked);
    const TextEditor::QuickFixOperations operations = findQmlJSQuickFixes(&interface);
    for (const TextEditor::QuickFixOperation::Ptr &operation : operations) {
        if (operation->description() == opDescription) {
            operation->perform();
            return widget->textDocument()->plainText();
        }
    }
    m_errorString = "Quick-fix not offered: " + opDescription;
    return {};
}

void QmlJSQuickFixTest::cleanup()
{
    EditorManager::closeAllEditors(false);
    delete m_tempDir;
    m_tempDir = nullptr;
    m_errorString.clear();
}

void QmlJSQuickFixTest::testSplitInitializer()
{
    const QString source =
        "import QtQuick\n"
        "Item {\n"
        "    Item { x: 10; y: 20; width: 10 }\n"
        "}\n";
    // Cursor on the inner "Item".
    const int cursorPos = source.indexOf("Item { x:") + 2;

    const QString result = applyQuickFix(source, cursorPos, "Split Initializer");
    QVERIFY2(!result.isEmpty(), qPrintable(m_errorString));

    const QString expected =
        "import QtQuick\n"
        "Item {\n"
        "    Item {\n"
        "        x: 10\n"
        "        y: 20\n"
        "        width: 10\n"
        "    }\n"
        "}\n";
    QCOMPARE(result, expected);
}

void QmlJSQuickFixTest::testWrapInLoader()
{
    const QString source =
        "import QtQuick\n"
        "Item {\n"
        "    Item { x: 10; y: 20; width: 10 }\n"
        "}\n";
    // Cursor on the inner "Item" type name (required by the wrap-in-loader fix).
    const int cursorPos = source.indexOf("Item { x:") + 2;

    const QString result = applyQuickFix(source, cursorPos, "Wrap Component in Loader");
    QVERIFY2(!result.isEmpty(), qPrintable(m_errorString));

    const QString expected =
        "import QtQuick\n"
        "Item {\n"
        "    // TODO: Move position bindings from the component to the Loader.\n"
        "    //       Check all uses of 'parent' inside the root element of the component.\n"
        "    Component {\n"
        "        id: component_Item\n"
        "        Item { x: 10; y: 20; width: 10 }\n"
        "    }\n"
        "    Loader {\n"
        "        id: loader_Item\n"
        "        sourceComponent: component_Item\n"
        "    }\n"
        "\n"
        "}\n";
    QCOMPARE(result, expected);
}

void QmlJSQuickFixTest::testSuppressMessage_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("cursorMarker");
    QTest::addColumn<QString>("expected");

    // The quick-fix is message-agnostic: it inserts a "@disable-check M<id>"
    // comment for whatever diagnostic sits at the cursor, on the line directly
    // above it. Both rows use warnings that come from pure scope/AST analysis,
    // so they fire without a resolved type environment. The differing message
    // ids (M107 vs M23) confirm the id is taken from the diagnostic and not
    // hard-coded.

    // A duplicate "let" declaration (WarnDuplicateDeclaration, M107).
    QTest::newRow("duplicate declaration")
        << QString(
            "import QtQuick\n"
            "Item {\n"
            "    function f() {\n"
            "        let a = 1;\n"
            "        let a = 2;\n"
            "    }\n"
            "}\n")
        << QString("a = 2")
        << QString(
            "import QtQuick\n"
            "Item {\n"
            "    function f() {\n"
            "        let a = 1;\n"
            "        // @disable-check M107\n"
            "        let a = 2;\n"
            "    }\n"
            "}\n");

    // A call to eval() (WarnEval, M23).
    QTest::newRow("eval")
        << QString(
            "import QtQuick\n"
            "Item {\n"
            "    function f() {\n"
            "        eval(\"1\");\n"
            "    }\n"
            "}\n")
        << QString("eval")
        << QString(
            "import QtQuick\n"
            "Item {\n"
            "    function f() {\n"
            "        // @disable-check M23\n"
            "        eval(\"1\");\n"
            "    }\n"
            "}\n");
}

void QmlJSQuickFixTest::testSuppressMessage()
{
    QFETCH(QString, source);
    QFETCH(QString, cursorMarker);
    QFETCH(QString, expected);

    const int cursorPos = source.indexOf(cursorMarker);
    QVERIFY(cursorPos >= 0);

    const QString result = applyQuickFix(source, cursorPos,
                                         "Add a Comment to Suppress This Message");
    QVERIFY2(!result.isEmpty(), qPrintable(m_errorString));
    QCOMPARE(result, expected);
}

QObject *createQmlJSQuickFixTest()
{
    return new QmlJSQuickFixTest;
}

} // namespace QmlJSEditor::Internal

#include "qmljsquickfix_test.moc"

#endif // WITH_TESTS
