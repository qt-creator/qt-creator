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

private:
    static InlineDiffRenderModel compute(const QString &baseline, const QString &editor);
};

InlineDiffRenderModel tst_InlineDiff::compute(const QString &baseline, const QString &editor)
{
    // mirrors the inline diff editor's diff pipeline
    Utils::Differ differ;
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
