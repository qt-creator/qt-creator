/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** No Commercial Usage
**
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
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
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "cppcodecompletion.h"
#include "cppmodelmanager.h"
#include "cppdoxygen.h"
#include "cpptoolsconstants.h"
#include "cpptoolseditorsupport.h"

#include <Control.h>
#include <AST.h>
#include <ASTVisitor.h>
#include <CoreTypes.h>
#include <Literals.h>
#include <Names.h>
#include <NameVisitor.h>
#include <Symbols.h>
#include <SymbolVisitor.h>
#include <Scope.h>
#include <TranslationUnit.h>

#include <cplusplus/ResolveExpression.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/Overview.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/LookupContext.h>

#include <cppeditor/cppeditorconstants.h>

#include <coreplugin/icore.h>
#include <coreplugin/mimedatabase.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/completionsettings.h>
#include <texteditor/basetexteditor.h>
#include <texteditor/snippets/snippet.h>
#include <projectexplorer/projectexplorer.h>

#include <utils/faketooltip.h>
#include <utils/qtcassert.h>

#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QStyle>
#include <QtGui/QTextDocument> // Qt::escape()
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>
#include <QtAlgorithms>

namespace {
    const bool debug = ! qgetenv("CPLUSPLUS_DEBUG").isEmpty();
}

using namespace CPlusPlus;

namespace CppTools {
namespace Internal {

class FunctionArgumentWidget : public QLabel
{
    Q_OBJECT

public:
    FunctionArgumentWidget();
    void showFunctionHint(QList<Function *> functionSymbols,
                          const LookupContext &context,
                          int startPosition);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private slots:
    void nextPage();
    void previousPage();

private:
    void updateArgumentHighlight();
    void updateHintText();
    void placeInsideScreen();

    Function *currentFunction() const
    { return m_items.at(m_current); }

    int m_startpos;
    int m_currentarg;
    int m_current;
    bool m_escapePressed;

    TextEditor::ITextEditor *m_editor;

    QWidget *m_pager;
    QLabel *m_numberLabel;
    Utils::FakeToolTip *m_popupFrame;
    QList<Function *> m_items;
    LookupContext m_context;
};

class ConvertToCompletionItem: protected NameVisitor
{
    // The completion collector.
    CppCodeCompletion *_collector;

    // The completion item.
    TextEditor::CompletionItem _item;

    // The current symbol.
    Symbol *_symbol;

    // The pretty printer.
    Overview overview;

public:
    ConvertToCompletionItem(CppCodeCompletion *collector)
        : _collector(collector),
          _item(0),
          _symbol(0)
    { }

    TextEditor::CompletionItem operator()(Symbol *symbol)
    {
        if (! symbol || ! symbol->name() || symbol->name()->isQualifiedNameId())
            return 0;

        TextEditor::CompletionItem previousItem = switchCompletionItem(0);
        Symbol *previousSymbol = switchSymbol(symbol);
        accept(symbol->unqualifiedName());
        if (_item.isValid())
            _item.data = QVariant::fromValue(symbol);
        (void) switchSymbol(previousSymbol);
        return switchCompletionItem(previousItem);
    }

protected:
    Symbol *switchSymbol(Symbol *symbol)
    {
        Symbol *previousSymbol = _symbol;
        _symbol = symbol;
        return previousSymbol;
    }

    TextEditor::CompletionItem switchCompletionItem(TextEditor::CompletionItem item)
    {
        TextEditor::CompletionItem previousItem = _item;
        _item = item;
        return previousItem;
    }

    TextEditor::CompletionItem newCompletionItem(const Name *name)
    {
        TextEditor::CompletionItem item(_collector);
        item.text = overview.prettyName(name);
        item.icon = _collector->iconForSymbol(_symbol);
        return item;
    }

    virtual void visit(const Identifier *name)
    { _item = newCompletionItem(name); }

    virtual void visit(const TemplateNameId *name)
    {
        _item = newCompletionItem(name);
        _item.text = QLatin1String(name->identifier()->chars());
    }

    virtual void visit(const DestructorNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(const OperatorNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(const ConversionNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(const QualifiedNameId *name)
    { _item = newCompletionItem(name->name()); }
};

struct CompleteFunctionDeclaration
{
    explicit CompleteFunctionDeclaration(Function *f = 0)
        : function(f)
    {}

    Function *function;
};

} // namespace Internal
} // namespace CppTools

using namespace CppTools::Internal;

Q_DECLARE_METATYPE(CompleteFunctionDeclaration)


FunctionArgumentWidget::FunctionArgumentWidget():
    m_startpos(-1),
    m_current(0),
    m_escapePressed(false)
{
    QObject *editorObject = Core::EditorManager::instance()->currentEditor();
    m_editor = qobject_cast<TextEditor::ITextEditor *>(editorObject);

    m_popupFrame = new Utils::FakeToolTip(m_editor->widget());

    QToolButton *downArrow = new QToolButton;
    downArrow->setArrowType(Qt::DownArrow);
    downArrow->setFixedSize(16, 16);
    downArrow->setAutoRaise(true);

    QToolButton *upArrow = new QToolButton;
    upArrow->setArrowType(Qt::UpArrow);
    upArrow->setFixedSize(16, 16);
    upArrow->setAutoRaise(true);

    setParent(m_popupFrame);
    setFocusPolicy(Qt::NoFocus);

    m_pager = new QWidget;
    QHBoxLayout *hbox = new QHBoxLayout(m_pager);
    hbox->setMargin(0);
    hbox->setSpacing(0);
    hbox->addWidget(upArrow);
    m_numberLabel = new QLabel;
    hbox->addWidget(m_numberLabel);
    hbox->addWidget(downArrow);

    QHBoxLayout *layout = new QHBoxLayout;
    layout->setMargin(0);
    layout->setSpacing(0);
    layout->addWidget(m_pager);
    layout->addWidget(this);
    m_popupFrame->setLayout(layout);

    connect(upArrow, SIGNAL(clicked()), SLOT(previousPage()));
    connect(downArrow, SIGNAL(clicked()), SLOT(nextPage()));

    setTextFormat(Qt::RichText);

    qApp->installEventFilter(this);
}

void FunctionArgumentWidget::showFunctionHint(QList<Function *> functionSymbols,
                                              const LookupContext &context,
                                              int startPosition)
{
    Q_ASSERT(!functionSymbols.isEmpty());

    if (m_startpos == startPosition)
        return;

    m_pager->setVisible(functionSymbols.size() > 1);

    m_items = functionSymbols;
    m_context = context;
    m_startpos = startPosition;
    m_current = 0;
    m_escapePressed = false;

    // update the text
    m_currentarg = -1;
    updateArgumentHighlight();

    m_popupFrame->show();
}

void FunctionArgumentWidget::nextPage()
{
    m_current = (m_current + 1) % m_items.size();
    updateHintText();
}

void FunctionArgumentWidget::previousPage()
{
    if (m_current == 0)
        m_current = m_items.size() - 1;
    else
        --m_current;

    updateHintText();
}

void FunctionArgumentWidget::updateArgumentHighlight()
{
    int curpos = m_editor->position();
    if (curpos < m_startpos) {
        m_popupFrame->close();
        return;
    }

    QString str = m_editor->textAt(m_startpos, curpos - m_startpos);
    int argnr = 0;
    int parcount = 0;
    SimpleLexer tokenize;
    QList<Token> tokens = tokenize(str);
    for (int i = 0; i < tokens.count(); ++i) {
        const Token &tk = tokens.at(i);
        if (tk.is(T_LPAREN))
            ++parcount;
        else if (tk.is(T_RPAREN))
            --parcount;
        else if (! parcount && tk.is(T_COMMA))
            ++argnr;
    }

    if (m_currentarg != argnr) {
        m_currentarg = argnr;
        updateHintText();
    }

    if (parcount < 0)
        m_popupFrame->close();
}

bool FunctionArgumentWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::ShortcutOverride:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        break;
    case QEvent::KeyPress:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
            m_escapePressed = true;
        }
        if (m_items.size() > 1) {
            QKeyEvent *ke = static_cast<QKeyEvent*>(e);
            if (ke->key() == Qt::Key_Up) {
                previousPage();
                return true;
            } else if (ke->key() == Qt::Key_Down) {
                nextPage();
                return true;
            }
            return false;
        }
        break;
    case QEvent::KeyRelease:
        if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape && m_escapePressed) {
            m_popupFrame->close();
            return false;
        }
        updateArgumentHighlight();
        break;
    case QEvent::WindowDeactivate:
    case QEvent::FocusOut:
        if (obj != m_editor->widget())
            break;
        m_popupFrame->close();
        break;
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel: {
            QWidget *widget = qobject_cast<QWidget *>(obj);
            if (! (widget == this || m_popupFrame->isAncestorOf(widget))) {
                m_popupFrame->close();
            }
        }
        break;
    default:
        break;
    }
    return false;
}

