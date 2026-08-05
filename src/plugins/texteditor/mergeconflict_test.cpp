// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergeconflict_test.h"

#include "mergeconflict.h"
#include "textdocument.h"
#include "texteditor.h"

#include <utils/plaintextedit/texteditorlayout.h>

#include <QLabel>
#include <QTest>
#include <QTextBlock>

#include <memory>

using namespace Utils;

namespace TextEditor::Internal {

// "a", the conflict, "b": "mine" is the current and "theirs" the incoming side
const char conflictText[] = "a\n<<<<<<< HEAD\nmine\n=======\ntheirs\n>>>>>>> branch\nb";

// Exercises the MergeConflictController on a live TextEditorWidget: the links,
// the lines reserved for them and the highlighting of the conflict sides.
class MergeConflictTest final : public QObject
{
    Q_OBJECT

private slots:
    void init()
    {
        m_widget = std::make_unique<TextEditorWidget>();
        m_widget->setTextDocument(TextDocumentPtr(new TextDocument));
    }

    void cleanup() { m_widget.reset(); }

    void testRows()
    {
        setText(QString(QLatin1StringView(conflictText)) + '\n'
                + QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 2);
        // a line is reserved above each "<<<<<<<" marker, and nowhere else
        QVERIFY(reservedHeight(2) > 0);
        QVERIFY(reservedHeight(9) > 0);
        QCOMPARE(reservedHeight(1), 0);
        QCOMPARE(reservedHeight(3), 0);
        // the links float on the line reserved above their marker
        QCOMPARE(links().first()->y(), reservedLineTop(2));
        QCOMPARE(links().last()->y(), reservedLineTop(9));

        // resolving the first conflict leaves the second one with its line
        setText("a\nb\nc\n" + QString(QLatin1StringView(conflictText)));
        QTRY_COMPARE(links().size(), 1);
        QVERIFY(reservedHeight(5) > 0);
        QCOMPARE(links().first()->y(), reservedLineTop(5));

        setText("nothing to resolve here");
        QTRY_VERIFY(links().isEmpty());
        QCOMPARE(reservedHeight(1), 0);
    }

    void testResolvesAfterEdit()
    {
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);

        // the links were made for the line numbers of an earlier scan, so a
        // resolution after an edit that moved the conflict must not use them
        QTextCursor cursor(m_widget->document());
        cursor.insertText("above\n");
        QLabel *label = links().first();
        // "Choose Current Change" is the leftmost of the three links
        QTest::mouseClick(label, Qt::LeftButton, {}, QPoint(2, label->height() / 2));
        QCOMPARE(m_widget->toPlainText(), QString("above\na\nmine\nb"));
        QTRY_VERIFY(links().isEmpty());
    }

    void testHighlightsSides()
    {
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);

        QVERIFY(isBold(2));  // <<<<<<<
        QVERIFY(!isBold(3)); // mine
        QVERIFY(isBold(4));  // =======
        QVERIFY(!isBold(5)); // theirs
        QVERIFY(isBold(6));  // >>>>>>>
        // the two sides get backgrounds of their own, the lines around the
        // conflict are left alone
        QVERIFY(formats(3).first().format.background()
                != formats(5).first().format.background());
        QVERIFY(formats(1).isEmpty());
        QVERIFY(formats(7).isEmpty());

        // the highlighting goes with the conflict
        setText("a\nmine\nb");
        QTRY_VERIFY(formats(2).isEmpty());
    }

    void testReadOnlyView()
    {
        // a read-only view cannot be edited, so it shows the sides of a
        // conflict but offers no links to resolve it
        m_widget->setReadOnly(true);
        setText(QLatin1StringView(conflictText));
        QTRY_VERIFY(!formats(2).isEmpty());
        QVERIFY(links().isEmpty());

        // ... and picks the links up once it becomes writable
        m_widget->setReadOnly(false);
        QTRY_COMPARE(links().size(), 1);
    }

    void testCleansUpWhenDisabled()
    {
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);

        // switching the resolution off gives the reserved line, the links and
        // the highlighting back, while the document stays as it is
        m_widget->setMergeConflictResolutionEnabled(false);
        QTRY_VERIFY(links().isEmpty());
        QCOMPARE(reservedHeight(2), 0);
        QVERIFY(formats(2).isEmpty());
        QCOMPARE(m_widget->toPlainText(), QString(QLatin1StringView(conflictText)));
    }

private:
    void setText(const QString &text) { m_widget->setPlainText(text); }

    TextEditorLayout *layout() const { return m_widget->editorLayout(); }

    QTextBlock block(int line) const
    {
        return m_widget->document()->findBlockByNumber(line - 1);
    }

    // The links that are in place, in document order. A rebuild deletes the
    // previous ones through the event loop, so this can briefly see both.
    QList<QLabel *> links() const
    {
        return m_widget->findChildren<QLabel *>(
            QLatin1StringView(MERGE_CONFLICT_CHOICES_OBJECT_NAME));
    }

    // How much room the layout keeps above the given line for the links.
    int reservedHeight(int line) const { return layout()->mainLayoutOffset(block(line)); }

    // The top of the line reserved above the "<<<<<<<" marker on the given line.
    int reservedLineTop(int markerLine) const
    {
        return m_widget->cursorRect(QTextCursor(block(markerLine))).top()
               - reservedHeight(markerLine);
    }

    QList<QTextLayout::FormatRange> formats(int line) const
    {
        return layout()->blockLayout(block(line))->formats();
    }

    bool isBold(int line) const
    {
        const QList<QTextLayout::FormatRange> ranges = formats(line);
        return !ranges.isEmpty() && ranges.first().format.fontWeight() == QFont::Bold;
    }

    std::unique_ptr<TextEditorWidget> m_widget;
};

QObject *createMergeConflictTest()
{
    return new MergeConflictTest;
}

} // namespace TextEditor::Internal

#include "mergeconflict_test.moc"
