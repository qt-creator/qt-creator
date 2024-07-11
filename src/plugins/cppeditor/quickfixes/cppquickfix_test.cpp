// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppquickfix_test.h"

#include "../cppcodestylepreferences.h"
#include "../cppeditorwidget.h"
#include "../cppmodelmanager.h"
#include "../cppsourceprocessertesthelper.h"
#include "../cpptoolssettings.h"
#include "cppquickfixassistant.h"

#include <projectexplorer/kitmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <texteditor/textdocument.h>
#include <utils/fileutils.h>

#include <QDebug>
#include <QDir>
#include <QtTest>

/*!
    Tests for quick-fixes.
 */
using namespace Core;
using namespace CPlusPlus;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

using CppEditor::Tests::TemporaryDir;
using CppEditor::Tests::Internal::TestIncludePaths;

namespace CppEditor {
namespace Internal {
namespace Tests {

QList<TestDocumentPtr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected)
{
    return {CppTestDocument::create("file.cpp", original, expected)};
}

BaseQuickFixTestCase::BaseQuickFixTestCase(const QList<TestDocumentPtr> &testDocuments,
                                           const ProjectExplorer::HeaderPaths &headerPaths,
                                           const QByteArray &clangFormatSettings)
    : m_testDocuments(testDocuments)
    , m_cppCodeStylePreferences(0)
    , m_restoreHeaderPaths(false)
{
    QVERIFY(succeededSoFar());
    m_succeededSoFar = false;

    // Check if there is exactly one cursor marker
    unsigned cursorMarkersCount = 0;
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (document->hasCursorMarker())
            ++cursorMarkersCount;
    }
    QVERIFY2(cursorMarkersCount == 1, "Exactly one cursor marker is allowed.");

    // Write documents to disk
    m_temporaryDirectory.reset(new TemporaryDir);
    QVERIFY(m_temporaryDirectory->isValid());
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (QFileInfo(document->m_fileName).isRelative())
            document->setBaseDirectory(m_temporaryDirectory->path());
        document->writeToDisk();
    }

    // Create .clang-format file
    if (!clangFormatSettings.isEmpty())
        m_temporaryDirectory->createFile(".clang-format", clangFormatSettings);

    // Set appropriate include paths
    if (!headerPaths.isEmpty()) {
        m_restoreHeaderPaths = true;
        m_headerPathsToRestore = CppModelManager::headerPaths();
        CppModelManager::setHeaderPaths(headerPaths);
    }

    // Update Code Model
    QSet<FilePath> filePaths;
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments))
        filePaths << document->filePath();
    QVERIFY(parseFiles(filePaths));

    // Open Files
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        QVERIFY(openCppEditor(document->filePath(), &document->m_editor,
                              &document->m_editorWidget));
        closeEditorAtEndOfTestCase(document->m_editor);

        // Set cursor position
        if (document->hasCursorMarker()) {
            if (document->hasAnchorMarker()) {
                document->m_editor->setCursorPosition(document->m_anchorPosition);
                document->m_editor->select(document->m_cursorPosition);
            } else {
                document->m_editor->setCursorPosition(document->m_cursorPosition);
            }
        } else {
            document->m_editor->setCursorPosition(0);
        }

        // Rehighlight
        QVERIFY(waitForRehighlightedSemanticDocument(document->m_editorWidget));
    }

    // Enforce the default cpp code style, so we are independent of config file settings.
    // This is needed by e.g. the GenerateGetterSetter quick fix.
    m_cppCodeStylePreferences = CppToolsSettings::cppCodeStyle();
    QVERIFY(m_cppCodeStylePreferences);
    m_cppCodeStylePreferencesOriginalDelegateId = m_cppCodeStylePreferences->currentDelegateId();
    m_cppCodeStylePreferences->setCurrentDelegate("qt");

    // Find the document having the cursor marker
    for (const TestDocumentPtr &document : std::as_const(m_testDocuments)) {
        if (document->hasCursorMarker()){
            m_documentWithMarker = document;
            break;
        }
    }

    QVERIFY(m_documentWithMarker);
    m_succeededSoFar = true;
}

BaseQuickFixTestCase::~BaseQuickFixTestCase()
{
    // Restore default cpp code style
    if (m_cppCodeStylePreferences)
        m_cppCodeStylePreferences->setCurrentDelegate(m_cppCodeStylePreferencesOriginalDelegateId);

    // Restore include paths
    if (m_restoreHeaderPaths)
        CppModelManager::setHeaderPaths(m_headerPathsToRestore);

    // Remove created files from file system
    for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments))
        QVERIFY(testDocument->filePath().removeFile());
}

