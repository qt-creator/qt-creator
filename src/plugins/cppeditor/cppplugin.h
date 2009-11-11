/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef CPPPLUGIN_H
#define CPPPLUGIN_H

#include <extensionsystem/iplugin.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <QtCore/QtPlugin>
#include <QtCore/QStringList>
#include <QtGui/QAction>

namespace TextEditor {
class TextEditorActionHandler;
} // namespace TextEditor

namespace CppEditor {
namespace Internal {

class CPPEditor;

class CppPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    CppPlugin();
    ~CppPlugin();

    static CppPlugin *instance();

    bool initialize(const QStringList &arguments, QString *error_message = 0);
    void extensionsInitialized();
    void shutdown();

    // Connect editor to settings changed signals.
    void initializeEditor(CPPEditor *editor);

    bool sortedMethodOverview() const;

signals:
    void methodOverviewSortingChanged(bool sort);

public slots:
    void setSortedMethodOverview(bool sorted);

private slots:
    void switchDeclarationDefinition();
    void jumpToDefinition();
    void renameSymbolUnderCursor();
    void onTaskStarted(const QString &type);
    void onAllTasksFinished(const QString &type);
    void findUsages();

private:
    Core::IEditor *createEditor(QWidget *parent);
    void writeSettings();
    void readSettings();

    static CppPlugin *m_instance;

    TextEditor::TextEditorActionHandler *m_actionHandler;
    bool m_sortedMethodOverview;
    QAction *m_renameSymbolUnderCursorAction;
    QAction *m_findUsagesAction;
    QAction *m_updateCodeModelAction;
};

class CppEditorFactory : public Core::IEditorFactory
{
    Q_OBJECT

public:
    CppEditorFactory(CppPlugin *owner);

    virtual QStringList mimeTypes() const;

    Core::IEditor *createEditor(QWidget *parent);

    virtual QString kind() const;
    Core::IFile *open(const QString &fileName);

private:
    const QString m_kind;
    CppPlugin *m_owner;
    QStringList m_mimeTypes;
};

} // namespace Internal
} // namespace CppEditor

#endif // CPPPLUGIN_H
