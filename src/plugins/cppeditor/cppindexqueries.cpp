// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppindexqueries.h"

#include <cplusplus/CppDocument.h>

#include <utils/algorithm.h>

#include <QDir>

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

QSet<FilePath> filesAffectedByNewFiles(const Snapshot &snapshot, const QSet<FilePath> &newFiles)
{
    if (newFiles.isEmpty())
        return {};

    // Case-insensitive throughout: that is right where the file system is, and merely
    // conservative where it is not.
    QHash<QString, FilePaths> newFilesByName;
    QSet<qsizetype> nameLengths;
    for (const FilePath &file : newFiles) {
        const QString name = file.fileName();
        newFilesByName[name.toCaseFolded()].append(file);
        nameLengths.insert(name.size());
    }

    // The directories an include names restrict what it can find. A leading "../" can
    // lead anywhere, so only the part after it counts.
    const auto mayFindNewFile = [&](const Document::Include &include) {
        // Almost no include names a new file, so reject without allocating first.
        const QString &spelled = include.unresolvedFileName();
        if (!nameLengths.contains(spelled.size() - spelled.lastIndexOf('/') - 1))
            return false;
        QString spelling = QDir::cleanPath(spelled);
        const FilePaths candidates
            = newFilesByName.value(spelling.section('/', -1).toCaseFolded());
        if (candidates.isEmpty())
            return false;
        if (FilePath::fromString(spelling).isAbsolutePath()) {
            return Utils::anyOf(candidates, [&](const FilePath &file) {
                return file.path().compare(spelling, Qt::CaseInsensitive) == 0;
            });
        }
        while (spelling.startsWith("../"))
            spelling.remove(0, 3);
        const int slash = spelling.indexOf('/');
        const QString inFramework = slash == -1 ? QString()
            : '/' + spelling.left(slash) + ".framework/Headers" + spelling.mid(slash);
        return Utils::anyOf(candidates, [&](const FilePath &file) {
            return file.endsWith('/' + spelling, Qt::CaseInsensitive)
                   || (!inFramework.isEmpty() && file.endsWith(inFramework, Qt::CaseInsensitive));
        });
    };

    // An include that found nothing may find a new file now, and one that found another
    // file of the same name may find the new one instead.
    const auto foundAnotherFile = [&](const Document::Include &include) {
        return mayFindNewFile(include) && !newFiles.contains(include.resolvedFileName());
    };
    FilePaths pending;
    for (const Document::Ptr &doc : snapshot) {
        if (Utils::anyOf(doc->resolvedIncludes(), foundAnotherFile)
            || Utils::anyOf(doc->unresolvedIncludes(), mayFindNewFile)) {
            pending.append(doc->filePath());
        }
    }
    if (pending.isEmpty())
        return {};

    // Their includers see different declarations, although their own contents are unchanged.
    QHash<FilePath, FilePaths> includers;
    for (const Document::Ptr &doc : snapshot) {
        for (const Document::Include &include : doc->resolvedIncludes())
            includers[include.resolvedFileName()].append(doc->filePath());
    }
    QSet<FilePath> affected;
    while (!pending.isEmpty()) {
        const FilePath current = pending.takeLast();
        if (Utils::insert(affected, current))
            pending += includers.value(current);
    }
    return affected;
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
    void testFilesAffectedByNewFiles();
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

void IndexQueriesTest::testFilesAffectedByNewFiles()
{
    const FilePath newHeader = FilePath::fromString("/project/new/header.h");
    const FilePath oldHeader = FilePath::fromString("/project/old/header.h");
    const FilePath findsNothing = FilePath::fromString("/project/findsnothing.h");
    const FilePath includesFindsNothing = FilePath::fromString("/project/indirect.cpp");
    const FilePath findsOldHeader = FilePath::fromString("/project/findsold.cpp");
    const FilePath findsNewHeader = FilePath::fromString("/project/findsnew.cpp");
    const FilePath unrelated = FilePath::fromString("/project/unrelated.cpp");
    const FilePath otherDirHeader = FilePath::fromString("/project/old/other/header.h");
    const FilePath findsOtherDir = FilePath::fromString("/project/findsotherdir.cpp");
    const FilePath spellsDir = FilePath::fromString("/project/spellsdir.cpp");
    const FilePath spellsParent = FilePath::fromString("/project/sub/spellsparent.cpp");
    const FilePath spellsOtherCase = FilePath::fromString("/project/spellsothercase.cpp");
    const FilePath spellsPath = FilePath::fromString("/project/spellspath.cpp");
    const FilePath spellsOtherPath = FilePath::fromString("/project/spellsotherpath.cpp");
    const FilePath spellsFramework = FilePath::fromString("/project/spellsframework.cpp");

    const auto unresolved = [](const QString &spelling) {
        return Document::Include(spelling, {}, 1, Client::IncludeLocal);
    };

    Snapshot snapshot;
    addDocument(snapshot, oldHeader, {});
    addDocument(snapshot, findsNothing, {unresolved("header.h")});
    addDocument(snapshot, includesFindsNothing, {include(findsNothing, 1)});
    addDocument(snapshot, findsOldHeader, {include(oldHeader, 1)});
    addDocument(snapshot, findsNewHeader, {include(newHeader, 1)});
    addDocument(snapshot, unrelated, {unresolved("other.h")});
    addDocument(snapshot, otherDirHeader, {});
    addDocument(snapshot, findsOtherDir,
                {Document::Include("other/header.h", otherDirHeader, 1, Client::IncludeLocal)});
    addDocument(snapshot, spellsDir, {unresolved("new/header.h")});
    addDocument(snapshot, spellsParent, {unresolved("../header.h")});
    addDocument(snapshot, spellsOtherCase, {unresolved("Header.h")});
    addDocument(snapshot, spellsPath, {unresolved(newHeader.path())});
    addDocument(snapshot, spellsOtherPath, {unresolved("/project/elsewhere/header.h")});
    addDocument(snapshot, spellsFramework, {unresolved("Fw/fw.h")});

    const QSet<FilePath> expected{findsNothing, includesFindsNothing, findsOldHeader, spellsDir,
                                  spellsParent, spellsOtherCase, spellsPath};
    QCOMPARE(filesAffectedByNewFiles(snapshot, {newHeader}), expected);

    const FilePath frameworkHeader
        = FilePath::fromString("/frameworks/Fw.framework/Headers/fw.h");
    QCOMPARE(filesAffectedByNewFiles(snapshot, {frameworkHeader}),
             QSet<FilePath>{spellsFramework});

    QVERIFY(filesAffectedByNewFiles(snapshot, {FilePath::fromString("/project/none.h")})
                .isEmpty());
    QVERIFY(filesAffectedByNewFiles(snapshot, {}).isEmpty());
}

QObject *createIndexQueriesTest()
{
    return new IndexQueriesTest;
}

} // namespace CppEditor::Internal

#include "cppindexqueries.moc"

#endif // WITH_TESTS
