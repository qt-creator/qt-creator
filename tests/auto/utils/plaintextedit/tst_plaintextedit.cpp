// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <utils/plaintextedit/plaintextedit.h>

#include <QFontMetrics>
#include <QTest>
#include <QTextCursor>

using namespace Utils;

class tst_PlainTextEdit : public QObject
{
    Q_OBJECT

private slots:
    void overwriteCursorOnWrappedLine();
};

void tst_PlainTextEdit::overwriteCursorOnWrappedLine()
{
    PlainTextEdit edit;
    const QFontMetrics fm(edit.font());
    if (fm.horizontalAdvance(u'W') == fm.horizontalAdvance(u' '))
        QSKIP("'W' and ' ' have the same advance in the default font.");

    edit.setLineWrapMode(PlainTextEdit::WidgetWidth);
    edit.setOverwriteMode(true);
    edit.resize(fm.horizontalAdvance(QString(20, u'W')), 200);
    edit.show();
    edit.setPlainText(QString(10, u'W') + QString(" WWWWWWWWWW").repeated(10));

    QTextCursor cursor(edit.document());
    const QRect firstLineRect = edit.cursorRect(cursor);
    cursor.movePosition(QTextCursor::End);
    cursor.movePosition(QTextCursor::Left);
    const QRect wrappedLineRect = edit.cursorRect(cursor);

    QVERIFY(wrappedLineRect.top() > firstLineRect.top());
    QCOMPARE(wrappedLineRect.width(), firstLineRect.width());
}

QTEST_MAIN(tst_PlainTextEdit)

#include "tst_plaintextedit.moc"
