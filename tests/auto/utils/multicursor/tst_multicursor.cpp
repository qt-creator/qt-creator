// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/algorithm.h>
#include <utils/multitextcursor.h>

#include <QPlainTextEdit>
#include <QtTest>

using namespace Utils;

class tst_MultiCursor : public QObject
{
    Q_OBJECT
public:
    tst_MultiCursor();

private slots:
    void init();
    void initTestCase() { init(); }

    void testMultiCursor_data();
    void testMultiCursor();
    void testMultiCursorMerge_data();
    void testMultiCursorMerge();
    void testMultiCursorMove_data();
    void testMultiCursorMove();
    void testMultiCursorSelection_data();
    void testMultiCursorSelection();
    void testMultiCursorInsert_data();
    void testMultiCursorInsert();

private:
    QString m_text;
    QPlainTextEdit m_edit;
};

tst_MultiCursor::tst_MultiCursor()
{
    m_text =
R"(You can move directly to the definition or the declaration of a symbol in the Edit mode
by holding the Ctrl key and clicking the symbol.

If you have multiple splits opened, you can open the link in the next split by holding
Ctrl and Alt while clicking the symbol.)";
}
void tst_MultiCursor::init()
{
    m_edit.setPlainText(m_text);
}

class Cursor : public QPair<int, int>
{
public:
    Cursor() = default;
    explicit Cursor(const QTextCursor &c) : p(c.position(), c.anchor()) {}
    QTextCursor toTextCursor(QTextDocument *doc)
    {
        if (p.first < 0)
            return {};
        QTextCursor c(doc);
        c.setPosition(p.second);
        c.setPosition(p.first, QTextCursor::KeepAnchor);
        return c;
    };
private:
    QPair<int, int> p;
};

class Cursors
{
public:
    Cursors() = default;
    explicit Cursors(QList<QTextCursor> cursors)
    { m_cursors = Utils::transform(cursors, [](const QTextCursor &c) { return Cursor(c); }); }
    void append(const QTextCursor &c)
    { m_cursors << Cursor(c); }
    QList<QTextCursor> toTextCurors(QTextDocument *doc)
    { return Utils::transform(m_cursors, [doc](Cursor c){ return c.toTextCursor(doc); }); }
private:
    QList<Cursor> m_cursors;
};

Cursors _(const QList<QTextCursor> &cursors)
{
    return Cursors(cursors);
}

Q_DECLARE_METATYPE(Cursor);
Q_DECLARE_METATYPE(Cursors);

void tst_MultiCursor::testMultiCursor_data()
{
    QTest::addColumn<Cursors>("cursors");
    QTest::addColumn<bool>("null");
    QTest::addColumn<bool>("multiple");
    QTest::addColumn<int>("cursorCount");
    QTest::addColumn<Cursor>("mainCursor");

    Cursors cursors;
    QTest::addRow("No Cursor") << cursors << true <<  false << 0 << Cursor(QTextCursor());
    QTest::addRow("Null Cursor") << _({QTextCursor()}) << true <<  false << 0 << Cursor(QTextCursor());

    QTextCursor c1 = m_edit.textCursor();
    c1.movePosition(QTextCursor::Start);
    cursors.append(c1);

    QTest::addRow("Single Cursor") << cursors << false <<  false << 1 << Cursor(c1);

    QString plainText = m_edit.toPlainText();
    QTextCursor c2 = m_edit.textCursor();
    c2.movePosition(QTextCursor::End);
    cursors.append(c2);

    QTest::addRow("Multiple Cursors") << cursors << false <<  true << 2 << Cursor(c2);
}

void tst_MultiCursor::testMultiCursor()
{
    QFETCH(Cursors, cursors);
    QFETCH(bool, null);
    QFETCH(bool, multiple);
    QFETCH(int, cursorCount);
    QFETCH(Cursor, mainCursor);

    MultiTextCursor cursor(cursors.toTextCurors(m_edit.document()));
    QCOMPARE(cursor.isNull(), null);
    QCOMPARE(cursor.hasMultipleCursors(), multiple);
    QCOMPARE(cursor.cursorCount(), cursorCount);
    QCOMPARE(cursor.mainCursor(), mainCursor.toTextCursor(m_edit.document()));
    MultiTextCursor cursor2;
    for (const auto &c : cursors.toTextCurors(m_edit.document()))
        cursor2.addCursor(c);
    QCOMPARE(cursor2, cursor);

}

