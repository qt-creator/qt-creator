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

private slots:
    void onTaskStarted(Core::Id type);
    void onAllTasksFinished(Core::Id type);
    void inspectCppCodeModel();

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
    void test_quickfix_GenerateGetterSetter_onlySetter();
    void test_quickfix_GenerateGetterSetter_offerGetterWhenSetterPresent();
    void test_quickfix_GenerateGetterSetter_offerSetterWhenGetterPresent();

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
#endif // WITH_TESTS

private:
    Core::IEditor *createEditor(QWidget *parent);

    static CppEditorPlugin *m_instance;

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
