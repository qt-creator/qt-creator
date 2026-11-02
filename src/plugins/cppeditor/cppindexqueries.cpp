// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppindexqueries.h"

#include <cplusplus/CppDocument.h>

#include <utils/algorithm.h>

#ifdef WITH_TESTS
#include <QTest>
#endif

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {

Links includersOf(const Snapshot &snapshot, const FilePath &filePath)
{
    Links including;
    for (const Document::Ptr &doc : snapshot) {
        const QList<Document::Include> includes = doc->resolvedIncludes();
        for (const Document::Include &include : includes) {
            // TODO: Break after the first match? Repeated inclusions of the same file
            //       are rarely of interest.
            if (include.resolvedFileName() == filePath)
                including.append(Link(doc->filePath(), int(include.line())));
        }
    }
    return including;
}

FilePaths includedFilesOf(const Snapshot &snapshot, const FilePath &filePath)
{
    const Document::Ptr doc = snapshot.document(filePath);
    if (!doc)
        return {};

    // TODO: Remove duplicates? Repeated inclusions of the same file are rarely of interest.
    return doc->includedFiles(Document::Duplicates::Keep);
}

QSet<FilePath> includeClosureOf(const Snapshot &snapshot, const FilePath &filePath)
{
    return snapshot.allIncludesForDocument(filePath);
}

FilePaths filesDependingOn(const Snapshot &snapshot, const FilePath &filePath)
{
    return snapshot.filesDependingOn(filePath);
}

FilePaths includersOfFileNamed(const Snapshot &snapshot, const QString &fileName)
{
    const auto isNamed = [&fileName](const Document::Include &include) {
        return FilePath::fromString(include.unresolvedFileName()).fileName() == fileName;
    };
    FilePaths includers;
    for (const Document::Ptr &doc : snapshot) {
        if (Utils::anyOf(doc->resolvedIncludes(), isNamed)
            || Utils::anyOf(doc->unresolvedIncludes(), isNamed)) {
            includers.append(doc->filePath());
        }
    }
    return includers;
}

} // namespace CppEditor

#ifdef WITH_TESTS

