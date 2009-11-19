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

#include "cppcodecompletion.h"
#include "cppmodelmanager.h"
#include "cppdoxygen.h"
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
#include <cplusplus/LookupContext.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/Overview.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/TokenUnderCursor.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/itexteditable.h>
#include <texteditor/basetexteditor.h>
#include <projectexplorer/projectexplorer.h>
#include <utils/qtcassert.h>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QStyleOption>
#include <QtGui/QStylePainter>
#include <QtGui/QTextDocument> // Qt::escape()
#include <QtGui/QToolButton>
#include <QtGui/QVBoxLayout>

using namespace CPlusPlus;

namespace CppTools {
namespace Internal {

class FakeToolTipFrame : public QWidget
{
public:
    FakeToolTipFrame(QWidget *parent = 0) :
        QWidget(parent, Qt::ToolTip | Qt::WindowStaysOnTopHint)
    {
        setFocusPolicy(Qt::NoFocus);
        setAttribute(Qt::WA_DeleteOnClose);
    }

protected:
    void paintEvent(QPaintEvent *e);
    void resizeEvent(QResizeEvent *e);
};

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

    Function *currentFunction() const
    { return m_items.at(m_current); }

    int m_startpos;
    int m_currentarg;
    int m_current;
    bool m_escapePressed;

    TextEditor::ITextEditor *m_editor;

    QWidget *m_pager;
    QLabel *m_numberLabel;
    FakeToolTipFrame *m_popupFrame;
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
        accept(symbol->identity());
        if (_item)
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

    TextEditor::CompletionItem newCompletionItem(Name *name)
    {
        TextEditor::CompletionItem item(_collector);
        item.text = overview.prettyName(name);
        item.icon = _collector->iconForSymbol(_symbol);
        return item;
    }

