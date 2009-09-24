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

#ifndef DUIEDITORPLUGIN_H
#define DUIEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace TextEditor {
class TextEditorActionHandler;
} // namespace TextEditor

namespace DuiEditor {

class DuiModelManagerInterface;
class QmlFileWizard;

namespace Internal {

class DuiEditorFactory;
class DuiCodeCompletion;
class ScriptEditor;

class DuiEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    DuiEditorPlugin();
    virtual ~DuiEditorPlugin();

    // IPlugin
    bool initialize(const QStringList &arguments, QString *errorMessage = 0);
    void extensionsInitialized();

    static DuiEditorPlugin *instance()
    { return m_instance; }

    void initializeEditor(ScriptEditor *editor);

private:
    void registerActions();

    static DuiEditorPlugin *m_instance;

    typedef QList<int> Context;
    Context m_context;
    Context m_scriptcontext;

    DuiModelManagerInterface *m_modelManager;
    QmlFileWizard *m_wizard;
    DuiEditorFactory *m_editor;
    TextEditor::TextEditorActionHandler *m_actionHandler;
    DuiCodeCompletion *m_completion;
};

} // namespace Internal
} // namespace DuiEditor

#endif // DUIEDITORPLUGIN_H
