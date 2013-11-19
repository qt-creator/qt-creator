/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#ifndef CPPEDITORPLUGIN_H
#define CPPEDITORPLUGIN_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <extensionsystem/iplugin.h>

#include <QtPlugin>
#include <QAction>

namespace TextEditor {
class TextEditorActionHandler;
class ITextEditor;
} // namespace TextEditor

namespace CppEditor {
namespace Internal {

class CPPEditorWidget;
class CppQuickFixCollector;
class CppQuickFixAssistProvider;

class CppEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppEditor.json")

public:
    CppEditorPlugin();
    ~CppEditorPlugin();

    static CppEditorPlugin *instance();

    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();
    ShutdownFlag aboutToShutdown();

    // Connect editor to settings changed signals.
    void initializeEditor(CPPEditorWidget *editor);

    bool sortedOutline() const;

    CppQuickFixAssistProvider *quickFixProvider() const;

signals:
    void outlineSortingChanged(bool sort);
    void typeHierarchyRequested();
    void includeHierarchyRequested();

public slots:
    void openDeclarationDefinitionInNextSplit();
    void openTypeHierarchy();
    void openIncludeHierarchy();
    void findUsages();
    void showPreProcessorDialog();
    void renameSymbolUnderCursor();
    void switchDeclarationDefinition();

    void setSortedOutline(bool sorted);

private slots:
    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);

#ifdef WITH_TESTS
private slots:
    // The following tests expect that no projects are loaded on start-up.
    void test_SwitchMethodDeclarationDefinition_data();
    void test_SwitchMethodDeclarationDefinition();

    void test_FollowSymbolUnderCursor_data();
    void test_FollowSymbolUnderCursor();

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

    void test_doxygen_comments_qt_style();
    void test_doxygen_comments_qt_style_continuation();
    void test_doxygen_comments_java_style();
    void test_doxygen_comments_java_style_continuation();
    void test_doxygen_comments_cpp_styleA();
    void test_doxygen_comments_cpp_styleB();
    void test_doxygen_comments_cpp_styleA_indented();
    void test_doxygen_comments_cpp_styleA_continuation();
    void test_doxygen_comments_cpp_styleA_indented_continuation();
    void test_doxygen_comments_cpp_styleA_corner_case();

    void test_quickfix_CompleteSwitchCaseStatement_basic1();
    void test_quickfix_CompleteSwitchCaseStatement_basic2();
    void test_quickfix_CompleteSwitchCaseStatement_oneValueMissing();
    void test_quickfix_CompleteSwitchCaseStatement_QTCREATORBUG10366_1();
    void test_quickfix_CompleteSwitchCaseStatement_QTCREATORBUG10366_2();

    void test_quickfix_GenerateGetterSetter_basicGetterWithPrefix();
    void test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespace();
    void test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespaceToCpp();
    void test_quickfix_GenerateGetterSetter_basicGetterWithoutPrefix();
    void test_quickfix_GenerateGetterSetter_customType();
    void test_quickfix_GenerateGetterSetter_constMember();
    void test_quickfix_GenerateGetterSetter_pointerToNonConst();
    void test_quickfix_GenerateGetterSetter_pointerToConst();
    void test_quickfix_GenerateGetterSetter_staticMember();
    void test_quickfix_GenerateGetterSetter_secondDeclarator();
    void test_quickfix_GenerateGetterSetter_triggeringRightAfterPointerSign();
    void test_quickfix_GenerateGetterSetter_notTriggeringOnMemberFunction();
    void test_quickfix_GenerateGetterSetter_notTriggeringOnMemberArray();
    void test_quickfix_GenerateGetterSetter_notTriggeringWhenGetterOrSetterExist();

    void test_quickfix_ReformatPointerDeclaration();