void tst_MultiCursor::testMultiCursorMerge_data()
{
    QTest::addColumn<Cursors>("input");
    QTest::addColumn<Cursors>("output");

    QTextCursor null;

    QTest::addRow("Null Cursor") << _({null, null}) << Cursors{};

    QTextCursor c1 = m_edit.textCursor();
    QTest::addRow("Null and Valid Cursor") << _({null, c1}) << _({c1});
    QTest::addRow("Identical Cursors") << _({c1, c1}) << _({c1});

    QTextCursor c2 = m_edit.textCursor();
    c2.movePosition(QTextCursor::End);
    QTest::addRow("Different Cursors") << _({c1, c2}) << _({c1, c2});

    c1.select(QTextCursor::LineUnderCursor);
    c2.movePosition(QTextCursor::Start);
    QTest::addRow("Position at Selection Start") << _({c1, c2}) << _({c1, c2});

    c2.movePosition(QTextCursor::EndOfBlock);
    QTest::addRow("Position at Selection End") << _({c1, c2}) << _({c1, c2});

    c2.movePosition(QTextCursor::PreviousCharacter);
    QTest::addRow("Position in Selection") << _({c1, c2}) << _({c1});

    auto setCursor = [](QTextCursor &c, int pos, int anchor){
        c.setPosition(anchor);
        c.setPosition(pos, QTextCursor::KeepAnchor);
    };
    setCursor(c1, 3, 6);
    setCursor(c2, 1, 2);

    QTest::addRow("Different Selections") << _({c1, c2}) << _({c1, c2});

    setCursor(c2, 2, 3);
    QTest::addRow("Adjacent Selections") << _({c1, c2}) << _({c1, c2});

    setCursor(c2, 2, 4);
    QTextCursor mc = m_edit.textCursor();
    setCursor(mc, 2, 6);
    QTest::addRow("Overlapping Selections") << _({c1, c2}) << _({mc});

    setCursor(c2, 4, 5);
    QTest::addRow("Enclosing Selection") << _({c1, c2}) << _({c1});
}

void tst_MultiCursor::testMultiCursorMerge()
{
    QFETCH(Cursors, input);
    QFETCH(Cursors, output);

    QCOMPARE(MultiTextCursor(input.toTextCurors(m_edit.document())).cursors(),
             output.toTextCurors(m_edit.document()));
}

Q_DECLARE_METATYPE(QTextCursor::MoveOperation)
Q_DECLARE_METATYPE(QTextCursor::MoveMode)

void tst_MultiCursor::testMultiCursorMove_data()
{
    QTest::addColumn<Cursors>("input");
    QTest::addColumn<QTextCursor::MoveOperation>("op");
    QTest::addColumn<QTextCursor::MoveMode>("mode");
    QTest::addColumn<int>("n");
    QTest::addColumn<Cursors>("output");

    QTextCursor c1 = m_edit.textCursor();

    QTextCursor cend = c1;
    cend.movePosition(QTextCursor::End);

    QTest::addRow("Single Cursor")
        << _({c1}) << QTextCursor::End << QTextCursor::MoveAnchor << 1 << _({cend});

    QTextCursor c1endl = c1;
    c1endl.movePosition(QTextCursor::EndOfLine);

    QTest::addRow("Double Cursor") << _({c1, cend}) << QTextCursor::EndOfLine
                                   << QTextCursor::MoveAnchor << 1 << _({c1endl, cend});

    QTextCursor cnn = c1;
    cnn.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 2);

    QTextCursor c1endlnn = c1endl;
    c1endlnn.movePosition(QTextCursor::NextCharacter, QTextCursor::MoveAnchor, 2);

    QTest::addRow("Double Cursor Move Twice")
        << _({c1, c1endl}) << QTextCursor::NextCharacter << QTextCursor::MoveAnchor << 2
        << _({cnn, c1endlnn});

    QTest::addRow("Double Cursor Merge")
        << Cursors({c1, cend}) << QTextCursor::End << QTextCursor::MoveAnchor << 1 << _({cend});

    QTextCursor cdoc = c1;
    cdoc.select(QTextCursor::Document);

    QTest::addRow("Double Cursor KeepAnchor")
        << _({c1, cend}) << QTextCursor::End << QTextCursor::KeepAnchor << 1 << _({cdoc, cend});
}

