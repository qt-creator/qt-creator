/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptoolsplugin.h"

#include "baseeditordocumentprocessor.h"
#include "cppmodelmanager.h"
#include "cppsourceprocessertesthelper.h"
#include "cppsourceprocessor.h"
#include "cpptoolstestcase.h"
#include "editordocumenthandle.h"

#include <coreplugin/testdatadir.h>
#include <texteditor/texteditor.h>

#include <cplusplus/CppDocument.h>
#include <utils/fileutils.h>

#include <QFile>
#include <QFileInfo>
#include <QtTest>

using namespace CPlusPlus;
using namespace CppTools;
using namespace CppTools::Tests;
using namespace CppTools::Internal;

typedef Document::Include Include;

class SourcePreprocessor
{
public:
    SourcePreprocessor()
        : m_cmm(CppModelManager::instance())
    {
        cleanUp();
    }

    Document::Ptr run(const QString &filePath)
    {
        QScopedPointer<CppSourceProcessor> sourceProcessor(
                    CppModelManager::createSourceProcessor());
        const ProjectPart::HeaderPath hp(TestIncludePaths::directoryOfTestFile(),
                                         ProjectPart::HeaderPath::IncludePath);
        sourceProcessor->setHeaderPaths(ProjectPart::HeaderPaths() << hp);
        sourceProcessor->run(filePath);

        Document::Ptr document = m_cmm->document(filePath);
        return document;
    }

    ~SourcePreprocessor()
    {
        cleanUp();
    }

private:
    void cleanUp()
    {
        m_cmm->GC();
        QVERIFY(m_cmm->snapshot().isEmpty());
    }

private:
    CppModelManager *m_cmm;
};

/// Check: Resolved and unresolved includes are properly tracked.
void CppToolsPlugin::test_cppsourceprocessor_includes_resolvedUnresolved()
{
    const QString testFilePath
            = TestIncludePaths::testFilePath(QLatin1String("test_main_resolvedUnresolved.cpp"));

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(testFilePath);
    QVERIFY(document);

    const QList<Document::Include> resolvedIncludes = document->resolvedIncludes();
    QCOMPARE(resolvedIncludes.size(), 1);
    QCOMPARE(resolvedIncludes.at(0).type(), Client::IncludeLocal);
    QCOMPARE(resolvedIncludes.at(0).unresolvedFileName(), QLatin1String("header.h"));
    const QString expectedResolvedFileName
            = TestIncludePaths::testFilePath(QLatin1String("header.h"));
    QCOMPARE(resolvedIncludes.at(0).resolvedFileName(), expectedResolvedFileName);

    const QList<Document::Include> unresolvedIncludes = document->unresolvedIncludes();
    QCOMPARE(unresolvedIncludes.size(), 1);
    QCOMPARE(unresolvedIncludes.at(0).type(), Client::IncludeLocal);
    QCOMPARE(unresolvedIncludes.at(0).unresolvedFileName(), QLatin1String("notresolvable.h"));
    QVERIFY(unresolvedIncludes.at(0).resolvedFileName().isEmpty());
}

/// Check: Avoid self-include entries due to cyclic includes.
void CppToolsPlugin::test_cppsourceprocessor_includes_cyclic()
{
    const QString fileName1 = TestIncludePaths::testFilePath(QLatin1String("cyclic1.h"));
    const QString fileName2 = TestIncludePaths::testFilePath(QLatin1String("cyclic2.h"));
    const QSet<QString> sourceFiles = QSet<QString>() << fileName1 << fileName2;

    // Create global snapshot (needed in BuiltinEditorDocumentParser)
    TestCase testCase;
    testCase.parseFiles(sourceFiles);

    // Open editor
    TextEditor::BaseTextEditor *editor;
    QVERIFY(testCase.openBaseTextEditor(fileName1, &editor));
    testCase.closeEditorAtEndOfTestCase(editor);

    // Check editor snapshot
    const QString filePath = editor->document()->filePath().toString();
    auto *processor = BaseEditorDocumentProcessor::get(filePath);
    QVERIFY(processor);
    QVERIFY(TestCase::waitForProcessedEditorDocument(filePath));
    Snapshot snapshot = processor->snapshot();
    QCOMPARE(snapshot.size(), 3); // Configuration file included

    // Check includes
    Document::Ptr doc1 = snapshot.document(fileName1);
    QVERIFY(doc1);
    Document::Ptr doc2 = snapshot.document(fileName2);
    QVERIFY(doc2);

    QCOMPARE(doc1->unresolvedIncludes().size(), 0);
    QCOMPARE(doc1->resolvedIncludes().size(), 1);
    QCOMPARE(doc1->resolvedIncludes().first().resolvedFileName(), fileName2);

    QCOMPARE(doc2->unresolvedIncludes().size(), 0);
    QCOMPARE(doc2->resolvedIncludes().size(), 1);
    QCOMPARE(doc2->resolvedIncludes().first().resolvedFileName(), fileName1);
}