    void test_quickfix_InsertDefFromDecl_basic();
    void test_quickfix_InsertDefFromDecl_afterClass();
    void test_quickfix_InsertDefFromDecl_headerSource_basic1();
    void test_quickfix_InsertDefFromDecl_headerSource_basic2();
    void test_quickfix_InsertDefFromDecl_headerSource_basic3();
    void test_quickfix_InsertDefFromDecl_headerSource_namespace1();
    void test_quickfix_InsertDefFromDecl_headerSource_namespace2();
    void test_quickfix_InsertDefFromDecl_freeFunction();
    void test_quickfix_InsertDefFromDecl_insideClass();
    void test_quickfix_InsertDefFromDecl_notTriggeringWhenDefinitionExists();
    void test_quickfix_InsertDefFromDecl_notTriggeringStatement();
    void test_quickfix_InsertDefFromDecl_findRightImplementationFile();
    void test_quickfix_InsertDefFromDecl_ignoreSurroundingGeneratedDeclarations();
    void test_quickfix_InsertDefFromDecl_respectWsInOperatorNames1();
    void test_quickfix_InsertDefFromDecl_respectWsInOperatorNames2();
    void test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile1();
    void test_quickfix_InsertDefFromDecl_macroUsesAtEndOfFile2();
    void test_quickfix_InsertDefFromDecl_erroneousStatementAtEndOfFile();

    void test_quickfix_InsertDeclFromDef();

    void test_quickfix_AddIncludeForUndefinedIdentifier_detectIncludeGroupsByNewLines();
    void test_quickfix_AddIncludeForUndefinedIdentifier_detectIncludeGroupsByIncludeDir();
    void test_quickfix_AddIncludeForUndefinedIdentifier_detectIncludeGroupsByIncludeType();
    void test_quickfix_AddIncludeForUndefinedIdentifier_normal();
    void test_quickfix_AddIncludeForUndefinedIdentifier_ignoremoc();
    void test_quickfix_AddIncludeForUndefinedIdentifier_sortingTop();
    void test_quickfix_AddIncludeForUndefinedIdentifier_sortingMiddle();
    void test_quickfix_AddIncludeForUndefinedIdentifier_sortingBottom();
    void test_quickfix_AddIncludeForUndefinedIdentifier_appendToUnsorted();
    void test_quickfix_AddIncludeForUndefinedIdentifier_firstLocalIncludeAtFront();
    void test_quickfix_AddIncludeForUndefinedIdentifier_firstGlobalIncludeAtBack();
    void test_quickfix_AddIncludeForUndefinedIdentifier_preferGroupWithLongerMatchingPrefix();
    void test_quickfix_AddIncludeForUndefinedIdentifier_newGroupIfOnlyDifferentIncludeDirs();
    void test_quickfix_AddIncludeForUndefinedIdentifier_mixedDirsSorted();
    void test_quickfix_AddIncludeForUndefinedIdentifier_mixedDirsUnsorted();
    void test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes1();
    void test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes2();
    void test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes3();
    void test_quickfix_AddIncludeForUndefinedIdentifier_mixedIncludeTypes4();
    void test_quickfix_AddIncludeForUndefinedIdentifier_noinclude();
    void test_quickfix_AddIncludeForUndefinedIdentifier_veryFirstIncludeCppStyleCommentOnTop();
    void test_quickfix_AddIncludeForUndefinedIdentifier_veryFirstIncludeCStyleCommentOnTop();
    void test_quickfix_AddIncludeForUndefinedIdentifier_checkQSomethingInQtIncludePaths();

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

    void test_quickfix_MoveFuncDefToDecl_MemberFunc();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncOutside();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNS();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncToCppNSUsing();
    void test_quickfix_MoveFuncDefToDecl_MemberFuncOutsideWithNs();
    void test_quickfix_MoveFuncDefToDecl_FreeFuncToCpp();
    void test_quickfix_MoveFuncDefToDecl_FreeFuncToCppNS();
    void test_quickfix_MoveFuncDefToDecl_CtorWithInitialization();
    void test_quickfix_MoveFuncDefToDecl_structWithAssignedVariable();

