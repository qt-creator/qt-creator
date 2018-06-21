/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include <extensionsystem/iplugin.h>

namespace CppEditor {
namespace Internal {

class CppEditorPluginPrivate;
class CppQuickFixAssistProvider;

class CppEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppEditor.json")

public:
    CppEditorPlugin();
    ~CppEditorPlugin();

    static CppEditorPlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage) override;
    void extensionsInitialized() override;

    CppQuickFixAssistProvider *quickFixProvider() const;

signals:
    void outlineSortingChanged(bool sort);
    void typeHierarchyRequested();
    void includeHierarchyRequested();

public:
    void openDeclarationDefinitionInNextSplit();
    void openTypeHierarchy();
    void openIncludeHierarchy();
    void findUsages();
    void showPreProcessorDialog();
    void renameSymbolUnderCursor();
    void switchDeclarationDefinition();

#ifdef WITH_TESTS
private:
    QList<QObject *> createTestObjects() const override;

private slots:
    // The following tests expect that no projects are loaded on start-up.
    void test_SwitchMethodDeclarationDefinition_data();
    void test_SwitchMethodDeclarationDefinition();

    void test_FollowSymbolUnderCursor_multipleDocuments_data();
    void test_FollowSymbolUnderCursor_multipleDocuments();

    void test_FollowSymbolUnderCursor_data();
    void test_FollowSymbolUnderCursor();

    void test_FollowSymbolUnderCursor_QTCREATORBUG7903_data();
    void test_FollowSymbolUnderCursor_QTCREATORBUG7903();

    void test_FollowSymbolUnderCursor_followCall_data();
    void test_FollowSymbolUnderCursor_followCall();

    void test_FollowSymbolUnderCursor_QObject_connect_data();
    void test_FollowSymbolUnderCursor_QObject_connect();

    void test_FollowSymbolUnderCursor_classOperator_onOperatorToken_data();
    void test_FollowSymbolUnderCursor_classOperator_onOperatorToken();

    void test_FollowSymbolUnderCursor_classOperator_data();
    void test_FollowSymbolUnderCursor_classOperator();

    void test_FollowSymbolUnderCursor_classOperator_inOp_data();
    void test_FollowSymbolUnderCursor_classOperator_inOp();

    void test_FollowSymbolUnderCursor_virtualFunctionCall_data();
    void test_FollowSymbolUnderCursor_virtualFunctionCall();
    void test_FollowSymbolUnderCursor_virtualFunctionCall_multipleDocuments();

    void test_quickfix_data();
    void test_quickfix();

    void test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespaceToCpp();
    void test_quickfix_GenerateGetterSetter_onlyGetter();
    void test_quickfix_GenerateGetterSetter_onlyGetter_DontPreferGetterWithGet();
    void test_quickfix_GenerateGetterSetter_onlySetter();
    void test_quickfix_GenerateGetterSetter_offerGetterWhenSetterPresent();
    void test_quickfix_GenerateGetterSetter_offerSetterWhenGetterPresent();

    void test_quickfix_ConvertQt4Connect_connectOutOfClass();
    void test_quickfix_ConvertQt4Connect_connectWithinClass_data();
    void test_quickfix_ConvertQt4Connect_connectWithinClass();
    void test_quickfix_ConvertQt4Connect_differentNamespace();

    void test_quickfix_InsertDefFromDecl_afterClass();
    void test_quickfix_InsertDefFromDecl_headerSource_basic1();
    void test_quickfix_InsertDefFromDecl_headerSource_basic2();
    void test_quickfix_InsertDefFromDecl_headerSource_basic3();
    void test_quickfix_InsertDefFromDecl_headerSource_namespace1();
    void test_quickfix_InsertDefFromDecl_headerSource_namespace2();
    void test_quickfix_InsertDefFromDecl_insideClass();
    void test_quickfix_InsertDefFromDecl_notTriggeringWhenDefinitionExists();
    void test_quickfix_InsertDefFromDecl_findRightImplementationFile();
    void test_quickfix_InsertDefFromDecl_ignoreSurroundingGeneratedDeclarations();
    void test_quickfix_InsertDefFromDecl_respectWsInOperatorNames1();
    void test_quickfix_InsertDefFromDecl_respectWsInOperatorNames2();
    void test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile1();
    void test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile2();
    void test_quickfix_InsertDefFromDecl_erroneousStatementAtEndOfFile();
    void test_quickfix_InsertDefFromDecl_rvalueReference();
    void test_quickfix_InsertDefFromDecl_findImplementationFile();
    void test_quickfix_InsertDefFromDecl_unicodeIdentifier();
    void test_quickfix_InsertDefFromDecl_templateClass();
    void test_quickfix_InsertDefFromDecl_templateFunction();

    void test_quickfix_InsertDeclFromDef();
    void test_quickfix_InsertDeclFromDef_templateFuncTypename();
    void test_quickfix_InsertDeclFromDef_templateFuncInt();
    void test_quickfix_InsertDeclFromDef_notTriggeredForTemplateFunc();

    void test_quickfix_AddIncludeForUndefinedIdentifier_data();
    void test_quickfix_AddIncludeForUndefinedIdentifier();
    void test_quickfix_AddIncludeForUndefinedIdentifier_noDoubleQtHeaderInclude();

