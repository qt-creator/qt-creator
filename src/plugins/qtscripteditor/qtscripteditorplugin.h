/***************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2008 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
**
** Non-Open Source Usage
**
** Licensees may use this file in accordance with the Qt Beta Version
** License Agreement, Agreement version 2.2 provided with the Software or,
** alternatively, in accordance with the terms contained in a written
** agreement between you and Nokia.
**
** GNU General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU General
** Public License versions 2.0 or 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the packaging
** of this file.  Please review the following information to ensure GNU
** General Public Licensing requirements will be met:
**
** http://www.fsf.org/licensing/licenses/info/GPLv2.html and
** http://www.gnu.org/copyleft/gpl.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt GPL Exception
** version 1.3, included in the file GPL_EXCEPTION.txt in this package.
**
***************************************************************************/

#ifndef QTSCRIPTEDITORPLUGIN_H
#define QTSCRIPTEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Core {
class ICore;
}

namespace TextEditor {
class FontSettingsPage;
class TextFileWizard;
}

namespace QtScriptEditor {
namespace Internal {

class QtScriptWizard;
class QtScriptEditorFactory;
class ScriptEditor;

class QtScriptEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    QtScriptEditorPlugin();
    virtual ~QtScriptEditorPlugin();

    //Plugin
    bool initialize(const QStringList &arguments, QString *error_message = 0);
    void extensionsInitialized();

    static void initializeEditor(ScriptEditor *editor);

private:
    void registerActions(Core::ICore *core);

    static QtScriptEditorPlugin *m_instance;

    typedef QList<int> Context;
    Context m_context;
    Context m_scriptcontext;

    TextEditor::TextFileWizard *m_wizard;
    QtScriptEditorFactory *m_editor;
};

} // namespace Internal
} // namespace QtScriptEditor

#endif // QTSCRIPTEDITORPLUGIN_H