/// Check: All include errors are reported as diagnostic messages.
void CppToolsPlugin::test_cppsourceprocessor_includes_allDiagnostics()
{
    const QString testFilePath
            = TestIncludePaths::testFilePath(QLatin1String("test_main_allDiagnostics.cpp"));

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(testFilePath);
    QVERIFY(document);

    QCOMPARE(document->resolvedIncludes().size(), 0);
    QCOMPARE(document->unresolvedIncludes().size(), 3);
    QCOMPARE(document->diagnosticMessages().size(), 3);
}

void CppToolsPlugin::test_cppsourceprocessor_macroUses()
{
    const QString testFilePath
            = TestIncludePaths::testFilePath(QLatin1String("test_main_macroUses.cpp"));

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(testFilePath);
    QVERIFY(document);
    const QList<Document::MacroUse> macroUses = document->macroUses();
    QCOMPARE(macroUses.size(), 1);
    const Document::MacroUse macroUse = macroUses.at(0);
    QCOMPARE(macroUse.bytesBegin(), 25U);
    QCOMPARE(macroUse.bytesEnd(), 35U);
    QCOMPARE(macroUse.utf16charsBegin(), 25U);
    QCOMPARE(macroUse.utf16charsEnd(), 35U);
    QCOMPARE(macroUse.beginLine(), 2U);
}

static bool isMacroDefinedInDocument(const QByteArray &macroName, const Document::Ptr &document)
{
    foreach (const Macro &macro, document->definedMacros()) {
        if (macro.name() == macroName)
            return true;
    }

    return false;
}

static inline QString _(const QByteArray &ba) { return QString::fromLatin1(ba, ba.size()); }

void CppToolsPlugin::test_cppsourceprocessor_includeNext()
{
    const Core::Tests::TestDataDir data(
        _(SRCDIR "/../../../tests/auto/cplusplus/preprocessor/data/include_next-data/"));
    const QString mainFilePath = data.file(QLatin1String("main.cpp"));
    const QString customHeaderPath = data.directory(QLatin1String("customIncludePath"));
    const QString systemHeaderPath = data.directory(QLatin1String("systemIncludePath"));

    CppSourceProcessor::DocumentCallback documentCallback = [](const Document::Ptr &){};
    CppSourceProcessor sourceProcessor(Snapshot(), documentCallback);
    ProjectPart::HeaderPaths headerPaths = ProjectPart::HeaderPaths()
        << ProjectPart::HeaderPath(customHeaderPath, ProjectPart::HeaderPath::IncludePath)
        << ProjectPart::HeaderPath(systemHeaderPath, ProjectPart::HeaderPath::IncludePath);
    sourceProcessor.setHeaderPaths(headerPaths);

    sourceProcessor.run(mainFilePath);
    const Snapshot snapshot = sourceProcessor.snapshot();
    QVERIFY(!snapshot.isEmpty());
    const Document::Ptr mainDocument = snapshot.document(mainFilePath);
    QVERIFY(mainDocument);
    QVERIFY(isMacroDefinedInDocument("OK_FEATURE_X_ENABLED", mainDocument));
}
