// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppuseselections_test.h"

#include "cppeditorwidget.h"
#include "cppmodelmanager.h"
#include "cpptoolstestcase.h"

#include <QElapsedTimer>
#include <QtTest>

// Uses 1-based line and 0-based column.
struct Selection
{
    Selection(int line, int column, int length) : line(line), column(column), length(length) {}

    friend bool operator==(const Selection &l, const Selection &r)
    { return l.line == r.line && l.column == r.column && l.length == r.length; }

    int line;
    int column;
    int length;
};
typedef QList<Selection> SelectionList;
Q_DECLARE_METATYPE(SelectionList)

QT_BEGIN_NAMESPACE
namespace QTest {
template<> char *toString(const Selection &selection)
{
    const QByteArray ba = "Selection("
            + QByteArray::number(selection.line) + ", "
            + QByteArray::number(selection.column) + ", "
            + QByteArray::number(selection.length)
            + ")";
    return qstrdup(ba.data());
}
}
QT_END_NAMESPACE

namespace CppEditor {
namespace Internal {
namespace Tests {

// Check: If the user puts the cursor on e.g. a function-local variable,
// a type name or a macro use, all occurrences of that entity are highlighted.
class UseSelectionsTestCase : public CppEditor::Tests::TestCase
{
public:
    UseSelectionsTestCase(CppTestDocument &testDocument,
                          const SelectionList &expectedSelections);

private:
    SelectionList toSelectionList(const QList<QTextEdit::ExtraSelection> &extraSelections) const;
    QList<QTextEdit::ExtraSelection> getExtraSelections() const;
    SelectionList waitForUseSelections(bool *hasTimedOut) const;

private:
    CppEditorWidget *m_editorWidget = nullptr;
};

UseSelectionsTestCase::UseSelectionsTestCase(CppTestDocument &testFile,
                                             const SelectionList &expectedSelections)
{
    QVERIFY(succeededSoFar());

    QVERIFY(testFile.hasCursorMarker());
    testFile.m_source.remove(testFile.m_cursorPosition, 1);

    CppEditor::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    testFile.setBaseDirectory(temporaryDir.path());
    testFile.writeToDisk();

    QVERIFY(openCppEditor(testFile.filePath(), &testFile.m_editor, &m_editorWidget));
    closeEditorAtEndOfTestCase(testFile.m_editor);

    testFile.m_editor->setCursorPosition(testFile.m_cursorPosition);
    waitForRehighlightedSemanticDocument(m_editorWidget);

    bool hasTimedOut;
    const SelectionList selections = waitForUseSelections(&hasTimedOut);
    const bool clangCodeModel = CppModelManager::isClangCodeModelActive();
    if (clangCodeModel) {
        QEXPECT_FAIL("local use as macro argument - argument eaten", "fails with CCM, find out why",
                     Abort);
    } else {
        QEXPECT_FAIL("non-local use as macro argument - argument expanded 1", "TODO", Abort);
    }
    QVERIFY(!hasTimedOut);
//    for (const Selection &selection : selections)
//        qDebug() << QTest::toString(selection);
    if (!clangCodeModel)
        QEXPECT_FAIL("non-local use as macro argument - argument expanded 2", "TODO", Abort);
    QCOMPARE(selections, expectedSelections);
}

SelectionList UseSelectionsTestCase::toSelectionList(
        const QList<QTextEdit::ExtraSelection> &extraSelections) const
{
    SelectionList result;
    for (const QTextEdit::ExtraSelection &selection : extraSelections) {
        int line, column;
        const int position = qMin(selection.cursor.position(), selection.cursor.anchor());
        m_editorWidget->convertPosition(position, &line, &column);
        result << Selection(line, column, selection.cursor.selectedText().length());
    }
    return result;
}

QList<QTextEdit::ExtraSelection> UseSelectionsTestCase::getExtraSelections() const
{
    return m_editorWidget->extraSelections(
        TextEditor::TextEditorWidget::CodeSemanticsSelection);
}

SelectionList UseSelectionsTestCase::waitForUseSelections(bool *hasTimedOut) const
{
    QElapsedTimer timer;
    timer.start();
    if (hasTimedOut)
        *hasTimedOut = false;

    QList<QTextEdit::ExtraSelection> extraSelections = getExtraSelections();
    while (extraSelections.isEmpty()) {
        if (timer.hasExpired(2500)) {
            if (hasTimedOut)
                *hasTimedOut = true;
            break;
        }
        QCoreApplication::processEvents();
        extraSelections = getExtraSelections();
    }

    return toSelectionList(extraSelections);
}

void SelectionsTest::testUseSelections_data()
{
    QTest::addColumn<QByteArray>("source");
    QTest::addColumn<SelectionList>("expectedSelections");
    typedef QByteArray _;

    QTest::newRow("local uses")
            << _("int f(int arg) { return @arg; }\n")
            << (SelectionList()
                << Selection(1, 10, 3)
                << Selection(1, 24, 3)
                );

    QTest::newRow("local use as macro argument 1 - argument expanded")
            << _("#define Q_UNUSED(x) (void)x\n"
                 "void f(int arg)\n"
                 "{\n"
                 "    Q_UNUSED(@arg)\n"
                 "}\n")
            << (SelectionList()
                << Selection(2, 11, 3)
                << Selection(4, 13, 3)
                );

    QTest::newRow("local use as macro argument 2 - argument eaten")
            << _("inline void qt_noop(void) {}\n"
                 "#define Q_ASSERT(cond) qt_noop()\n"
                 "void f(char *text)\n"
                 "{\n"
                 "    Q_ASSERT(@text);\n"
                 "}\n")
            << (SelectionList()
                << Selection(3, 13, 4)
                << Selection(5, 13, 4)
                );

    QTest::newRow("non-local uses")
            << _("struct Foo { @Foo(); };\n")
            << (SelectionList()
                << Selection(1, 7, 3)
                << Selection(1, 13, 3)
                );

    QTest::newRow("non-local use as macro argument - argument expanded 1")
            << _("#define QT_FORWARD_DECLARE_CLASS(name) class name;\n"
                 "QT_FORWARD_DECLARE_CLASS(Class)\n"
                 "@Class *foo;\n")
            << (SelectionList()
                << Selection(2, 25, 5)
                << Selection(3, 1, 5)
                );

    QTest::newRow("non-local use as macro argument - argument expanded 2")
            << _("#define Q_DISABLE_COPY(Class) \\\n"
                 "    Class(const Class &);\\\n"
                 "    Class &operator=(const Class &);\n"
                 "class @Super\n"
                 "{\n"
                 "    Q_DISABLE_COPY(Super)\n"
                 "};\n")
            << (SelectionList()
                << Selection(4, 6, 5)
                << Selection(6, 19, 5)
                );

    const SelectionList macroUseSelections = SelectionList()
            << Selection(1, 8, 3)
            << Selection(2, 0, 3);

    QTest::newRow("macro use 1")
            << _("#define FOO\n"
                 "@FOO\n")
            << macroUseSelections;

    QTest::newRow("macro use 2")
            << _("#define FOO\n"
                 "FOO@\n")
            << macroUseSelections;

    QTest::newRow("macro use 3")
            << _("#define @FOO\n"
                 "FOO\n")
            << macroUseSelections;

    QTest::newRow("macro use 4")
            << _("#define FOO@\n"
                 "FOO\n")
            << macroUseSelections;
}

void SelectionsTest::testUseSelections()
{
    QFETCH(QByteArray, source);
    QFETCH(SelectionList, expectedSelections);

    Tests::CppTestDocument testDocument("file.cpp", source);
    Tests::UseSelectionsTestCase(testDocument, expectedSelections);
}

void SelectionsTest::testSelectionFiltering_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<SelectionList>("original");
    QTest::addColumn<SelectionList>("filtered");

