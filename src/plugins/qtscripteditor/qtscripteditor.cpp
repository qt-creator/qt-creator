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

#include "qtscripteditor.h"
#include "qtscripteditorconstants.h"
#include "qtscripthighlighter.h"
#include "qtscripteditorplugin.h"

#include "parser/javascriptengine_p.h"
#include "parser/javascriptparser_p.h"
#include "parser/javascriptlexer_p.h"
#include "parser/javascriptnodepool_p.h"
#include "parser/javascriptastvisitor_p.h"
#include "parser/javascriptast_p.h"

#include <indenter.h>
#include <utils/uncommentselection.h>

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textblockiterator.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <QtCore/QTimer>
#include <QtCore/QtDebug>

#include <QtGui/QComboBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QMenu>
#include <QtGui/QToolBar>

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 100
};

using namespace JavaScript::AST;


namespace QtScriptEditor {
namespace Internal {

class FindDeclarations: protected Visitor
{
    QList<Declaration> declarations;

public:
    QList<Declaration> accept(JavaScript::AST::Node *node)
    {
        JavaScript::AST::Node::acceptChild(node, this);
        return declarations;
    }

protected:
    using Visitor::visit;

    virtual bool visit(FunctionExpression *)
    {
        return false;
    }

    virtual bool visit(FunctionDeclaration *ast)
    {
        if (! ast->name)
            return false;

        QString text = ast->name->asString();

        text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (it->name)
                text += it->name->asString();

            if (it->next)
                text += QLatin1String(", ");
        }

        text += QLatin1Char(')');

        Declaration d;
        d.text = text;
        d.startLine = ast->startLine;
        d.startColumn = ast->startColumn;
        d.endLine = ast->endLine;
        d.endColumn = ast->endColumn;

        declarations.append(d);

        return false;
    }

    virtual bool visit(VariableDeclaration *ast)
    {
        if (! ast->name)
            return false;

        Declaration d;
        d.text = ast->name->asString();
        d.startLine= ast->startLine;
        d.startColumn = ast->startColumn;
        d.endLine = ast->endLine;
        d.endColumn = ast->endColumn;

        declarations.append(d);
        return false;
    }
};

ScriptEditorEditable::ScriptEditorEditable(ScriptEditor *editor, const QList<int>& context)
    : BaseTextEditorEditable(editor), m_context(context)
{
}

ScriptEditor::ScriptEditor(const Context &context,
                           QWidget *parent) :
    TextEditor::BaseTextEditor(parent),
    m_context(context),
    m_methodCombo(0)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setCodeFoldingVisible(true);
    setMimeType(QtScriptEditor::Constants::C_QTSCRIPTEDITOR_MIMETYPE);

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);

    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));

    baseTextDocument()->setSyntaxHighlighter(new QtScriptHighlighter);
}

ScriptEditor::~ScriptEditor()
{
}

QList<Declaration> ScriptEditor::declarations() const
{ return m_declarations; }

QStringList ScriptEditor::words() const
{ return m_words; }

Core::IEditor *ScriptEditorEditable::duplicate(QWidget *parent)
{
    ScriptEditor *newEditor = new ScriptEditor(m_context, parent);
    newEditor->duplicateFrom(editor());
    QtScriptEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editableInterface();
}

const char *ScriptEditorEditable::kind() const
{
    return QtScriptEditor::Constants::C_QTSCRIPTEDITOR;
}

ScriptEditor::Context ScriptEditorEditable::context() const
{
    return m_context;
}

