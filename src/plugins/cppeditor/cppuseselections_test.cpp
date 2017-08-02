/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "cppeditor.h"
#include "cppeditorwidget.h"
#include "cppeditorplugin.h"
#include "cppeditortestcase.h"

#include <QElapsedTimer>
#include <QtTest>

struct Selection {
    Selection(int line, int column, int length) : line(line), column(column), length(length) {}
    int line;
    int column;
    int length;
};
typedef QList<Selection> SelectionList;
Q_DECLARE_METATYPE(SelectionList)

inline bool operator==(const Selection &l, const Selection &r)
{ return l.line == r.line && l.column == r.column && l.length == r.length; }

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
class UseSelectionsTestCase : public TestCase
{
public:
    UseSelectionsTestCase(TestDocument &testDocument,
                          const SelectionList &expectedSelections);

private:
    SelectionList toSelectionList(const QList<QTextEdit::ExtraSelection> &extraSelections) const;
    QList<QTextEdit::ExtraSelection> getExtraSelections() const;
    SelectionList waitForUseSelections(bool *hasTimedOut) const;

private:
    CppEditorWidget *m_editorWidget = nullptr;
};

UseSelectionsTestCase::UseSelectionsTestCase(TestDocument &testFile,
                                             const SelectionList &expectedSelections)
{
    QVERIFY(succeededSoFar());

    QVERIFY(testFile.hasCursorMarker());
    testFile.m_source.remove(testFile.m_cursorPosition, 1);

    CppTools::Tests::TemporaryDir temporaryDir;
    QVERIFY(temporaryDir.isValid());
    testFile.setBaseDirectory(temporaryDir.path());
    testFile.writeToDisk();

    QVERIFY(openCppEditor(testFile.filePath(), &testFile.m_editor, &m_editorWidget));
    closeEditorAtEndOfTestCase(testFile.m_editor);

    testFile.m_editor->setCursorPosition(testFile.m_cursorPosition);
    waitForRehighlightedSemanticDocument(m_editorWidget);

    bool hasTimedOut;
    const SelectionList selections = waitForUseSelections(&hasTimedOut);
    QEXPECT_FAIL("non-local use as macro argument - argument expanded 1", "TODO", Abort);
    QEXPECT_FAIL("macro use 2", "TODO", Abort);
    QVERIFY(!hasTimedOut);
//    foreach (const Selection &selection, selections)
//        qDebug() << QTest::toString(selection);
    QEXPECT_FAIL("non-local use as macro argument - argument expanded 2", "TODO", Abort);
    QCOMPARE(selections, expectedSelections);
}

SelectionList UseSelectionsTestCase::toSelectionList(
        const QList<QTextEdit::ExtraSelection> &extraSelections) const
{
    SelectionList result;
    foreach (const QTextEdit::ExtraSelection &selection, extraSelections) {
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

} // namespace Tests

void CppEditorPlugin::test_useSelections_data()
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
            << _("#define Q_UNUSED(x) (void)x;\n"
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

void CppEditorPlugin::test_useSelections()
{
    QFETCH(QByteArray, source);
    QFETCH(SelectionList, expectedSelections);

    Tests::TestDocument testDocument("file.cpp", source);
    Tests::UseSelectionsTestCase(testDocument, expectedSelections);
}

} // namespace Internal
} // namespace CppEditor
