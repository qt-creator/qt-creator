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

#ifndef CPPPLUGIN_H
#define CPPPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtPlugin>
#include <QStringList>
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

class CppPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "CppEditor.json")

public:
    CppPlugin();
    ~CppPlugin();

    static CppPlugin *instance();

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

public slots:
    void setSortedOutline(bool sorted);

private slots:
    void switchDeclarationDefinition();
    void renameSymbolUnderCursor();
    void onTaskStarted(const QString &type);
    void onAllTasksFinished(const QString &type);
    void findUsages();
    void currentEditorChanged(Core::IEditor *editor);
    void openTypeHierarchy();

#ifdef WITH_TESTS
private slots: // quickfix tests
    void test_quickfix_GetterSetter_basicGetterWithPrefix();
    void test_quickfix_GetterSetter_basicGetterWithoutPrefix();
    void test_quickfix_GetterSetter_customType();
    void test_quickfix_GetterSetter_constMember();
    void test_quickfix_GetterSetter_pointerToNonConst();
    void test_quickfix_GetterSetter_pointerToConst();
    void test_quickfix_GetterSetter_staticMember();
    void test_quickfix_GetterSetter_secondDeclarator();
    void test_quickfix_GetterSetter_triggeringRightAfterPointerSign();
    void test_quickfix_GetterSetter_notTriggeringOnMemberFunction();
    void test_quickfix_GetterSetter_notTriggeringOnMemberArray();
    void test_quickfix_GetterSetter_notTriggeringWhenGetterOrSetterExist();
#endif // WITH_TESTS

private:
    Core::IEditor *createEditor(QWidget *parent);
    void writeSettings();
    void readSettings();

    static CppPlugin *m_instance;

    TextEditor::TextEditorActionHandler *m_actionHandler;
    bool m_sortedOutline;
    QAction *m_renameSymbolUnderCursorAction;
    QAction *m_findUsagesAction;
    QAction *m_updateCodeModelAction;
    QAction *m_openTypeHierarchyAction;

    CppQuickFixAssistProvider *m_quickFixProvider;

    QPointer<TextEditor::ITextEditor> m_currentEditor;
};

class CppEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    CppEditorFactory(CppPlugin *owner);

    // IEditorFactory
    QStringList mimeTypes() const;
    Core::IEditor *createEditor(QWidget *parent);
    Core::Id id() const;
    QString displayName() const;

private:
    CppPlugin *m_owner;
    QStringList m_mimeTypes;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPPLUGIN_H
