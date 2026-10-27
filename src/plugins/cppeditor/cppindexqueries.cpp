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

} // namespace CppEditor

#ifdef WITH_TESTS

namespace CppEditor::Internal {

class IndexQueriesTest : public QObject
{
    Q_OBJECT

private slots:
    void testIncludersOf();
};

void IndexQueriesTest::testIncludersOf()
{
    const FilePath header = FilePath::fromString("/project/header.h");
    const FilePath otherHeader = FilePath::fromString("/project/other.h");
    const FilePath includer = FilePath::fromString("/project/includer.cpp");
    const FilePath repeatedIncluder = FilePath::fromString("/project/repeated.cpp");
    const FilePath otherIncluder = FilePath::fromString("/project/other.cpp");
    const FilePath unresolvedIncluder = FilePath::fromString("/project/unresolved.cpp");

    const auto addDocument = [](Snapshot &snapshot, const FilePath &filePath,
                                const QList<Document::Include> &includes) {
        const Document::Ptr doc = Document::create(filePath);
        for (const Document::Include &include : includes)
            doc->addIncludeFile(include);
        snapshot.insert(doc);
    };
    const auto include = [](const FilePath &filePath, int line) {
        return Document::Include(filePath.fileName(), filePath, line, Client::IncludeLocal);
    };

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

QObject *createIndexQueriesTest()
{
    return new IndexQueriesTest;
}

} // namespace CppEditor::Internal

#include "cppindexqueries.moc"

#endif // WITH_TESTS