void tst_MultiCursor::testMultiCursorMove()
{
    QFETCH(Cursors, input);
    QFETCH(QTextCursor::MoveOperation, op);
    QFETCH(QTextCursor::MoveMode, mode);
    QFETCH(int, n);
    QFETCH(Cursors, output);

    MultiTextCursor cursor(input.toTextCurors(m_edit.document()));
    cursor.movePosition(op, mode, n);

    QCOMPARE(cursor.cursors(), output.toTextCurors(m_edit.document()));
}

void tst_MultiCursor::testMultiCursorSelection_data()
{
    QTest::addColumn<Cursors>("input");
    QTest::addColumn<bool>("hasSelection");
    QTest::addColumn<QString>("selectedText");
    QTest::addColumn<QString>("removedText");

    QTest::addRow("No Cursor") << Cursors() << false << QString() << m_text;
    QTextCursor c1 = m_edit.textCursor();
    QTest::addRow("One Cursor No Selection") << _({c1}) << false << QString() << m_text;
    QTextCursor c2 = c1;
    c2.movePosition(QTextCursor::NextBlock);
    QTest::addRow("Multiple Cursor No Selection") << _({c1, c2}) << false << QString() << m_text;
    c1.select(QTextCursor::WordUnderCursor);
    QTest::addRow("One Cursor With Selection") << _({c1}) << true << "You" << m_text.mid(3);
    QTest::addRow("Two Cursor Mixed Selection") << _({c1, c2}) << true << "You" << m_text.mid(3);
    c2.select(QTextCursor::WordUnderCursor);
    QTest::addRow("Two Cursor With Selection") << _({c1, c2}) << true << "You\nby" << R"( can move directly to the definition or the declaration of a symbol in the Edit mode
 holding the Ctrl key and clicking the symbol.

If you have multiple splits opened, you can open the link in the next split by holding
Ctrl and Alt while clicking the symbol.)";

}

void tst_MultiCursor::testMultiCursorSelection()
{
    QFETCH(Cursors, input);
    QFETCH(bool, hasSelection);
    QFETCH(QString, selectedText);
    QFETCH(QString, removedText);

    MultiTextCursor cursor(input.toTextCurors(m_edit.document()));
    QCOMPARE(cursor.hasSelection(), hasSelection);
    QCOMPARE(cursor.selectedText(), selectedText);
    if (cursor.cursorCount() > 0) // prevent triggering an assert in MultiTextCursor::beginEditBlock
        cursor.removeSelectedText();
    QCOMPARE(m_edit.toPlainText(), removedText);
}

void tst_MultiCursor::testMultiCursorInsert_data()
{
    QTest::addColumn<Cursors>("input");
    QTest::addColumn<QString>("insertText");
    QTest::addColumn<QString>("editedText");

    QString foo = "foo";
    QString bar = "bar";
    QString foobar = foo + '\n' + bar;
    QTest::addRow("No Cursor") << Cursors() << foo << m_text;
    QTextCursor c1 = m_edit.textCursor();
    QTest::addRow("Single Cursor") << _({c1}) << foo << QString(foo + m_text);
    QTextCursor c2 = m_edit.textCursor();
    c2.movePosition(QTextCursor::NextCharacter);
    QTest::addRow("Multi Cursor") << _({c1, c2}) << foo
                                  << QString(foo + m_text.at(0) + foo + m_text.mid(1));
    QTest::addRow("Single Cursor Multi Line Insert Text")
        << _({c1}) << foobar << QString(foobar + m_text);
    QTest::addRow("Multi Cursor Multi Line Insert Text")
        << _({c1, c2}) << foobar << QString(foo + m_text.at(0) + bar + m_text.mid(1));
}

void tst_MultiCursor::testMultiCursorInsert()
{
    QFETCH(Cursors, input);
    QFETCH(QString, insertText);
    QFETCH(QString, editedText);

    MultiTextCursor cursor(input.toTextCurors(m_edit.document()));
    cursor.insertText(insertText);
    QString pt = m_edit.toPlainText();
    QCOMPARE(m_edit.toPlainText(), editedText);
}

QTEST_MAIN(tst_MultiCursor)

#include "tst_multicursor.moc"
