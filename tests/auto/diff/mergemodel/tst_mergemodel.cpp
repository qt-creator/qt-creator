// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <diffeditor/mergemodel.h>
#include <diffeditor/textdiffmodel.h>

#include <QTest>
#include <QPromise>

Q_DECLARE_METATYPE(DiffEditor::MergeRegionKind)

using namespace DiffEditor;

class tst_MergeModel final : public QObject
{
    Q_OBJECT

private slots:
    void lineMapping_data();
    void lineMapping();
    void canceledLineMappingReturnsWithoutPadding();
    void parseResult_data();
    void parseResult();
    void oneSidedChanges_data();
    void oneSidedChanges();
    void identicalTexts();
    void identicalChange();
    void conflictingChange();
    void multiLineConflict();
    void identicalDeletion();
    void disjointChanges();
    void adjacentChangesAreAutoMerged();
    void insertionAtSameAnchor();
    void deletionAgainstEdit();
    void addAddConflict();
    void absentSideIsNotEmptyFile();
    void deleteVsEmptyFile();
    void emptyFileExistenceChange();
    void trailingNewlineDifference();
    void repeatedLinesWithPatience();
};

void tst_MergeModel::lineMapping_data()
{
    QTest::addColumn<QString>("source");
    QTest::addColumn<QString>("result");
    QTest::addColumn<QList<int>>("lines");
    QTest::newRow("unchanged") << "a\nb\n" << "a\nb\n" << QList<int>{1, 2, 3};
    QTest::newRow("inserted lines") << "a\nb\n" << "a\nx\ny\nb\n"
                                    << QList<int>{1, 1, 1, 2, 3};
    QTest::newRow("deleted lines") << "a\nx\ny\nb\n" << "a\nb\n"
                                   << QList<int>{1, 4, 5};
    QTest::newRow("empty source") << "" << "a\nb" << QList<int>{1, 1};
    QTest::newRow("empty result") << "a\nb" << "" << QList<int>{2};
}

void tst_MergeModel::lineMapping()
{
    QFETCH(QString, source);
    QFETCH(QString, result);
    QFETCH(QList<int>, lines);
    QCOMPARE(computeTextLineMapping(source, result), lines);
}

void tst_MergeModel::canceledLineMappingReturnsWithoutPadding()
{
    QPromise<void> promise;
    QFuture<void> cancellation = promise.future();
    cancellation.cancel();
    const QString manyLines = QString("line\n").repeated(10000);
    QVERIFY(computeTextLineMapping(manyLines, "different", cancellation).isEmpty());
}

void tst_MergeModel::parseResult_data()
{
    QTest::addColumn<QString>("input");
    QTest::addColumn<int>("count");
    QTest::addColumn<QString>("ours");
    QTest::addColumn<QString>("theirs");
    QTest::addColumn<bool>("hasBase");
    QTest::newRow("normal") << "before\n<<<<<<< HEAD\nours\n=======\ntheirs\n>>>>>>> branch\nafter\n"
                            << 1 << "ours" << "theirs" << false;
    QTest::newRow("diff3") << "<<<<<<< HEAD\nours\n||||||| base\nbase\n=======\ntheirs\n>>>>>>> branch\n"
                           << 1 << "ours" << "theirs" << true;
    QTest::newRow("custom width") << "<<<<<<<<<< HEAD\nours\n==========\ntheirs\n>>>>>>>>>> branch"
                                  << 1 << "ours" << "theirs" << false;
    QTest::newRow("empty side") << "<<<<<<< HEAD\n=======\ntheirs\n>>>>>>> branch\n"
                                << 1 << "" << "theirs" << false;
    QTest::newRow("unterminated") << "before\n<<<<<<< HEAD\nours\n=======\ntheirs\n"
                                  << 0 << "" << "" << false;
    QTest::newRow("no separator") << "<<<<<<< HEAD\nours\n>>>>>>> branch\nafter\n"
                                  << 0 << "" << "" << false;
    QTest::newRow("literal prefix") << "<<<<<<<literal\nkeep\n>>>>>>> branch\n"
                                    << 0 << "" << "" << false;
    QTest::newRow("mismatched width") << "<<<<<<< HEAD\nours\n========\ntheirs\n>>>>>>> branch\n"
                                      << 0 << "" << "" << false;
    QTest::newRow("collision") << "Unresolved conflict 1\n<<<<<<< HEAD\nours\n=======\ntheirs\n>>>>>>> branch\n"
                               << 1 << "ours" << "theirs" << false;
}