    void test_quickfix_AssignToLocalVariable_freeFunction();
    void test_quickfix_AssignToLocalVariable_memberFunction();
    void test_quickfix_AssignToLocalVariable_staticMemberFunction();
    void test_quickfix_AssignToLocalVariable_newExpression();
    void test_quickfix_AssignToLocalVariable_templates();
    void test_quickfix_AssignToLocalVariable_noInitializationList();
    void test_quickfix_AssignToLocalVariable_noVoidFunction();
    void test_quickfix_AssignToLocalVariable_noVoidMemberFunction();
    void test_quickfix_AssignToLocalVariable_noVoidStaticMemberFunction();
    void test_quickfix_AssignToLocalVariable_noFunctionInExpression();
    void test_quickfix_AssignToLocalVariable_noFunctionInFunction();
    void test_quickfix_AssignToLocalVariable_noReturnClass1();
    void test_quickfix_AssignToLocalVariable_noReturnClass2();
    void test_quickfix_AssignToLocalVariable_noReturnFunc1();
    void test_quickfix_AssignToLocalVariable_noReturnFunc2();
    void test_quickfix_AssignToLocalVariable_noSignatureMatch();

    void test_quickfix_ExtractLiteralAsParameter_typeDeduction_data();
    void test_quickfix_ExtractLiteralAsParameter_typeDeduction();
    void test_quickfix_ExtractLiteralAsParameter_freeFunction();
    void test_quickfix_ExtractLiteralAsParameter_freeFunction_separateFiles();
    void test_quickfix_ExtractLiteralAsParameter_memberFunction();
    void test_quickfix_ExtractLiteralAsParameter_memberFunction_separateFiles();
    void test_quickfix_ExtractLiteralAsParameter_memberFunctionInline();

    void test_quickfix_InsertVirtualMethods_onlyDecl();
    void test_quickfix_InsertVirtualMethods_onlyDeclWithoutVirtual();
    void test_quickfix_InsertVirtualMethods_Access();
    void test_quickfix_InsertVirtualMethods_Superclass();
    void test_quickfix_InsertVirtualMethods_SuperclassOverride();
    void test_quickfix_InsertVirtualMethods_PureVirtualOnlyDecl();
    void test_quickfix_InsertVirtualMethods_PureVirtualInside();
    void test_quickfix_InsertVirtualMethods_inside();
    void test_quickfix_InsertVirtualMethods_outside();
    void test_quickfix_InsertVirtualMethods_implementationFile();
    void test_quickfix_InsertVirtualMethods_notrigger_allImplemented();
    void test_quickfix_InsertVirtualMethods_BaseClassInNamespace();

    void test_quickfix_OptimizeForLoop_postcrement();
    void test_quickfix_OptimizeForLoop_condition();
    void test_quickfix_OptimizeForLoop_flipedCondition();
    void test_quickfix_OptimizeForLoop_alterVariableName();
    void test_quickfix_OptimizeForLoop_optimizeBoth();
    void test_quickfix_OptimizeForLoop_emptyInitializer();
    void test_quickfix_OptimizeForLoop_wrongInitializer();
    void test_quickfix_OptimizeForLoop_noTriggerNumeric1();
    void test_quickfix_OptimizeForLoop_noTriggerNumeric2();

    void test_functionhelper_virtualFunctions();
    void test_functionhelper_virtualFunctions_data();

    // tests for "Include Hiererchy"
    void test_includeHierarchyModel_simpleIncludes();
    void test_includeHierarchyModel_simpleIncludedBy();
    void test_includeHierarchyModel_simpleIncludesAndIncludedBy();

    // The following tests depend on the projects that are loaded on startup
    // and will be skipped in case no projects are loaded.
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
#endif // WITH_TESTS

private:
    Core::IEditor *createEditor(QWidget *parent);
    void writeSettings();
    void readSettings();

    static CppEditorPlugin *m_instance;

    TextEditor::TextEditorActionHandler *m_actionHandler;
    bool m_sortedOutline;
    QAction *m_renameSymbolUnderCursorAction;
    QAction *m_findUsagesAction;
    QAction *m_reparseExternallyChangedFiles;
    QAction *m_openTypeHierarchyAction;
    QAction *m_openIncludeHierarchyAction;

    CppQuickFixAssistProvider *m_quickFixProvider;

    QPointer<TextEditor::ITextEditor> m_currentEditor;
};

class CppEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    CppEditorFactory(CppEditorPlugin *owner);

    Core::IEditor *createEditor(QWidget *parent);

private:
    CppEditorPlugin *m_owner;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITORPLUGIN_H
