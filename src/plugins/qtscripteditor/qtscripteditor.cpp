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

#include <qscripthighlighter/qscriptindenter.h>
#include <qscripthighlighter/qscriptincrementalscanner.h>

#include "parser/javascriptengine_p.h"
#include "parser/javascriptparser_p.h"
#include "parser/javascriptlexer_p.h"
#include "parser/javascriptnodepool_p.h"
#include "parser/javascriptastvisitor_p.h"
#include "parser/javascriptast_p.h"

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
using namespace SharedTools;

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

QString ScriptEditorEditable::id() const
{
    return QtScriptEditor::Constants::C_QTSCRIPTEDITOR_ID;
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

void ScriptEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    TextEditor::TabSettings ts = tabSettings();
    SharedTools::QScriptIndenter indenter;
    indenter.setTabSize(ts.m_tabSize);
    indenter.setIndentSize(ts.m_indentSize);

    const int indent = indenter.indentForBottomLine(doc->begin(), block.next(), typedChar);
    ts.indentLine(block, indent);
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

bool ScriptEditor::contextAllowsAutoParentheses(const QTextCursor &cursor, const QString &textToInsert) const
{
    QChar ch;

    if (! textToInsert.isEmpty())
        ch = textToInsert.at(0);

    const QString blockText = cursor.block().text();
    const int blockState = cursor.block().userState() & 0xFF;

    QScriptIncrementalScanner tokenize;
    const QList<QScriptIncrementalScanner::Token> tokens = tokenize(blockText, blockState);
    const int pos = cursor.columnNumber();

    int tokenIndex = tokens.size() - 1;
    for (; tokenIndex != -1; --tokenIndex) {
        const QScriptIncrementalScanner::Token &token = tokens.at(tokenIndex);
        if (pos >= token.begin() && pos <= token.end())
            break;
    }

    if (tokenIndex != -1) {
        const QScriptIncrementalScanner::Token &token = tokens.at(tokenIndex);

        switch (token.kind) {
        case QScriptIncrementalScanner::Token::Comment:
            return false;

        case QScriptIncrementalScanner::Token::String: {
            if (ch == blockText.at(token.begin())) {
                if (token.end() - 1 == pos && blockText.at(token.end() - 2) != QLatin1Char('\\'))
                    break;
            }
            return false;
        }

        default:
            break;
        } // end of switch
    }

    switch (ch.unicode()) {
    case '\'':
    case '"':

    case '(':
    case '[':
    case '{':

    case ')':
    case ']':
    case '}':

    case ';':
        return true;

    default:
        if (ch.isNull())
            return true;
    } // end of switch

    return false;
}

bool ScriptEditor::isInComment(const QTextCursor &) const
{
    // ### implement me
    return false;
}

QString ScriptEditor::insertMatchingBrace(const QTextCursor &tc, const QString &text, const QChar &, int *skippedChars) const
{
    if (text.length() != 1)
        return QString();

    const QChar la = characterAt(tc.position());

    const QChar ch = text.at(0);
    switch (ch.unicode()) {
    case '\'':
        if (la != ch)
            return QString(ch);
        ++*skippedChars;
        break;

    case '"':
        if (la != ch)
            return QString(ch);
        ++*skippedChars;
        break;

    case '(':
        return QString(QLatin1Char(')'));

    case '[':
        return QString(QLatin1Char(']'));

    case '{':
        return QString(); // nothing to do.

    case ')':
    case ']':
    case '}':
    case ';':
        if (la == ch)
            ++*skippedChars;
        break;

    default:
        break;
    } // end of switch

    return QString();
}

QString ScriptEditor::insertParagraphSeparator(const QTextCursor &) const
{
    return QLatin1String("}\n");
}

} // namespace Internal
} // namespace QtScriptEditor
