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

#include <cplusplus/CppDocument.h>
#include <cplusplus/TranslationUnit.h>

#include <coreplugin/editormanager/editormanager.h>
#include <cppeditor/cppeditor.h>
#include <cppeditor/cppeditorplugin.h>
#include <cppeditor/cppquickfixassistant.h>
#include <cppeditor/cppquickfixes.h>
#include <cppeditor/cppquickfix.h>
#include <cppeditor/cppquickfix_test_utils.h>
#include <cpptools/cppmodelmanagerinterface.h>
#include <cpptools/cpptoolsplugin.h>
#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/projectexplorer.h>
#include <projectexplorer/project.h>
#include <texteditor/basetextdocument.h>

#include <QDebug>
#include <QtAlgorithms>
#include <QTextDocument>
#include <QtTest>

#if  QT_VERSION >= 0x050000
#define MSKIP_SINGLE(x) QSKIP(x)
#else
#define MSKIP_SINGLE(x) QSKIP(x, SkipSingle)
#endif

/*!
    Tests for executing "test actions" for
        (1) each file of each loaded project (file actions) or
            e.g. "Switch Header/Source"
        (2) each word/token of each file of each loaded project (token actions)
            e.g. "Trigger and perform all Quick Fixes"

    Purpose:
        Detect corner cases for which Qt Creator crashes or outputs QTC_ASSERTS.
        Correct behavior is *not* tested.

    Prerequisites:
        These tests depend on the projects that are loaded on startup of Qt Creator.
        Make sure to load the test projects via command line or the session manager.
        The test projects must be properly configured for a Kit.
 */

using namespace Core;
using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppEditor::Internal;
using namespace CppTools;
using namespace TextEditor;

namespace {

class TestActionsTestCase
{
public:
    class AbstractAction
    {
    public:
        /// Every Action is expected to undo its changes.
        virtual void run(CPPEditorWidget *editorWidget) = 0;
        virtual ~AbstractAction() {}
    };
    typedef QSharedPointer<AbstractAction> ActionPointer;
    typedef QList<ActionPointer> Actions;

public:
    /// Run the given fileActions for each file and the given tokenActions for each token.
    /// The cursor is positioned on the very first character of each token.
    void run(const Actions &tokenActions = Actions(),
             const Actions &fileActions = Actions());

    /// Simulate pressing ESC, which will close popups, search results pane, etc...
    /// This works only if the Qt Creator window is active.
    static void escape();

    /// Undoing changes
    static void undoChangesInEditorWidget(BaseTextEditorWidget *editorWidget);
    static void undoChangesInAllEditorWidgets();

    /// Execute actions for the current cursor position of editorWidget.
    static void executeActionsOnEditorWidget(CPPEditorWidget *editorWidget, Actions actions);

private:
    /// Move word camel case wise from current cursor position until given token (not included)
    /// and execute the tokenActions for each new position.
    static void moveWordCamelCaseToToken(TranslationUnit *translationUnit, Token token,
                                         CPPEditor *editor, const Actions &tokenActions);

    static void undoAllChangesAndCloseAllEditors();

    /// This function expects:
    /// (1) Only Qt4 projects are loaded (qmake in PATH should point to Qt4/bin).
    /// (2) No *.pro.user file exists for the projects.
    static void configureAllProjects(const QList<QPointer<ProjectExplorer::Project> > &projects);

