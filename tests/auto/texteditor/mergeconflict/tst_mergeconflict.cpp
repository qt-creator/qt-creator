// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <texteditor/mergeconflict.h>

#include <QTest>
#include <QTextDocument>

// The MergeConflictController needs a text editor widget, and that needs a
// running Core: it is tested by the TextEditor plugin test instead.

using namespace TextEditor;

// "a", the conflict, "b": "mine" is the current and "theirs" the incoming side
const char conflictText[] = "a\n<<<<<<< HEAD\nmine\n=======\ntheirs\n>>>>>>> branch\nb";

class tst_MergeConflict : public QObject
{
    Q_OBJECT

private slots:
    void findConflicts();
    void resolveConflicts();
};

void tst_MergeConflict::findConflicts()
{
    QTextDocument doc;
    doc.setPlainText(QLatin1StringView(conflictText));
    const QList<MergeConflict> conflicts = findMergeConflicts(&doc);
    QCOMPARE(conflicts.size(), 1);
    QCOMPARE(conflicts.first().startLine, 2);
    QCOMPARE(conflicts.first().baseLine, -1);
    QCOMPARE(conflicts.first().separatorLine, 4);
    QCOMPARE(conflicts.first().endLine, 6);

    // diff3 style adds a "|||||||" base section
    QTextDocument diff3;
    diff3.setPlainText("<<<<<<< HEAD\nmine\n||||||| base\nold\n=======\ntheirs\n>>>>>>> branch");
    const QList<MergeConflict> diff3Conflicts = findMergeConflicts(&diff3);
    QCOMPARE(diff3Conflicts.size(), 1);
    QCOMPARE(diff3Conflicts.first().baseLine, 3);
    QCOMPARE(diff3Conflicts.first().separatorLine, 5);

    QTextDocument clean;
    clean.setPlainText("a\nb\nc");
    QVERIFY(findMergeConflicts(&clean).isEmpty());

    // Git names the side each marker opens, so a bare row of sevens is text
    QTextDocument unnamed;
    unnamed.setPlainText("<<<<<<<\nmine\n=======\ntheirs\n>>>>>>>");
    QVERIFY(findMergeConflicts(&unnamed).isEmpty());

    // ... and writes the separator alone on its line, so one with something
    // after it is text as well, leaving the conflict unterminated
    QTextDocument trailing;
    trailing.setPlainText("<<<<<<< HEAD\nmine\n======= too\ntheirs\n>>>>>>> branch");
    QVERIFY(findMergeConflicts(&trailing).isEmpty());

    // the conflict-marker-size gitattribute makes Git write longer markers
    QTextDocument long17;
    long17.setPlainText("<<<<<<<<<<<<<<<<< HEAD\nmine\n=================\ntheirs\n"
                        ">>>>>>>>>>>>>>>>> branch");
    const QList<MergeConflict> longConflicts = findMergeConflicts(&long17);
    QCOMPARE(longConflicts.size(), 1);
    QCOMPARE(longConflicts.first().separatorLine, 3);
    QCOMPARE(longConflicts.first().endLine, 5);

    // a run shorter than seven is not a marker at any length
    QTextDocument shortRun;
    shortRun.setPlainText("<<<<<< HEAD\nmine\n======\ntheirs\n>>>>>> branch");
    QVERIFY(findMergeConflicts(&shortRun).isEmpty());

    // only whitespace may follow the separator, and it may
    QTextDocument padded;
    padded.setPlainText("<<<<<<< HEAD\nmine\n=======  \ntheirs\n>>>>>>> branch");
    QCOMPARE(findMergeConflicts(&padded).size(), 1);
}

void tst_MergeConflict::resolveConflicts()
{
    const auto resolved = [](const QString &text, MergeConflictChoice choice) {
        QTextDocument doc;
        doc.setPlainText(text);
        resolveMergeConflict(&doc, findMergeConflicts(&doc).first(), choice);
        return doc.toPlainText();
    };
    const QString text = QLatin1StringView(conflictText);
    QCOMPARE(resolved(text, MergeConflictChoice::Current), QString("a\nmine\nb"));
    QCOMPARE(resolved(text, MergeConflictChoice::Incoming), QString("a\ntheirs\nb"));
    QCOMPARE(resolved(text, MergeConflictChoice::Both), QString("a\nmine\ntheirs\nb"));

    // diff3: the "|||||||" base section is dropped, never taken as content
    const QString diff3
        = "a\n<<<<<<< HEAD\nmine\n||||||| base\nold\n=======\ntheirs\n>>>>>>> branch\nb";
    QCOMPARE(resolved(diff3, MergeConflictChoice::Current), QString("a\nmine\nb"));
    QCOMPARE(resolved(diff3, MergeConflictChoice::Both), QString("a\nmine\ntheirs\nb"));

    // choosing a side that is a single blank line keeps that line, rather than
    // collapsing the block as if the side were empty
    const QString blank = "a\n<<<<<<< HEAD\n\n=======\ntheirs\n>>>>>>> branch\nb";
    QCOMPARE(resolved(blank, MergeConflictChoice::Current), QString("a\n\nb"));
}

QTEST_GUILESS_MAIN(tst_MergeConflict)

#include "tst_mergeconflict.moc"