void FunctionArgumentWidget::updateHintText()
{
    Overview overview;
    overview.setShowReturnTypes(true);
    overview.setShowArgumentNames(true);
    overview.setMarkedArgument(m_currentarg + 1);
    Function *f = currentFunction();

    const QString prettyMethod = overview(f->type(), f->name());
    const int begin = overview.markedArgumentBegin();
    const int end = overview.markedArgumentEnd();

    QString hintText;
    hintText += Qt::escape(prettyMethod.left(begin));
    hintText += "<b>";
    hintText += Qt::escape(prettyMethod.mid(begin, end - begin));
    hintText += "</b>";
    hintText += Qt::escape(prettyMethod.mid(end));
    setText(hintText);

    m_numberLabel->setText(tr("%1 of %2").arg(m_current + 1).arg(m_items.size()));

    placeInsideScreen();
}

void FunctionArgumentWidget::placeInsideScreen()
{
    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_editor->widget()));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_editor->widget()));
#endif

    m_pager->setFixedWidth(m_pager->minimumSizeHint().width());

    setWordWrap(false);
    const int maxDesiredWidth = screen.width() - 10;
    const QSize minHint = m_popupFrame->minimumSizeHint();
    if (minHint.width() > maxDesiredWidth) {
        setWordWrap(true);
        m_popupFrame->setFixedWidth(maxDesiredWidth);
        const int extra =
            m_popupFrame->contentsMargins().bottom() + m_popupFrame->contentsMargins().top();
        m_popupFrame->setFixedHeight(heightForWidth(maxDesiredWidth - m_pager->width()) + extra);
    } else {
        m_popupFrame->setFixedSize(minHint);
    }

    const QSize sz = m_popupFrame->size();
    QPoint pos = m_editor->cursorRect(m_startpos).topLeft();
    pos.setY(pos.y() - sz.height() - 1);

    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());

    m_popupFrame->move(pos);
}

CppCodeCompletion::CppCodeCompletion(CppModelManager *manager)
    : ICompletionCollector(manager),
      m_manager(manager),
      m_editor(0),
      m_startPosition(-1),
      m_shouldRestartCompletion(false),
      m_automaticCompletion(false),
      m_completionOperator(T_EOF_SYMBOL),
      m_objcEnabled(true),
      m_snippetProvider(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID,
                        QIcon(QLatin1String(":/texteditor/images/snippet.png")))
{
}

QIcon CppCodeCompletion::iconForSymbol(Symbol *symbol) const
{
    return m_icons.iconForSymbol(symbol);
}

/*
  Searches backwards for an access operator.
*/
static int startOfOperator(TextEditor::ITextEditor *editor,
                           int pos, unsigned *kind,
                           bool wantFunctionCall)
{
    const QChar ch  = pos > -1 ? editor->characterAt(pos - 1) : QChar();
    const QChar ch2 = pos >  0 ? editor->characterAt(pos - 2) : QChar();
    const QChar ch3 = pos >  1 ? editor->characterAt(pos - 3) : QChar();

    int start = pos;
    int completionKind = T_EOF_SYMBOL;

    switch (ch.toLatin1()) {
    case '.':
        if (ch2 != QLatin1Char('.')) {
            completionKind = T_DOT;
            --start;
        }
        break;
    case ',':
        completionKind = T_COMMA;
        --start;
        break;
    case '(':
        if (wantFunctionCall) {
            completionKind = T_LPAREN;
            --start;
        }
        break;
    case ':':
        if (ch3 != QLatin1Char(':') && ch2 == QLatin1Char(':')) {
            completionKind = T_COLON_COLON;
            start -= 2;
        }
        break;
    case '>':
        if (ch2 == QLatin1Char('-')) {
            completionKind = T_ARROW;
            start -= 2;
        }
        break;
    case '*':
        if (ch2 == QLatin1Char('.')) {
            completionKind = T_DOT_STAR;
            start -= 2;
        } else if (ch3 == QLatin1Char('-') && ch2 == QLatin1Char('>')) {
            completionKind = T_ARROW_STAR;
            start -= 3;
        }
        break;
    case '\\':
    case '@':
        if (ch2.isNull() || ch2.isSpace()) {
            completionKind = T_DOXY_COMMENT;
            --start;
        }
        break;
    case '<':
        completionKind = T_ANGLE_STRING_LITERAL;
        --start;
        break;
    case '"':
        completionKind = T_STRING_LITERAL;
        --start;
        break;
    case '/':
        completionKind = T_SLASH;
        --start;
        break;
    case '#':
        completionKind = T_POUND;
        --start;
        break;
    }

    if (start == pos)
        return start;

    TextEditor::BaseTextEditorWidget *edit = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
    QTextCursor tc(edit->textCursor());
    tc.setPosition(pos);

    // Include completion: make sure the quote character is the first one on the line
    if (completionKind == T_STRING_LITERAL) {
        QTextCursor s = tc;
        s.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString sel = s.selectedText();
        if (sel.indexOf(QLatin1Char('"')) < sel.length() - 1) {
            completionKind = T_EOF_SYMBOL;
            start = pos;
        }
    }

    if (completionKind == T_COMMA) {
        ExpressionUnderCursor expressionUnderCursor;
        if (expressionUnderCursor.startOfFunctionCall(tc) == -1) {
            completionKind = T_EOF_SYMBOL;
            start = pos;
        }
    }

    SimpleLexer tokenize;
    tokenize.setQtMocRunEnabled(true);
    tokenize.setObjCEnabled(true);
    tokenize.setSkipComments(false);
    const QList<Token> &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
    const int tokenIdx = SimpleLexer::tokenBefore(tokens, qMax(0, tc.positionInBlock() - 1)); // get the token at the left of the cursor
    const Token tk = (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);

    if (completionKind == T_DOXY_COMMENT && !(tk.is(T_DOXY_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))) {
        completionKind = T_EOF_SYMBOL;
        start = pos;
    }
    // Don't complete in comments or strings, but still check for include completion
    else if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT) ||
             (tk.isLiteral() && (completionKind != T_STRING_LITERAL
                                 && completionKind != T_ANGLE_STRING_LITERAL
                                 && completionKind != T_SLASH))) {
        completionKind = T_EOF_SYMBOL;
        start = pos;
    }
    // Include completion: can be triggered by slash, but only in a string
    else if (completionKind == T_SLASH && (tk.isNot(T_STRING_LITERAL) && tk.isNot(T_ANGLE_STRING_LITERAL))) {
        completionKind = T_EOF_SYMBOL;
        start = pos;
    }
    else if (completionKind == T_LPAREN) {
        if (tokenIdx > 0) {
            const Token &previousToken = tokens.at(tokenIdx - 1); // look at the token at the left of T_LPAREN
            switch (previousToken.kind()) {
            case T_IDENTIFIER:
            case T_GREATER:
            case T_SIGNAL:
            case T_SLOT:
                break; // good

            default:
                // that's a bad token :)
                completionKind = T_EOF_SYMBOL;
                start = pos;
            }
        }
    }
    // Check for include preprocessor directive
    else if (completionKind == T_STRING_LITERAL || completionKind == T_ANGLE_STRING_LITERAL || completionKind == T_SLASH) {
        bool include = false;
        if (tokens.size() >= 3) {
            if (tokens.at(0).is(T_POUND) && tokens.at(1).is(T_IDENTIFIER) && (tokens.at(2).is(T_STRING_LITERAL) ||
                                                                              tokens.at(2).is(T_ANGLE_STRING_LITERAL))) {
                const Token &directiveToken = tokens.at(1);
                QString directive = tc.block().text().mid(directiveToken.begin(),
                                                          directiveToken.length());
                if (directive == QLatin1String("include") ||
                    directive == QLatin1String("include_next") ||
                    directive == QLatin1String("import")) {
                    include = true;
                }
            }
        }

        if (!include) {
            completionKind = T_EOF_SYMBOL;
            start = pos;
        }
    }

    if (kind)
        *kind = completionKind;

    return start;
}

