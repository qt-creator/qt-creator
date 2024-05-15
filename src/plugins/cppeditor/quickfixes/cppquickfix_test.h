// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../cpptoolstestcase.h"
#include "cppquickfix.h"
#include "cppquickfixsettings.h"

#include <projectexplorer/headerpath.h>

#include <QByteArray>
#include <QList>
#include <QObject>
#include <QSharedPointer>
#include <QStringList>

namespace TextEditor { class QuickFixOperation; }

namespace CppEditor {
class CppCodeStylePreferences;

namespace Internal {
namespace Tests {

class QuickFixSettings
{
    const CppQuickFixSettings original = *CppQuickFixSettings::instance();

public:
    CppQuickFixSettings *operator->() { return CppQuickFixSettings::instance(); }
    ~QuickFixSettings() { *CppQuickFixSettings::instance() = original; }
};

class BaseQuickFixTestCase : public CppEditor::Tests::TestCase
{
public:
    /// Exactly one QuickFixTestDocument must contain the cursor position marker '@'
    /// or "@{start}" and "@{end}"
    BaseQuickFixTestCase(const QList<TestDocumentPtr> &testDocuments,
                         const ProjectExplorer::HeaderPaths &headerPaths,
                         const QByteArray &clangFormatSettings = {});

    ~BaseQuickFixTestCase();

protected:
    TestDocumentPtr m_documentWithMarker;
    QList<TestDocumentPtr> m_testDocuments;

private:
    QScopedPointer<CppEditor::Tests::TemporaryDir> m_temporaryDirectory;

    CppCodeStylePreferences *m_cppCodeStylePreferences;
    QByteArray m_cppCodeStylePreferencesOriginalDelegateId;

    ProjectExplorer::HeaderPaths m_headerPathsToRestore;
    bool m_restoreHeaderPaths;
};

/// Tests the offered operations provided by a given CppQuickFixFactory
class QuickFixOfferedOperationsTest : public BaseQuickFixTestCase
{
public:
    QuickFixOfferedOperationsTest(const QList<TestDocumentPtr> &testDocuments,
                                  CppQuickFixFactory *factory,
                                  const ProjectExplorer::HeaderPaths &headerPaths
                                  = ProjectExplorer::HeaderPaths(),
                                  const QStringList &expectedOperations = QStringList());
};

/// Tests a concrete QuickFixOperation of a given CppQuickFixFactory
class QuickFixOperationTest : public BaseQuickFixTestCase
{
public:
    QuickFixOperationTest(const QList<TestDocumentPtr> &testDocuments,
                          CppQuickFixFactory *factory,
                          const ProjectExplorer::HeaderPaths &headerPaths
                            = ProjectExplorer::HeaderPaths(),
                          int operationIndex = 0,
                          const QByteArray &expectedFailMessage = {},
                          const QByteArray &clangFormatSettings = {});

    static void run(const QList<TestDocumentPtr> &testDocuments,
                    CppQuickFixFactory *factory,
                    const QString &headerPath,
                    int operationIndex = 0);
};

QList<TestDocumentPtr> singleDocument(const QByteArray &original,
                                                const QByteArray &expected);

class QuickfixTest : public QObject
{
    Q_OBJECT

private slots:
    void testGeneric_data();
    void testGeneric();

    void testAssignToLocalVariableTemplates();

    void testExtractFunction_data();
    void testExtractFunction();

    void testExtractLiteralAsParameterTypeDeduction_data();
    void testExtractLiteralAsParameterTypeDeduction();
    void testExtractLiteralAsParameterFreeFunctionSeparateFiles();
    void testExtractLiteralAsParameterMemberFunctionSeparateFiles();
    void testExtractLiteralAsParameterNotTriggeringForInvalidCode();

    void testAddCurlyBraces_data();
    void testAddCurlyBraces();

    void testChangeCommentType_data();
    void testChangeCommentType();

    void testMoveComments_data();
    void testMoveComments();

    void testConvertToMetaMethodInvocation_data();
    void testConvertToMetaMethodInvocation();
};

} // namespace Tests
} // namespace Internal
} // namespace CppEditor
