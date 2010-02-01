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

#include "qmljseditor.h"
#include "qmljseditorconstants.h"
#include "qmlhighlighter.h"
#include "qmljseditorplugin.h"
#include "qmlmodelmanager.h"

#include <qmljs/qmljsindenter.h>
#include <qmljs/qmljsinterpreter.h>
#include <qmljs/qmljsbind.h>
#include <qmljs/qmljscheck.h>
#include <qmljs/qmljsdocument.h>
#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/parser/qmljsast_p.h>
#include <qmljs/parser/qmljsengine_p.h>

#include <coreplugin/actionmanager/actionmanager.h>
#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/uniqueidmanager.h>
#include <extensionsystem/pluginmanager.h>
#include <texteditor/basetextdocument.h>
#include <texteditor/fontsettings.h>
#include <texteditor/textblockiterator.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditorsettings.h>

#include <utils/changeset.h>
#include <utils/uncommentselection.h>

#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QMenu>
#include <QtGui/QComboBox>
#include <QtGui/QInputDialog>
#include <QtGui/QMainWindow>
#include <QtGui/QToolBar>

enum {
    UPDATE_DOCUMENT_DEFAULT_INTERVAL = 50,
    UPDATE_USES_DEFAULT_INTERVAL = 150
};

using namespace QmlJS;
using namespace QmlJS::AST;
using namespace QmlJSEditor::Internal;

namespace {

int blockBraceDepth(const QTextBlock &block)
{
    int state = block.userState();
    if (state == -1)
        return 0;

    return (state >> 8) & 0xFF;
}

int blockStartState(const QTextBlock &block)
{
    int state = block.userState();

    if (state == -1)
        return 0;
    else
        return state & 0xff;
}

bool shouldInsertMatchingText(const QChar &lookAhead)
{
    switch (lookAhead.unicode()) {
    case '{': case '}':
    case ']': case ')':
    case ';': case ',':
    case '"': case '\'':
        return true;

    default:
        if (lookAhead.isSpace())
            return true;

        return false;
    } // switch
}

bool shouldInsertMatchingText(const QTextCursor &tc)
{
    QTextDocument *doc = tc.document();
    return shouldInsertMatchingText(doc->characterAt(tc.selectionEnd()));
}

class FindIdDeclarations: protected Visitor
{
public:
    typedef QHash<QString, QList<AST::SourceLocation> > Result;