bool CppCodeCompletion::supportsPolicy(TextEditor::CompletionPolicy policy) const
{
    return policy == TextEditor::SemanticCompletion;
}

bool CppCodeCompletion::supportsEditor(TextEditor::ITextEditor *editor) const
{
    return m_manager->isCppEditor(editor);
}

TextEditor::ITextEditor *CppCodeCompletion::editor() const
{ return m_editor; }

int CppCodeCompletion::startPosition() const
{ return m_startPosition; }

bool CppCodeCompletion::shouldRestartCompletion()
{ return m_shouldRestartCompletion; }

bool CppCodeCompletion::triggersCompletion(TextEditor::ITextEditor *editor)
{
    m_editor = editor;
    m_automaticCompletion = false;

    const int pos = editor->position();
    unsigned token = T_EOF_SYMBOL;

    if (startOfOperator(editor, pos, &token, /*want function call=*/ true) != pos) {
        if (token == T_POUND) {
            int line, column;
            editor->convertPosition(pos, &line, &column);
            if (column != 1)
                return false;
        }

        return true;
    } else if (completionSettings().m_completionTrigger == TextEditor::AutomaticCompletion) {
        // Trigger completion after three characters of a name have been typed, when not editing an existing name
        QChar characterUnderCursor = editor->characterAt(pos);
        if (!characterUnderCursor.isLetterOrNumber()) {
            const int startOfName = findStartOfName(pos);
            if (pos - startOfName >= 3) {
                const QChar firstCharacter = editor->characterAt(startOfName);
                if (firstCharacter.isLetter() || firstCharacter == QLatin1Char('_')) {
                    // Finally check that we're not inside a comment or string (code copied from startOfOperator)
                    TextEditor::BaseTextEditorWidget *edit = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
                    QTextCursor tc(edit->textCursor());
                    tc.setPosition(pos);

                    SimpleLexer tokenize;
                    tokenize.setQtMocRunEnabled(true);
                    tokenize.setObjCEnabled(true);
                    tokenize.setSkipComments(false);
                    const QList<Token> &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
                    const int tokenIdx = SimpleLexer::tokenBefore(tokens, qMax(0, tc.positionInBlock() - 1));
                    const Token tk = (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);

                    if (!tk.isComment() && !tk.isLiteral()) {
                        m_automaticCompletion = true;
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

int CppCodeCompletion::startCompletion(TextEditor::ITextEditor *editor)
{
    int index = startCompletionHelper(editor);
    if (index != -1) {
        if (m_automaticCompletion) {
            const int pos = editor->position();
            const QChar ch = editor->characterAt(pos);
            if (! (ch.isLetterOrNumber() || ch == QLatin1Char('_'))) {
                for (int i = pos - 1;; --i) {
                    const QChar ch = editor->characterAt(i);
                    if (ch.isLetterOrNumber() || ch == QLatin1Char('_')) {
                        const QString wordUnderCursor = editor->textAt(i, pos - i);
                        if (wordUnderCursor.at(0).isLetter() || wordUnderCursor.at(0) == QLatin1Char('_')) {
                            foreach (const TextEditor::CompletionItem &i, m_completions) {
                                if (i.text == wordUnderCursor) {
                                    cleanup();
                                    return -1;
                                }
                            }
                        } else {
                            cleanup();
                            return -1;
                        }
                    } else
                        break;
                }
            }
        }

        if (m_completionOperator != T_EOF_SYMBOL)
            sortCompletion(m_completions);

        // always remove duplicates
        m_completions = removeDuplicates(m_completions);
    }

    for (int i = 0; i < m_completions.size(); ++i)
        m_completions[i].originalIndex = i;

    return index;
}

void CppCodeCompletion::completeObjCMsgSend(ClassOrNamespace *binding,
                                            bool staticClassAccess)
{
    QList<Scope*> memberScopes;
    foreach (Symbol *s, binding->symbols()) {
        if (ObjCClass *c = s->asObjCClass())
            memberScopes.append(c);
    }

    foreach (Scope *scope, memberScopes) {
        for (unsigned i = 0; i < scope->memberCount(); ++i) {
            Symbol *symbol = scope->memberAt(i);

            if (ObjCMethod *method = symbol->type()->asObjCMethodType()) {
                if (method->isStatic() == staticClassAccess) {
                    Overview oo;
                    const SelectorNameId *selectorName =
                            method->name()->asSelectorNameId();
                    QString text;
                    QString data;
                    if (selectorName->hasArguments()) {
                        for (unsigned i = 0; i < selectorName->nameCount(); ++i) {
                            if (i > 0)
                                text += QLatin1Char(' ');
                            Symbol *arg = method->argumentAt(i);
                            text += selectorName->nameAt(i)->identifier()->chars();
                            text += QLatin1Char(':');
                            text += TextEditor::Snippet::kVariableDelimiter;
                            text += QLatin1Char('(');
                            text += oo(arg->type());
                            text += QLatin1Char(')');
                            text += oo(arg->name());
                            text += TextEditor::Snippet::kVariableDelimiter;
                        }
                    } else {
                        text = selectorName->identifier()->chars();
                    }
                    data = text;

                    if (!text.isEmpty()) {
                        TextEditor::CompletionItem item(this);
                        item.text = text;
                        item.data = QVariant::fromValue(data);
                        m_completions.append(item);
                    }
                }
            }
        }
    }
}

bool CppCodeCompletion::tryObjCCompletion(TextEditor::BaseTextEditorWidget *edit)
{
    Q_ASSERT(edit);

    int end = m_editor->position();
    while (m_editor->characterAt(end).isSpace())
        ++end;
    if (m_editor->characterAt(end) != QLatin1Char(']'))
        return false;

    QTextCursor tc(edit->document());
    tc.setPosition(end);
    BackwardsScanner tokens(tc);
    if (tokens[tokens.startToken() - 1].isNot(T_RBRACKET))
        return false;

    const int start = tokens.startOfMatchingBrace(tokens.startToken());
    if (start == tokens.startToken())
        return false;

    const int startPos = tokens[start].begin() + tokens.startPosition();
    const QString expr = m_editor->textAt(startPos, m_editor->position() - startPos);

    const Snapshot snapshot = m_manager->snapshot();
    Document::Ptr thisDocument = snapshot.document(m_editor->file()->fileName());
    if (! thisDocument)
        return false;

    typeOfExpression.init(thisDocument, snapshot);
    int line = 0, column = 0;
    edit->convertPosition(m_editor->position(), &line, &column);
    Scope *scope = thisDocument->scopeAt(line, column);
    if (!scope)
        return false;

    const QList<LookupItem> items = typeOfExpression(expr, scope);
    LookupContext lookupContext(thisDocument, snapshot);

    foreach (const LookupItem &item, items) {
        FullySpecifiedType ty = item.type().simplified();
        if (ty->isPointerType()) {
            ty = ty->asPointerType()->elementType().simplified();

            if (NamedType *namedTy = ty->asNamedType()) {
                ClassOrNamespace *binding = lookupContext.lookupType(namedTy->name(), item.scope());
                completeObjCMsgSend(binding, false);
            }
        } else {
            if (ObjCClass *clazz = ty->asObjCClassType()) {
                ClassOrNamespace *binding = lookupContext.lookupType(clazz->name(), item.scope());
                completeObjCMsgSend(binding, true);
            }
        }
    }

    if (m_completions.isEmpty())
        return false;

    m_startPosition = m_editor->position();
    return true;
}

int CppCodeCompletion::startCompletionHelper(TextEditor::ITextEditor *editor)
{
    TextEditor::BaseTextEditorWidget *edit = qobject_cast<TextEditor::BaseTextEditorWidget *>(editor->widget());
    if (! edit)
        return -1;

    m_editor = editor;

    if (m_objcEnabled) {
        if (tryObjCCompletion(edit))
            return m_startPosition;
    }

    const int startOfName = findStartOfName();
    m_startPosition = startOfName;
    m_completionOperator = T_EOF_SYMBOL;

    int endOfOperator = m_startPosition;

    // Skip whitespace preceding this position
    while (editor->characterAt(endOfOperator - 1).isSpace())
        --endOfOperator;

    int endOfExpression = startOfOperator(editor, endOfOperator,
                                          &m_completionOperator,
                                          /*want function call =*/ true);

    Core::IFile *file = editor->file();
    QString fileName = file->fileName();

    if (m_completionOperator == T_DOXY_COMMENT) {
        for (int i = 1; i < T_DOXY_LAST_TAG; ++i) {
            TextEditor::CompletionItem item(this);
            item.text.append(QString::fromLatin1(doxygenTagSpell(i)));
            item.icon = m_icons.keywordIcon();
            m_completions.append(item);
        }

        return m_startPosition;
    }

    // Pre-processor completion
    if (m_completionOperator == T_POUND) {
        completePreprocessor();
        m_startPosition = startOfName;
        return m_startPosition;
    }

    // Include completion
    if (m_completionOperator == T_STRING_LITERAL
        || m_completionOperator == T_ANGLE_STRING_LITERAL
        || m_completionOperator == T_SLASH) {

        QTextCursor c = edit->textCursor();
        c.setPosition(endOfExpression);
        if (completeInclude(c))
            m_startPosition = startOfName;
        return m_startPosition;
    }

    ExpressionUnderCursor expressionUnderCursor;
    QTextCursor tc(edit->document());

    if (m_completionOperator == T_COMMA) {
        tc.setPosition(endOfExpression);
        const int start = expressionUnderCursor.startOfFunctionCall(tc);
        if (start == -1) {
            m_completionOperator = T_EOF_SYMBOL;
            return -1;
        }

        endOfExpression = start;
        m_startPosition = start + 1;
        m_completionOperator = T_LPAREN;
    }

    QString expression;
    int startOfExpression = editor->position();
    tc.setPosition(endOfExpression);

    if (m_completionOperator) {
        expression = expressionUnderCursor(tc);
        startOfExpression = endOfExpression - expression.length();

        if (m_completionOperator == T_LPAREN) {
            if (expression.endsWith(QLatin1String("SIGNAL")))
                m_completionOperator = T_SIGNAL;

            else if (expression.endsWith(QLatin1String("SLOT")))
                m_completionOperator = T_SLOT;

            else if (editor->position() != endOfOperator) {
                // We don't want a function completion when the cursor isn't at the opening brace
                expression.clear();
                m_completionOperator = T_EOF_SYMBOL;
                m_startPosition = startOfName;
                startOfExpression = editor->position();
            }
        }
    } else if (expression.isEmpty()) {
        while (startOfExpression > 0 && editor->characterAt(startOfExpression).isSpace())
            --startOfExpression;
    }

    int line = 0, column = 0;
    edit->convertPosition(startOfExpression, &line, &column);
//    qDebug() << "***** line:" << line << "column:" << column;
//    qDebug() << "***** expression:" << expression;
    return startCompletionInternal(edit, fileName, line, column, expression, endOfExpression);
}

int CppCodeCompletion::startCompletionInternal(TextEditor::BaseTextEditorWidget *edit,
                                               const QString fileName,
                                               unsigned line, unsigned column,
                                               const QString &expr,
                                               int endOfExpression)
{
    QString expression = expr.trimmed();
    const Snapshot snapshot = m_manager->snapshot();

    Document::Ptr thisDocument = snapshot.document(fileName);
    if (! thisDocument)
        return -1;

    typeOfExpression.init(thisDocument, snapshot);

    Scope *scope = thisDocument->scopeAt(line, column);
    Q_ASSERT(scope != 0);

    if (expression.isEmpty()) {
        if (m_completionOperator == T_EOF_SYMBOL || m_completionOperator == T_COLON_COLON) {
            (void) typeOfExpression(expression, scope);
            globalCompletion(scope);
            if (m_completions.isEmpty())
                return -1;
            return m_startPosition;
        }

        else if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
            // Apply signal/slot completion on 'this'
            expression = QLatin1String("this");
        }
    }

    QList<LookupItem> results = typeOfExpression(expression, scope, TypeOfExpression::Preprocess);

    if (results.isEmpty()) {
        if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
            if (! (expression.isEmpty() || expression == QLatin1String("this"))) {
                expression = QLatin1String("this");
                results = typeOfExpression(expression, scope);
            }

            if (results.isEmpty())
                return -1;

        } else if (m_completionOperator == T_LPAREN) {
            // Find the expression that precedes the current name
            int index = endOfExpression;
            while (m_editor->characterAt(index - 1).isSpace())
                --index;
            index = findStartOfName(index);

            QTextCursor tc(edit->document());
            tc.setPosition(index);

            ExpressionUnderCursor expressionUnderCursor;
            const QString baseExpression = expressionUnderCursor(tc);

            // Resolve the type of this expression
            const QList<LookupItem> results =
                    typeOfExpression(baseExpression, scope,
                                     TypeOfExpression::Preprocess);

            // If it's a class, add completions for the constructors
            foreach (const LookupItem &result, results) {
                if (result.type()->isClassType()) {
                    if (completeConstructorOrFunction(results, endOfExpression, true))
                        return m_startPosition;

                    break;
                }
            }
            return -1;

        } else {
            // nothing to do.
            return -1;

        }
    }

    switch (m_completionOperator) {
    case T_LPAREN:
        if (completeConstructorOrFunction(results, endOfExpression, false))
            return m_startPosition;
        break;

    case T_DOT:
    case T_ARROW:
        if (completeMember(results))
            return m_startPosition;
        break;

    case T_COLON_COLON:
        if (completeScope(results))
            return m_startPosition;
        break;

    case T_SIGNAL:
        if (completeSignal(results))
            return m_startPosition;
        break;

    case T_SLOT:
        if (completeSlot(results))
            return m_startPosition;
        break;

    default:
        break;
    } // end of switch

    // nothing to do.
    return -1;
}

void CppCodeCompletion::globalCompletion(Scope *currentScope)
{
    const LookupContext &context = typeOfExpression.context();

    if (m_completionOperator == T_COLON_COLON) {
        completeNamespace(context.globalNamespace());
        return;
    }

    QList<ClassOrNamespace *> usingBindings;
    ClassOrNamespace *currentBinding = 0;

    for (Scope *scope = currentScope; scope; scope = scope->enclosingScope()) {
        if (scope->isBlock()) {
            if (ClassOrNamespace *binding = context.lookupType(scope)) {
                for (unsigned i = 0; i < scope->memberCount(); ++i) {
                    Symbol *member = scope->memberAt(i);
                    if (! member->name())
                        continue;
                    else if (UsingNamespaceDirective *u = member->asUsingNamespaceDirective()) {
                        if (ClassOrNamespace *b = binding->lookupType(u->name()))
                            usingBindings.append(b);
                    }
                }
            }
        } else if (scope->isFunction() || scope->isClass() || scope->isNamespace()) {
            currentBinding = context.lookupType(scope);
            break;
        }
    }

    for (Scope *scope = currentScope; scope; scope = scope->enclosingScope()) {
        if (scope->isBlock()) {
            for (unsigned i = 0; i < scope->memberCount(); ++i) {
                addCompletionItem(scope->memberAt(i));
            }
        } else if (scope->isFunction()) {
            Function *fun = scope->asFunction();
            for (unsigned i = 0; i < fun->argumentCount(); ++i) {
                addCompletionItem(fun->argumentAt(i));
            }
            break;
        } else {
            break;
        }
    }

    for (; currentBinding; currentBinding = currentBinding->parent()) {
        const QList<Symbol *> symbols = currentBinding->symbols();

        if (! symbols.isEmpty()) {
            if (symbols.first()->isNamespace())
                completeNamespace(currentBinding);
            else
                completeClass(currentBinding);
        }
    }

    foreach (ClassOrNamespace *b, usingBindings)
        completeNamespace(b);

    addKeywords();
    addMacros(QLatin1String("<configuration>"), context.snapshot());
    addMacros(context.thisDocument()->fileName(), context.snapshot());
    addSnippets();
}

static Scope *enclosingNonTemplateScope(Symbol *symbol)
{
    if (symbol) {
        if (Scope *scope = symbol->enclosingScope()) {
            if (Template *templ = scope->asTemplate())
                return templ->enclosingScope();
            return scope;
        }
    }
    return 0;
}

static Function *asFunctionOrTemplateFunctionType(FullySpecifiedType ty)
{
    if (Function *funTy = ty->asFunctionType())
        return funTy;
    else if (Template *templ = ty->asTemplateType()) {
        if (Symbol *decl = templ->declaration())
            return decl->asFunction();
    }
    return 0;
}

static Class *asClassOrTemplateClassType(FullySpecifiedType ty)
{
    if (Class *classTy = ty->asClassType())
        return classTy;
    else if (Template *templ = ty->asTemplateType()) {
        if (Symbol *decl = templ->declaration())
            return decl->asClass();
    }
    return 0;
}

bool CppCodeCompletion::completeConstructorOrFunction(const QList<LookupItem> &results,
                                                      int endOfExpression, bool toolTipOnly)
{
    const LookupContext &context = typeOfExpression.context();
    QList<Function *> functions;

    foreach (const LookupItem &result, results) {
        FullySpecifiedType exprTy = result.type().simplified();

        if (Class *klass = asClassOrTemplateClassType(exprTy)) {
            const Name *className = klass->name();
            if (! className)
                continue; // nothing to do for anonymous classes.

            for (unsigned i = 0; i < klass->memberCount(); ++i) {
                Symbol *member = klass->memberAt(i);
                const Name *memberName = member->name();

                if (! memberName)
                    continue; // skip anonymous member.

                else if (memberName->isQualifiedNameId())
                    continue; // skip

                if (Function *funTy = member->type()->asFunctionType()) {
                    if (memberName->isEqualTo(className)) {
                        // it's a ctor.
                        functions.append(funTy);
                    }
                }
            }

            break;
        }
    }

    if (functions.isEmpty()) {
        foreach (const LookupItem &result, results) {
            FullySpecifiedType ty = result.type().simplified();

            if (Function *fun = asFunctionOrTemplateFunctionType(ty)) {

                if (! fun->name())
                    continue;
                else if (! functions.isEmpty() && enclosingNonTemplateScope(functions.first()) != enclosingNonTemplateScope(fun))
                    continue; // skip fun, it's an hidden declaration.

                bool newOverload = true;

                foreach (Function *f, functions) {
                    if (fun->isEqualTo(f)) {
                        newOverload = false;
                        break;
                    }
                }

                if (newOverload)
                    functions.append(fun);
            }
        }
    }

    if (functions.isEmpty()) {
        const Name *functionCallOp = context.control()->operatorNameId(OperatorNameId::FunctionCallOp);

        foreach (const LookupItem &result, results) {
            FullySpecifiedType ty = result.type().simplified();
            Scope *scope = result.scope();

            if (NamedType *namedTy = ty->asNamedType()) {
                if (ClassOrNamespace *b = context.lookupType(namedTy->name(), scope)) {
                    foreach (const LookupItem &r, b->lookup(functionCallOp)) {
                        Symbol *overload = r.declaration();
                        FullySpecifiedType overloadTy = overload->type().simplified();

                        if (Function *funTy = overloadTy->asFunctionType()) {
                            functions.append(funTy);
                        }
                    }
                }
            }
        }
    }

    // There are two different kinds of completion we want to provide:
    // 1. If this is a function call, we want to pop up a tooltip that shows the user
    // the possible overloads with their argument types and names.
    // 2. If this is a function definition, we want to offer autocompletion of
    // the function signature.

    // check if function signature autocompletion is appropriate
    // Also check if the function name is a destructor name.
    bool isDestructor = false;
    if (! functions.isEmpty() && ! toolTipOnly) {

        // function definitions will only happen in class or namespace scope,
        // so get the current location's enclosing scope.

        // get current line and column
        TextEditor::BaseTextEditorWidget *edit = qobject_cast<TextEditor::BaseTextEditorWidget *>(m_editor->widget());
        int lineSigned = 0, columnSigned = 0;
        edit->convertPosition(m_editor->position(), &lineSigned, &columnSigned);
        unsigned line = lineSigned, column = columnSigned;

        // find a scope that encloses the current location, starting from the lastVisibileSymbol
        // and moving outwards

        Scope *sc = context.thisDocument()->scopeAt(line, column);

        if (sc && (sc->isClass() || sc->isNamespace())) {
            // It may still be a function call. If the whole line parses as a function
            // declaration, we should be certain that it isn't.
            bool autocompleteSignature = false;

            QTextCursor tc(edit->document());
            tc.setPosition(endOfExpression);
            BackwardsScanner bs(tc);
            const int startToken = bs.startToken();
            int lineStartToken = bs.startOfLine(startToken);
            // make sure the required tokens are actually available
            bs.LA(startToken - lineStartToken);
            QString possibleDecl = bs.mid(lineStartToken).trimmed().append("();");

            Document::Ptr doc = Document::create(QLatin1String("<completion>"));
            doc->setSource(possibleDecl.toLatin1());
            if (doc->parse(Document::ParseDeclaration)) {
                doc->check();
                if (SimpleDeclarationAST *sd = doc->translationUnit()->ast()->asSimpleDeclaration()) {
                    if (sd->declarator_list &&
                        sd->declarator_list && sd->declarator_list->value->postfix_declarator_list
                        && sd->declarator_list->value->postfix_declarator_list->value->asFunctionDeclarator()) {

                        autocompleteSignature = true;

                        CoreDeclaratorAST *coreDecl = sd->declarator_list->value->core_declarator;
                        if (coreDecl && coreDecl->asDeclaratorId() && coreDecl->asDeclaratorId()->name) {
                            NameAST *declName = coreDecl->asDeclaratorId()->name;
                            if (declName->asDestructorName()) {
                                isDestructor = true;
                            } else if (QualifiedNameAST *qName = declName->asQualifiedName()) {
                                if (qName->unqualified_name && qName->unqualified_name->asDestructorName())
                                    isDestructor = true;
                            }
                        }
                    }
                }
            }

            if (autocompleteSignature && !isDestructor) {
                // set up signature autocompletion
                foreach (Function *f, functions) {
                    Overview overview;
                    overview.setShowArgumentNames(true);
                    overview.setShowDefaultArguments(false);

                    // gets: "parameter list) cv-spec",
                    QString completion = overview(f->type()).mid(1);

                    TextEditor::CompletionItem item(this);
                    item.text = completion;
                    item.data = QVariant::fromValue(CompleteFunctionDeclaration(f));
                    m_completions.append(item);
                }
                return true;
            }
        }
    }

    if (! functions.empty() && !isDestructor) {
        // set up function call tooltip

        // Recreate if necessary
        if (!m_functionArgumentWidget)
            m_functionArgumentWidget = new FunctionArgumentWidget;

        m_functionArgumentWidget->showFunctionHint(functions,
                                                   typeOfExpression.context(),
                                                   m_startPosition);
    }

    return false;
}

bool CppCodeCompletion::completeMember(const QList<LookupItem> &baseResults)
{
    const LookupContext &context = typeOfExpression.context();

//    if (debug)
//        qDebug() << Q_FUNC_INFO << __LINE__;

    if (baseResults.isEmpty())
        return false;

    ResolveExpression resolveExpression(context);

    bool replacedDotOperator = false;

    if (ClassOrNamespace *binding = resolveExpression.baseExpression(baseResults,
                                                                     m_completionOperator,
                                                                     &replacedDotOperator)) {
//        if (debug)
//            qDebug() << "cool we got a binding for the base expression";

        if (replacedDotOperator && binding) {
            // Replace . with ->
            int length = m_editor->position() - m_startPosition + 1;
            m_editor->setCursorPosition(m_startPosition - 1);
            m_editor->replace(length, QLatin1String("->"));
            ++m_startPosition;
        }

        if (binding)
            completeClass(binding, /*static lookup = */ false);

        return ! m_completions.isEmpty();
    }

//    if (debug) {
//        Overview oo;
//        qDebug() << "hmm, got:" << oo(baseResults.first().type());
//    }

    return false;
}

bool CppCodeCompletion::completeScope(const QList<LookupItem> &results)
{
    const LookupContext &context = typeOfExpression.context();
    if (results.isEmpty())
        return false;

    foreach (const LookupItem &result, results) {
        FullySpecifiedType ty = result.type();
        Scope *scope = result.scope();

        if (NamedType *namedTy = ty->asNamedType()) {
            if (ClassOrNamespace *b = context.lookupType(namedTy->name(), scope)) {
                completeClass(b);
                break;
            }

        } else if (Class *classTy = ty->asClassType()) {
            if (ClassOrNamespace *b = context.lookupType(classTy)) {
                completeClass(b);
                break;
            }

        } else if (Namespace *nsTy = ty->asNamespaceType()) {
            if (ClassOrNamespace *b = context.lookupType(nsTy)) {
                completeNamespace(b);
                break;
            }

        }
    }

    return ! m_completions.isEmpty();
}

void CppCodeCompletion::addKeywords()
{
    int keywordLimit = T_FIRST_OBJC_AT_KEYWORD;
    if (objcKeywordsWanted())
        keywordLimit = T_LAST_OBJC_AT_KEYWORD + 1;

    // keyword completion items.
    for (int i = T_FIRST_KEYWORD; i < keywordLimit; ++i) {
        TextEditor::CompletionItem item(this);
        item.text = QLatin1String(Token::name(i));
        item.icon = m_icons.keywordIcon();
        m_completions.append(item);
    }
}

void CppCodeCompletion::addMacros(const QString &fileName, const Snapshot &snapshot)
{
    QSet<QString> processed;
    QSet<QString> definedMacros;

    addMacros_helper(snapshot, fileName, &processed, &definedMacros);

    foreach (const QString &macroName, definedMacros) {
        TextEditor::CompletionItem item(this);
        item.text = macroName;
        item.icon = m_icons.macroIcon();
        m_completions.append(item);
    }
}

void CppCodeCompletion::addMacros_helper(const Snapshot &snapshot,
                                         const QString &fileName,
                                         QSet<QString> *processed,
                                         QSet<QString> *definedMacros)
{
    Document::Ptr doc = snapshot.document(fileName);

    if (! doc || processed->contains(doc->fileName()))
        return;

    processed->insert(doc->fileName());

    foreach (const Document::Include &i, doc->includes()) {
        addMacros_helper(snapshot, i.fileName(), processed, definedMacros);
    }

    foreach (const Macro &macro, doc->definedMacros()) {
        const QString macroName = QString::fromUtf8(macro.name().constData(), macro.name().length());
        if (! macro.isHidden())
            definedMacros->insert(macroName);
        else
            definedMacros->remove(macroName);
    }
}

void CppCodeCompletion::addCompletionItem(Symbol *symbol)
{
    ConvertToCompletionItem toCompletionItem(this);
    TextEditor::CompletionItem item = toCompletionItem(symbol);
    if (item.isValid())
        m_completions.append(item);
}

bool CppCodeCompletion::completeInclude(const QTextCursor &cursor)
{
    QString directoryPrefix;
    if (m_completionOperator == T_SLASH) {
        QTextCursor c = cursor;
        c.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString sel = c.selectedText();
        int startCharPos = sel.indexOf(QLatin1Char('"'));
        if (startCharPos == -1) {
            startCharPos = sel.indexOf(QLatin1Char('<'));
            m_completionOperator = T_ANGLE_STRING_LITERAL;
        } else {
            m_completionOperator = T_STRING_LITERAL;
        }
        if (startCharPos != -1)
            directoryPrefix = sel.mid(startCharPos + 1, sel.length() - 1);
    }

    // Make completion for all relevant includes
    if (ProjectExplorer::Project *project = ProjectExplorer::ProjectExplorerPlugin::instance()->currentProject()) {
        QStringList includePaths = m_manager->projectInfo(project).includePaths;
        const QString currentFilePath = QFileInfo(m_editor->file()->fileName()).path();
        if (!includePaths.contains(currentFilePath))
            includePaths.append(currentFilePath);

        foreach (const QString &includePath, includePaths) {
            QString realPath = includePath;
            if (!directoryPrefix.isEmpty()) {
                realPath += QLatin1Char('/');
                realPath += directoryPrefix;
            }
            foreach (const QString &itemText, m_manager->includesInPath(realPath)) {
                TextEditor::CompletionItem item(this);
                item.text += itemText;
                // TODO: Icon for include files
                item.icon = m_icons.keywordIcon();
                m_completions.append(item);
            }
        }

        QStringList frameworkPaths = m_manager->projectInfo(project).frameworkPaths;
        foreach (const QString &frameworkPath, frameworkPaths) {
            QString realPath = frameworkPath;
            if (!directoryPrefix.isEmpty()) {
                realPath += QLatin1Char('/');
                realPath += directoryPrefix;
                realPath += QLatin1String(".framework/Headers");
            }
            foreach (const QString &itemText, m_manager->includesInPath(realPath)) {
                TextEditor::CompletionItem item(this);
                item.text += itemText;
                // TODO: Icon for include files
                item.icon = m_icons.keywordIcon();
                m_completions.append(item);
            }
        }
    }

    return !m_completions.isEmpty();
}

QStringList CppCodeCompletion::preprocessorCompletions
        = QStringList()
          << QLatin1String("define")
          << QLatin1String("error")
          << QLatin1String("include")
          << QLatin1String("line")
          << QLatin1String("pragma")
          << QLatin1String("undef")
          << QLatin1String("if")
          << QLatin1String("ifdef")
          << QLatin1String("ifndef")
          << QLatin1String("elif")
          << QLatin1String("else")
          << QLatin1String("endif")
          ;

void CppCodeCompletion::completePreprocessor()
{
    TextEditor::CompletionItem item(this);

    foreach (const QString &preprocessorCompletion, preprocessorCompletions) {
        item.text = preprocessorCompletion;
        m_completions.append(item);
    }

    if (objcKeywordsWanted()) {
        item.text = QLatin1String("import");
        m_completions.append(item);
    }
}

void CppCodeCompletion::completeNamespace(ClassOrNamespace *b)
{
    QSet<ClassOrNamespace *> bindingsVisited;
    QList<ClassOrNamespace *> bindingsToVisit;
    bindingsToVisit.append(b);

    while (! bindingsToVisit.isEmpty()) {
        ClassOrNamespace *binding = bindingsToVisit.takeFirst();
        if (! binding || bindingsVisited.contains(binding))
            continue;

        bindingsVisited.insert(binding);
        bindingsToVisit += binding->usings();

        QList<Scope *> scopesToVisit;
        QSet<Scope *> scopesVisited;

        foreach (Symbol *bb, binding->symbols()) {
            if (Namespace *ns = bb->asNamespace())
                scopesToVisit.append(ns);
        }

        foreach (Enum *e, binding->enums()) {
            scopesToVisit.append(e);
        }

        while (! scopesToVisit.isEmpty()) {
            Scope *scope = scopesToVisit.takeFirst();
            if (! scope || scopesVisited.contains(scope))
                continue;

            scopesVisited.insert(scope);

            for (Scope::iterator it = scope->firstMember(); it != scope->lastMember(); ++it) {
                Symbol *member = *it;
                addCompletionItem(member);
            }
        }
    }
}

void CppCodeCompletion::completeClass(ClassOrNamespace *b, bool staticLookup)
{
    QSet<ClassOrNamespace *> bindingsVisited;
    QList<ClassOrNamespace *> bindingsToVisit;
    bindingsToVisit.append(b);

    while (! bindingsToVisit.isEmpty()) {
        ClassOrNamespace *binding = bindingsToVisit.takeFirst();
        if (! binding || bindingsVisited.contains(binding))
            continue;

        bindingsVisited.insert(binding);
        bindingsToVisit += binding->usings();

        QList<Scope *> scopesToVisit;
        QSet<Scope *> scopesVisited;

        foreach (Symbol *bb, binding->symbols()) {
            if (Class *k = bb->asClass())
                scopesToVisit.append(k);
        }

        foreach (Enum *e, binding->enums())
            scopesToVisit.append(e);

        while (! scopesToVisit.isEmpty()) {
            Scope *scope = scopesToVisit.takeFirst();
            if (! scope || scopesVisited.contains(scope))
                continue;

            scopesVisited.insert(scope);

            addCompletionItem(scope); // add a completion item for the injected class name.

            for (Scope::iterator it = scope->firstMember(); it != scope->lastMember(); ++it) {
                Symbol *member = *it;
                if (member->isFriend()
                        || member->isQtPropertyDeclaration()
                        || member->isQtEnum()) {
                    continue;
                } else if (! staticLookup && (member->isTypedef() ||
                                            member->isEnum()    ||
                                            member->isClass())) {
                    continue;
                }

                addCompletionItem(member);
            }
        }
    }
}

bool CppCodeCompletion::completeQtMethod(const QList<LookupItem> &results,
                                         bool wantSignals)
{
    if (results.isEmpty())
        return false;

    const LookupContext &context = typeOfExpression.context();

    ConvertToCompletionItem toCompletionItem(this);
    Overview o;
    o.setShowReturnTypes(false);
    o.setShowArgumentNames(false);
    o.setShowFunctionSignatures(true);

    QSet<QString> signatures;
    foreach (const LookupItem &p, results) {
        FullySpecifiedType ty = p.type().simplified();

        if (PointerType *ptrTy = ty->asPointerType())
            ty = ptrTy->elementType().simplified();
        else
            continue; // not a pointer or a reference to a pointer.

        NamedType *namedTy = ty->asNamedType();
        if (! namedTy) // not a class name.
            continue;

        ClassOrNamespace *b = context.lookupType(namedTy->name(), p.scope());
        if (! b)
            continue;

        QList<ClassOrNamespace *>todo;
        QSet<ClassOrNamespace *> processed;
        QList<Scope *> scopes;
        todo.append(b);
        while (!todo.isEmpty()) {
            ClassOrNamespace *binding = todo.takeLast();
            if (!processed.contains(binding)) {
                processed.insert(binding);

                foreach (Symbol *s, binding->symbols())
                    if (Class *clazz = s->asClass())
                        scopes.append(clazz);

                todo.append(binding->usings());
            }
        }

        foreach (Scope *scope, scopes) {
            if (! scope->isClass())
                continue;

            for (unsigned i = 0; i < scope->memberCount(); ++i) {
                Symbol *member = scope->memberAt(i);
                Function *fun = member->type()->asFunctionType();
                if (! fun)
                    continue;
                if (wantSignals && ! fun->isSignal())
                    continue;
                else if (! wantSignals && ! fun->isSlot())
                    continue;
                TextEditor::CompletionItem item = toCompletionItem(fun);
                if (item.isValid()) {
                    unsigned count = fun->argumentCount();
                    while (true) {
                        TextEditor::CompletionItem ci = item;

                        QString signature;
                        signature += overview.prettyName(fun->name());
                        signature += QLatin1Char('(');
                        for (unsigned i = 0; i < count; ++i) {
                            Symbol *arg = fun->argumentAt(i);
                            if (i != 0)
                                signature += QLatin1Char(',');
                            signature += o.prettyType(arg->type());
                        }
                        signature += QLatin1Char(')');

                        const QByteArray normalized =
                                QMetaObject::normalizedSignature(signature.toLatin1());

                        signature = QString::fromLatin1(normalized, normalized.size());

                        if (! signatures.contains(signature)) {
                            signatures.insert(signature);

                            ci.text = signature; // fix the completion item.
                            m_completions.append(ci);
                        }

                        if (count && fun->argumentAt(count - 1)->asArgument()->hasInitializer())
                            --count;
                        else
                            break;
                    }
                }
            }
        }
    }

    return ! m_completions.isEmpty();
}

void CppCodeCompletion::completions(QList<TextEditor::CompletionItem> *completions)
{
    const int length = m_editor->position() - m_startPosition;
    if (length < 0)
        return;

    const QString key = m_editor->textAt(m_startPosition, length);

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        /* Close on the trailing slash for include completion, to enable the slash to
         * trigger a new completion list. */
        if ((m_completionOperator == T_STRING_LITERAL ||
             m_completionOperator == T_ANGLE_STRING_LITERAL) && key.endsWith(QLatin1Char('/')))
            return;

        if (m_completionOperator != T_LPAREN) {
            filter(m_completions, completions, key);

        } else if (m_completionOperator == T_LPAREN ||
                   m_completionOperator == T_SIGNAL ||
                   m_completionOperator == T_SLOT) {
            foreach (const TextEditor::CompletionItem &item, m_completions) {
                if (item.text.startsWith(key, Qt::CaseInsensitive)) {
                    completions->append(item);
                }
            }
        }
    }

    if (m_automaticCompletion && completions->size() == 1 && key == completions->first().text) {
        completions->clear();
    }
}

QList<TextEditor::CompletionItem> CppCodeCompletion::removeDuplicates(const QList<TextEditor::CompletionItem> &items)
{
    // Duplicates are kept only if they are snippets.
    QList<TextEditor::CompletionItem> uniquelist;
    QSet<QString> processed;
    foreach (const TextEditor::CompletionItem &item, items) {
        if (!processed.contains(item.text) || item.isSnippet) {
            uniquelist.append(item);
            if (!item.isSnippet) {
                processed.insert(item.text);
                if (Symbol *symbol = qvariant_cast<Symbol *>(item.data)) {
                    if (Function *funTy = symbol->type()->asFunctionType()) {
                        if (funTy->hasArguments())
                            ++uniquelist.back().duplicateCount;
                    }
                }
            }
        }
    }

    return uniquelist;
}

QList<TextEditor::CompletionItem> CppCodeCompletion::getCompletions()
{
    QList<TextEditor::CompletionItem> completionItems;
    completions(&completionItems);

    return completionItems;
}

bool CppCodeCompletion::typedCharCompletes(const TextEditor::CompletionItem &item, QChar typedChar)
{
    if (m_automaticCompletion)
        return false;

    if (item.data.canConvert<QString>()) // snippet
        return false;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT)
        return typedChar == QLatin1Char('(')
                || typedChar == QLatin1Char(',');

    if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL)
        return typedChar == QLatin1Char('/')
                && item.text.endsWith(QLatin1Char('/'));

    if (item.data.value<Symbol *>())
        return typedChar == QLatin1Char(':')
                || typedChar == QLatin1Char(';')
                || typedChar == QLatin1Char('.')
                || typedChar == QLatin1Char(',')
                || typedChar == QLatin1Char('(');

    if (item.data.canConvert<CompleteFunctionDeclaration>())
        return typedChar == QLatin1Char('(');

    return false;
}

void CppCodeCompletion::complete(const TextEditor::CompletionItem &item, QChar typedChar)
{
    m_shouldRestartCompletion = false; // Enabled for specific cases

    Symbol *symbol = 0;

    if (item.data.isValid()) {
        if (item.data.canConvert<QString>()) {
            TextEditor::BaseTextEditorWidget *edit = qobject_cast<TextEditor::BaseTextEditorWidget *>(m_editor->widget());
            QTextCursor tc = edit->textCursor();
            tc.setPosition(m_startPosition, QTextCursor::KeepAnchor);
            edit->insertCodeSnippet(tc, item.data.toString());
            return;
        } else {
            symbol = item.data.value<Symbol *>();
        }
    }

    QString toInsert;
    QString extraChars;
    int extraLength = 0;
    int cursorOffset = 0;

    bool autoParenthesesEnabled = true;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        toInsert = item.text;
        extraChars += QLatin1Char(')');

        if (typedChar == QLatin1Char('(')) // Eat the opening parenthesis
            typedChar = QChar();
    } else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL) {
        toInsert = item.text;
        if (!toInsert.endsWith(QLatin1Char('/'))) {
            extraChars += QLatin1Char((m_completionOperator == T_ANGLE_STRING_LITERAL) ? '>' : '"');
        } else {
            m_shouldRestartCompletion = true;  // Re-trigger for subdirectory
            if (typedChar == QLatin1Char('/')) // Eat the slash
                typedChar = QChar();
        }
    } else {
        toInsert = item.text;

        //qDebug() << "current symbol:" << overview.prettyName(symbol->name())
        //<< overview.prettyType(symbol->type());

        const bool autoInsertBrackets = completionSettings().m_autoInsertBrackets;

        if (autoInsertBrackets && symbol && symbol->type()) {
            if (Function *function = symbol->type()->asFunctionType()) {
                // If the member is a function, automatically place the opening parenthesis,
                // except when it might take template parameters.
                if (! function->hasReturnType() && (function->unqualifiedName() && !function->unqualifiedName()->isDestructorNameId())) {
                    // Don't insert any magic, since the user might have just wanted to select the class

                    /// ### port me
#if 0
                } else if (function->templateParameterCount() != 0 && typedChar != QLatin1Char('(')) {
                    // If there are no arguments, then we need the template specification
                    if (function->argumentCount() == 0) {
                        extraChars += QLatin1Char('<');
                    }
#endif
                } else if (! function->isAmbiguous()) {
                    // When the user typed the opening parenthesis, he'll likely also type the closing one,
                    // in which case it would be annoying if we put the cursor after the already automatically
                    // inserted closing parenthesis.
                    const bool skipClosingParenthesis = typedChar != QLatin1Char('(');

                    if (completionSettings().m_spaceAfterFunctionName)
                        extraChars += QLatin1Char(' ');
                    extraChars += QLatin1Char('(');
                    if (typedChar == QLatin1Char('('))
                        typedChar = QChar();

                    // If the function doesn't return anything, automatically place the semicolon,
                    // unless we're doing a scope completion (then it might be function definition).
                    const QChar characterAtCursor = m_editor->characterAt(m_editor->position());
                    bool endWithSemicolon = typedChar == QLatin1Char(';')
                            || (function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON);
                    const QChar semicolon = typedChar.isNull() ? QLatin1Char(';') : typedChar;

                    if (endWithSemicolon && characterAtCursor == semicolon) {
                        endWithSemicolon = false;
                        typedChar = QChar();
                    }

                    // If the function takes no arguments, automatically place the closing parenthesis
                    if (item.duplicateCount == 0 && ! function->hasArguments() && skipClosingParenthesis) {
                        extraChars += QLatin1Char(')');
                        if (endWithSemicolon) {
                            extraChars += semicolon;
                            typedChar = QChar();
                        }
                    } else if (autoParenthesesEnabled) {
                        const QChar lookAhead = m_editor->characterAt(m_editor->position() + 1);
                        if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                            extraChars += QLatin1Char(')');
                            --cursorOffset;
                            if (endWithSemicolon) {
                                extraChars += semicolon;
                                --cursorOffset;
                                typedChar = QChar();
                            }
                        }
                        // TODO: When an opening parenthesis exists, the "semicolon" should really be
                        // inserted after the matching closing parenthesis.
                    }
                }
            }
        }

        if (autoInsertBrackets && item.data.canConvert<CompleteFunctionDeclaration>()) {
            if (typedChar == QLatin1Char('('))
                typedChar = QChar();

            // everything from the closing parenthesis on are extra chars, to
            // make sure an auto-inserted ")" gets replaced by ") const" if necessary
            int closingParen = toInsert.lastIndexOf(QLatin1Char(')'));
            extraChars = toInsert.mid(closingParen);
            toInsert.truncate(closingParen);
        }
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!typedChar.isNull()) {
        extraChars += typedChar;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    if (!extraChars.isEmpty() && extraChars.length() + cursorOffset > 0) {
        const QChar c = extraChars.at(extraChars.length() - 1 + cursorOffset);
        if (c == QLatin1Char('.') || c == QLatin1Char('('))
            m_shouldRestartCompletion = true;
    }

    // Avoid inserting characters that are already there
    for (int i = 0; i < extraChars.length(); ++i) {
        const QChar a = extraChars.at(i);
        const QChar b = m_editor->characterAt(m_editor->position() + i);
        if (a == b)
            ++extraLength;
        else
            break;
    }

    toInsert += extraChars;

    // Insert the remainder of the name
    int length = m_editor->position() - m_startPosition + extraLength;
    m_editor->setCursorPosition(m_startPosition);
    m_editor->replace(length, toInsert);
    if (cursorOffset)
        m_editor->setCursorPosition(m_editor->position() + cursorOffset);
}

bool CppCodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        return false;
    } else if (completionItems.count() == 1) {
        complete(completionItems.first(), QChar());
        return true;
    } else if (m_completionOperator != T_LPAREN) {
        return TextEditor::ICompletionCollector::partiallyComplete(completionItems);
    }

    return false;
}

void CppCodeCompletion::cleanup()
{
    m_automaticCompletion = false;
    m_completions.clear();

    // Set empty map in order to avoid referencing old versions of the documents
    // until the next completion
    typeOfExpression.reset();
}

int CppCodeCompletion::findStartOfName(int pos) const
{
    if (pos == -1)
        pos = m_editor->position();
    QChar chr;

    // Skip to the start of a name
    do {
        chr = m_editor->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));

    return pos + 1;
}

bool CppCodeCompletion::objcKeywordsWanted() const
{
    if (!m_objcEnabled)
        return false;

    Core::IFile *file = m_editor->file();
    QString fileName = file->fileName();

    const Core::MimeDatabase *mdb = Core::ICore::instance()->mimeDatabase();
    return mdb->findByFile(fileName).type() == CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE;
}

void CppCodeCompletion::addSnippets()
{
    m_completions.append(m_snippetProvider.getSnippets(this));
}

#include "cppcodecompletion.moc"