    QTest::addRow("QTCREATORBUG-18659")
            << QString("int main()\n"
                       "{\n"
                       "    [](const Foo &foo) -> Foo {\n"
                       "        return foo;\n"
                       "    };\n"
                       "}\n")
            << SelectionList{{3, 4, 53}}
            << SelectionList{{3, 4, 27}, {4, 8, 11}, {5, 4, 1}};
    QTest::addRow("indentation-selected-in-first-line")
            << QString("int main()\n"
                       "{\n"
                       "    [](const Foo &foo) -> Foo {\n"
                       "        return foo;\n"
                       "    };\n"
                       "}\n")
            << SelectionList{{3, 0, 57}}
            << SelectionList{{3, 4, 27}, {4, 8, 11}, {5, 4, 1}};
}

void SelectionsTest::testSelectionFiltering()
{
    QFETCH(QString, source);
    QFETCH(SelectionList, original);
    QFETCH(SelectionList, filtered);

    QTextDocument doc;
    doc.setPlainText(source);

    const auto convertList = [&doc](const SelectionList &in) {
        QList<QTextEdit::ExtraSelection> out;
        for (const Selection &selIn : in) {
            QTextEdit::ExtraSelection selOut;
            selOut.format.setFontItalic(true);
            const QTextBlock startBlock = doc.findBlockByLineNumber(selIn.line - 1);
            const int startPos = startBlock.position() + selIn.column;
            selOut.cursor = QTextCursor(&doc);
            selOut.cursor.setPosition(startPos);
            selOut.cursor.setPosition(startPos + selIn.length, QTextCursor::KeepAnchor);
            out << selOut;
        }
        return out;
    };

    const QList<QTextEdit::ExtraSelection> expected = convertList(filtered);
    const QList<QTextEdit::ExtraSelection> actual
            = CppEditorWidget::unselectLeadingWhitespace(convertList(original));

    QCOMPARE(actual.length(), expected.length());
    for (int i = 0; i < expected.length(); ++i) {
        const QTextEdit::ExtraSelection &expectedSelection = expected.at(i);
        const QTextEdit::ExtraSelection &actualSelection = actual.at(i);
        QCOMPARE(actualSelection.format, expectedSelection.format);
        QCOMPARE(actualSelection.cursor.document(), expectedSelection.cursor.document());
        QCOMPARE(actualSelection.cursor.position(), expectedSelection.cursor.position());
        QCOMPARE(actualSelection.cursor.anchor(), expectedSelection.cursor.anchor());
    }
}

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