void tst_MergeModel::parseResult()
{
    QFETCH(QString, input);
    QFETCH(int, count);
    QFETCH(QString, ours);
    QFETCH(QString, theirs);
    QFETCH(bool, hasBase);
    const ParsedMergeResult parsed = parseMergeResult(input);
    QCOMPARE(parsed.conflicts.size(), count);
    if (!count) {
        QCOMPARE(parsed.text, input);
        return;
    }
    QCOMPARE(parsed.conflicts.first().ours.join('\n'), ours);
    QCOMPARE(parsed.conflicts.first().theirs.join('\n'), theirs);
    QCOMPARE(parsed.conflicts.first().base.has_value(), hasBase);
    if (hasBase)
        QCOMPARE(*parsed.conflicts.first().base, QStringList{"base"});
    QVERIFY(!input.contains(parsed.placeholderPrefix));
    QVERIFY(parsed.text.contains(parsed.placeholderPrefix + '1'));
    QCOMPARE(parsed.text.endsWith('\n'), input.endsWith('\n'));
}

void tst_MergeModel::oneSidedChanges_data()
{
    QTest::addColumn<QString>("base");
    QTest::addColumn<QString>("ours");
    QTest::addColumn<QString>("theirs");
    QTest::addColumn<MergeRegionKind>("kind");
    QTest::addColumn<QStringList>("oursLines");
    QTest::addColumn<QStringList>("theirsLines");

    QTest::newRow("ours only") << "a\nb\nc\n" << "a\nours\nc\n" << "a\nb\nc\n"
                               << MergeRegionKind::OursOnly << QStringList{"ours"}
                               << QStringList{"b"};
    QTest::newRow("theirs only") << "a\nb\nc\n" << "a\nb\nc\n" << "a\ntheirs\nc\n"
                                 << MergeRegionKind::TheirsOnly << QStringList{"b"}
                                 << QStringList{"theirs"};
}

void tst_MergeModel::oneSidedChanges()
{
    QFETCH(QString, base);
    QFETCH(QString, ours);
    QFETCH(QString, theirs);
    QFETCH(MergeRegionKind, kind);
    QFETCH(QStringList, oursLines);
    QFETCH(QStringList, theirsLines);

    const MergeModel model = computeMergeModel(base, ours, theirs);
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, kind);
    QCOMPARE(model.regions.first().oursLines, oursLines);
    QCOMPARE(model.regions.first().theirsLines, theirsLines);
}

void tst_MergeModel::identicalTexts()
{
    const MergeModel model = computeMergeModel("a\nb\n", "a\nb\n", "a\nb\n");
    QVERIFY(model.computed);
    QVERIFY(model.regions.isEmpty());
}

