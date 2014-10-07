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

#ifndef CPPEDITORPLUGIN_H
#define CPPEDITORPLUGIN_H

#include <coreplugin/editormanager/ieditorfactory.h>

#include <extensionsystem/iplugin.h>

#include <QtPlugin>
#include <QAction>

namespace TextEditor { class BaseTextEditor; }

namespace CppEditor {
namespace Internal {

class CppEditorWidget;
class CppCodeModelInspectorDialog;
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
    void inspectCppCodeModel();

#ifdef WITH_TESTS
private slots:
    // The following tests expect that no projects are loaded on start-up.
    void test_SwitchMethodDeclarationDefinition_data();
    void test_SwitchMethodDeclarationDefinition();

    void test_FollowSymbolUnderCursor_multipleDocuments_data();
    void test_FollowSymbolUnderCursor_multipleDocuments();

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

    void test_doxygen_comments_data();
    void test_doxygen_comments();

    void test_quickfix_data();
    void test_quickfix();

    void test_quickfix_GenerateGetterSetter_basicGetterWithPrefixAndNamespaceToCpp();

    void test_quickfix_ConvertQt4Connect_connectOutOfClass();
    void test_quickfix_ConvertQt4Connect_connectWithinClass_data();
    void test_quickfix_ConvertQt4Connect_connectWithinClass();

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

    void test_quickfix_InsertDeclFromDef();

    void test_quickfix_AddIncludeForUndefinedIdentifier_onSimpleName();
    void test_quickfix_AddIncludeForUndefinedIdentifier_onNameOfQualifiedName();
    void test_quickfix_AddIncludeForUndefinedIdentifier_onBaseOfQualifiedName();
    void test_quickfix_AddIncludeForUndefinedIdentifier_onTemplateName();
    void test_quickfix_AddIncludeForUndefinedIdentifier_onTemplateNameInsideArguments();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_ignoremoc();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_sortingTop();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_sortingMiddle();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_sortingBottom();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_appendToUnsorted();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_firstLocalIncludeAtFront();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_firstGlobalIncludeAtBack();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_preferGroupWithLongerMatchingPrefix();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_newGroupIfOnlyDifferentIncludeDirs();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_mixedDirsSorted();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_mixedDirsUnsorted();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_mixedIncludeTypes1();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_mixedIncludeTypes2();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_mixedIncludeTypes3();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_mixedIncludeTypes4();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_noinclude();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_onlyIncludeGuard();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_veryFirstIncludeCppStyleCommentOnTop();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_veryFirstIncludeCStyleCommentOnTop();
    void test_quickfix_AddIncludeForUndefinedIdentifier_inserting_checkQSomethingInQtIncludePaths();

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

    void test_quickfix_AssignToLocalVariable_templates();

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

    bool m_sortedOutline;
    QAction *m_renameSymbolUnderCursorAction;
    QAction *m_findUsagesAction;
    QAction *m_reparseExternallyChangedFiles;
    QAction *m_openTypeHierarchyAction;
    QAction *m_openIncludeHierarchyAction;

    CppQuickFixAssistProvider *m_quickFixProvider;

    QPointer<CppCodeModelInspectorDialog> m_cppCodeModelInspectorDialog;

    QPointer<TextEditor::BaseTextEditor> m_currentEditor;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPEDITORPLUGIN_H