QuickFixOfferedOperationsTest::QuickFixOfferedOperationsTest(
        const QList<TestDocumentPtr> &testDocuments,
        CppQuickFixFactory *factory,
        const ProjectExplorer::HeaderPaths &headerPaths,
        const QStringList &expectedOperations)
    : BaseQuickFixTestCase(testDocuments, headerPaths)
{
    // Get operations
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations actualOperations;
    factory->match(quickFixInterface, actualOperations);

    // Convert to QStringList
    QStringList actualOperationsAsStringList;
    for (const QuickFixOperation::Ptr &operation : std::as_const(actualOperations))
        actualOperationsAsStringList << operation->description();

    QCOMPARE(actualOperationsAsStringList, expectedOperations);
}

/// Leading whitespace is not removed, so we can check if the indetation ranges
/// have been set correctly by the quick-fix.
static QString &removeTrailingWhitespace(QString &input)
{
    const QStringList lines = input.split(QLatin1Char('\n'));
    input.resize(0);
    for (int i = 0, total = lines.size(); i < total; ++i) {
        QString line = lines.at(i);
        while (line.length() > 0) {
            QChar lastChar = line[line.length() - 1];
            if (lastChar == QLatin1Char(' ') || lastChar == QLatin1Char('\t'))
                line.chop(1);
            else
                break;
        }
        input.append(line);

        const bool isLastLine = i == lines.size() - 1;
        if (!isLastLine)
            input.append(QLatin1Char('\n'));
    }
    return input;
}

QuickFixOperationTest::QuickFixOperationTest(const QList<TestDocumentPtr> &testDocuments,
                                             CppQuickFixFactory *factory,
                                             const ProjectExplorer::HeaderPaths &headerPaths,
                                             int operationIndex,
                                             const QByteArray &expectedFailMessage,
                                             const QByteArray &clangFormatSettings)
    : BaseQuickFixTestCase(testDocuments, headerPaths, clangFormatSettings)
{
    if (factory->clangdReplacement() && CppModelManager::isClangCodeModelActive())
        return;

    QVERIFY(succeededSoFar());

    // Perform operation if there is one
    CppQuickFixInterface quickFixInterface(m_documentWithMarker->m_editorWidget, ExplicitlyInvoked);
    QuickFixOperations operations;
    factory->match(quickFixInterface, operations);
    if (operations.isEmpty()) {
        QEXPECT_FAIL("QTCREATORBUG-25998", "FIXME", Abort);
        QVERIFY(testDocuments.first()->m_expectedSource.isEmpty());
        return;
    }

    QVERIFY(operationIndex < operations.size());
    const QuickFixOperation::Ptr operation = operations.at(operationIndex);
    operation->perform();

    // Compare all files
    for (const TestDocumentPtr &testDocument : std::as_const(m_testDocuments)) {
        // Check
        QString result = testDocument->m_editorWidget->document()->toPlainText();
        removeTrailingWhitespace(result);
        QEXPECT_FAIL("escape string literal: raw string literal", "FIXME", Continue);
        QEXPECT_FAIL("escape string literal: unescape adjacent literals", "FIXME", Continue);
        if (!expectedFailMessage.isEmpty())
            QEXPECT_FAIL("", expectedFailMessage.data(), Continue);
        else if (result != testDocument->m_expectedSource) {
            qDebug() << "---" << testDocument->m_expectedSource;
            qDebug() << "+++" << result;
        }
        QCOMPARE(result, testDocument->m_expectedSource);

        // Undo the change
        for (int i = 0; i < 100; ++i)
            testDocument->m_editorWidget->undo();
        result = testDocument->m_editorWidget->document()->toPlainText();
        QCOMPARE(result, testDocument->m_source);
    }
}

void QuickFixOperationTest::run(const QList<TestDocumentPtr> &testDocuments,
                                CppQuickFixFactory *factory,
                                const QString &headerPath,
                                int operationIndex)
{
    ProjectExplorer::HeaderPaths headerPaths;
    headerPaths.push_back(ProjectExplorer::HeaderPath::makeUser(headerPath));
    QuickFixOperationTest(testDocuments, factory, headerPaths, operationIndex);
}

} // namespace Tests
} // namespace Internal
} // namespace CppEditor