void ScriptEditor::updateDocument()
{
    m_updateDocumentTimer->start(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
}

void ScriptEditor::updateDocumentNow()
{
    // ### move in the parser thread.

    m_updateDocumentTimer->stop();

    const QString fileName = file()->fileName();
    const QString code = toPlainText();

    JavaScriptParser parser;
    JavaScriptEnginePrivate driver;

    JavaScript::NodePool nodePool(fileName, &driver);
    driver.setNodePool(&nodePool);

    JavaScript::Lexer lexer(&driver);
    lexer.setCode(code, /*line = */ 1);
    driver.setLexer(&lexer);

    if (parser.parse(&driver)) {
        JavaScript::AST::Visitor v;
        driver.ast()->accept(&v);

        FindDeclarations decls;
        m_declarations = decls.accept(driver.ast());

        m_words.clear();
        foreach (const JavaScriptNameIdImpl &id, driver.literals())
            m_words.append(id.asString());

        QStringList items;
        items.append(tr("<Select Symbol>"));

        foreach (Declaration decl, m_declarations)
            items.append(decl.text);

        m_methodCombo->clear();
        m_methodCombo->addItems(items);
        updateMethodBoxIndex();
    }

    QList<QTextEdit::ExtraSelection> selections;

    QTextCharFormat errorFormat;
    errorFormat.setUnderlineColor(Qt::red);
    errorFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);

    QTextCharFormat warningFormat;
    warningFormat.setUnderlineColor(Qt::darkYellow);
    warningFormat.setUnderlineStyle(QTextCharFormat::WaveUnderline);

    QTextEdit::ExtraSelection sel;

    foreach (const JavaScriptParser::DiagnosticMessage &d, parser.diagnosticMessages()) {
        int line = d.line;
        int column = d.column;

        if (column == 0)
            column = 1;

        if (d.isWarning())
            sel.format = warningFormat;
        else
            sel.format = errorFormat;

        QTextCursor c(document()->findBlockByNumber(line - 1));
        sel.cursor = c;
        sel.cursor.setPosition(c.position() + column - 1);
        sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        selections.append(sel);
    }

    setExtraSelections(CodeWarningsSelection, selections);
}

void ScriptEditor::jumpToMethod(int index)
{
    if (index) {
        Declaration d = m_declarations.at(index - 1);
        gotoLine(d.startLine, d.startColumn - 1);
        setFocus();
    }
}

void ScriptEditor::updateMethodBoxIndex()
{
    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    int currentSymbolIndex = 0;

    int index = 0;
    while (index < m_declarations.size()) {
        const Declaration &d = m_declarations.at(index++);

        if (line < d.startLine)
            break;
        else
            currentSymbolIndex = index;
    }

    m_methodCombo->setCurrentIndex(currentSymbolIndex);
}

void ScriptEditor::updateMethodBoxToolTip()
{
}

void ScriptEditor::updateFileName()
{
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
                   << QLatin1String(TextEditor::Constants::C_COMMENT)
                   << QLatin1String(TextEditor::Constants::C_VISUAL_WHITESPACE);
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
    indenter.setTabSize(ts.m_tabSize);
    indenter.setIndentSize(ts.m_indentSize);
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

TextEditor::BaseTextEditorEditable *ScriptEditor::createEditableInterface()
{
    ScriptEditorEditable *editable = new ScriptEditorEditable(this, m_context);
    createToolBar(editable);
    return editable;
}

void ScriptEditor::createToolBar(ScriptEditorEditable *editable)
{
    m_methodCombo = new QComboBox;
    m_methodCombo->setMinimumContentsLength(22);
    //m_methodCombo->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // Make the combo box prefer to expand
    QSizePolicy policy = m_methodCombo->sizePolicy();
    policy.setHorizontalPolicy(QSizePolicy::Expanding);
    m_methodCombo->setSizePolicy(policy);

    connect(m_methodCombo, SIGNAL(activated(int)), this, SLOT(jumpToMethod(int)));
    connect(this, SIGNAL(cursorPositionChanged()), this, SLOT(updateMethodBoxIndex()));
    connect(m_methodCombo, SIGNAL(currentIndexChanged(int)), this, SLOT(updateMethodBoxToolTip()));

    connect(file(), SIGNAL(changed()), this, SLOT(updateFileName()));

    QToolBar *toolBar = static_cast<QToolBar*>(editable->toolBar());

    QList<QAction*> actions = toolBar->actions();
    QWidget *w = toolBar->widgetForAction(actions.first());
    static_cast<QHBoxLayout*>(w->layout())->insertWidget(0, m_methodCombo, 1);
}

void ScriptEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = new QMenu();

    if (Core::ActionContainer *mcontext = Core::ICore::instance()->actionManager()->actionContainer(QtScriptEditor::Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions())
            menu->addAction(action);
    }

    appendStandardContextMenuActions(menu);
    menu->exec(e->globalPos());
    delete menu;
}

void ScriptEditor::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

} // namespace Internal
} // namespace QtScriptEditor
