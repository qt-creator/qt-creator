/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
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
class PlainTextEditorWidget;
class TextEditorSettings;
class TextFileWizard;

namespace Internal {

class LineNumberFilter;
class PlainTextEditorFactory;
class OutlineFactory;

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

    void initializeEditor(PlainTextEditorWidget *editor);

    PlainTextEditorFactory *editorFactory() { return m_editorFactory; }
    LineNumberFilter *lineNumberFilter() { return m_lineNumberFilter; }

private slots:
    void invokeCompletion();
    void invokeQuickFix();
    void updateSearchResultsFont(const TextEditor::FontSettings &);
    void updateVariable(const QByteArray &variable);
    void updateCurrentSelection(const QString &text);

private:
    static TextEditorPlugin *m_instance;
    TextEditorSettings *m_settings;
    TextFileWizard *m_wizard;
    PlainTextEditorFactory *m_editorFactory;
    LineNumberFilter *m_lineNumberFilter;
    Find::SearchResultWindow *m_searchResultWindow;
    OutlineFactory *m_outlineFactory;
};

} // namespace Internal
} // namespace TextEditor

#endif // TEXTEDITORPLUGIN_H
