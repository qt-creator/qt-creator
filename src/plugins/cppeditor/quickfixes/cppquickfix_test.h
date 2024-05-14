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

    void testInsertMemberFromUse_data();
    void testInsertMemberFromUse();

    void testConvertQt4ConnectConnectOutOfClass();
    void testConvertQt4ConnectConnectWithinClass_data();
    void testConvertQt4ConnectConnectWithinClass();
    void testConvertQt4ConnectDifferentNamespace();

    void testInsertDefFromDeclAfterClass();
    void testInsertDefFromDeclHeaderSourceBasic1();
    void testInsertDefFromDeclHeaderSourceBasic2();
    void testInsertDefFromDeclHeaderSourceBasic3();
    void testInsertDefFromDeclHeaderSourceNamespace1();
    void testInsertDefFromDeclHeaderSourceNamespace2();
    void testInsertDefFromDeclInsideClass();
    void testInsertDefFromDeclNotTriggeringWhenDefinitionExists();
    void testInsertDefFromDeclFindRightImplementationFile();
    void testInsertDefFromDeclIgnoreSurroundingGeneratedDeclarations();
    void testInsertDefFromDeclRespectWsInOperatorNames1();
    void testInsertDefFromDeclRespectWsInOperatorNames2();
    void testInsertDefFromDeclNoexceptSpecifier();
    void testInsertDefFromDeclMacroUsesAtEndOfFile1();
    void testInsertDefFromDeclMacroUsesAtEndOfFile2();
    void testInsertDefFromDeclErroneousStatementAtEndOfFile();
    void testInsertDefFromDeclRvalueReference();
    void testInsertDefFromDeclFunctionTryBlock();
    void testInsertDefFromDeclUsingDecl();
    void testInsertDefFromDeclFindImplementationFile();
    void testInsertDefFromDeclUnicodeIdentifier();
    void testInsertDefFromDeclTemplateClass();
    void testInsertDefFromDeclTemplateClassWithValueParam();
    void testInsertDefFromDeclTemplateFunction();
    void testInsertDefFromDeclTemplateClassAndTemplateFunction();
    void testInsertDefFromDeclTemplateClassAndFunctionInsideNamespace();
    void testInsertDefFromDeclFunctionWithSignedUnsignedArgument();
    void testInsertDefFromDeclNotTriggeredForFriendFunc();
    void testInsertDefFromDeclMinimalFunctionParameterType();
    void testInsertDefFromDeclAliasTemplateAsReturnType();
    void testInsertDefsFromDecls_data();
    void testInsertDefsFromDecls();
    void testInsertAndFormatDefsFromDecls();

    void testInsertDefOutsideFromDeclTemplateClassAndTemplateFunction();
    void testInsertDefOutsideFromDeclTemplateClass();
    void testInsertDefOutsideFromDeclTemplateFunction();
    void testInsertDefOutsideFromDeclFunction();

    void testInsertDeclFromDef();
    void testInsertDeclFromDefTemplateFuncTypename();
    void testInsertDeclFromDefTemplateFuncInt();
    void testInsertDeclFromDefTemplateReturnType();
    void testInsertDeclFromDefNotTriggeredForTemplateFunc();

    void testAddIncludeForUndefinedIdentifier_data();
    void testAddIncludeForUndefinedIdentifier();
    void testAddIncludeForUndefinedIdentifierNoDoubleQtHeaderInclude();

    void testAddForwardDeclForUndefinedIdentifier_data();
    void testAddForwardDeclForUndefinedIdentifier();

    void testMoveFuncDefOutsideMemberFuncToCpp();
    void testMoveFuncDefOutsideMemberFuncToCppInsideNS();
    void testMoveFuncDefOutsideMemberFuncOutside1();
    void testMoveFuncDefOutsideMemberFuncOutside2();
    void testMoveFuncDefOutsideMemberFuncToCppNS();
    void testMoveFuncDefOutsideMemberFuncToCppNSUsing();
    void testMoveFuncDefOutsideMemberFuncOutsideWithNs();
    void testMoveFuncDefOutsideFreeFuncToCpp();
    void testMoveFuncDefOutsideFreeFuncToCppNS();
    void testMoveFuncDefOutsideCtorWithInitialization1();
    void testMoveFuncDefOutsideCtorWithInitialization2();
    void testMoveFuncDefOutsideAfterClass();
    void testMoveFuncDefOutsideRespectWsInOperatorNames1();
    void testMoveFuncDefOutsideRespectWsInOperatorNames2();
    void testMoveFuncDefOutsideMacroUses();
    void testMoveFuncDefOutsideTemplate();
    void testMoveFuncDefOutsideMemberFunctionTemplate();
    void testMoveFuncDefOutsideTemplateSpecializedClass();
    void testMoveFuncDefOutsideUnnamedTemplate();
    void testMoveFuncDefOutsideMemberFuncToCppStatic();
    void testMoveFuncDefOutsideMemberFuncToCppWithInlinePartOfName();
    void testMoveFuncDefOutsideMixedQualifiers();

    void testMoveAllFuncDefOutsideMemberFuncToCpp();
    void testMoveAllFuncDefOutsideMemberFuncOutside();
    void testMoveAllFuncDefOutsideDoNotTriggerOnBaseClass();
    void testMoveAllFuncDefOutsideClassWithBaseClass();
    void testMoveAllFuncDefOutsideIgnoreMacroCode();

    void testMoveFuncDefToDecl_data();
    void testMoveFuncDefToDecl();

    void testMoveFuncDefToDeclMacroUses();

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

    void testRemoveUsingNamespace_data();
    void testRemoveUsingNamespace();
    void testRemoveUsingNamespaceSimple_data();
    void testRemoveUsingNamespaceSimple();
    void testRemoveUsingNamespaceDifferentSymbols();

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