void tst_MergeModel::identicalChange()
{
    const MergeModel model = computeMergeModel("a\nb\nc\n", "a\nchanged\nc\n",
                                               "a\nchanged\nc\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::BothSame);
    QCOMPARE(model.regions.first().base, (MergeTextRange{2, 1}));
}

void tst_MergeModel::conflictingChange()
{
    const MergeModel model = computeMergeModel("a\nb\nc\n", "a\nours\nc\n",
                                               "a\ntheirs\nc\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QCOMPARE(model.regions.first().baseLines, QStringList{"b"});
}

void tst_MergeModel::multiLineConflict()
{
    const MergeModel model = computeMergeModel("a\nb\nc\nd\n", "a\nours1\nours2\nd\n",
                                               "a\ntheirs1\ntheirs2\nd\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QCOMPARE(model.regions.first().base, (MergeTextRange{2, 2}));
    QCOMPARE(model.regions.first().baseLines, QStringList({"b", "c"}));
    QCOMPARE(model.regions.first().oursLines, QStringList({"ours1", "ours2"}));
    QCOMPARE(model.regions.first().theirsLines, QStringList({"theirs1", "theirs2"}));
}

void tst_MergeModel::identicalDeletion()
{
    const MergeModel model = computeMergeModel("a\nb\nc\n", "a\nc\n", "a\nc\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::BothSame);
    QCOMPARE(model.regions.first().base, (MergeTextRange{2, 1}));
    QVERIFY(model.regions.first().oursLines.isEmpty());
    QVERIFY(model.regions.first().theirsLines.isEmpty());
}

void tst_MergeModel::disjointChanges()
{
    const MergeModel model = computeMergeModel("a\nb\nc\nd\n", "ours\nb\nc\nd\n",
                                               "a\nb\nc\ntheirs\n");
    QCOMPARE(model.regions.size(), 2);
    QCOMPARE(model.regions.at(0).kind, MergeRegionKind::OursOnly);
    QCOMPARE(model.regions.at(1).kind, MergeRegionKind::TheirsOnly);
}

void tst_MergeModel::adjacentChangesAreAutoMerged()
{
    const MergeModel model = computeMergeModel("a\nb\nc\n", "ours\nb\nc\n",
                                               "a\ntheirs\nc\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::AutoMerged);
    QCOMPARE(model.regions.first().oursLines, QStringList({"ours", "b"}));
    QCOMPARE(model.regions.first().theirsLines, QStringList({"a", "theirs"}));
}

void tst_MergeModel::insertionAtSameAnchor()
{
    const MergeModel model = computeMergeModel("a\nc\n", "a\nours\nc\n",
                                               "a\ntheirs\nc\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QCOMPARE(model.regions.first().base.lineCount, 0);
    QCOMPARE(model.regions.first().base.startLine, 2);
}

void tst_MergeModel::deletionAgainstEdit()
{
    const MergeModel model = computeMergeModel("a\nb\nc\n", "a\nc\n",
                                               "a\nchanged\nc\n");
    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QCOMPARE(model.regions.first().baseLines, QStringList{"b"});
}

void tst_MergeModel::addAddConflict()
{
    const MergeSnapshot base{false, {}, "BASE"};
    const MergeSnapshot ours{true, "ours\n", "OURS"};
    const MergeSnapshot theirs{true, "theirs\n", "THEIRS"};
    const MergeModel model = computeMergeModel(base, ours, theirs);

    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QCOMPARE(model.regions.first().base, (MergeTextRange{1, 0}));
    QCOMPARE(model.regions.first().oursLines, QStringList{"ours"});
    QCOMPARE(model.regions.first().theirsLines, QStringList{"theirs"});
}

void tst_MergeModel::absentSideIsNotEmptyFile()
{
    const MergeSnapshot base{false, {}, "BASE"};
    const MergeSnapshot ours{true, "new\n", "OURS"};
    const MergeSnapshot theirs{true, "other\n", "THEIRS"};
    const MergeModel model = computeMergeModel(base, ours, theirs);

    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QCOMPARE(model.regions.first().base.lineCount, 0);
}

void tst_MergeModel::deleteVsEmptyFile()
{
    const MergeSnapshot base{true, "base\n", "BASE"};
    const MergeSnapshot ours{false, {}, "OURS"};
    const MergeSnapshot theirs{true, {}, "THEIRS"};
    const MergeModel model = computeMergeModel(base, ours, theirs);

    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::Conflict);
    QVERIFY(model.regions.first().oursLines.isEmpty());
    QVERIFY(model.regions.first().theirsLines.isEmpty());
}

void tst_MergeModel::emptyFileExistenceChange()
{
    const MergeSnapshot base{true, {}, "BASE"};
    const MergeSnapshot ours{false, {}, "OURS"};
    const MergeSnapshot theirs{true, {}, "THEIRS"};
    const MergeModel model = computeMergeModel(base, ours, theirs);

    QCOMPARE(model.regions.size(), 1);
    QCOMPARE(model.regions.first().kind, MergeRegionKind::OursOnly);
}

void tst_MergeModel::trailingNewlineDifference()
{
    const MergeModel model = computeMergeModel("a\nb\n", "a\nb", "a\nb\n");
    QVERIFY(model.computed);
    QVERIFY(model.regions.isEmpty());
    QVERIFY(model.baseEndsWithNewline);
    QVERIFY(!model.oursEndsWithNewline);
    QVERIFY(model.theirsEndsWithNewline);
}

void tst_MergeModel::repeatedLinesWithPatience()
{
    const QString base = "delta();\n{\nalpha();\nbeta();\ndelta();\ndelta();\nbeta();\n";
    const QString ours = "delta();\n{\nbeta();\ndelta();\n\ndelta();\nalpha();\nbeta();\n";
    const MergeModel model = computeMergeModel(base, ours, base, true);
    QVERIFY(!model.regions.isEmpty());
    for (const MergeRegion &region : model.regions)
        QCOMPARE(region.kind, MergeRegionKind::OursOnly);
}

QTEST_MAIN(tst_MergeModel)

#include "tst_mergemodel.moc"
