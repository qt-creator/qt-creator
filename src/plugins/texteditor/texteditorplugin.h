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

#ifndef TEXTEDITORPLUGIN_H
#define TEXTEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Find {
class SearchResultWindow;
}

namespace TextEditor {

class FontSettings;
class PlainTextEditor;
class TextEditorSettings;
class TextFileWizard;

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

    // ExtensionSystem::PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

    void initializeEditor(PlainTextEditor *editor);

    LineNumberFilter *lineNumberFilter() { return m_lineNumberFilter; }

private slots:
    void invokeCompletion();
    void invokeQuickFix();
    void updateSearchResultsFont(const TextEditor::FontSettings &);

private:
    static TextEditorPlugin *m_instance;
    TextEditorSettings *m_settings;
    TextFileWizard *m_wizard;
    PlainTextEditorFactory *m_editorFactory;
    LineNumberFilter *m_lineNumberFilter;
    Find::SearchResultWindow *m_searchResultWindow;
};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITORPLUGIN_H