    static bool allProjectsConfigured;
};

bool TestActionsTestCase::allProjectsConfigured = false;

typedef TestActionsTestCase::Actions Actions;
typedef TestActionsTestCase::ActionPointer ActionPointer;

void TestActionsTestCase::run(const Actions &tokenActions, const Actions &fileActions)
{
    CppModelManagerInterface *mm = CppModelManagerInterface::instance();
    EditorManager *em = EditorManager::instance();

    // Collect files to process
    QStringList filesToOpen;
    QList<QPointer<ProjectExplorer::Project> > projects;
    const QList<CppModelManagerInterface::ProjectInfo> projectInfos = mm->projectInfos();
    if (projectInfos.isEmpty())
        MSKIP_SINGLE("No project(s) loaded. Test operates only on loaded projects.");

    foreach (const CppModelManagerInterface::ProjectInfo &info, projectInfos) {
        QPointer<ProjectExplorer::Project> project = info.project();
        if (!projects.contains(project))
            projects << project;
        qDebug() << "Project" << info.project()->displayName() << "- files to process:"
                 << info.sourceFiles().size();
        foreach (const QString &sourceFile, info.sourceFiles())
            filesToOpen << sourceFile;
    }

    // Configure all projects on first execution of this function (= very first test)
    if (!TestActionsTestCase::allProjectsConfigured) {
        configureAllProjects(projects);
        TestActionsTestCase::allProjectsConfigured = true;
    }

    qSort(filesToOpen);

    // Process all files from the projects
    foreach (const QString filePath, filesToOpen) {
        // Skip e.g. "<configuration>"
        const QFileInfo fileInfo(filePath);
        if (!fileInfo.exists())
            continue;

        qDebug() << " --" << filePath;

        // Clean up
        undoAllChangesAndCloseAllEditors();

        // Open editor
        QCOMPARE(em->openedEditors().size(), 0);
        CPPEditor *editor = dynamic_cast<CPPEditor *>(em->openEditor(filePath));
        QVERIFY(editor);
        QCOMPARE(em->openedEditors().size(), 1);
        QVERIFY(mm->isCppEditor(editor));
        QVERIFY(mm->workingCopy().contains(filePath));

        // Rehighlight
        CPPEditorWidget *editorWidget = dynamic_cast<CPPEditorWidget *>(editor->editorWidget());
        QVERIFY(editorWidget);
        editorWidget->semanticRehighlight(true);
        while (editorWidget->semanticInfo().doc.isNull())
            QApplication::processEvents();

        // Run all file actions
        executeActionsOnEditorWidget(editorWidget, fileActions);

        if (tokenActions.empty())
            continue;

        Snapshot snapshot = mm->snapshot();
        Document::Ptr document = snapshot.preprocessedDocument(
            editorWidget->document()->toPlainText(), filePath);
        QVERIFY(document);
        document->parse();
        TranslationUnit *translationUnit = document->translationUnit();
        QVERIFY(translationUnit);
        QVERIFY(translationUnit->tokenCount() >= 2); // First invalid token included

        // Move word camel case wise from the top until the first user visible token
        editor->gotoLine(1);
        const Token token = translationUnit->tokenAt(1);
        moveWordCamelCaseToToken(translationUnit, token, editor, tokenActions);

        // Move token wise through the document
        // translationUnit->tokenAt(0) is the invalid token, so start with 1.
        for (unsigned i = 1, ei = translationUnit->tokenCount(); i < ei; ++i) {
            const Token token = translationUnit->tokenAt(i);
            if (token.kind() == T_EOF_SYMBOL)
                continue;

            if (token.expanded()) {
                // Move word camel case wise until first not expanded token
                unsigned j = i + 1;
                for (; translationUnit->tokenAt(j).expanded() && j < ei; ++j) ;
                if (j == ei) {
                    if (j == i + 1)
                        break;
                    else // All tokens are expanded until end
                        j = j - 1;
                }
                const Token token = translationUnit->tokenAt(j);
                moveWordCamelCaseToToken(translationUnit, token, editor, tokenActions);
                i = j - 1; // Continue with next not expanded token
            } else {
                // Position the cursor on the token
                unsigned line, column;
                translationUnit->getPosition(token.offset, &line, &column);
                editor->gotoLine(line, column - 1);
                QApplication::processEvents();

                // Run all token actions
                executeActionsOnEditorWidget(editorWidget, tokenActions);
            }
        } // for tokens
    } // for files

    // Clean up
    undoAllChangesAndCloseAllEditors();
}

void TestActionsTestCase::escape()
{
    if (QWidget *w = QApplication::focusWidget())
        QTest::keyClick(w, Qt::Key_Escape);
}

void TestActionsTestCase::undoChangesInEditorWidget(BaseTextEditorWidget *editorWidget)
{
    QTextDocument * const document = editorWidget->document();
    QVERIFY(document);
    while (document->isUndoAvailable())
        editorWidget->undo();
}

void TestActionsTestCase::undoChangesInAllEditorWidgets()
{
    EditorManager *em = EditorManager::instance();
    foreach (IEditor *editor, em->openedEditors()) {
        BaseTextEditor *baseTextEditor = qobject_cast<BaseTextEditor*>(editor);
        BaseTextEditorWidget *baseTextEditorWidget = baseTextEditor->editorWidget();
        undoChangesInEditorWidget(baseTextEditorWidget);
    }
}

void TestActionsTestCase::executeActionsOnEditorWidget(CPPEditorWidget *editorWidget,
                                                        TestActionsTestCase::Actions actions)
{
    foreach (const ActionPointer &action, actions)
        action->run(editorWidget);
    QApplication::processEvents();
}

void TestActionsTestCase::moveWordCamelCaseToToken(TranslationUnit *translationUnit, Token token,
                                                    CPPEditor *editor, const Actions &tokenActions)
{
    QVERIFY(translationUnit);
    QVERIFY(editor);
    CPPEditorWidget *editorWidget = dynamic_cast<CPPEditorWidget *>(editor->editorWidget());
    QVERIFY(editorWidget);

    unsigned line, column;
    translationUnit->getPosition(token.offset, &line, &column);

    while (editor->currentLine() < (int) line
           || (editor->currentLine() == (int) line
               && editor->currentColumn() < (int) column)) {
        editorWidget->gotoNextWordCamelCase();
        QApplication::processEvents();

        // Run all token actions
        executeActionsOnEditorWidget(editorWidget, tokenActions);
    }
}

void TestActionsTestCase::undoAllChangesAndCloseAllEditors()
{
    EditorManager *em = EditorManager::instance();
    undoChangesInAllEditorWidgets();
    em->closeAllEditors(/*askAboutModifiedEditors =*/ false);
    QApplication::processEvents();
    QCOMPARE(em->openedEditors().size(), 0);
}

void TestActionsTestCase::configureAllProjects(const QList<QPointer<ProjectExplorer::Project> >
                                               &projects)
{
    foreach (const QPointer<ProjectExplorer::Project> &project, projects) {
        qDebug() << "*** Configuring project" << project->displayName();
        project->configureAsExampleProject(QStringList());
    }
}

class NoOpTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Do nothing on each token
    void run(CPPEditorWidget *) {}
};

class FollowSymbolUnderCursorTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Follow symbol under cursor
    /// Warning: May block if file does not exists (e.g. a not generated ui_* file).
    void run(CPPEditorWidget *editorWidget);
};

void FollowSymbolUnderCursorTokenAction::run(CPPEditorWidget *editorWidget)
{
    EditorManager *em = EditorManager::instance();

    // Follow link
    IEditor *editorBefore = em->currentEditor();
    const int originalLine = editorBefore->currentLine();
    const int originalColumn = editorBefore->currentColumn();
    editorWidget->openLinkUnderCursor();
    TestActionsTestCase::escape();
    QApplication::processEvents();

    // Go back
    IEditor *editorAfter = em->currentEditor();
    if (editorAfter != editorBefore)
        em->goBackInNavigationHistory();
    else
        editorBefore->gotoLine(originalLine, originalColumn);
    QApplication::processEvents();
}

class SwitchDeclarationDefinitionTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Switch Declaration/Definition on each token
    void run(CPPEditorWidget *);
};

void SwitchDeclarationDefinitionTokenAction::run(CPPEditorWidget *)
{
    EditorManager *em = EditorManager::instance();

    // Switch Declaration/Definition
    IEditor *editorBefore = em->currentEditor();
    const int originalLine = editorBefore->currentLine();
    const int originalColumn = editorBefore->currentColumn();
    CppEditor::Internal::CppEditorPlugin::instance()->switchDeclarationDefinition();
    QApplication::processEvents();

    // Go back
    IEditor *editorAfter = em->currentEditor();
    if (editorAfter != editorBefore)
        em->goBackInNavigationHistory();
    else
        editorBefore->gotoLine(originalLine, originalColumn);
    QApplication::processEvents();
}

class FindUsagesTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Find Usages on each token
    void run(CPPEditorWidget *);
};

void FindUsagesTokenAction::run(CPPEditorWidget *)
{
    CppEditor::Internal::CppEditorPlugin::instance()->findUsages();
    QApplication::processEvents();
}

class RenameSymbolUnderCursorTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Rename Symbol Under Cursor on each token (Renaming is not applied)
    void run(CPPEditorWidget *);
};

void RenameSymbolUnderCursorTokenAction::run(CPPEditorWidget *)
{
    CppEditor::Internal::CppEditorPlugin::instance()->renameSymbolUnderCursor();
    QApplication::processEvents();
}

class OpenTypeHierarchyTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Open Type Hierarchy on each token
    void run(CPPEditorWidget *);
};

void OpenTypeHierarchyTokenAction::run(CPPEditorWidget *)
{
    CppEditor::Internal::CppEditorPlugin::instance()->openTypeHierarchy();
    QApplication::processEvents();
}

class InvokeCompletionTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Invoke completion menu on each token.
    /// Warning: May create tool tip artefacts if focus is lost.
    void run(CPPEditorWidget *editorWidget);
};

