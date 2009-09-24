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

#include "duieditor.h"
#include "duieditorconstants.h"
#include "duihighlighter.h"
#include "duieditorplugin.h"
#include "duidocument.h"
#include "duimodelmanager.h"

#include "qmljsastvisitor_p.h"
#include "qmljsast_p.h"
#include "qmljsengine_p.h"
#include "qmlexpressionundercursor.h"
#include "qmllookupcontext.h"
#include "qmlresolveexpression.h"
#include "rewriter_p.h"

#include "idcollector.h"

#include <coreplugin/icore.h>
#include <coreplugin/actionmanager/actionmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textblockiterator.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/uncommentselection.h>

#include <QtCore/QTimer>

#include <QtGui/QMenu>
#include <QtGui/QComboBox>
#include <QtGui/QInputDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 250
};

using namespace QmlJS;
using namespace QmlJS::AST;

namespace DuiEditor {
namespace Internal {

class FindWords: protected Visitor
{
public:
    QStringList operator()(AST::Node *node)
    {
        _words.clear();
        accept(node);
        return QStringList(_words.toList());
    }

protected:
    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;
    using Visitor::endVisit;

    void addWords(AST::UiQualifiedId *id)
    {
        for (; id; id = id->next) {
            if (id->name)
                _words.insert(id->name->asString());
        }
    }

    virtual bool visit(AST::UiPublicMember *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        return true;
    }

    virtual bool visit(AST::UiQualifiedId *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        return true;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        return true;
    }

    virtual bool visit(AST::FieldMemberExpression *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        return true;
    }

    virtual bool visit(AST::FunctionExpression *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        for (AST::FormalParameterList *it = node->formals; it; it = it->next) {
            if (it->name)
                _words.insert(it->name->asString());
        }

        return true;
    }

    virtual bool visit(AST::FunctionDeclaration *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        for (AST::FormalParameterList *it = node->formals; it; it = it->next) {
            if (it->name)
                _words.insert(it->name->asString());
        }

        return true;
    }

    virtual bool visit(AST::VariableDeclaration *node)
    {
        if (node->name)
            _words.insert(node->name->asString());

        return true;
    }

private:
    QSet<QString> _words;
};

class FindIdDeclarations: protected Visitor
{
public:
    typedef QMap<QString, QList<AST::SourceLocation> > Result;

    Result operator()(AST::Node *node)
    {
        _ids.clear();
        _maybeIds.clear();
        accept(node);
        return _ids;
    }

protected:
    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (id->name)
                text += id->name->asString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    using Visitor::visit;
    using Visitor::endVisit;

    virtual bool visit(AST::UiScriptBinding *node)
    {
        if (asString(node->qualifiedId) == QLatin1String("id")) {
            if (AST::ExpressionStatement *stmt = AST::cast<AST::ExpressionStatement*>(node->statement)) {
                if (AST::IdentifierExpression *idExpr = AST::cast<AST::IdentifierExpression *>(stmt->expression)) {
                    if (idExpr->name) {
                        const QString id = idExpr->name->asString();
                        QList<AST::SourceLocation> *locs = &_ids[id];
                        locs->append(idExpr->firstSourceLocation());
                        locs->append(_maybeIds.value(id));
                        _maybeIds.remove(id);
                        return false;
                    }
                }
            }
        }

        accept(node->statement);

        return false;
    }

    virtual bool visit(AST::IdentifierExpression *node)
    {
        if (node->name) {
            const QString name = node->name->asString();

            if (_ids.contains(name))
                _ids[name].append(node->identifierToken);
            else
                _maybeIds[name].append(node->identifierToken);
        }
        return false;
    }

private:
    Result _ids;
    Result _maybeIds;
};

class FindDeclarations: protected Visitor
{
    QList<Declaration> _declarations;
    int _depth;

public:
    QList<Declaration> operator()(AST::Node *node)
    {
        _depth = -1;
        _declarations.clear();
        accept(node);
        return _declarations;
    }

protected:
    using Visitor::visit;
    using Visitor::endVisit;

    QString asString(AST::UiQualifiedId *id)
    {
        QString text;
        for (; id; id = id->next) {
            if (id->name)
                text += id->name->asString();
            else
                text += QLatin1Char('?');

            if (id->next)
                text += QLatin1Char('.');
        }

        return text;
    }

    void accept(AST::Node *node)
    { AST::Node::acceptChild(node, this); }

