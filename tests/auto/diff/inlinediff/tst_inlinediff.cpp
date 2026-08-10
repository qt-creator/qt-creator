// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <diffeditor/diffutils.h>
#include <diffeditor/inlinediff.h>

#include <utils/differ.h>

#include <QTest>

using namespace DiffEditor;
using namespace TextEditor;

class tst_InlineDiff : public QObject
{
    Q_OBJECT

private slots:
    void identicalTexts();
    void modifiedLine();
    void pureDeletion();
    void deletionAtStart();
    void deletionAtEnd();
    void pureAddition();
    void multiLineModification();
    void modifyDeleteAdd();
    void newlineOnlyDifference();
    void changeOnLastLineWithoutNewline();
    void patienceAnchorsUniqueLines();
    void patienceIsOff();
    void patienceAvoidsSpuriousModification();

private:
    static InlineDiffRenderModel compute(const QString &baseline, const QString &editor,
                                         bool patience = false);
};

// The repeated lines make the smallest set of changes ambiguous: the Myers diff
// may pair up any of them, while the patience diff lines the two sides up at the
// lines that occur only once on each side. Here that is the difference between
// reporting one removed line and reporting two of them plus a wider change.
static QString patienceBaseline()
{
    return QStringList{"delta();", "{", "alpha();", "beta();", "delta();", "delta();", "beta();"}
               .join('\n') + '\n';
}

static QString patienceEditor()
{
    return QStringList{"delta();", "{", "beta();", "delta();", "", "delta();", "alpha();",
                       "beta();"}.join('\n') + '\n';
}

static QString patchFor(const QString &baseline, const QString &editor, bool patience)
{
    Utils::Differ differ;
    differ.setPatience(patience);
    const QList<Utils::Diff> diffList
        = Utils::Differ::cleanupSemantics(differ.diff(baseline, editor));
    QList<Utils::Diff> left;
    QList<Utils::Diff> right;
    Utils::Differ::splitDiffList(diffList, &left, &right);
    const ChunkData chunk = DiffUtils::calculateOriginalData(left, right);
    FileData fileData = DiffUtils::calculateContextData(chunk, 3);
    fileData.fileInfo[LeftSide] = DiffFileInfo("example");
    fileData.fileInfo[RightSide] = DiffFileInfo("example");
    return DiffUtils::makePatch({fileData});
}

// What the option is for, as a patch: a file full of repeated calls with two
// changes in it. Without patience the added line is reported as a modification
// of an unchanged line above it, because that set of changes is just as small.
void tst_InlineDiff::patienceAvoidsSpuriousModification()
{
    const QStringList baselineLines{"void Session::sync()",
                                    "{",
                                    "    trace();",
                                    "    trace();",
                                    "",
                                    "    connect();",
                                    "    prepare();",
                                    "",
                                    "    lock();",
                                    "    trace();",
                                    "    flush();",
                                    "    prepare();",
                                    "    send();",
                                    "    send();",
                                    "    flush();",
                                    "    prepare();",
                                    "",
                                    "    send();",
                                    "    trace();",
                                    "    unlock();",
                                    "}"};
    QStringList editorLines = baselineLines;
    editorLines[2] = "    send();";         // a changed line near the top
    editorLines.insert(15, "    flush();"); // an added line further down
    const QString baseline = baselineLines.join('\n') + '\n';
    const QString editor = editorLines.join('\n') + '\n';

    const QString withoutPatience = patchFor(baseline, editor, false);
    const QString withPatience = patchFor(baseline, editor, true);

    // both report the changed line
    QVERIFY(withoutPatience.contains("-    trace();\n+    send();\n"));
    QVERIFY(withPatience.contains("-    trace();\n+    send();\n"));

    // the added line: patience reports just that, while the Myers alignment
    // takes an unchanged "send();" line along as a modification of itself
    QVERIFY(withPatience.contains("     flush();\n+    flush();\n"));
    QVERIFY(!withPatience.contains("-    send();\n+    send();\n"));
    QVERIFY(withoutPatience.contains("-    send();\n+    send();\n"));
}