    Result operator()(Document::Ptr doc)
    {
        _ids.clear();
        _maybeIds.clear();
        if (doc && doc->qmlProgram())
            doc->qmlProgram()->accept(this);
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

    void init(Declaration *decl, AST::ExpressionNode *expressionNode)
    {
        const SourceLocation first = expressionNode->firstSourceLocation();
        const SourceLocation last = expressionNode->lastSourceLocation();
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

    virtual bool visit(AST::UiScriptBinding *)
    {
        ++_depth;

#if 0 // ### ignore script bindings for now.
        Declaration decl;
        init(&decl, node);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text.append(asString(node->qualifiedId));

        _declarations.append(decl);
#endif

        return false; // more more bindings in this subtree.
    }

    virtual void endVisit(AST::UiScriptBinding *)
    {
        --_depth;
    }

    virtual bool visit(AST::FunctionExpression *)
    {
        return false;
    }

    virtual bool visit(AST::FunctionDeclaration *ast)
    {
        if (! ast->name)
            return false;

        Declaration decl;
        init(&decl, ast);

        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name->asString();

        decl.text += QLatin1Char('(');
        for (FormalParameterList *it = ast->formals; it; it = it->next) {
            if (it->name)
                decl.text += it->name->asString();

            if (it->next)
                decl.text += QLatin1String(", ");
        }

        decl.text += QLatin1Char(')');

        _declarations.append(decl);

        return false;
    }

    virtual bool visit(AST::VariableDeclaration *ast)
    {
        if (! ast->name)
            return false;

        Declaration decl;
        decl.text.fill(QLatin1Char(' '), _depth);
        decl.text += ast->name->asString();

        const SourceLocation first = ast->identifierToken;
        decl.startLine = first.startLine;
        decl.startColumn = first.startColumn;
        decl.endLine = first.startLine;
        decl.endColumn = first.startColumn + first.length;

        _declarations.append(decl);

        return false;
    }
};

class CreateRanges: protected AST::Visitor
{
    QTextDocument *_textDocument;
    QList<Range> _ranges;

public:
    QList<Range> operator()(QTextDocument *textDocument, Document::Ptr doc)
    {
        _textDocument = textDocument;
        _ranges.clear();
        if (doc && doc->qmlProgram() != 0)
            doc->qmlProgram()->accept(this);
        return _ranges;
    }

protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiObjectBinding *ast)
    {
        if (ast->initializer)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    virtual bool visit(AST::UiObjectDefinition *ast)
    {
        if (ast->initializer)
            _ranges.append(createRange(ast, ast->initializer));
        return true;
    }

    Range createRange(AST::UiObjectMember *member, AST::UiObjectInitializer *ast)
    {
        Range range;

        range.ast = member;

        range.begin = QTextCursor(_textDocument);
        range.begin.setPosition(ast->lbraceToken.begin());

        range.end = QTextCursor(_textDocument);
        range.end.setPosition(ast->rbraceToken.end());
        return range;
    }
};


class CollectASTNodes: protected AST::Visitor
{
public:
    QList<AST::UiQualifiedId *> qualifiedIds;
    QList<AST::IdentifierExpression *> identifiers;
    QList<AST::FieldMemberExpression *> fieldMembers;

    void accept(AST::Node *node)
    {
        if (node)
            node->accept(this);
    }

protected:
    using AST::Visitor::visit;

    virtual bool visit(AST::UiQualifiedId *ast)
    {
        qualifiedIds.append(ast);
        return false;
    }

    virtual bool visit(AST::IdentifierExpression *ast)
    {
        identifiers.append(ast);
        return false;
    }

    virtual bool visit(AST::FieldMemberExpression *ast)
    {
        fieldMembers.append(ast);
        return true;
    }
};

} // end of anonymous namespace


AST::UiObjectMember *SemanticInfo::declaringMember(int cursorPosition) const
{
    AST::UiObjectMember *declaringMember = 0;

    for (int i = ranges.size() - 1; i != -1; --i) {
        const Range &range = ranges.at(i);

        if (range.begin.isNull() || range.end.isNull()) {
            continue;
        } else if (cursorPosition >= range.begin.position() && cursorPosition <= range.end.position()) {
            declaringMember = range.ast;
            break;
        }
    }

    return declaringMember;
}

int SemanticInfo::revision() const
{
    if (document)
        return document->documentRevision();

    return 0;
}

QmlJSEditorEditable::QmlJSEditorEditable(QmlJSTextEditor *editor)
    : BaseTextEditorEditable(editor)
{

    Core::UniqueIDManager *uidm = Core::UniqueIDManager::instance();
    m_context << uidm->uniqueIdentifier(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);
    m_context << uidm->uniqueIdentifier(TextEditor::Constants::C_TEXTEDITOR);
}

QmlJSTextEditor::QmlJSTextEditor(QWidget *parent) :
    TextEditor::BaseTextEditor(parent),
    m_methodCombo(0),
    m_modelManager(0)
{
    setParenthesesMatchingEnabled(true);
    setMarksVisible(true);
    setCodeFoldingSupported(true);
    setCodeFoldingVisible(true);

    m_updateDocumentTimer = new QTimer(this);
    m_updateDocumentTimer->setInterval(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
    m_updateDocumentTimer->setSingleShot(true);
    connect(m_updateDocumentTimer, SIGNAL(timeout()), this, SLOT(updateDocumentNow()));

    m_updateUsesTimer = new QTimer(this);
    m_updateUsesTimer->setInterval(UPDATE_USES_DEFAULT_INTERVAL);
    m_updateUsesTimer->setSingleShot(true);
    connect(m_updateUsesTimer, SIGNAL(timeout()), this, SLOT(updateUsesNow()));

    connect(this, SIGNAL(textChanged()), this, SLOT(updateDocument()));
    connect(this, SIGNAL(textChanged()), this, SLOT(updateUses()));

    baseTextDocument()->setSyntaxHighlighter(new QmlHighlighter);

    m_modelManager = ExtensionSystem::PluginManager::instance()->getObject<QmlModelManagerInterface>();

    if (m_modelManager) {
        connect(m_modelManager, SIGNAL(documentUpdated(QmlJS::Document::Ptr)),
                this, SLOT(onDocumentUpdated(QmlJS::Document::Ptr)));
    }
}

QmlJSTextEditor::~QmlJSTextEditor()
{
}

Core::IEditor *QmlJSEditorEditable::duplicate(QWidget *parent)
{
    QmlJSTextEditor *newEditor = new QmlJSTextEditor(parent);
    newEditor->duplicateFrom(editor());
    QmlJSEditorPlugin::instance()->initializeEditor(newEditor);
    return newEditor->editableInterface();
}

QString QmlJSEditorEditable::id() const
{
    return QLatin1String(QmlJSEditor::Constants::C_QMLJSEDITOR_ID);
}

bool QmlJSEditorEditable::open(const QString &fileName)
{
    bool b = TextEditor::BaseTextEditorEditable::open(fileName);
    editor()->setMimeType(Core::ICore::instance()->mimeDatabase()->findByFile(QFileInfo(fileName)).type());
    return b;
}

QmlJSTextEditor::Context QmlJSEditorEditable::context() const
{
    return m_context;
}

void QmlJSTextEditor::updateDocument()
{
    m_updateDocumentTimer->start(UPDATE_DOCUMENT_DEFAULT_INTERVAL);
}

void QmlJSTextEditor::updateDocumentNow()
{
    // ### move in the parser thread.

    m_updateDocumentTimer->stop();

    const QString fileName = file()->fileName();

    m_modelManager->updateSourceFiles(QStringList() << fileName);
}

void QmlJSTextEditor::onDocumentUpdated(QmlJS::Document::Ptr doc)
{
    if (file()->fileName() != doc->fileName())
        return;

    if (doc->documentRevision() != document()->revision()) {
        // got an outdated document.
        return;
    }

    if (doc->ast()) {
        // got a correctly parsed (or recovered) file.

        // create the ranges and update the semantic info.
        CreateRanges createRanges;
        SemanticInfo sem;
        sem.snapshot = m_modelManager->snapshot();
        sem.document = doc;
        sem.ranges = createRanges(document(), doc);

        // Refresh the ids
        FindIdDeclarations updateIds;
        sem.idLocations = updateIds(doc);

        if (doc->isParsedCorrectly()) {
            FindDeclarations findDeclarations;
            sem.declarations = findDeclarations(doc->ast());

            QStringList items;
            items.append(tr("<Select Symbol>"));

            foreach (Declaration decl, sem.declarations)
                items.append(decl.text);

            m_methodCombo->clear();
            m_methodCombo->addItems(items);
            updateMethodBoxIndex();
        }

        m_semanticInfo = sem;
    }

    QList<QTextEdit::ExtraSelection> selections;

    foreach (const DiagnosticMessage &d, doc->diagnosticMessages()) {
        if (d.isWarning())
            continue;

        const int line = d.loc.startLine;
        const int column = qMax(1U, d.loc.startColumn);

        QTextEdit::ExtraSelection sel;
        QTextCursor c(document()->findBlockByNumber(line - 1));
        sel.cursor = c;

        sel.cursor.setPosition(c.position() + column - 1);
        if (sel.cursor.atBlockEnd())
            sel.cursor.movePosition(QTextCursor::StartOfWord, QTextCursor::KeepAnchor);
        else
            sel.cursor.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);

        sel.format.setUnderlineColor(Qt::red);
        sel.format.setUnderlineStyle(QTextCharFormat::WaveUnderline);
        sel.format.setToolTip(d.message);

        selections.append(sel);
    }

    setExtraSelections(CodeWarningsSelection, selections);
}

void QmlJSTextEditor::jumpToMethod(int index)
{
    if (index > 0 && index <= m_semanticInfo.declarations.size()) { // indexes are 1-based
        Declaration d = m_semanticInfo.declarations.at(index - 1);
        gotoLine(d.startLine, d.startColumn - 1);
        setFocus();
    }
}

void QmlJSTextEditor::updateMethodBoxIndex()
{
    int line = 0, column = 0;
    convertPosition(position(), &line, &column);

    int currentSymbolIndex = 0;

    int index = 0;
    while (index < m_semanticInfo.declarations.size()) {
        const Declaration &d = m_semanticInfo.declarations.at(index++);

        if (line < d.startLine)
            break;
        else
            currentSymbolIndex = index;
    }

    m_methodCombo->setCurrentIndex(currentSymbolIndex);
    updateUses();
}

void QmlJSTextEditor::updateUses()
{
    m_updateUsesTimer->start();
}

void QmlJSTextEditor::updateUsesNow()
{
    if (document()->revision() != m_semanticInfo.revision()) {
        updateUses();
        return;
    }

    m_updateUsesTimer->stop();

    QList<QTextEdit::ExtraSelection> selections;
    foreach (const AST::SourceLocation &loc, m_semanticInfo.idLocations.value(wordUnderCursor())) {
        if (! loc.isValid())
            continue;

        QTextEdit::ExtraSelection sel;
        sel.format = m_occurrencesFormat;
        sel.cursor = textCursor();
        sel.cursor.setPosition(loc.begin());
        sel.cursor.setPosition(loc.end(), QTextCursor::KeepAnchor);
        selections.append(sel);
    }

    setExtraSelections(CodeSemanticsSelection, selections);
}

void QmlJSTextEditor::updateMethodBoxToolTip()
{
}

void QmlJSTextEditor::updateFileName()
{
}

void QmlJSTextEditor::renameIdUnderCursor()
{
    const QString id = wordUnderCursor();
    bool ok = false;
    const QString newId = QInputDialog::getText(Core::ICore::instance()->mainWindow(),
                                                tr("Rename..."),
                                                tr("New id:"),
                                                QLineEdit::Normal,
                                                id, &ok);
    if (ok) {
        Utils::ChangeSet changeSet;

        foreach (const AST::SourceLocation &loc, m_semanticInfo.idLocations.value(id)) {
            changeSet.replace(loc.offset, loc.length, newId);
        }

        QTextCursor tc = textCursor();
        changeSet.apply(&tc);
    }
}

void QmlJSTextEditor::setFontSettings(const TextEditor::FontSettings &fs)
{
    TextEditor::BaseTextEditor::setFontSettings(fs);
    QmlHighlighter *highlighter = qobject_cast<QmlHighlighter*>(baseTextDocument()->syntaxHighlighter());
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

    m_occurrencesFormat = fs.toTextCharFormat(QLatin1String(TextEditor::Constants::C_OCCURRENCES));
    m_occurrencesFormat.clearForeground();

    highlighter->setFormats(fs.toTextCharFormats(categories));
    highlighter->rehighlight();
}

QString QmlJSTextEditor::wordUnderCursor() const
{
    QTextCursor tc = textCursor();
    tc.movePosition(QTextCursor::StartOfWord);
    tc.movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    const QString word= tc.selectedText();
    return word;
}

bool QmlJSTextEditor::isElectricCharacter(const QChar &ch) const
{
    if (ch == QLatin1Char('}')
        || ch == QLatin1Char(']'))
        return true;
    return false;
}

bool QmlJSTextEditor::isClosingBrace(const QList<Token> &tokens) const
{

    if (tokens.size() == 1) {
        const Token firstToken = tokens.first();

        return firstToken.is(Token::RightBrace) || firstToken.is(Token::RightBracket);
    }

    return false;
}

void QmlJSTextEditor::indentBlock(QTextDocument *doc, QTextBlock block, QChar typedChar)
{
    TextEditor::TabSettings ts = tabSettings();
    QmlJSIndenter indenter;
    indenter.setTabSize(ts.m_tabSize);
    indenter.setIndentSize(ts.m_indentSize);

    const int indent = indenter.indentForBottomLine(doc->begin(), block.next(), typedChar);
    ts.indentLine(block, indent);
}

TextEditor::BaseTextEditorEditable *QmlJSTextEditor::createEditableInterface()
{
    QmlJSEditorEditable *editable = new QmlJSEditorEditable(this);
    createToolBar(editable);
    return editable;
}

void QmlJSTextEditor::createToolBar(QmlJSEditorEditable *editable)
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

TextEditor::BaseTextEditor::Link QmlJSTextEditor::findLinkAt(const QTextCursor &cursor, bool /*resolveTarget*/)
{
    Link link;
    return link;
}

void QmlJSTextEditor::contextMenuEvent(QContextMenuEvent *e)
{
    QMenu *menu = new QMenu();

    if (Core::ActionContainer *mcontext = Core::ICore::instance()->actionManager()->actionContainer(QmlJSEditor::Constants::M_CONTEXT)) {
        QMenu *contextMenu = mcontext->menu();
        foreach (QAction *action, contextMenu->actions())
            menu->addAction(action);
    }

    const QString id = wordUnderCursor();
    const QList<AST::SourceLocation> &locations = m_semanticInfo.idLocations.value(id);
    if (! locations.isEmpty()) {
        menu->addSeparator();
        QAction *a = menu->addAction(tr("Rename id '%1'...").arg(id));
        connect(a, SIGNAL(triggered()), this, SLOT(renameIdUnderCursor()));
    }

    appendStandardContextMenuActions(menu);

    menu->exec(e->globalPos());
    menu->deleteLater();
}

void QmlJSTextEditor::unCommentSelection()
{
    Utils::unCommentSelection(this);
}

static bool isCompleteStringLiteral(const QStringRef &text)
{
    if (text.length() < 2)
        return false;

    const QChar quote = text.at(0);

    if (text.at(text.length() - 1) == quote)
        return text.at(text.length() - 2) != QLatin1Char('\\'); // ### not exactly.

    return false;
}

bool QmlJSTextEditor::contextAllowsAutoParentheses(const QTextCursor &cursor, const QString &textToInsert) const
{
    QChar ch;

    if (! textToInsert.isEmpty())
        ch = textToInsert.at(0);

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
        break;

    default:
        if (ch.isNull())
            break;

        return false;
    } // end of switch