    void init(Declaration *decl, AST::UiObjectMember *member)
    {
        const SourceLocation first = member->firstSourceLocation();
        const SourceLocation last = member->lastSourceLocation();
        decl->startLine = first.startLine;
        decl->startColumn = first.startColumn;
        decl->endLine = last.startLine;
        decl->endColumn = last.startColumn + last.length;
    }

    virtual bool visit(AST::UiObjectDefinition *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    virtual void endVisit(AST::UiObjectDefinition *)
    {
        --_depth;
    }

    virtual bool visit(AST::UiObjectBinding *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);

        decl.text.append(asString(node->qualifiedId));
        decl.text.append(QLatin1String(": "));

        if (node->qualifiedTypeNameId)
            decl.text.append(asString(node->qualifiedTypeNameId));
        else
            decl.text.append(QLatin1Char('?'));

        _declarations.append(decl);

        return true; // search for more bindings
    }

    virtual void endVisit(AST::UiObjectBinding *)
    {
        --_depth;
    }

#if 0 // ### ignore script bindings for now.
    virtual bool visit(AST::UiScriptBinding *node)
    {
        ++_depth;

        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text.append(asString(node->qualifiedId));

        _declarations.append(decl);

        return false; // more more bindings in this subtree.
    }

    virtual void endVisit(AST::UiScriptBinding *)
    {
        --_depth;
    }
#endif
};

ScriptEditorEditable::ScriptEditorEditable(ScriptEditor *editor, const QList<int> &context)
    : BaseTextEditorEditable(editor), m_context(context)
{
}

ScriptEditor::ScriptEditor(const Context &context, QWidget *parent) :
    TextEditor::BaseTextEditor(parent),
    m_context(context),
    m_methodCombo(0),
    m_modelManager(0)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setCodeFoldingVisible(true);
    setMimeType(DuiEditor::Constants::DUIEDITOR_MIMETYPE);

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);

    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));

    baseTextDocument()->setSyntaxHighlighter(new DuiHighlighter);

    m_modelManager = ExtensionSystem::PluginManager::instance()->getObject<DuiModelManagerInterface>();

    if (m_modelManager) {
        connect(m_modelManager, SIGNAL(documentUpdated(DuiDocument::Ptr)),
                this, SLOT(onDocumentUpdated(DuiDocument::Ptr)));
    }
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
    DuiEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editableInterface();
}

const char *ScriptEditorEditable::kind() const
{
    return DuiEditor::Constants::C_DUIEDITOR;
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

    m_modelManager->updateSourceFiles(QStringList() << fileName);
}

void ScriptEditor::onDocumentUpdated(DuiDocument::Ptr doc)
{
    if (file()->fileName() != doc->fileName())
        return;

    m_document = doc;

    FindIdDeclarations updateIds;
    m_ids = updateIds(doc->program());

    if (doc->isParsedCorrectly()) {
        FindDeclarations findDeclarations;
        m_declarations = findDeclarations(doc->program());

        FindWords findWords;
        m_words = findWords(doc->program());

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

    QTextEdit::ExtraSelection sel;

    m_diagnosticMessages = doc->diagnosticMessages();

    foreach (const DiagnosticMessage &d, m_diagnosticMessages) {
        int line = d.loc.startLine;
        int column = d.loc.startColumn;

        if (column == 0)
            column = 1;

        QTextCursor c(document()->findBlockByNumber(line - 1));
        sel.cursor = c;

        sel.cursor.setPosition(c.position() + column - 1);
        if (sel.cursor.atBlockEnd())
            sel.cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        else
            sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        sel.format = errorFormat;

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

    QList<QTextEdit::ExtraSelection> selections;
    foreach (const AST::SourceLocation &loc, m_ids.value(wordUnderCursor())) {
        if (! loc.isValid())
            continue;

        QTextEdit::ExtraSelection sel;
        sel.format.setBackground(Qt::yellow);
        sel.cursor = textCursor();
        sel.cursor.setPosition(loc.begin());
        sel.cursor.setPosition(loc.end(), QTextCursor::KeepAnchor);
        selections.append(sel);
    }

    setExtraSelections(CodeSemanticsSelection, selections);
}

void ScriptEditor::updateMethodBoxToolTip()
{
}

void ScriptEditor::updateFileName()
{
}

void ScriptEditor::renameIdUnderCursor()
{
    const QString id = wordUnderCursor();
    bool ok = false;
    const QString newId = QInputDialog::getText(Core::ICore::instance()->mainWindow(),
                                                tr("Rename..."),
                                                tr("New id:"),
                                                QLineEdit::Normal,
                                                id, &ok);
    if (ok) {
        TextWriter writer;

        QString code = toPlainText();

        foreach (const AST::SourceLocation &loc, m_ids.value(id)) {
            writer.replace(loc.offset, loc.length, newId);
        }

        QTextCursor tc = textCursor();
        writer.write(&tc);
    }
}

QStringList ScriptEditor::keywords() const
{
    QStringList words;

    if (DuiHighlighter *highlighter = qobject_cast<DuiHighlighter*>(baseTextDocument()->syntaxHighlighter()))
        words = highlighter->keywords().toList();

    return words;
}

void ScriptEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    DuiHighlighter *highlighter = qobject_cast<DuiHighlighter*>(baseTextDocument()->syntaxHighlighter());
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

QString ScriptEditor::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word= tc.selectedText();
    return word;
}

bool ScriptEditor::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('}'))
        return true;
    return false;
}

