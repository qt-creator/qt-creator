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

#ifndef TEXTEDITORPLUGIN_H
#define TEXTEDITORPLUGIN_H

#include <extensionsystem/iplugin.h>

namespace Find {
class SearchResultWindow;
}

namespace TextEditor {

class FontSettings;
class PlainTextEditorWidget;
class TextEditorSettings;

namespace Internal {

class LineNumberFilter;
class PlainTextEditorFactory;
class OutlineFactory;
class BaseTextMarkRegistry;

class TextEditorPlugin : public ExtensionSystem::IPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QtCreatorPlugin" FILE "TextEditor.json")

public:
    TextEditorPlugin();
    virtual ~TextEditorPlugin();

    static TextEditorPlugin *instance();

    // ExtensionSystem::PluginInterface
    bool initialize(const QStringList &arguments, QString *errorMessage);
    void extensionsInitialized();

    void initializeEditor(PlainTextEditorWidget *editor);

    PlainTextEditorFactory *editorFactory() { return m_editorFactory; }
    LineNumberFilter *lineNumberFilter() { return m_lineNumberFilter; }
    BaseTextMarkRegistry *baseTextMarkRegistry() { return m_baseTextMarkRegistry; }

private slots:
    void invokeCompletion();
    void invokeQuickFix();
    void updateSearchResultsFont(const TextEditor::FontSettings &);
    void updateVariable(const QByteArray &variable);
    void updateCurrentSelection(const QString &text);

private:
    static TextEditorPlugin *m_instance;
    TextEditorSettings *m_settings;
    PlainTextEditorFactory *m_editorFactory;
    LineNumberFilter *m_lineNumberFilter;
    Find::SearchResultWindow *m_searchResultWindow;
    OutlineFactory *m_outlineFactory;
    BaseTextMarkRegistry *m_baseTextMarkRegistry;
};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITORPLUGIN_H
