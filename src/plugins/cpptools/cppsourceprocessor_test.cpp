/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "cpptoolsplugin.h"

#include "cppmodelmanager.h"
#include "cppsourceprocessertesthelper.h"
#include "cppsourceprocessor.h"
#include "cppsnapshotupdater.h"
#include "cpptoolseditorsupport.h"
#include "cpptoolstestcase.h"

#include <texteditor/basetexteditor.h>

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

    Document::Ptr run(const QByteArray &source)
    {
        const QString fileName = TestIncludePaths::testFilePath();

        FileWriterAndRemover scopedFile(fileName, source);
        if (!scopedFile.writtenSuccessfully())
            return Document::Ptr();

        QScopedPointer<CppSourceProcessor> sourceProcessor(
                    CppModelManager::createSourceProcessor());
        const ProjectPart::HeaderPath hp(TestIncludePaths::directoryOfTestFile(),
                                         ProjectPart::HeaderPath::IncludePath);
        sourceProcessor->setHeaderPaths(ProjectPart::HeaderPaths() << hp);
        sourceProcessor->run(fileName);

        Document::Ptr document = m_cmm->document(fileName);
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
    QByteArray source =
        "#include \"header.h\"\n"
        "#include \"notresolvable.h\"\n"
        "\n"
        ;

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(source);
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
    const QStringList sourceFiles = QStringList() << fileName1 << fileName2;

    // Create global snapshot (needed in SnapshotUpdater)
    TestCase testCase;
    testCase.parseFiles(sourceFiles);

    // Open editor
    TextEditor::BaseTextEditor *editor;
    QVERIFY(testCase.openBaseTextEditor(fileName1, &editor));
    testCase.closeEditorAtEndOfTestCase(editor);

    // Get editor snapshot
    CppEditorSupport *cppEditorSupport = CppModelManagerInterface::instance()
        ->cppEditorSupport(editor);
    QVERIFY(cppEditorSupport);
    QSharedPointer<SnapshotUpdater> snapshotUpdater = cppEditorSupport->snapshotUpdater();
    QVERIFY(snapshotUpdater);
    Snapshot snapshot = snapshotUpdater->snapshot();
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
    QByteArray source =
        "#include <NotResolvable1>\n"
        "#include <NotResolvable2>\n"
        "#include \"/some/nonexisting/file123.h\"\n"
        "\n"
        ;

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(source);
    QVERIFY(document);

    QCOMPARE(document->resolvedIncludes().size(), 0);
    QCOMPARE(document->unresolvedIncludes().size(), 3);
    QCOMPARE(document->diagnosticMessages().size(), 3);
}

void CppToolsPlugin::test_cppsourceprocessor_macroUses()
{
    QByteArray source =
        "#define SOMEDEFINE 1\n"
        "#if SOMEDEFINE == 1\n"
        "    int someNumber;\n"
        "#endif\n"
        ;

    SourcePreprocessor processor;
    Document::Ptr document = processor.run(source);
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