void tst_InlineDiff::patienceAnchorsUniqueLines()
{
    const InlineDiffRenderModel model = compute(patienceBaseline(), patienceEditor(),
                                                /*patience=*/true);
    // "alpha();" occurs once on each side, so it is lined up: the only line
    // that went away above it is the first "beta();"
    QCOMPARE(model.ghosts.size(), 1);
    QCOMPARE(model.ghosts.first().anchorLine, 3);
    QCOMPARE(model.ghosts.first().lines, QStringList("alpha();"));
}

void tst_InlineDiff::patienceIsOff()
{
    // without it the same texts are lined up at the repeated lines instead,
    // which spreads the change over more rows
    const InlineDiffRenderModel model = compute(patienceBaseline(), patienceEditor());
    QCOMPARE(model.ghosts.size(), 2);
    QCOMPARE(model.ghosts.first().lines, QStringList({"alpha();", "beta();"}));
}

InlineDiffRenderModel tst_InlineDiff::compute(const QString &baseline, const QString &editor,
                                             bool patience)
{
    // mirrors the inline diff editor's diff pipeline
    Utils::Differ differ;
    differ.setPatience(patience);
    const QList<Utils::Diff> diffList
        = Utils::Differ::cleanupSemantics(differ.diff(baseline, editor));
    QList<Utils::Diff> leftDiffList;
    QList<Utils::Diff> rightDiffList;
    Utils::Differ::splitDiffList(diffList, &leftDiffList, &rightDiffList);
    return mapChunkToRenderModel(DiffUtils::calculateOriginalData(leftDiffList, rightDiffList),
                                 baseline.endsWith('\n'), editor.endsWith('\n'));
}

void tst_InlineDiff::identicalTexts()
{
    const InlineDiffRenderModel model = compute("a\nb\nc\n", "a\nb\nc\n");
    QVERIFY(model.isEmpty());
}

void tst_InlineDiff::modifiedLine()
{
    const InlineDiffRenderModel model = compute("a\nfoo bar\nc\n", "a\nfoo baz\nc\n");
    QCOMPARE(model.ghosts.size(), 1);
    QCOMPARE(model.ghosts.first().anchorLine, 2);
    QCOMPARE(model.ghosts.first().lines, QStringList("foo bar"));
    QCOMPARE(model.changes.size(), 1);
    QCOMPARE(model.changes.first().startLine, 2);
    QCOMPARE(model.changes.first().endLine, 2);
    QVERIFY(model.changes.first().charHighlights.contains(2));
}

void tst_InlineDiff::pureDeletion()
{
    const InlineDiffRenderModel model = compute("a\nb\nc\n", "a\nc\n");
    QCOMPARE(model.ghosts.size(), 1);
    QCOMPARE(model.ghosts.first().anchorLine, 2); // shown above the "c" line
    QCOMPARE(model.ghosts.first().lines, QStringList("b"));
    QVERIFY(model.changes.isEmpty());
}

void tst_InlineDiff::deletionAtStart()
{
    const InlineDiffRenderModel model = compute("a\nb\nc\n", "b\nc\n");
    QCOMPARE(model.ghosts.size(), 1);
    QCOMPARE(model.ghosts.first().anchorLine, 1);
    QCOMPARE(model.ghosts.first().lines, QStringList("a"));
    QVERIFY(model.changes.isEmpty());
}

void tst_InlineDiff::deletionAtEnd()
{
    const InlineDiffRenderModel model = compute("a\nb\nc", "a\nb");
    QCOMPARE(model.ghosts.size(), 1);
    // editor has two lines, the removed line shows below the last one
    QCOMPARE(model.ghosts.first().anchorLine, 3);
    QCOMPARE(model.ghosts.first().lines, QStringList("c"));
}