void InvokeCompletionTokenAction::run(CPPEditorWidget *editorWidget)
{
    // Invoke assistant and wait until it is finished
    QEventLoop loop;
    QObject::connect(editorWidget, SIGNAL(assistFinished()), &loop, SLOT(quit()));
    editorWidget->invokeAssist(Completion);
    loop.exec();

    // Close the completion popup widget
    QApplication::processEvents();
    editorWidget->abortAssist();
    // An alternative way to close the completion pop up is to give the editor widget the focus.
    // This works only if the Qt Creator window is always the active window.
    //    editorWidget->setFocus();
    QApplication::processEvents();

    TestActionsTestCase::undoChangesInEditorWidget(editorWidget);
}

class RunAllQuickFixesTokenAction : public TestActionsTestCase::AbstractAction
{
public:
    /// Trigger all Quick Fixes and apply the matching ones
    void run(CPPEditorWidget *editorWidget);
};

// TODO: Some QuickFixes operate on selections.
void RunAllQuickFixesTokenAction::run(CPPEditorWidget *editorWidget)
{
    // Calling editorWidget->invokeAssist(QuickFix) would be not enough
    // since we also want to execute the ones that match.

    const QList<CppQuickFixFactory *> quickFixFactories
        = ExtensionSystem::PluginManager::getObjects<CppQuickFixFactory>();
    QVERIFY(!quickFixFactories.isEmpty());

    CppQuickFixInterface qfi(new CppQuickFixAssistInterface(editorWidget, ExplicitlyInvoked));
    // This guard is important since the Quick Fixes expect to get a non-empty path().
    if (qfi->path().isEmpty())
        return;

    foreach (CppQuickFixFactory *quickFixFactory, quickFixFactories) {
        TextEditor::QuickFixOperations operations;
        // Some Quick Fixes pop up a dialog and are therefore inappropriate for this test.
        // Where possible, use a guiless version of the factory.
        if (qobject_cast<InsertVirtualMethods *>(quickFixFactory)) {
            QScopedPointer<CppQuickFixFactory> factoryProducingGuiLessOperations;
            factoryProducingGuiLessOperations.reset(
                new InsertVirtualMethods(
                    new InsertVirtualMethodsDialogTest(
                        InsertVirtualMethodsDialog::ModeOutsideClass, true)));
            factoryProducingGuiLessOperations->match(qfi, operations);
        } else {
            quickFixFactory->match(qfi, operations);
        }

        foreach (QuickFixOperation::Ptr operation, operations) {
            qDebug() << "    -- Performing Quick Fix" << operation->description();
            operation->perform();
            TestActionsTestCase::escape();
            TestActionsTestCase::undoChangesInAllEditorWidgets();
            QApplication::processEvents();
        }
    }
}

class SwitchHeaderSourceFileAction : public TestActionsTestCase::AbstractAction
{
public:
    void run(CPPEditorWidget *);
};

void SwitchHeaderSourceFileAction::run(CPPEditorWidget *)
{
    EditorManager *em = EditorManager::instance();

    // Switch Header/Source
    IEditor *editorBefore = em->currentEditor();
    CppTools::Internal::CppToolsPlugin::instance()->switchHeaderSource();
    QApplication::processEvents();

    // Go back
    IEditor *editorAfter = em->currentEditor();
    if (editorAfter != editorBefore)
        em->goBackInNavigationHistory();
    QApplication::processEvents();
}

} // anonymous namespace

void CppEditorPlugin::test_openEachFile()
{
    TestActionsTestCase test;
    test.run();
}

void CppEditorPlugin::test_switchHeaderSourceOnEachFile()
{
    Actions fileActions;
    fileActions << ActionPointer(new SwitchHeaderSourceFileAction);

    TestActionsTestCase test;
    test.run(Actions(), fileActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFile()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new NoOpTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

/// May block if file does not exists (e.g. a not generated ui_* file).
void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndFollowSymbol()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new FollowSymbolUnderCursorTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndSwitchDeclarationDefinition()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new SwitchDeclarationDefinitionTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndFindUsages()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new FindUsagesTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndRenameUsages()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new RenameSymbolUnderCursorTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndOpenTypeHierarchy()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new OpenTypeHierarchyTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndInvokeCompletion()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new InvokeCompletionTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}

void CppEditorPlugin::test_moveTokenWiseThroughEveryFileAndTriggerQuickFixes()
{
    Actions tokenActions;
    tokenActions << ActionPointer(new RunAllQuickFixesTokenAction());

    TestActionsTestCase test;
    test.run(tokenActions);
}