    const QString blockText = cursor.block().text();
    const int blockState = blockStartState(cursor.block());

    QmlJSScanner tokenize;
    const QList<Token> tokens = tokenize(blockText, blockState);
    const int pos = cursor.columnNumber();

    int tokenIndex = 0;
    for (; tokenIndex < tokens.size(); ++tokenIndex) {
        const Token &token = tokens.at(tokenIndex);

        if (pos >= token.begin()) {
            if (pos < token.end())
                break;

            else if (pos == token.end() && (token.is(Token::Comment) ||
                                            token.is(Token::String)))
                break;
        }
    }

    if (tokenIndex != tokens.size()) {
        const Token &token = tokens.at(tokenIndex);

        switch (token.kind) {
        case Token::Comment:
            return false;

        case Token::String: {
            const QStringRef tokenText = blockText.midRef(token.offset, token.length);
            const QChar quote = tokenText.at(0);

            if (ch == quote && isCompleteStringLiteral(tokenText))
                break;

            return false;
        }

        default:
            break;
        } // end of switch
    }

    return true;
}

bool QmlJSTextEditor::isInComment(const QTextCursor &) const
{
    // ### implement me
    return false;
}

QString QmlJSTextEditor::insertMatchingBrace(const QTextCursor &tc, const QString &text, const QChar &, int *skippedChars) const
{
    if (text.length() != 1)
        return QString();

    if (! shouldInsertMatchingText(tc))
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

QString QmlJSTextEditor::insertParagraphSeparator(const QTextCursor &) const
{
    return QLatin1String("}\n");
}