void tst_InlineDiff::pureAddition()
{
    const InlineDiffRenderModel model = compute("a\nc\n", "a\nb\nc\n");
    QVERIFY(model.ghosts.isEmpty());
    QCOMPARE(model.changes.size(), 1);
    QCOMPARE(model.changes.first().startLine, 2);
    QCOMPARE(model.changes.first().endLine, 2);
}

void tst_InlineDiff::multiLineModification()
{
    const InlineDiffRenderModel model
        = compute("a\none\ntwo\nb\n", "a\neins\nzwei\ndrei\nb\n");
    QCOMPARE(model.ghosts.size(), 1);
    QCOMPARE(model.ghosts.first().anchorLine, 2);
    QCOMPARE(model.ghosts.first().lines, (QStringList{"one", "two"}));
    QCOMPARE(model.changes.size(), 1);
    QCOMPARE(model.changes.first().startLine, 2);
    QCOMPARE(model.changes.first().endLine, 4);
}

void tst_InlineDiff::modifyDeleteAdd()
{
    // line 2 modified, line 3 removed, "five" added at the end
    const InlineDiffRenderModel model
        = compute("one\ntwo\nthree\nfour\n", "one\ntwo changed\nfour\nfive\n");
    QCOMPARE(model.ghosts.size(), 1);
    QCOMPARE(model.ghosts.first().anchorLine, 2);
    QCOMPARE(model.ghosts.first().lines, (QStringList{"two", "three"}));
    QCOMPARE(model.changes.size(), 2);
    QCOMPARE(model.changes.first().startLine, 2);
    QCOMPARE(model.changes.first().endLine, 2);
    QCOMPARE(model.changes.last().startLine, 4);
    QCOMPARE(model.changes.last().endLine, 4);

    // side by side data: baseline lines 2-3 changed. The row alignment of the
    // two views is derived from the hunks below (the side by side aligner pads
    // the shrunken first run and reserves a row for the added "five").
    QCOMPARE(model.baselineChanges.size(), 1);
    QCOMPARE(model.baselineChanges.first().startLine, 2);
    QCOMPARE(model.baselineChanges.first().endLine, 3);

    QCOMPARE(model.hunks.size(), 2);
    QCOMPARE(model.hunks.first().editorStartLine, 2);
    QCOMPARE(model.hunks.first().editorLineCount, 1);
    QCOMPARE(model.hunks.first().baselineStartLine, 2);
    QCOMPARE(model.hunks.first().baselineLines, (QStringList{"two", "three"}));
    QCOMPARE(model.hunks.last().editorStartLine, 4);
    QCOMPARE(model.hunks.last().editorLineCount, 1);
    QCOMPARE(model.hunks.last().baselineStartLine, 5);
    QVERIFY(model.hunks.last().baselineLines.isEmpty());
}

void tst_InlineDiff::newlineOnlyDifference()
{
    // a difference only in the trailing newline has no visible line: it must
    // not produce decorations, and especially no hunk offering actions
    for (const auto &[baseline, editor] : std::initializer_list<std::pair<QString, QString>>{
             {"a\nb\n", "a\nb"}, {"a\nb", "a\nb\n"}}) {
        const InlineDiffRenderModel model = compute(baseline, editor);
        QVERIFY(model.isEmpty());
        QVERIFY(model.hunks.isEmpty());
        QCOMPARE(model.baselineEndsWithNewline, baseline.endsWith('\n'));
        QCOMPARE(model.editorEndsWithNewline, editor.endsWith('\n'));
    }
}

void tst_InlineDiff::changeOnLastLineWithoutNewline()
{
    const InlineDiffRenderModel model = compute("a\nlast\n", "a\nlast changed");
    QCOMPARE(model.hunks.size(), 1);
    QCOMPARE(model.hunks.first().editorStartLine, 2);
    QCOMPARE(model.hunks.first().editorLineCount, 1);
    QCOMPARE(model.hunks.first().baselineLines, QStringList("last"));
    QVERIFY(model.baselineEndsWithNewline);
    QVERIFY(!model.editorEndsWithNewline);
}

QTEST_GUILESS_MAIN(tst_InlineDiff)

#include "tst_inlinediff.moc"
