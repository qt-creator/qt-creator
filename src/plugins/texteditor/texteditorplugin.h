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
** rights. These rights are described in the Nokia Qt GPL Exception version
** 1.2, included in the file GPL_EXCEPTION.txt in this package.  
** 
***************************************************************************/
#ifndef TEXTEDITORPLUGIN_H
#define TEXTEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

QT_BEGIN_NAMESPACE
class QAction;
QT_END_NAMESPACE

namespace Core {
class ICore;
class IEditor;
}

namespace TextEditor {

class FontSettings;
class FontSettingsPage;
class TextEditorSettings;
class TextFileWizard;
class PlainTextEditor;

namespace Internal {

class LineNumberFilter;
class PlainTextEditorFactory;

class TextEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT

public:
    TextEditorPlugin();
    virtual ~TextEditorPlugin();

    static TextEditorPlugin *instance();
    static Core::ICore *core();

    // ExtensionSystem::PluginInterface
    bool initialize(const QStringList &arguments, QString *);
    void extensionsInitialized();

    void initializeEditor(PlainTextEditor *editor);

    LineNumberFilter *lineNumberFilter() { return m_lineNumberFilter; }

private slots:
    void invokeCompletion();

private:
    static TextEditorPlugin *m_instance;
    Core::ICore *m_core;
    TextEditorSettings *m_settings;
    TextFileWizard *m_wizard;
    PlainTextEditorFactory *m_editorFactory;
    LineNumberFilter *m_lineNumberFilter;
};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITORPLUGIN_H