namespace CppEditor::Internal {

static void addDocument(Snapshot &snapshot, const FilePath &filePath,
                        const QList<Document::Include> &includes)
{
    const Document::Ptr doc = Document::create(filePath);
    for (const Document::Include &include : includes)
        doc->addIncludeFile(include);
    snapshot.insert(doc);
}

static Document::Include include(const FilePath &filePath, int line)
{
    return Document::Include(filePath.fileName(), filePath, line, Client::IncludeLocal);
}

class IndexQueriesTest : public QObject
{
    Q_OBJECT

private slots:
    void testIncludersOf();
    void testIncludedFilesOf();
    void testIncludeClosureOf();
    void testFilesDependingOn();
    void testIncludersOfFileNamed();
};

void IndexQueriesTest::testIncludersOf()
{
    const FilePath header = FilePath::fromString("/project/header.h");
    const FilePath otherHeader = FilePath::fromString("/project/other.h");
    const FilePath includer = FilePath::fromString("/project/includer.cpp");
    const FilePath repeatedIncluder = FilePath::fromString("/project/repeated.cpp");
    const FilePath otherIncluder = FilePath::fromString("/project/other.cpp");
    const FilePath unresolvedIncluder = FilePath::fromString("/project/unresolved.cpp");

    Snapshot snapshot;
    addDocument(snapshot, header, {});
    addDocument(snapshot, otherHeader, {});
    addDocument(snapshot, includer, {include(otherHeader, 1), include(header, 2)});
    addDocument(snapshot, repeatedIncluder, {include(header, 3), include(header, 7)});
    addDocument(snapshot, otherIncluder, {include(otherHeader, 1)});
    addDocument(snapshot, unresolvedIncluder,
                {Document::Include("header.h", {}, 1, Client::IncludeLocal)});

    Links actual = includersOf(snapshot, header);
    Utils::sort(actual);
    const Links expected{Link(includer, 2), Link(repeatedIncluder, 3), Link(repeatedIncluder, 7)};
    QCOMPARE(actual, expected);

    QVERIFY(includersOf(snapshot, includer).isEmpty());
}

void IndexQueriesTest::testIncludedFilesOf()
{
    const FilePath header = FilePath::fromString("/project/header.h");
    const FilePath otherHeader = FilePath::fromString("/project/other.h");
    const FilePath source = FilePath::fromString("/project/source.cpp");
    const FilePath unknown = FilePath::fromString("/project/unknown.cpp");

    Snapshot snapshot;
    addDocument(snapshot, header, {});
    addDocument(snapshot, otherHeader, {});
    addDocument(snapshot, source,
                {include(otherHeader, 1),
                 include(header, 2),
                 Document::Include("missing.h", {}, 3, Client::IncludeLocal),
                 include(otherHeader, 4)});

    const FilePaths expected{otherHeader, header, otherHeader};
    QCOMPARE(includedFilesOf(snapshot, source), expected);
    QVERIFY(includedFilesOf(snapshot, header).isEmpty());
    QVERIFY(includedFilesOf(snapshot, unknown).isEmpty());
}

void IndexQueriesTest::testIncludeClosureOf()
{
    const FilePath source = FilePath::fromString("/project/source.cpp");
    const FilePath direct = FilePath::fromString("/project/direct.h");
    const FilePath shared = FilePath::fromString("/project/shared.h");
    const FilePath nested = FilePath::fromString("/project/nested.h");
    const FilePath unparsed = FilePath::fromString("/project/unparsed.h");
    const FilePath cycleStart = FilePath::fromString("/project/cyclestart.h");
    const FilePath cycleEnd = FilePath::fromString("/project/cycleend.h");
    const FilePath unrelated = FilePath::fromString("/project/unrelated.h");
    const FilePath unknown = FilePath::fromString("/project/unknown.cpp");

    Snapshot snapshot;
    addDocument(snapshot, source,
                {include(direct, 1),
                 include(shared, 2),
                 Document::Include("missing.h", {}, 3, Client::IncludeLocal)});
    addDocument(snapshot, direct, {include(shared, 1), include(nested, 2)});
    addDocument(snapshot, shared, {});
    addDocument(snapshot, nested, {include(unparsed, 1)});
    addDocument(snapshot, cycleStart, {include(cycleEnd, 1)});
    addDocument(snapshot, cycleEnd, {include(cycleStart, 1)});
    addDocument(snapshot, unrelated, {});

    const QSet<FilePath> expectedForSource{direct, shared, nested, unparsed};
    QCOMPARE(includeClosureOf(snapshot, source), expectedForSource);
    const QSet<FilePath> expectedForCycle{cycleStart, cycleEnd};
    QCOMPARE(includeClosureOf(snapshot, cycleStart), expectedForCycle);

    QVERIFY(includeClosureOf(snapshot, shared).isEmpty());
    QVERIFY(includeClosureOf(snapshot, unknown).isEmpty());
}

void IndexQueriesTest::testFilesDependingOn()
{
    const FilePath header = FilePath::fromString("/project/header.h");
    const FilePath directIncluder = FilePath::fromString("/project/direct.cpp");
    const FilePath middle = FilePath::fromString("/project/middle.h");
    const FilePath indirectIncluder = FilePath::fromString("/project/indirect.cpp");
    const FilePath otherHeader = FilePath::fromString("/project/other.h");
    const FilePath otherIncluder = FilePath::fromString("/project/other.cpp");
    const FilePath cycleStart = FilePath::fromString("/project/cyclestart.h");
    const FilePath cycleEnd = FilePath::fromString("/project/cycleend.h");
    const FilePath unknown = FilePath::fromString("/project/unknown.h");

    Snapshot snapshot;
    addDocument(snapshot, header, {});
    addDocument(snapshot, directIncluder, {include(header, 1)});
    addDocument(snapshot, middle, {include(header, 1)});
    addDocument(snapshot, indirectIncluder, {include(middle, 1)});
    addDocument(snapshot, otherHeader, {});
    addDocument(snapshot, otherIncluder, {include(otherHeader, 1)});
    addDocument(snapshot, cycleStart, {include(cycleEnd, 1)});
    addDocument(snapshot, cycleEnd, {include(cycleStart, 1)});

    FilePaths actual = filesDependingOn(snapshot, header);
    Utils::sort(actual);
    FilePaths expected{directIncluder, middle, indirectIncluder};
    Utils::sort(expected);
    QCOMPARE(actual, expected);

    actual = filesDependingOn(snapshot, cycleStart);
    Utils::sort(actual);
    expected = {cycleStart, cycleEnd};
    Utils::sort(expected);
    QCOMPARE(actual, expected);

    QVERIFY(filesDependingOn(snapshot, directIncluder).isEmpty());
    QVERIFY(filesDependingOn(snapshot, unknown).isEmpty());
}

void IndexQueriesTest::testIncludersOfFileNamed()
{
    const FilePath generated = FilePath::fromString("/build/forms/ui_form.h");
    const FilePath resolvedIncluder = FilePath::fromString("/project/resolved.cpp");
    const FilePath unresolvedIncluder = FilePath::fromString("/project/unresolved.cpp");
    const FilePath repeatedIncluder = FilePath::fromString("/project/repeated.cpp");
    const FilePath otherIncluder = FilePath::fromString("/project/other.cpp");
    const FilePath prefixIncluder = FilePath::fromString("/project/prefix.cpp");

    const auto named = [](const QString &name, int line) {
        return Document::Include(name, {}, line, Client::IncludeLocal);
    };

    Snapshot snapshot;
    addDocument(snapshot, generated, {});
    addDocument(snapshot, resolvedIncluder,
                {Document::Include("forms/ui_form.h", generated, 1, Client::IncludeLocal)});
    addDocument(snapshot, unresolvedIncluder, {named("ui_form.h", 1)});
    addDocument(snapshot, repeatedIncluder, {named("ui_form.h", 1), named("ui_form.h", 2)});
    addDocument(snapshot, otherIncluder, {named("ui_other.h", 1)});
    addDocument(snapshot, prefixIncluder, {named("my_ui_form.h", 1)});

    FilePaths actual = includersOfFileNamed(snapshot, "ui_form.h");
    Utils::sort(actual);
    FilePaths expected{resolvedIncluder, unresolvedIncluder, repeatedIncluder};
    Utils::sort(expected);
    QCOMPARE(actual, expected);

    QVERIFY(includersOfFileNamed(snapshot, "nothing_includes_this.h").isEmpty());
}

QObject *createIndexQueriesTest()
{
    return new IndexQueriesTest;
}

} // namespace CppEditor::Internal

#include "cppindexqueries.moc"

#endif // WITH_TESTS