void ScriptEditor::indentBlock(QTextDocument *, QTextBlock block, QChar typedChar)
{
    TextEditor::TabSettings ts = tabSettings();

    if (typedChar == QLatin1Char('}')
        || ((typedChar == QChar::Null) && block.text().trimmed() == "}")) {

        QTextCursor tc(block);

        if (typedChar == QLatin1Char('}'))
            tc = textCursor();

        if (TextEditor::TextBlockUserData::findPreviousBlockOpenParenthesis(&tc)) {
            const QString text = tc.block().text();
            int indent = ts.columnAt(text, ts.firstNonSpace(text));
            ts.indentLine(block, indent);
            return;
        }
    }

    int indent = 0;
    int extraIndent = 0;

    if (block.previous().isValid()) {
        const int braceDepth = qMax(0, block.previous().userState() >> 8);
        const int previousBraceDepth = qMax(0, block.previous().previous().userState() >> 8);

        if (braceDepth > previousBraceDepth)
            extraIndent = ts.m_indentSize * (braceDepth - previousBraceDepth);
    }

    QTextBlock it = block.previous();
    for (; it.isValid(); it = it.previous()) {
        const QString text = it.text();

        if (! text.isEmpty()) {
            indent = ts.columnAt(text, ts.firstNonSpace(text));
            break;
        }
    }

    ts.indentLine(block, extraIndent + indent);
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
    toolBar->insertWidget(actions.first(), m_methodCombo);
}

TextEditor::BaseTextEditor::Link ScriptEditor::findLinkAt(const QTextCursor &cursor, bool /*resolveTarget*/)
{
    Link link;

    if (!m_modelManager)
        return link;

    const Snapshot snapshot = m_modelManager->snapshot();
    DuiDocument::Ptr doc = snapshot.document(file()->fileName());
    if (!doc)
        return link;

    QmlExpressionUnderCursor expressionUnderCursor;
    expressionUnderCursor(cursor, doc->program());

    QmlLookupContext context(expressionUnderCursor.expressionScopes(), doc, snapshot);
    QmlResolveExpression resolve(context);
    QmlSymbol *symbol = resolve(expressionUnderCursor.expressionNode());

    if (!symbol)
        return link;

    if (const QmlSymbolFromFile *target = symbol->asSymbolFromFile()) {
        link.pos = expressionUnderCursor.expressionOffset();
        link.length = expressionUnderCursor.expressionLength();
        link.fileName = target->fileName();
        link.line = target->line();
        link.column = target->column();
        if (link.column > 0)
            --link.column;
    }

    return link;
}

void ScriptEditor::reformat(QTextDocument *, QTextBlock)
{
    // TODO (EV)
}

void ScriptEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = createStandardContextMenu();

    if (Core::ActionContainer *mcontext = Core::ICore::instance()->actionManager()->actionContainer(DuiEditor::Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions())
            menu->addAction(action);
    }

    const QString id = wordUnderCursor();
    const QList<AST::SourceLocation> &locations = m_ids.value(id);
    if (! locations.isEmpty()) {
        menu->addSeparator();
        QAction *a = menu->addAction(tr("Rename id '%1'...").arg(id));
        connect(a, SIGNAL(triggered()), this, SLOT(renameIdUnderCursor()));
    }

    menu->exec(e->globalPos());
    menu->deleteLater();
}

void ScriptEditor::unCommentSelection()
{
    Core::Utils::unCommentSelection(this);
}

} // namespace Internal
} // namespace DuiEditor
