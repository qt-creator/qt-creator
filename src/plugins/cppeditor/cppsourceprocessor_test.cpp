// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppsourceprocessor_test.h"

#include "baseeditordocumentprocessor.h"
#include "cppmodelmanager.h"
#include "cppsourceprocessertesthelper.h"
#include "cppsourceprocessor.h"
#include "cpptoolstestcase.h"
#include "editordocumenthandle.h"

#include <texteditor/texteditor.h>

#include <cplusplus/CppDocument.h>

#include <QTest>

using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace Utils;

using Include = Document::Include;
using CppEditor::Tests::TestCase;
using namespace CppEditor::Tests::Internal;

namespace CppEditor::Internal {

class SourcePreprocessor
{
public:
    SourcePreprocessor()
    {
        cleanUp();
    }

    Document::Ptr run(const FilePath &filePath) const
    {
        QScopedPointer<CppSourceProcessor> sourceProcessor(
                    CppModelManager::createSourceProcessor());
        sourceProcessor->setHeaderPaths({ProjectExplorer::HeaderPath::makeUser(
                                         TestIncludePaths::directoryOfTestFile())});
        sourceProcessor->run(filePath);

        Document::Ptr document = CppModelManager::document(filePath);
        return document;
    }

    ~SourcePreprocessor()
    {
        cleanUp();
    }

private:
    void cleanUp()
    {
        CppModelManager::GC();
        QVERIFY(CppModelManager::snapshot().isEmpty());
    }
};

/// Check: Resolved and unresolved includes are properly tracked.
void SourceProcessorTest::testIncludesResolvedUnresolved()
{
    const FilePath testFilePath
            = TestIncludePaths::testFilePath(QLatin1String("test_main_resolvedUnresolved.cpp"));

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(testFilePath);
    QVERIFY(document);

    const QList<Document::Include> resolvedIncludes = document->resolvedIncludes();
    QCOMPARE(resolvedIncludes.size(), 1);
    QCOMPARE(resolvedIncludes.at(0).type(), Client::IncludeLocal);
    QCOMPARE(resolvedIncludes.at(0).unresolvedFileName(), QLatin1String("header.h"));
    const FilePath expectedResolvedFileName
            = TestIncludePaths::testFilePath(QLatin1String("header.h"));
    QCOMPARE(resolvedIncludes.at(0).resolvedFileName(), expectedResolvedFileName);

    const QList<Document::Include> unresolvedIncludes = document->unresolvedIncludes();
    QCOMPARE(unresolvedIncludes.size(), 1);
    QCOMPARE(unresolvedIncludes.at(0).type(), Client::IncludeLocal);
    QCOMPARE(unresolvedIncludes.at(0).unresolvedFileName(), QLatin1String("notresolvable.h"));
    QVERIFY(unresolvedIncludes.at(0).resolvedFileName().isEmpty());
}

/// Check: Avoid self-include entries due to cyclic includes.
void SourceProcessorTest::testIncludesCyclic()
{
    const FilePath filePath1 = TestIncludePaths::testFilePath(QLatin1String("cyclic1.h"));
    const FilePath filePath2 = TestIncludePaths::testFilePath(QLatin1String("cyclic2.h"));
    const QSet<FilePath> sourceFiles = {filePath1, filePath2};

    // Create global snapshot (needed in BuiltinEditorDocumentParser)
    TestCase testCase;
    testCase.parseFiles(sourceFiles);

    // Open editor
    TextEditor::BaseTextEditor *editor;
    QVERIFY(testCase.openCppEditor(filePath1, &editor));
    testCase.closeEditorAtEndOfTestCase(editor);

    // Check editor snapshot
    const FilePath filePath = editor->document()->filePath();
    auto processor = CppModelManager::cppEditorDocumentProcessor(filePath);
    QVERIFY(processor);
    QVERIFY(TestCase::waitForProcessedEditorDocument(filePath));
    Snapshot snapshot = processor->snapshot();
    QCOMPARE(snapshot.size(), 3); // Configuration file included

    // Check includes
    Document::Ptr doc1 = snapshot.document(filePath1);
    QVERIFY(doc1);
    Document::Ptr doc2 = snapshot.document(filePath2);
    QVERIFY(doc2);

    QCOMPARE(doc1->unresolvedIncludes().size(), 0);
    QCOMPARE(doc1->resolvedIncludes().size(), 1);
    QCOMPARE(doc1->resolvedIncludes().first().resolvedFileName(), filePath2);

    QCOMPARE(doc2->unresolvedIncludes().size(), 0);
    QCOMPARE(doc2->resolvedIncludes().size(), 1);
    QCOMPARE(doc2->resolvedIncludes().first().resolvedFileName(), filePath1);
}

/// Check: All include errors are reported as diagnostic messages.
void SourceProcessorTest::testIncludesAllDiagnostics()
{
    const FilePath testFilePath
            = TestIncludePaths::testFilePath(QLatin1String("test_main_allDiagnostics.cpp"));

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(testFilePath);
    QVERIFY(document);

    QCOMPARE(document->resolvedIncludes().size(), 0);
    QCOMPARE(document->unresolvedIncludes().size(), 3);
    QCOMPARE(document->diagnosticMessages().size(), 3);
}

void SourceProcessorTest::testMacroUses()
{
    const FilePath testFilePath
            = TestIncludePaths::testFilePath(QLatin1String("test_main_macroUses.cpp"));

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(testFilePath);
    QVERIFY(document);
    const QList<Document::MacroUse> macroUses = document->macroUses();
    QCOMPARE(macroUses.size(), 1);
    const Document::MacroUse macroUse = macroUses.at(0);
    QCOMPARE(macroUse.bytesBegin(), 25);
    QCOMPARE(macroUse.bytesEnd(), 35);
    QCOMPARE(macroUse.utf16charsBegin(), 25);
    QCOMPARE(macroUse.utf16charsEnd(), 35);
    QCOMPARE(macroUse.beginLine(), 2);
}

static bool isMacroDefinedInDocument(const QByteArray &macroName, const Document::Ptr &document)
{
    for (const CPlusPlus::Macro &macro : document->definedMacros()) {
        if (macro.name() == macroName)
            return true;
    }

    return false;
}

void SourceProcessorTest::testIncludeNext()
{
    FilePath data = SRCDIR "/../../../tests/auto/cplusplus/preprocessor/data/include_next-data";
    data = data.canonicalPath();
    QVERIFY(data.isReadableDir());
    const FilePath mainFilePath = data / "main.cpp";
    const HeaderPath customHeaderPath(data / "customIncludePath", HeaderPathType::User);
    const HeaderPath systemHeaderPath(data / "systemIncludePath", HeaderPathType::User);

    CppSourceProcessor::DocumentCallback documentCallback = [](const Document::Ptr &){};
    CppSourceProcessor sourceProcessor(Snapshot(), documentCallback);
    sourceProcessor.setHeaderPaths({customHeaderPath, systemHeaderPath});

    sourceProcessor.run(mainFilePath);
    const Snapshot snapshot = sourceProcessor.snapshot();
    QVERIFY(!snapshot.isEmpty());
    const Document::Ptr mainDocument = snapshot.document(mainFilePath);
    QVERIFY(mainDocument);
    QVERIFY(isMacroDefinedInDocument("OK_FEATURE_X_ENABLED", mainDocument));
}

// An earlier, unrelated header path must not shadow the real next header once
// #include_next's own forward search has to step past a self-resolving entry.
// The three directories/names here - shadowIncludePath (an
// unrelated, earlier path that happens to also have a plain "t.h"), customIncludePath
// (found via a "sub/" prefix, as Qt's own forwarding headers are), and
// customIncludePath/sub (the forwarding header's own directory, listed again later to
// make #include_next resolve back to itself) - mirror the directory-prefix split real
// Qt 6.12 forwarding headers have, which is what keeps the earlier, unrelated path from
// also shadowing the very first #include: the first #include looks up "sub/t.h", found
// only via customIncludePath; the #include_next "t.h" it triggers looks up the bare
// "t.h" that both shadowIncludePath and the self-resolving entry provide. This whole
// shape - and the fact that it still has to reach systemIncludePath/t.h rather than
// shadowIncludePath/t.h - was verified against both gcc and clang before writing this
// test; see [[project_qtcreatorbug_35090]] for the exact construction.
void SourceProcessorTest::testIncludeNextWithShadowingPath()
{
    FilePath data
        = SRCDIR "/../../../tests/auto/cplusplus/preprocessor/data/include_next_shadow-data";
    data = data.canonicalPath();
    QVERIFY(data.isReadableDir());
    const FilePath mainFilePath = data / "main.cpp";
    const HeaderPath shadowHeaderPath(data / "shadowIncludePath", HeaderPathType::User);
    const HeaderPath customHeaderPath(data / "customIncludePath", HeaderPathType::User);
    const HeaderPath customSubHeaderPath(data / "customIncludePath/sub", HeaderPathType::User);
    const HeaderPath systemHeaderPath(data / "systemIncludePath", HeaderPathType::User);

    CppSourceProcessor::DocumentCallback documentCallback = [](const Document::Ptr &){};
    CppSourceProcessor sourceProcessor(Snapshot(), documentCallback);
    // customSubHeaderPath is listed twice: the second occurrence is what resolves
    // #include_next <t.h> (from inside customIncludePath/sub/t.h) back to the very file
    // that is currently being processed.
    sourceProcessor.setHeaderPaths(
        {shadowHeaderPath, customHeaderPath, customSubHeaderPath, customSubHeaderPath,
         systemHeaderPath});

    sourceProcessor.run(mainFilePath);
    const Snapshot snapshot = sourceProcessor.snapshot();
    QVERIFY(!snapshot.isEmpty());

    // The direct, precise check: customIncludePath/sub/t.h's own #include_next <t.h>
    // must resolve to systemIncludePath/t.h, not to shadowIncludePath/t.h (a macro
    // defined by an included file is attributed to that file's own document, not merged
    // into every document that (transitively) includes it, so checking mainDocument's
    // defined macros cannot tell these two outcomes apart).
    const Document::Ptr customDocument = snapshot.document(data / "customIncludePath/sub/t.h");
    QVERIFY(customDocument);
    const QList<Document::Include> resolvedIncludes = customDocument->resolvedIncludes();
    QCOMPARE(resolvedIncludes.size(), 1);
    QCOMPARE(resolvedIncludes.at(0).resolvedFileName(), data / "systemIncludePath/t.h");

    // End-to-end check: with the correct header reached, its macro should be visible by
    // the time main.cpp's own #ifdef is evaluated.
    const Document::Ptr mainDocument = snapshot.document(mainFilePath);
    QVERIFY(mainDocument);
    QVERIFY(isMacroDefinedInDocument("OK_FEATURE_X_ENABLED", mainDocument));
}

} // namespace CppEditor::Internal