    virtual void visit(NameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(TemplateNameId *name)
    {
        _item = newCompletionItem(name);
        _item.text = QLatin1String(name->identifier()->chars());
    }

    virtual void visit(DestructorNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(OperatorNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(ConversionNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(QualifiedNameId *name)
    { _item = newCompletionItem(name->unqualifiedNameId()); }
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

void FakeToolTipFrame::paintEvent(QPaintEvent *)
{
    QStylePainter p(this);
    QStyleOptionFrame opt;
    opt.init(this);
    p.drawPrimitive(QStyle::PE_PanelTipLabel, opt);
    p.end();
}

void FakeToolTipFrame::resizeEvent(QResizeEvent *)
{
    QStyleHintReturnMask frameMask;
    QStyleOption option;
    option.init(this);
    if (style()->styleHint(QStyle::SH_ToolTip_Mask, &option, this, &frameMask))
        setMask(frameMask.region);
}


FunctionArgumentWidget::FunctionArgumentWidget():
    m_startpos(-1),
    m_current(0),
    m_escapePressed(false)
{
    QObject *editorObject = Core::EditorManager::instance()->currentEditor();
    m_editor = qobject_cast<TextEditor::ITextEditor *>(editorObject);

    m_popupFrame = new FakeToolTipFrame(m_editor->widget());

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
    setMargin(1);

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
    QList<SimpleToken> tokens = tokenize(str);
    for (int i = 0; i < tokens.count(); ++i) {
        const SimpleToken &tk = tokens.at(i);
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

    m_popupFrame->setFixedWidth(m_popupFrame->minimumSizeHint().width());

    const QDesktopWidget *desktop = QApplication::desktop();
#ifdef Q_WS_MAC
    const QRect screen = desktop->availableGeometry(desktop->screenNumber(m_editor->widget()));
#else
    const QRect screen = desktop->screenGeometry(desktop->screenNumber(m_editor->widget()));
#endif

    const QSize sz = m_popupFrame->sizeHint();
    QPoint pos = m_editor->cursorRect(m_startpos).topLeft();
    pos.setY(pos.y() - sz.height() - 1);

    if (pos.x() + sz.width() > screen.right())
        pos.setX(screen.right() - sz.width());

    m_popupFrame->move(pos);
}

CppQuickFixCollector::CppQuickFixCollector(CppModelManager *modelManager)
    : _modelManager(modelManager), _editor(0)
{ }

CppQuickFixCollector::~CppQuickFixCollector()
{ }

bool CppQuickFixCollector::supportsEditor(TextEditor::ITextEditable *editor)
{ return _modelManager->isCppEditor(editor); }

bool CppQuickFixCollector::triggersCompletion(TextEditor::ITextEditable *)
{ return false; }

int CppQuickFixCollector::startCompletion(TextEditor::ITextEditable *editor)
{
    _editor = editor;

    if (CppEditorSupport *extra = _modelManager->editorSupport(editor)) {
        const QList<QuickFixOperationPtr> quickFixes = extra->quickFixes();
        if (! quickFixes.isEmpty()) {
            int i = 0;
            foreach (QuickFixOperationPtr op, quickFixes) {
                TextEditor::CompletionItem item(this);
                item.text = op->description();
                item.data = QVariant::fromValue(i);
                _completions.append(item);
                ++i;
            }
            return editor->position();
        }
    }
    return -1;
}

void CppQuickFixCollector::completions(QList<TextEditor::CompletionItem> *completions)
{
    completions->append(_completions);
}

void CppQuickFixCollector::complete(const TextEditor::CompletionItem &item)
{
    CppEditorSupport *extra = _modelManager->editorSupport(_editor);
    const QList<QuickFixOperationPtr> quickFixes = extra->quickFixes();
    QuickFixOperationPtr quickFix = quickFixes.at(item.data.toInt());
    TextEditor::BaseTextEditor *ed = qobject_cast<TextEditor::BaseTextEditor *>(_editor->widget());
    quickFix->apply(ed->textCursor());
}

void CppQuickFixCollector::cleanup()
{
    _completions.clear();
}

CppCodeCompletion::CppCodeCompletion(CppModelManager *manager)
    : ICompletionCollector(manager),
      m_manager(manager),
      m_editor(0),
      m_startPosition(-1),
      m_caseSensitivity(Qt::CaseSensitive),
      m_autoInsertBrackets(true),
      m_partialCompletionEnabled(true),
      m_forcedCompletion(false),
      m_completionOperator(T_EOF_SYMBOL)
{
}

QIcon CppCodeCompletion::iconForSymbol(Symbol *symbol) const
{
    return m_icons.iconForSymbol(symbol);
}

Qt::CaseSensitivity CppCodeCompletion::caseSensitivity() const
{
    return m_caseSensitivity;
}

void CppCodeCompletion::setCaseSensitivity(Qt::CaseSensitivity caseSensitivity)
{
    m_caseSensitivity = caseSensitivity;
}

bool CppCodeCompletion::autoInsertBrackets() const
{
    return m_autoInsertBrackets;
}

void CppCodeCompletion::setAutoInsertBrackets(bool autoInsertBrackets)
{
    m_autoInsertBrackets = autoInsertBrackets;
}

bool CppCodeCompletion::isPartialCompletionEnabled() const
{
    return m_partialCompletionEnabled;
}

void CppCodeCompletion::setPartialCompletionEnabled(bool partialCompletionEnabled)
{
    m_partialCompletionEnabled = partialCompletionEnabled;
}

/*
  Searches backwards for an access operator.
*/
static int startOfOperator(TextEditor::ITextEditable *editor,
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
    }

    if (start == pos)
        return start;

    TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor *>(editor->widget());
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

    static CPlusPlus::TokenUnderCursor tokenUnderCursor;
    const SimpleToken tk = tokenUnderCursor(tc);

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
        const QList<SimpleToken> &tokens = tokenUnderCursor.tokens();
        int i = 0;
        for (; i < tokens.size(); ++i) {
            const SimpleToken &token = tokens.at(i);
            if (token.position() == tk.position()) {
                if (i == 0) // no token on the left, but might be on a previous line
                    break;
                const SimpleToken &previousToken = tokens.at(i - 1);
                if (previousToken.is(T_IDENTIFIER) || previousToken.is(T_GREATER)
                    || previousToken.is(T_SIGNAL) || previousToken.is(T_SLOT))
                    break;
            }
        }

        if (i == tokens.size()) {
            completionKind = T_EOF_SYMBOL;
            start = pos;
        }
    }
    // Check for include preprocessor directive
    else if (completionKind == T_STRING_LITERAL || completionKind == T_ANGLE_STRING_LITERAL || completionKind == T_SLASH) {
        bool include = false;
        const QList<SimpleToken> &tokens = tokenUnderCursor.tokens();
        if (tokens.size() >= 3) {
            if (tokens.at(0).is(T_POUND) && tokens.at(1).is(T_IDENTIFIER) && (tokens.at(2).is(T_STRING_LITERAL) ||
                                                                              tokens.at(2).is(T_ANGLE_STRING_LITERAL))) {
                QStringRef directive = tokens.at(1).text();
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

bool CppCodeCompletion::supportsEditor(TextEditor::ITextEditable *editor)
{ return m_manager->isCppEditor(editor); }

bool CppCodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    const int pos = editor->position();
    if (startOfOperator(editor, pos, /*token =*/ 0,
                        /*want function call=*/ true) != pos)
        return true;

    return false;
}

int CppCodeCompletion::startCompletion(TextEditor::ITextEditable *editor)
{
    TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor *>(editor->widget());
    if (! edit)
        return -1;

    m_editor = editor;
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

    int line = 0, column = 0;
    edit->convertPosition(editor->position(), &line, &column);
    // qDebug() << "line:" << line << "column:" << column;

    if (m_completionOperator == T_DOXY_COMMENT) {
        for (int i = 1; i < T_DOXY_LAST_TAG; ++i) {
            TextEditor::CompletionItem item(this);
            item.text.append(QString::fromLatin1(doxygenTagSpell(i)));
            item.icon = m_icons.keywordIcon();
            m_completions.append(item);
        }

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
    tc.setPosition(endOfExpression);

    if (m_completionOperator) {
        expression = expressionUnderCursor(tc);

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
            }
        }
    }

    //qDebug() << "***** expression:" << expression;

    const Snapshot snapshot = m_manager->snapshot();

    if (Document::Ptr thisDocument = snapshot.value(fileName)) {
        Symbol *lastVisibleSymbol = thisDocument->findSymbolAt(line, column);
        typeOfExpression.setSnapshot(m_manager->snapshot());

        QList<TypeOfExpression::Result> resolvedTypes = typeOfExpression(expression, thisDocument, lastVisibleSymbol,
                                                                         TypeOfExpression::Preprocess);
        LookupContext context = typeOfExpression.lookupContext();

        if (!typeOfExpression.expressionAST() && (! m_completionOperator ||
                                                    m_completionOperator == T_COLON_COLON)) {
            if (!m_completionOperator) {
                addKeywords();
                addMacros(context);
            }

            const QList<Scope *> scopes = context.expand(context.visibleScopes());
            foreach (Scope *scope, scopes) {
                for (unsigned i = 0; i < scope->symbolCount(); ++i) {
                    addCompletionItem(scope->symbolAt(i));
                }
            }
            return m_startPosition;
        }

        // qDebug() << "found" << resolvedTypes.count() << "symbols for expression:" << expression;

        if (resolvedTypes.isEmpty() && (m_completionOperator == T_SIGNAL ||
                                        m_completionOperator == T_SLOT)) {
            // Apply signal/slot completion on 'this'
            expression = QLatin1String("this");
            resolvedTypes = typeOfExpression(expression, thisDocument, lastVisibleSymbol);
            context = typeOfExpression.lookupContext();
        }

        if (! resolvedTypes.isEmpty()) {
            if (m_completionOperator == T_LPAREN &&
                completeConstructorOrFunction(resolvedTypes, context, endOfExpression, false)) {
                return m_startPosition;

            } else if ((m_completionOperator == T_DOT || m_completionOperator == T_ARROW) &&
                      completeMember(resolvedTypes, context)) {
                return m_startPosition;

            } else if (m_completionOperator == T_COLON_COLON && completeScope(resolvedTypes, context)) {
                return m_startPosition;

            } else if (m_completionOperator == T_SIGNAL      && completeSignal(resolvedTypes, context)) {
                return m_startPosition;

            } else if (m_completionOperator == T_SLOT        && completeSlot(resolvedTypes, context)) {
                return m_startPosition;
            }
        }

        if (m_completionOperator == T_LPAREN) {
            // Find the expression that precedes the current name
            int index = endOfExpression;
            while (m_editor->characterAt(index - 1).isSpace())
                --index;
            index = findStartOfName(index);

            QTextCursor tc(edit->document());
            tc.setPosition(index);

            const QString baseExpression = expressionUnderCursor(tc);

            // Resolve the type of this expression
            const QList<TypeOfExpression::Result> results =
                    typeOfExpression(baseExpression, thisDocument,
                                     lastVisibleSymbol,
                                     TypeOfExpression::Preprocess);

            // If it's a class, add completions for the constructors
            foreach (const TypeOfExpression::Result &result, results) {
                if (result.first->isClassType()) {
                    if (completeConstructorOrFunction(results, context, endOfExpression, true))
                        return m_startPosition;
                    break;
                }
            }
        }
    }

    // nothing to do.
    return -1;
}

bool CppCodeCompletion::completeConstructorOrFunction(const QList<TypeOfExpression::Result> &results,
                                                      const LookupContext &context,
                                                      int endOfExpression, bool toolTipOnly)
{
    QList<Function *> functions;

    foreach (const TypeOfExpression::Result &result, results) {
        FullySpecifiedType exprTy = result.first.simplified();

        if (Class *klass = exprTy->asClassType()) {
            Name *className = klass->name();
            if (! className)
                continue; // nothing to do for anonymoous classes.

            for (unsigned i = 0; i < klass->memberCount(); ++i) {
                Symbol *member = klass->memberAt(i);
                Name *memberName = member->name();

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
        foreach (const TypeOfExpression::Result &result, results) {
            FullySpecifiedType ty = result.first.simplified();

            if (Function *fun = ty->asFunctionType()) {

                if (! fun->name())
                    continue;
                else if (! functions.isEmpty() && functions.first()->scope() != fun->scope())
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
        ResolveExpression resolveExpression(context);
        ResolveClass resolveClass;
        Name *functionCallOp = context.control()->operatorNameId(OperatorNameId::FunctionCallOp);

        foreach (const TypeOfExpression::Result &result, results) {
            FullySpecifiedType ty = result.first.simplified();

            if (NamedType *namedTy = ty->asNamedType()) {
                const QList<Symbol *> classObjectCandidates = resolveClass(namedTy->name(), result, context);

                foreach (Symbol *classObjectCandidate, classObjectCandidates) {
                    if (Class *klass = classObjectCandidate->asClass()) {
                        const QList<TypeOfExpression::Result> overloads =
                                resolveExpression.resolveMember(functionCallOp, klass,
                                                                namedTy->name());

                        foreach (const TypeOfExpression::Result &overloadResult, overloads) {
                            FullySpecifiedType overloadTy = overloadResult.first.simplified();

                            if (Function *funTy = overloadTy->asFunctionType())
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
    if (! functions.isEmpty() && ! toolTipOnly) {

        // function definitions will only happen in class or namespace scope,
        // so get the current location's enclosing scope.

        // get current line and column
        TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor *>(m_editor->widget());
        int lineSigned = 0, columnSigned = 0;
        edit->convertPosition(m_editor->position(), &lineSigned, &columnSigned);
        unsigned line = lineSigned, column = columnSigned;

        // find a scope that encloses the current location, starting from the lastVisibileSymbol
        // and moving outwards
        Scope *sc = 0;
        if (context.symbol())
            sc = context.symbol()->scope();
        else if (context.thisDocument())
            sc = context.thisDocument()->globalSymbols();

        while (sc && sc->enclosingScope()) {
            unsigned startLine, startColumn;
            context.thisDocument()->translationUnit()->getPosition(sc->owner()->startOffset(), &startLine, &startColumn);
            unsigned endLine, endColumn;
            context.thisDocument()->translationUnit()->getPosition(sc->owner()->endOffset(), &endLine, &endColumn);

            if (startLine <= line && line <= endLine) {
                if ((startLine != line || startColumn <= column)
                    && (endLine != line || column <= endColumn))
                    break;
            }

            sc = sc->enclosingScope();
        }

        if (sc && (sc->isClassScope() || sc->isNamespaceScope())) {
            // It may still be a function call. If the whole line parses as a function
            // declaration, we should be certain that it isn't.
            bool autocompleteSignature = false;

            QTextCursor tc(edit->document());
            tc.setPosition(endOfExpression);
            BackwardsScanner bs(tc);
            QString possibleDecl = bs.mid(bs.startOfLine(bs.startToken())).trimmed().append("();");

            Document::Ptr doc = Document::create(QLatin1String("<completion>"));
            doc->setSource(possibleDecl.toLatin1());
            if (doc->parse(Document::ParseDeclaration)) {
                doc->check();
                if (SimpleDeclarationAST *sd = doc->translationUnit()->ast()->asSimpleDeclaration()) {
                    if (sd->declarators &&
                        sd->declarators->declarator->postfix_declarators
                        && sd->declarators->declarator->postfix_declarators->asFunctionDeclarator()) {
                        autocompleteSignature = true;
                    }
                }
            }

            if (autocompleteSignature) {
                // set up signature autocompletion
                foreach (Function *f, functions) {
                    Overview overview;
                    overview.setShowArgumentNames(true);

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

    if (! functions.empty()) {
        // set up function call tooltip

        // Recreate if necessary
        if (!m_functionArgumentWidget)
            m_functionArgumentWidget = new FunctionArgumentWidget;

        m_functionArgumentWidget->showFunctionHint(functions,
                                                   typeOfExpression.lookupContext(),
                                                   m_startPosition);
    }

    return false;
}

bool CppCodeCompletion::completeMember(const QList<TypeOfExpression::Result> &baseResults,
                                       const LookupContext &context)
{
    if (baseResults.isEmpty())
        return false;

    ResolveExpression resolveExpression(context);
    ResolveClass resolveClass;

    bool replacedDotOperator = false;
    const QList<TypeOfExpression::Result> classObjectResults =
            resolveExpression.resolveBaseExpression(baseResults,
                                                    m_completionOperator,
                                                    &replacedDotOperator);

    QList<Symbol *> classObjectCandidates;
    foreach (const TypeOfExpression::Result &r, classObjectResults) {
        FullySpecifiedType ty = r.first.simplified();

        if (Class *klass = ty->asClassType())
            classObjectCandidates.append(klass);

        else if (NamedType *namedTy = ty->asNamedType()) {
            Name *className = namedTy->name();
            const QList<Symbol *> classes = resolveClass(className, r, context);

            foreach (Symbol *c, classes) {
                if (Class *klass = c->asClass())
                    classObjectCandidates.append(klass);
            }
        }
    }

    if (replacedDotOperator && ! classObjectCandidates.isEmpty()) {
        // Replace . with ->
        int length = m_editor->position() - m_startPosition + 1;
        m_editor->setCurPos(m_startPosition - 1);
        m_editor->replace(length, QLatin1String("->"));
        ++m_startPosition;
    }

    completeClass(classObjectCandidates, context, /*static lookup = */ false);
    if (! m_completions.isEmpty())
        return true;

    return false;
}

bool CppCodeCompletion::completeScope(const QList<TypeOfExpression::Result> &results,
                                      const LookupContext &context)
{
    QList<Symbol *> classes, namespaces;

    foreach (TypeOfExpression::Result result, results) {
        FullySpecifiedType ty = result.first;

        if (Class *classTy = ty->asClassType())
            classes.append(classTy);

        else if (Namespace *namespaceTy = ty->asNamespaceType())
            namespaces.append(namespaceTy);
    }

    if (! classes.isEmpty())
        completeClass(classes, context);

    else if (! namespaces.isEmpty() && m_completions.isEmpty())
        completeNamespace(namespaces, context);

    return ! m_completions.isEmpty();
}

void CppCodeCompletion::addKeywords()
{
    // keyword completion items.
    for (int i = T_FIRST_KEYWORD; i < T_FIRST_OBJC_AT_KEYWORD; ++i) {
        TextEditor::CompletionItem item(this);
        item.text = QLatin1String(Token::name(i));
        item.icon = m_icons.keywordIcon();
        m_completions.append(item);
    }
}

void CppCodeCompletion::addMacros(const LookupContext &context)
{
    QSet<QString> processed;
    QSet<QString> definedMacros;

    addMacros_helper(context, context.thisDocument()->fileName(),
                     &processed, &definedMacros);

    foreach (const QString &macroName, definedMacros) {
        TextEditor::CompletionItem item(this);
        item.text = macroName;
        item.icon = m_icons.macroIcon();
        m_completions.append(item);
    }
}

void CppCodeCompletion::addMacros_helper(const LookupContext &context,
                                         const QString &fileName,
                                         QSet<QString> *processed,
                                         QSet<QString> *definedMacros)
{
    Document::Ptr doc = context.document(fileName);

    if (! doc || processed->contains(doc->fileName()))
        return;

    processed->insert(doc->fileName());

    foreach (const Document::Include &i, doc->includes()) {
        addMacros_helper(context, i.fileName(), processed, definedMacros);
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
    if (TextEditor::CompletionItem item = toCompletionItem(symbol))
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

void CppCodeCompletion::completeNamespace(const QList<Symbol *> &candidates,
                                          const LookupContext &context)
{
    QList<Scope *> todo;
    QList<Scope *> visibleScopes = context.visibleScopes();
    foreach (Symbol *candidate, candidates) {
        if (Namespace *ns = candidate->asNamespace())
            context.expand(ns->members(), visibleScopes, &todo);
    }

    foreach (Scope *scope, todo) {
        addCompletionItem(scope->owner());

        for (unsigned i = 0; i < scope->symbolCount(); ++i) {
            addCompletionItem(scope->symbolAt(i));
        }
    }
}

void CppCodeCompletion::completeClass(const QList<Symbol *> &candidates,
                                      const LookupContext &context,
                                      bool staticLookup)
{
    if (candidates.isEmpty())
        return;

    Class *klass = candidates.first()->asClass();

    QList<Scope *> todo;
    context.expand(klass->members(), context.visibleScopes(), &todo);

    foreach (Scope *scope, todo) {
        addCompletionItem(scope->owner());

        for (unsigned i = 0; i < scope->symbolCount(); ++i) {
            Symbol *symbol = scope->symbolAt(i);

            if (symbol->type().isFriend())
                continue;
            else if (! staticLookup && (symbol->isTypedef() ||
                                        symbol->isEnum()    ||
                                        symbol->isClass()))
                continue;

            addCompletionItem(symbol);
        }
    }
}

bool CppCodeCompletion::completeQtMethod(const QList<TypeOfExpression::Result> &results,
                                         const LookupContext &context,
                                         bool wantSignals)
{
    if (results.isEmpty())
        return false;

    ResolveClass resolveClass;

    ConvertToCompletionItem toCompletionItem(this);
    Overview o;
    o.setShowReturnTypes(false);
    o.setShowArgumentNames(false);
    o.setShowFunctionSignatures(true);

    QSet<QString> signatures;
    foreach (const TypeOfExpression::Result &p, results) {
        FullySpecifiedType ty = p.first.simplified();

        if (PointerType *ptrTy = ty->asPointerType())
            ty = ptrTy->elementType().simplified();
        else
            continue; // not a pointer or a reference to a pointer.

        NamedType *namedTy = ty->asNamedType();
        if (! namedTy) // not a class name.
            continue;

        const QList<Symbol *> classObjects =
                resolveClass(namedTy->name(), p, context);

        if (classObjects.isEmpty())
            continue;

        Class *klass = classObjects.first()->asClass();

        QList<Scope *> todo;
        const QList<Scope *> visibleScopes = context.visibleScopes(p);
        context.expand(klass->members(), visibleScopes, &todo);

        foreach (Scope *scope, todo) {
            if (! scope->isClassScope())
                continue;

            for (unsigned i = 0; i < scope->symbolCount(); ++i) {
                Symbol *member = scope->symbolAt(i);
                Function *fun = member->type()->asFunctionType();
                if (! fun)
                    continue;
                if (wantSignals && ! fun->isSignal())
                    continue;
                else if (! wantSignals && ! fun->isSlot())
                    continue;
                if (TextEditor::CompletionItem item = toCompletionItem(fun)) {
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

    if (length == 0)
        *completions = m_completions;
    else if (length > 0) {
        const QString key = m_editor->textAt(m_startPosition, length);

        /* Close on the trailing slash for include completion, to enable the slash to
         * trigger a new completion list. */
        if ((m_completionOperator == T_STRING_LITERAL ||
             m_completionOperator == T_ANGLE_STRING_LITERAL) && key.endsWith(QLatin1Char('/')))
            return;

        if (m_completionOperator != T_LPAREN) {
            /*
             * This code builds a regular expression in order to more intelligently match
             * camel-case style. This means upper-case characters will be rewritten as follows:
             *
             *   A => [a-z0-9_]*A (for any but the first capital letter)
             *
             * Meaning it allows any sequence of lower-case characters to preceed an
             * upper-case character. So for example gAC matches getActionController.
             */
            QString keyRegExp;
            keyRegExp += QLatin1Char('^');
            bool first = true;
            foreach (const QChar &c, key) {
                if (c.isUpper() && !first) {
                    keyRegExp += QLatin1String("[a-z0-9_]*");
                    keyRegExp += c;
                } else {
                    keyRegExp += QRegExp::escape(c);
                }
                first = false;
            }
            const QRegExp regExp(keyRegExp, m_caseSensitivity);

            foreach (TextEditor::CompletionItem item, m_completions) {
                if (regExp.indexIn(item.text) == 0) {
                    item.relevance = (key.length() > 0 &&
                                         item.text.startsWith(key, Qt::CaseInsensitive)) ? 1 : 0;
                    (*completions) << item;
                }
            }
        } else if (m_completionOperator == T_LPAREN ||
                   m_completionOperator == T_SIGNAL ||
                   m_completionOperator == T_SLOT) {
            foreach (TextEditor::CompletionItem item, m_completions) {
                if (item.text.startsWith(key, Qt::CaseInsensitive)) {
                    (*completions) << item;
                }
            }
        }
    }
}

void CppCodeCompletion::complete(const TextEditor::CompletionItem &item)
{
    Symbol *symbol = 0;

    if (item.data.isValid())
        symbol = item.data.value<Symbol *>();

    QString toInsert;
    QString extraChars;
    int extraLength = 0;
    int cursorOffset = 0;

    bool autoParenthesesEnabled = true;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        toInsert = item.text;
        extraChars += QLatin1Char(')');
    } else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL) {
        toInsert = item.text;
        if (!toInsert.endsWith(QLatin1Char('/')))
            extraChars += QLatin1Char((m_completionOperator == T_ANGLE_STRING_LITERAL) ? '>' : '"');
    } else {
        toInsert = item.text;

        //qDebug() << "current symbol:" << overview.prettyName(symbol->name())
        //<< overview.prettyType(symbol->type());

        if (m_autoInsertBrackets && symbol && symbol->type()) {
            if (Function *function = symbol->type()->asFunctionType()) {
                // If the member is a function, automatically place the opening parenthesis,
                // except when it might take template parameters.
                if (! function->hasReturnType() && (function->identity() && !function->identity()->isDestructorNameId())) {
                    // Don't insert any magic, since the user might have just wanted to select the class

                } else if (function->templateParameterCount() != 0) {
                    // If there are no arguments, then we need the template specification
                    if (function->argumentCount() == 0) {
                        extraChars += QLatin1Char('<');
                    }
                } else if (! function->isAmbiguous()) {
                    extraChars += QLatin1Char('(');

                    // If the function doesn't return anything, automatically place the semicolon,
                    // unless we're doing a scope completion (then it might be function definition).
                    bool endWithSemicolon = function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON;

                    // If the function takes no arguments, automatically place the closing parenthesis
                    if (item.duplicateCount == 0 && ! function->hasArguments()) {
                        extraChars += QLatin1Char(')');
                        if (endWithSemicolon)
                            extraChars += QLatin1Char(';');
                    } else if (autoParenthesesEnabled) {
                        const QChar lookAhead = m_editor->characterAt(m_editor->position() + 1);
                        if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                            extraChars += QLatin1Char(')');
                            --cursorOffset;
                            if (endWithSemicolon) {
                                extraChars += QLatin1Char(';');
                                --cursorOffset;
                            }
                        }
                    }
                }
            }
        }

        if (m_autoInsertBrackets && item.data.canConvert<CompleteFunctionDeclaration>()) {
            // everything from the closing parenthesis on are extra chars, to
            // make sure an auto-inserted ")" gets replaced by ") const" if necessary
            int closingParen = toInsert.lastIndexOf(QLatin1Char(')'));
            extraChars = toInsert.mid(closingParen);
            toInsert.truncate(closingParen);
        }
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
    m_editor->setCurPos(m_startPosition);
    m_editor->replace(length, toInsert);
    if (cursorOffset)
        m_editor->setCurPos(m_editor->position() + cursorOffset);
}

bool CppCodeCompletion::partiallyComplete(const QList<TextEditor::CompletionItem> &completionItems)
{
    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        return false;
    } else if (completionItems.count() == 1) {
        complete(completionItems.first());
        return true;
    } else if (m_partialCompletionEnabled && m_completionOperator != T_LPAREN) {
        // Compute common prefix
        QString firstKey = completionItems.first().text;
        QString lastKey = completionItems.last().text;
        const int length = qMin(firstKey.length(), lastKey.length());
        firstKey.truncate(length);
        lastKey.truncate(length);

        while (firstKey != lastKey) {
            firstKey.chop(1);
            lastKey.chop(1);
        }

        int typedLength = m_editor->position() - m_startPosition;
        if (!firstKey.isEmpty() && firstKey.length() > typedLength) {
            m_editor->setCurPos(m_startPosition);
            m_editor->replace(typedLength, firstKey);
        }
    }

    return false;
}

void CppCodeCompletion::cleanup()
{
    m_completions.clear();

    // Set empty map in order to avoid referencing old versions of the documents
    // until the next completion
    typeOfExpression.setSnapshot(Snapshot());
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

#include "cppcodecompletion.moc"
