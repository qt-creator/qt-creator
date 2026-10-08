// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "mergeconflict_test.h"

#include "displaysettings.h"
#include "fontsettings.h"
#include "mergeconflict.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorconstants.h"

#include <coreplugin/find/highlightscrollbarcontroller.h>

#include <utils/plaintextedit/texteditorlayout.h>

#include <QLabel>
#include <QMouseEvent>
#include <QScrollBar>
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
    void initTestCase() { m_previewMode = displaySettings().mergeConflictPreviewMode(); }

    void init()
    {
        displaySettings().mergeConflictPreviewMode.setValue(
            MergeConflictPreviewMode::HideMarkers);
        m_widget = std::make_unique<TextEditorWidget>();
        m_widget->setTextDocument(TextDocumentPtr(new TextDocument));
    }

    void cleanup()
    {
        // Find follows the focused editor, and is told when it hides: let that
        // happen while the widget is still whole, not from its destructor
        m_widget->hide();
        m_widget.reset();
        displaySettings().mergeConflictPreviewMode.setValue(m_previewMode);
    }

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

    void testPreviewsChoice_data()
    {
        QTest::addColumn<MergeConflictPreviewMode>("mode");
        QTest::addColumn<QString>("text");
        QTest::addColumn<QString>("link");
        QTest::addColumn<QList<int>>("hiddenLines");
        QTest::addColumn<QList<int>>("disabledLines");

        using enum MergeConflictPreviewMode;
        // 1 "a", 2 "<<<<<<<", 3 "mine", 4 "=======", 5 "theirs", 6 ">>>>>>>", 7 "b"
        const QString text = QLatin1StringView(conflictText);
        // 1 "a", 2 "<<<<<<<", 3 "mine", 4 "|||||||", 5 "base", 6 "=======",
        // 7 "theirs", 8 ">>>>>>>", 9 "b"
        const QString diff3
            = "a\n<<<<<<< HEAD\nmine\n||||||| base\nbase\n=======\ntheirs\n>>>>>>> branch\nb";

        QTest::newRow("result only, current")
            << ResultOnly << text << "current" << QList<int>{2, 4, 5, 6} << QList<int>{};
        QTest::newRow("result only, incoming")
            << ResultOnly << text << "incoming" << QList<int>{2, 3, 4, 6} << QList<int>{};
        QTest::newRow("result only, both")
            << ResultOnly << text << "both" << QList<int>{2, 4, 6} << QList<int>{};
        QTest::newRow("result only, both, diff3")
            << ResultOnly << diff3 << "both" << QList<int>{2, 4, 5, 6, 8} << QList<int>{};

        QTest::newRow("hide markers, current")
            << HideMarkers << text << "current" << QList<int>{2, 4, 6} << QList<int>{5};
        QTest::newRow("hide markers, incoming")
            << HideMarkers << text << "incoming" << QList<int>{2, 4, 6} << QList<int>{3};
        QTest::newRow("hide markers, both")
            << HideMarkers << text << "both" << QList<int>{2, 4, 6} << QList<int>{};
        QTest::newRow("hide markers, both, diff3")
            << HideMarkers << diff3 << "both" << QList<int>{2, 4, 6, 8} << QList<int>{5};

        QTest::newRow("highlight only, current")
            << HighlightOnly << text << "current" << QList<int>{} << QList<int>{2, 4, 5, 6};
        QTest::newRow("highlight only, incoming")
            << HighlightOnly << text << "incoming" << QList<int>{} << QList<int>{2, 3, 4, 6};
        QTest::newRow("highlight only, both, diff3")
            << HighlightOnly << diff3 << "both" << QList<int>{} << QList<int>{2, 4, 5, 6, 8};
    }

    // hovering a link previews the resolution in this view, as the preview
    // mode says, without touching the document; the links stay under the mouse
    void testPreviewsChoice()
    {
        QFETCH(MergeConflictPreviewMode, mode);
        QFETCH(QString, text);
        QFETCH(QString, link);
        QFETCH(QList<int>, hiddenLines);
        QFETCH(QList<int>, disabledLines);

        displaySettings().mergeConflictPreviewMode.setValue(mode);
        setText(text);
        QTRY_COMPARE(links().size(), 1);
        QLabel *label = links().first();
        const int labelTop = label->y();
        const int lineCount = m_widget->document()->blockCount();
        QList<QTextCharFormat> unpreviewed;
        for (int line = 1; line <= lineCount; ++line) {
            QVERIFY(!isDisabled(line));
            unpreviewed.append(formats(line).value(0).format);
        }

        emit label->linkHovered(link);
        for (int line = 1; line <= lineCount; ++line) {
            QCOMPARE(layout()->isBlockVisibleInEditor(block(line)), !hiddenLines.contains(line));
            if (!hiddenLines.contains(line))
                QCOMPARE(isDisabled(line), disabledLines.contains(line));
        }
        QCOMPARE(label->y(), labelTop);
        QVERIFY(!label->isHidden());
        QCOMPARE(m_widget->toPlainText(), text);

        emit label->linkHovered(QString());
        QVERIFY(!layout()->hasEditorHiddenBlocks());
        QCOMPARE(reservedHeight(3), 0); // the first line below "<<<<<<<"
        for (int line = 1; line <= lineCount; ++line)
            QCOMPARE(formats(line).value(0).format, unpreviewed.at(line - 1));
        QTRY_COMPARE(label->y(), reservedLineTop(2));
        QCOMPARE(label->y(), labelTop);
    }

    void testPreviewKeepsScrollPosition_data()
    {
        QTest::addColumn<MergeConflictPreviewMode>("mode");

        QTest::newRow("result only") << MergeConflictPreviewMode::ResultOnly;
        QTest::newRow("hide markers") << MergeConflictPreviewMode::HideMarkers;
    }

    // hiding lines must not shorten the document, or a view scrolled to the
    // bottom scrolls up under the pointer and stays there after the preview
    void testPreviewKeepsScrollPosition()
    {
        QFETCH(MergeConflictPreviewMode, mode);

        displaySettings().mergeConflictPreviewMode.setValue(mode);
        QString text;
        for (int line = 1; line <= 100; ++line)
            text += QString("line %1\n").arg(line);
        // the conflict's "<<<<<<<" marker is on line 102
        setText(text + QLatin1StringView(conflictText));
        m_widget->resize(400, 200);
        m_widget->show();
        QVERIFY(QTest::qWaitForWindowExposed(m_widget.get()));
        QTRY_COMPARE(links().size(), 1);
        QScrollBar *scrollBar = m_widget->verticalScrollBar();
        QTRY_VERIFY(scrollBar->maximum() > 0);
        scrollBar->setValue(scrollBar->maximum());
        QLabel *label = links().first();
        QTRY_COMPARE(label->y(), reservedLineTop(102));
        QVERIFY(!label->isHidden());
        const int scrolled = scrollBar->value();
        const int labelTop = label->y();

        emit label->linkHovered("current");
        QVERIFY(layout()->hasEditorHiddenBlocks());
        QTest::qWait(50); // the scroll bar follows the document size queued
        QCOMPARE(scrollBar->value(), scrolled);
        QCOMPARE(label->y(), labelTop);

        emit label->linkHovered(QString());
        QVERIFY(!layout()->hasEditorHiddenBlocks());
        QTest::qWait(50);
        QCOMPARE(scrollBar->value(), scrolled);
        QCOMPARE(label->y(), labelTop);
    }

    // with nothing left of the conflict and nothing below it, the "<<<<<<<"
    // marker stays to carry the links
    void testPreviewsEmptyResultAtEnd()
    {
        displaySettings().mergeConflictPreviewMode.setValue(MergeConflictPreviewMode::ResultOnly);
        setText("a\n<<<<<<< HEAD\n=======\ntheirs\n>>>>>>> branch");
        QTRY_COMPARE(links().size(), 1);
        QLabel *label = links().first();
        const int labelTop = label->y();

        emit label->linkHovered("current");
        QVERIFY(layout()->isBlockVisibleInEditor(block(2)));
        for (int line = 3; line <= 5; ++line)
            QVERIFY(!layout()->isBlockVisibleInEditor(block(line)));
        QCOMPARE(label->y(), labelTop);
    }

    // with both sides empty and nothing below the conflict, the "<<<<<<<"
    // marker stays to carry the links
    void testPreviewsEmptyConflictAtEnd()
    {
        setText("a\n<<<<<<< HEAD\n=======\n>>>>>>> branch");
        QTRY_COMPARE(links().size(), 1);
        QLabel *label = links().first();
        const int labelTop = label->y();

        emit label->linkHovered("both");
        QVERIFY(layout()->isBlockVisibleInEditor(block(2)));
        QVERIFY(!layout()->isBlockVisibleInEditor(block(3)));
        QVERIFY(!layout()->isBlockVisibleInEditor(block(4)));
        QCOMPARE(label->y(), labelTop);
    }

    // a conflict with empty sides passes its row to the next conflict's
    // marker, whose own row moves below it rather than on top of it
    void testPreviewsAdjacentConflicts()
    {
        setText("<<<<<<< HEAD\n=======\n>>>>>>> branch\n"
                "<<<<<<< HEAD\nmine\n=======\ntheirs\n>>>>>>> branch\nb");
        QTRY_COMPARE(links().size(), 2);
        QLabel *first = links().first();
        QLabel *second = links().last();
        const int firstTop = first->y();

        emit first->linkHovered("current");
        QVERIFY(layout()->isBlockVisibleInEditor(block(4)));
        QCOMPARE(first->y(), firstTop);
        QCOMPARE(second->y(), firstTop + first->height());
    }

    void testPreviewEndsOnEdit()
    {
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);

        emit links().first()->linkHovered("incoming");
        QVERIFY(isDisabled(3)); // mine
        // an edit inside the conflict leaves it on the same lines, so only
        // ending the preview can bring the sides back
        QTextCursor cursor(block(3));
        cursor.movePosition(QTextCursor::EndOfBlock);
        cursor.insertText("!");
        QTRY_VERIFY(!isDisabled(3)); // mine!
        QVERIFY(!layout()->hasEditorHiddenBlocks());
        for (int line = 1; line <= 7; ++line)
            QVERIFY(!isDisabled(line));
        QVERIFY(isBold(2)); // <<<<<<<
    }

    // QLabel unhovers a link only when the pointer moves onto the text between
    // two links; leaving the row is what has to end the preview
    void testPreviewEndsWhenPointerLeaves()
    {
        displaySettings().mergeConflictPreviewMode.setValue(MergeConflictPreviewMode::ResultOnly);
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);
        QLabel *label = links().first();

        emit label->linkHovered("current");
        QVERIFY(layout()->hasEditorHiddenBlocks());
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(label, &leave);
        QVERIFY(!layout()->hasEditorHiddenBlocks());
        QVERIFY(!isDisabled(5)); // theirs
    }

    // QLabel reports a link as hovered only when it differs from the last one
    // it reported, so coming back onto the link the preview ended on must
    // preview it again
    void testPreviewsAgainWhenPointerReturns()
    {
        displaySettings().mergeConflictPreviewMode.setValue(MergeConflictPreviewMode::ResultOnly);
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);
        QLabel *label = links().first();
        // "Choose Current Change" is the leftmost of the three links
        const QPoint onFirstLink(2, label->height() / 2);

        movePointer(label, onFirstLink);
        QVERIFY(layout()->hasEditorHiddenBlocks());
        QEvent leave(QEvent::Leave);
        QCoreApplication::sendEvent(label, &leave);
        QVERIFY(!layout()->hasEditorHiddenBlocks());
        movePointer(label, onFirstLink);
        QVERIFY(layout()->hasEditorHiddenBlocks());
    }

    // hovering uses the conflicts where they are now, also after an edit that
    // moved them without touching a conflict, which starts no rescan
    void testPreviewsAfterEdit()
    {
        displaySettings().mergeConflictPreviewMode.setValue(
            MergeConflictPreviewMode::HighlightOnly);
        // 1 "a", 2-6 the first conflict, 7 "b", 8 "a", 9-13 the second
        // conflict, 14 "b"
        setText(QString(QLatin1StringView(conflictText)) + '\n'
                + QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 2);

        // a line between the two conflicts moves the second one down
        QTextCursor cursor(block(8));
        cursor.insertText("between\n");
        emit links().first()->linkHovered("current");
        QVERIFY(isDisabled(5)); // theirs
        QVERIFY(isBold(10));    // <<<<<<< of the second conflict
        QVERIFY(!isBold(9));    // a
        emit links().first()->linkHovered(QString());
        QVERIFY(!isDisabled(5));
        QVERIFY(isBold(10));
        QVERIFY(!isBold(9));

        // a line above both moves the hovered conflict, too
        cursor.setPosition(0);
        cursor.insertText("above\n");
        emit links().first()->linkHovered("current");
        QVERIFY(isDisabled(6)); // theirs
        QVERIFY(isBold(11));    // <<<<<<< of the second conflict
        QVERIFY(!isBold(10));   // a
    }

    void testResolvesWhilePreviewing()
    {
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(links().size(), 1);
        QLabel *label = links().first();

        emit label->linkHovered("current");
        // "Choose Current Change" is the leftmost of the three links
        QTest::mouseClick(label, Qt::LeftButton, {}, QPoint(2, label->height() / 2));
        QCOMPARE(m_widget->toPlainText(), QString("a\nmine\nb"));
        QTRY_VERIFY(formats(2).isEmpty());
        QVERIFY(!layout()->hasEditorHiddenBlocks());
        QCOMPARE(reservedHeight(2), 0);
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

    void testMarksSidesOnScrollBar()
    {
        setText(QLatin1StringView(conflictText));
        QTRY_COMPARE(scrollBarHighlights().size(), 2);

        // one marker per side, in the side's own color, the current side
        // above the incoming one
        QCOMPARE(scrollBarHighlights().first().color,
                 Utils::Theme::TextEditor_MergeConflictCurrent_ScrollBarColor);
        QCOMPARE(scrollBarHighlights().last().color,
                 Utils::Theme::TextEditor_MergeConflictIncoming_ScrollBarColor);
        QVERIFY(scrollBarHighlights().first().position
                < scrollBarHighlights().last().position);

        // the markers go with the conflict
        setText("a\nmine\nb");
        QTRY_VERIFY(scrollBarHighlights().isEmpty());
    }

    void testMarksEmptySides_data()
    {
        QTest::addColumn<QString>("text");

        QTest::newRow("empty current side")
            << "a\n<<<<<<< HEAD\n=======\ntheirs\n>>>>>>> branch\nb";
        QTest::newRow("empty incoming side")
            << "a\n<<<<<<< HEAD\nmine\n=======\n>>>>>>> branch\nb";
        QTest::newRow("both sides empty")
            << "a\n<<<<<<< HEAD\n=======\n>>>>>>> branch\nb";
    }

    // a side that holds no lines still owns its marker line, so both sides
    // are marked whichever of them is empty
    void testMarksEmptySides()
    {
        QFETCH(QString, text);

        setText(text);
        QTRY_COMPARE(scrollBarHighlights().size(), 2);
        QCOMPARE(scrollBarHighlights().first().color,
                 Utils::Theme::TextEditor_MergeConflictCurrent_ScrollBarColor);
        QCOMPARE(scrollBarHighlights().last().color,
                 Utils::Theme::TextEditor_MergeConflictIncoming_ScrollBarColor);
        QVERIFY(scrollBarHighlights().first().position < scrollBarHighlights().last().position);
        QVERIFY(scrollBarHighlights().first().length > 0);
        QVERIFY(scrollBarHighlights().last().length > 0);
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

    // Moves the pointer over the widget as the window system reports it,
    // without moving the real mouse cursor as QTest::mouseMove() does.
    static void movePointer(QWidget *widget, const QPoint &pos)
    {
        QMouseEvent move(QEvent::MouseMove, pos, widget->mapToGlobal(pos), Qt::NoButton,
                         Qt::NoButton, Qt::NoModifier);
        QCoreApplication::sendEvent(widget, &move);
    }

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

    QVector<Core::Highlight> scrollBarHighlights() const
    {
        Core::HighlightScrollBarController *controller
            = m_widget->highlightScrollBarController();
        return controller ? controller->highlights().value(Constants::SCROLL_BAR_MERGE_CONFLICT)
                          : QVector<Core::Highlight>();
    }

    QList<QTextLayout::FormatRange> formats(int line) const
    {
        return layout()->blockLayout(block(line))->formats();
    }

    // Whether the line is shown in the "Disabled Code" style, without its
    // syntax colors.
    bool isDisabled(int line) const
    {
        const QTextCharFormat disabled = m_widget->textDocument()->fontSettings().toTextCharFormat(
            C_DISABLED_CODE);
        const QList<QTextLayout::FormatRange> ranges = formats(line);
        return !ranges.isEmpty() && ranges.first().format.background() == disabled.background()
               && ranges.first().format.foreground() == disabled.foreground()
               && ranges.first().format.hasProperty(SUPPRESS_COLORS_FORMAT_PROPERTY_ID);
    }

    bool isBold(int line) const
    {
        const QList<QTextLayout::FormatRange> ranges = formats(line);
        return !ranges.isEmpty() && ranges.first().format.fontWeight() == QFont::Bold;
    }

    std::unique_ptr<TextEditorWidget> m_widget;
    MergeConflictPreviewMode m_previewMode = MergeConflictPreviewMode::HideMarkers;
};

QObject *createMergeConflictTest()
{
    return new MergeConflictTest;
}

} // namespace TextEditor::Internal

#include "mergeconflict_test.moc"