    void test_quickfix_MoveFuncDefOutside_MemberFuncToCpp();
    void test_quickfix_MoveFuncDefOutside_MemberFuncToCppInsideNS();
    void test_quickfix_MoveFuncDefOutside_MemberFuncOutside1();
    void test_quickfix_MoveFuncDefOutside_MemberFuncOutside2();
    void test_quickfix_MoveFuncDefOutside_MemberFuncToCppNS();
    void test_quickfix_MoveFuncDefOutside_MemberFuncToCppNSUsing();
    void test_quickfix_MoveFuncDefOutside_MemberFuncOutsideWithNs();
    void test_quickfix_MoveFuncDefOutside_FreeFuncToCpp();
    void test_quickfix_MoveFuncDefOutside_FreeFuncToCppNS();
    void test_quickfix_MoveFuncDefOutside_CtorWithInitialization1();
    void test_quickfix_MoveFuncDefOutside_CtorWithInitialization2();
    void test_quickfix_MoveFuncDefOutside_afterClass();
    void test_quickfix_MoveFuncDefOutside_respectWsInOperatorNames1();
    void test_quickfix_MoveFuncDefOutside_respectWsInOperatorNames2();
    void test_quickfix_MoveFuncDefOutside_macroUses();
    void test_quickfix_MoveFuncDefOutside_template();

    void test_quickfix_MoveAllFuncDefOutside_MemberFuncToCpp();
    void test_quickfix_MoveAllFuncDefOutside_MemberFuncOutside();
    void test_quickfix_MoveAllFuncDefOutside_DoNotTriggerOnBaseClass();
    void test_quickfix_MoveAllFuncDefOutside_classWithBaseClass();
    void test_quickfix_MoveAllFuncDefOutside_ignoreMacroCode();

    void test_quickfix_MoveFuncDefToDecl_MemberFunc();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncOutside();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNS();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNSUsing();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncOutsideWithNs();
    void test_quickfix_MoveFuncDefToDecl_FreeFuncToCpp();
    void test_quickfix_MoveFuncDefToDecl_FreeFuncToCppNS();
    void test_quickfix_MoveFuncDefToDecl_CtorWithInitialization();
    void test_quickfix_MoveFuncDefToDecl_structWithAssignedVariable();
    void test_quickfix_MoveFuncDefToDecl_macroUses();
    void test_quickfix_MoveFuncDefToDecl_override();
    void test_quickfix_MoveFuncDefToDecl_template();
    void test_quickfix_MoveFuncDefToDecl_templateFunction();

    void test_quickfix_AssignToLocalVariable_templates();

    void test_quickfix_ExtractFunction_data();
    void test_quickfix_ExtractFunction();

    void test_quickfix_ExtractLiteralAsParameter_typeDeduction_data();
    void test_quickfix_ExtractLiteralAsParameter_typeDeduction();
    void test_quickfix_ExtractLiteralAsParameter_freeFunction_separateFiles();
    void test_quickfix_ExtractLiteralAsParameter_memberFunction_separateFiles();
    void test_quickfix_ExtractLiteralAsParameter_notTriggeringForInvalidCode();

    void test_quickfix_InsertVirtualMethods_data();
    void test_quickfix_InsertVirtualMethods();
    void test_quickfix_InsertVirtualMethods_implementationFile();
    void test_quickfix_InsertVirtualMethods_BaseClassInNamespace();

    void test_useSelections_data();
    void test_useSelections();

    // tests for "Include Hierarchy"
    void test_includehierarchy_data();
    void test_includehierarchy();

    // The following tests operate on a project and require special invocation:
    //
    // Ensure that the project is properly configured for a given settings path:
    //   $ ./qtcreator -settingspath /your/settings/path /path/to/project
    //
    // ...and that it builds, which might prevent blocking dialogs for not
    // existing files (e.g. ui_*.h).
    //
    // Run a test:
    //   $ export QTC_TEST_WAIT_FOR_LOADED_PROJECT=1
    //   $ ./qtcreator -settingspath /your/settings/path -test CppEditor,test_openEachFile /path/to/project
    void test_openEachFile();
    void test_switchHeaderSourceOnEachFile();
    void test_moveTokenWiseThroughEveryFile();
    void test_moveTokenWiseThroughEveryFileAndFollowSymbol();
    void test_moveTokenWiseThroughEveryFileAndSwitchDeclarationDefinition();
    void test_moveTokenWiseThroughEveryFileAndFindUsages();
    void test_moveTokenWiseThroughEveryFileAndRenameUsages();
    void test_moveTokenWiseThroughEveryFileAndOpenTypeHierarchy();
    void test_moveTokenWiseThroughEveryFileAndInvokeCompletion();
    void test_moveTokenWiseThroughEveryFileAndTriggerQuickFixes();

    // CppAutoCompleter tests
    void test_autoComplete_data();
    void test_autoComplete();
    void test_surroundWithSelection_data();
    void test_surroundWithSelection();
    void test_autoBackspace_data();
    void test_autoBackspace();
    void test_insertParagraph_data();
    void test_insertParagraph();
#endif // WITH_TESTS

private:
    CppEditorPluginPrivate *d = nullptr;
};

} // namespace Internal
} // namespace CppEditor
