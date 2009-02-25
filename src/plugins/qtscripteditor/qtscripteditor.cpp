/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
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
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#include "qtscripteditor.h"
#include "qtscripteditorconstants.h"
#include "qtscripthighlighter.h"
#include "qtscripteditorplugin.h"

#include <indenter.h>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textblockiterator.h>
#include <texteditor/texteditorconstants.h>

#include <QtGui/QMenu>

namespace QtScriptEditor {
namespace Internal {

ScriptEditorEditable::ScriptEditorEditable(ScriptEditor *editor, const QList<int>& context)
    : BaseTextEditorEditable(editor), m_context(context)
{

}

ScriptEditor::ScriptEditor(const Context &context,
                           TextEditor::TextEditorActionHandler *ah,
                           QWidget *parent) :
    TextEditor::BaseTextEditor(parent),
    m_context(context),
    m_ah(ah)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingVisible(true);
    setMimeType(QtScriptEditor::Constants::C_QTSCRIPTEDITOR_MIMETYPE);

    baseTextDocument()->setSyntaxHighlighter(new QtScriptHighlighter);
}

ScriptEditor::~ScriptEditor()
{
}

Core::IEditor *ScriptEditorEditable::duplicate(QWidget *parent)
{
    return qobject_cast<ScriptEditor*>(editor())->duplicate(parent)->editableInterface();
}

ScriptEditor *ScriptEditor::duplicate(QWidget *parent)
{
    ScriptEditor *editor = new ScriptEditor(m_context, m_ah, parent);
    editor->duplicateFrom(this);
    QtScriptEditorPlugin::initializeEditor(editor);
    return editor;
}

const char *ScriptEditorEditable::kind() const
{
    return QtScriptEditor::Constants::C_QTSCRIPTEDITOR;
}

ScriptEditor::Context ScriptEditorEditable::context() const
{
    return m_context;
}

void ScriptEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    QtScriptHighlighter *highlighter = qobject_cast<QtScriptHighlighter*>(baseTextDocument()->syntaxHighlighter());
    if (!highlighter)
        return;

    static QVector<QString> categories;
    if (categories.isEmpty()) {
        categories << QLatin1String(TextEditor::Constants::C_NUMBER)
                   << QLatin1String(TextEditor::Constants::C_STRING)
                   << QLatin1String(TextEditor::Constants::C_TYPE)
                   << QLatin1String(TextEditor::Constants::C_KEYWORD)
                   << QLatin1String(TextEditor::Constants::C_PREPROCESSOR)
                   << QLatin1String(TextEditor::Constants::C_LABEL)
                   << QLatin1String(TextEditor::Constants::C_COMMENT);
    }

    highlighter->setFormats(fs.toTextCharFormats(categories));
    highlighter->rehighlight();
}

bool ScriptEditor::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('{') || ch == QLatin1Char('}'))
        return true;
    return false;
}

    // Indent a code line based on previous
template <class Iterator>
static void indentScriptBlock(const TextEditor::TabSettings &ts,
                              const QTextBlock &block,
                              const Iterator &programBegin,
                              const Iterator &programEnd,
                              QChar typedChar)
{
    typedef typename SharedTools::Indenter<Iterator> Indenter ;
    Indenter &indenter = Indenter::instance();
    indenter.setIndentSize(ts.m_tabSize);
    const TextEditor::TextBlockIterator current(block);
    const int indent = indenter.indentForBottomLine(current, programBegin,
                                                    programEnd, typedChar);
    ts.indentLine(block, indent);
}

void ScriptEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    const TextEditor::TextBlockIterator begin(doc->begin());
    const TextEditor::TextBlockIterator end(block.next());
    indentScriptBlock(tabSettings(), block, begin, end, typedChar);
}

void ScriptEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();

    if (Core::ActionContainer *mcontext = Core::ICore::instance()->actionManager()->actionContainer(QtScriptEditor::Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions())
            menu->addAction(action);
    }

    menu->exec(e->globalPos());
    delete menu;
}

} // namespace Internal
} // namespace QtScriptEditor
