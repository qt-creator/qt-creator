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

#include "cppcodecompletion.h"

#include "cppmodelmanager.h"

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
#include <cplusplus/Overview.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/TokenUnderCursor.h>

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <texteditor/itexteditor.h>
#include <texteditor/itexteditable.h>
#include <utils/qtcassert.h>
#include <texteditor/basetexteditor.h>

#include <QtCore/QDebug>
#include <QtCore/QMap>
#include <QtCore/QFile>
#include <QtGui/QAction>
#include <QtGui/QKeyEvent>
#include <QtGui/QLabel>
#include <QtGui/QVBoxLayout>
#include <QtGui/QApplication>

using namespace CPlusPlus;

namespace CppTools {
namespace Internal {

class FunctionArgumentWidget : public QLabel
{
public:
    FunctionArgumentWidget();
    void showFunctionHint(Function *functionSymbol, const LookupContext &context);

protected:
    bool eventFilter(QObject *obj, QEvent *e);

private:
    void update();
    void close();
    void updateHintText();

    int m_startpos;
    int m_currentarg;

    TextEditor::ITextEditor *m_editor;

    QFrame *m_popupFrame;
    Function *m_item;
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
            _item.m_data = QVariant::fromValue(symbol);
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
        item.m_text = overview.prettyName(name);
        item.m_icon = _collector->iconForSymbol(_symbol);
        return item;
    }

    virtual void visit(NameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(TemplateNameId *name)
    {
        _item = newCompletionItem(name);
        _item.m_text = QLatin1String(name->identifier()->chars());
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


} // namespace Internal
} // namespace CppTools

using namespace CppTools::Internal;

FunctionArgumentWidget::FunctionArgumentWidget()
    : m_item(0)
{
    QObject *editorObject = Core::EditorManager::instance()->currentEditor();
    m_editor = qobject_cast<TextEditor::ITextEditor *>(editorObject);

    m_popupFrame = new QFrame(0, Qt::ToolTip | Qt::WindowStaysOnTopHint);
    m_popupFrame->setFocusPolicy(Qt::NoFocus);
    m_popupFrame->setAttribute(Qt::WA_DeleteOnClose);

    setFrameStyle(QFrame::Box);
    setFrameShadow(QFrame::Plain);

    setParent(m_popupFrame);
    setFocusPolicy(Qt::NoFocus);

    QVBoxLayout *layout = new QVBoxLayout();
    layout->addWidget(this);
    layout->setMargin(0);
    m_popupFrame->setLayout(layout);

    QPalette pal = palette();
    setAutoFillBackground(true);
    pal.setColor(QPalette::Background, QColor(255, 255, 220));
    setPalette(pal);

    setTextFormat(Qt::RichText);
    setMargin(1);
}

void FunctionArgumentWidget::showFunctionHint(Function *functionSymbol,
                                              const LookupContext &context)
{
    m_popupFrame->hide();

    m_item = functionSymbol;
    m_context = context;
    m_startpos = m_editor->position();

    // update the text
    m_currentarg = -1;
    update();

    QPoint pos = m_editor->cursorRect().topLeft();
    pos.setY(pos.y() - sizeHint().height());
    m_popupFrame->move(pos);
    m_popupFrame->show();

    qApp->installEventFilter(this);
}

void FunctionArgumentWidget::update()
{
    int curpos = m_editor->position();
    if (curpos < m_startpos) {
        close();
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
        close();
}

bool FunctionArgumentWidget::eventFilter(QObject *obj, QEvent *e)
{
    switch (e->type()) {
    case QEvent::KeyRelease:
        {
            if (static_cast<QKeyEvent*>(e)->key() == Qt::Key_Escape) {
                close();
                return false;
            }
            update();
            break;
        }
    case QEvent::WindowDeactivate:
    case QEvent::Leave:
    case QEvent::FocusOut:
        {
            if (obj != m_editor->widget())
                break;
        }
    case QEvent::MouseButtonPress:
    case QEvent::MouseButtonRelease:
    case QEvent::MouseButtonDblClick:
    case QEvent::Wheel:
        close();
        break;
    default:
        break;
    }

    return false;
}

void FunctionArgumentWidget::close()
{
    m_popupFrame->close();
}

void FunctionArgumentWidget::updateHintText()
{
    Overview overview;
    overview.setShowReturnTypes(true);
    overview.setShowArgumentNames(true);
    overview.setMarkArgument(m_currentarg + 1);
    QString text = overview(m_item->type(), m_item->name());
    setText(text);
}

CppCodeCompletion::CppCodeCompletion(CppModelManager *manager)
    : ICompletionCollector(manager),
      m_manager(manager),
      m_caseSensitivity(Qt::CaseSensitive),
      m_autoInsertBraces(true),
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

bool CppCodeCompletion::autoInsertBraces() const
{
    return m_autoInsertBraces;
}

void CppCodeCompletion::setAutoInsertBraces(bool autoInsertBraces)
{
    m_autoInsertBraces = autoInsertBraces;
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
  Searches beckward for an access operator.
*/
static int startOfOperator(TextEditor::ITextEditable *editor,
                           int pos, unsigned *kind,
                           bool wantFunctionCall)
{
    const QChar ch  = pos > -1 ? editor->characterAt(pos - 1) : QChar();
    const QChar ch2 = pos >  0 ? editor->characterAt(pos - 2) : QChar();
    const QChar ch3 = pos >  1 ? editor->characterAt(pos - 3) : QChar();

    int start = pos;

    if        (ch2 != QLatin1Char('.') && ch == QLatin1Char('.')) {
        if (kind)
            *kind = T_DOT;
        --start;
    } else if (wantFunctionCall        && ch == QLatin1Char('(')) {
        if (kind)
            *kind = T_LPAREN;
        --start;
    } else if (ch2 == QLatin1Char(':') && ch == QLatin1Char(':')) {
        if (kind)
            *kind = T_COLON_COLON;
        start -= 2;
    } else if (ch2 == QLatin1Char('-') && ch == QLatin1Char('>')) {
        if (kind)
            *kind = T_ARROW;
        start -= 2;
    } else if (ch2 == QLatin1Char('.') && ch == QLatin1Char('*')) {
        if (kind)
            *kind = T_DOT_STAR;
        start -= 2;
    } else if (ch3 == QLatin1Char('-') && ch2 == QLatin1Char('>') && ch == QLatin1Char('*')) {
        if (kind)
            *kind = T_ARROW_STAR;
        start -= 3;
    }

    if (start != pos) {
        TextEditor::BaseTextEditor *edit = qobject_cast<TextEditor::BaseTextEditor *>(editor->widget());
        QTextCursor tc(edit->textCursor());
        tc.setPosition(pos);
        static CPlusPlus::TokenUnderCursor tokenUnderCursor;
        const SimpleToken tk = tokenUnderCursor(tc);
        if (tk.is(T_COMMENT) || tk.isLiteral()) {
            if (kind)
                *kind = T_EOF_SYMBOL;
            return pos;
        }
    }

    return start;
}

bool CppCodeCompletion::triggersCompletion(TextEditor::ITextEditable *editor)
{
    if (! m_manager->isCppEditor(editor)) // ### remove me
        return false;

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
    m_startPosition = findStartOfName();
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

    ExpressionUnderCursor expressionUnderCursor;
    QString expression;

    if (m_completionOperator) {
        QTextCursor tc(edit->document());
        tc.setPosition(endOfExpression);
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
            }
        }
    }

    //if (! expression.isEmpty())
        //qDebug() << "***** expression:" << expression;

    const Snapshot snapshot = m_manager->snapshot();

    if (Document::Ptr thisDocument = snapshot.value(fileName)) {
        Symbol *symbol = thisDocument->findSymbolAt(line, column);

        typeOfExpression.setSnapshot(m_manager->snapshot());

        QList<TypeOfExpression::Result> resolvedTypes = typeOfExpression(expression, thisDocument, symbol,
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
            resolvedTypes = typeOfExpression(expression, thisDocument, symbol);
            context = typeOfExpression.lookupContext();
        }

        if (! resolvedTypes.isEmpty()) {
            FullySpecifiedType exprTy = resolvedTypes.first().first;

            if (exprTy->isReferenceType())
                exprTy = exprTy->asReferenceType()->elementType();

            if (m_completionOperator == T_LPAREN && completeConstructorOrFunction(exprTy, resolvedTypes)) {
                return m_startPosition;
            } else if ((m_completionOperator == T_DOT || m_completionOperator == T_ARROW) &&
                      completeMember(resolvedTypes, context)) {
                return m_startPosition;
            } else if (m_completionOperator == T_COLON_COLON && completeScope(resolvedTypes, context)) {
                return m_startPosition;
            } else if (m_completionOperator == T_SIGNAL && completeSignal(exprTy, resolvedTypes, context)) {
                return m_startPosition;
            } else if (m_completionOperator == T_SLOT && completeSlot(exprTy, resolvedTypes, context)) {
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
            QString baseExpression = expressionUnderCursor(tc);

            // Resolve the type of this expression
            QList<TypeOfExpression::Result> results =
                    typeOfExpression(baseExpression, thisDocument, symbol, TypeOfExpression::Preprocess);

            // If it's a class, add completions for the constructors
            foreach (const TypeOfExpression::Result &result, results) {
                if (result.first->isClass()) {
                    FullySpecifiedType exprTy = result.first;
                    if (completeConstructorOrFunction(exprTy, QList<TypeOfExpression::Result>()))
                        return m_startPosition;
                    break;
                }
            }
        }
    }

    // nothing to do.
    return -1;
}

bool CppCodeCompletion::completeConstructorOrFunction(FullySpecifiedType exprTy,
                                                      const QList<TypeOfExpression::Result> &resolvedTypes)
{
    ConvertToCompletionItem toCompletionItem(this);
    Overview o;
    o.setShowReturnTypes(true);
    o.setShowArgumentNames(true);

    if (Class *klass = exprTy->asClass()) {
        for (unsigned i = 0; i < klass->memberCount(); ++i) {
            Symbol *member = klass->memberAt(i);
            if (! member->type()->isFunction())
                continue;
            else if (! member->identity())
                continue;
            else if (! member->identity()->isEqualTo(klass->identity()))
                continue;
            if (TextEditor::CompletionItem item = toCompletionItem(member)) {
                item.m_text = o(member->type(), member->name());
                m_completions.append(item);
            }
        }
    } else {
        QSet<QString> signatures;
        foreach (TypeOfExpression::Result p, resolvedTypes) {
            FullySpecifiedType ty = p.first;
            if (Function *fun = ty->asFunction()) {
                if (TextEditor::CompletionItem item = toCompletionItem(fun)) {
                    QString signature;
                    signature += overview.prettyName(fun->name());
                    signature += overview.prettyType(fun->type());
                    if (signatures.contains(signature))
                        continue;
                    signatures.insert(signature);

                    item.m_text = o(ty, fun->name());
                    m_completions.append(item);
                }
            }
        }
    }

    // If there is only one item, show the function argument widget immediately
    if (m_completions.size() == 1)
        complete(m_completions.takeFirst());

    return ! m_completions.isEmpty();
}

bool CppCodeCompletion::completeMember(const QList<TypeOfExpression::Result> &results,
                                       const LookupContext &context)
{
    if (results.isEmpty())
        return false;

    TypeOfExpression::Result result = results.first();
    QList<Symbol *> classObjectCandidates;

    if (m_completionOperator == T_ARROW)  {
        FullySpecifiedType ty = result.first;

        if (ReferenceType *refTy = ty->asReferenceType())
            ty = refTy->elementType();

        if (Class *classTy = ty->asClass()) {
            Symbol *symbol = result.second;
            if (symbol && ! symbol->isClass())
                classObjectCandidates.append(classTy);
        } else if (NamedType *namedTy = ty->asNamedType()) {
            // ### This code is pretty slow.
            const QList<Symbol *> candidates = context.resolve(namedTy->name());
            foreach (Symbol *candidate, candidates) {
                if (candidate->isTypedef()) {
                    ty = candidate->type();
                    const ResolveExpression::Result r(ty, candidate);
                    result = r;
                    break;
                }
            }
        }

        if (NamedType *namedTy = ty->asNamedType()) {
            ResolveExpression resolveExpression(context);
            ResolveClass resolveClass;

            const QList<Symbol *> candidates = resolveClass(result, context);
            foreach (Symbol *classObject, candidates) {
                const QList<TypeOfExpression::Result> overloads =
                        resolveExpression.resolveArrowOperator(result, namedTy,
                                                               classObject->asClass());

                foreach (TypeOfExpression::Result r, overloads) {
                    FullySpecifiedType ty = r.first;
                    Function *funTy = ty->asFunction();
                    if (! funTy)
                        continue;

                    ty = funTy->returnType();

                    if (ReferenceType *refTy = ty->asReferenceType())
                        ty = refTy->elementType();

                    if (PointerType *ptrTy = ty->asPointerType()) {
                        if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                            const QList<Symbol *> classes =
                                    resolveClass(namedTy, result, context);

                            foreach (Symbol *c, classes) {
                                if (! classObjectCandidates.contains(c))
                                    classObjectCandidates.append(c);
                            }
                        }
                    }
                }
            }
        } else if (PointerType *ptrTy = ty->asPointerType()) {
            if (NamedType *namedTy = ptrTy->elementType()->asNamedType()) {
                ResolveClass resolveClass;

                const QList<Symbol *> classes = resolveClass(namedTy, result,
                                                             context);

                foreach (Symbol *c, classes) {
                    if (! classObjectCandidates.contains(c))
                        classObjectCandidates.append(c);
                }
            } else if (Class *classTy = ptrTy->elementType()->asClass()) {
                // typedef struct { int x } *Ptr;
                // Ptr p;
                // p->
                classObjectCandidates.append(classTy);
            }
        }
    } else if (m_completionOperator == T_DOT) {
        FullySpecifiedType ty = result.first;

        if (ReferenceType *refTy = ty->asReferenceType())
            ty = refTy->elementType();

        NamedType *namedTy = 0;

        if (ArrayType *arrayTy = ty->asArrayType()) {
            // Replace . with [0]. when `ty' is an array type.
            FullySpecifiedType elementTy = arrayTy->elementType();

            if (ReferenceType *refTy = elementTy->asReferenceType())
                elementTy = refTy->elementType();

            if (elementTy->isNamedType() || elementTy->isPointerType()) {
                ty = elementTy;

                const int length = m_editor->position() - m_startPosition + 1;
                m_editor->setCurPos(m_startPosition - 1);
                m_editor->replace(length, QLatin1String("[0]."));
                m_startPosition += 3;
            }
        }

        if (PointerType *ptrTy = ty->asPointerType()) {
            if (ptrTy->elementType()->isNamedType()) {
                // Replace . with ->
                int length = m_editor->position() - m_startPosition + 1;
                m_editor->setCurPos(m_startPosition - 1);
                m_editor->replace(length, QLatin1String("->"));
                ++m_startPosition;
                namedTy = ptrTy->elementType()->asNamedType();
            }
        } else if (Class *classTy = ty->asClass()) {
            Symbol *symbol = result.second;
            if (symbol && ! symbol->isClass())
                classObjectCandidates.append(classTy);
        } else {
            namedTy = ty->asNamedType();
            if (! namedTy) {
                Function *fun = ty->asFunction();
                if (fun && (fun->scope()->isBlockScope() || fun->scope()->isNamespaceScope()))
                    namedTy = fun->returnType()->asNamedType();
            }
        }

        if (namedTy) {
            ResolveClass resolveClass;
            const QList<Symbol *> symbols = resolveClass(namedTy, result,
                                                         context);
            foreach (Symbol *symbol, symbols) {
                if (classObjectCandidates.contains(symbol))
                    continue;
                if (Class *klass = symbol->asClass())
                    classObjectCandidates.append(klass);
            }
        }
    }

    completeClass(classObjectCandidates, context, /*static lookup = */ false);
    if (! m_completions.isEmpty())
        return true;

    return false;
}

bool CppCodeCompletion::completeScope(const QList<TypeOfExpression::Result> &results,
                                      const LookupContext &context)
{
    if (results.isEmpty())
        return false; // nothing to do.

    // Search for a class or a namespace.
    TypeOfExpression::Result result;
    foreach (result, results) {
        FullySpecifiedType ty = result.first;

        if (ty->isClass() || ty->isNamespace())
            break;
    }

    FullySpecifiedType exprTy = result.first;
    if (! exprTy) {
        return false;
    } else if (exprTy->asNamespace()) {
        QList<Symbol *> candidates;
        foreach (TypeOfExpression::Result p, results) {
            if (Namespace *ns = p.first->asNamespace())
                candidates.append(ns);
        }
        completeNamespace(candidates, context);
    } else if (exprTy->isClass()) {
        QList<Symbol *> candidates;
        foreach (TypeOfExpression::Result p, results) {
            if (Class *k = p.first->asClass())
                candidates.append(k);
        }
        completeClass(candidates, context);
    } else if (Symbol *symbol = result.second) {
        if (symbol->isTypedef()) {
            ResolveClass resolveClass;
            const QList<Symbol *> candidates = resolveClass(result, context);
            completeClass(candidates, context);
        }
    }

    return ! m_completions.isEmpty();
}

void CppCodeCompletion::addKeywords()
{
    // keyword completion items.
    for (int i = T_FIRST_KEYWORD; i < T_FIRST_OBJC_AT_KEYWORD; ++i) {
        TextEditor::CompletionItem item(this);
        item.m_text = QLatin1String(Token::name(i));
        item.m_icon = m_icons.keywordIcon();
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
        item.m_text = macroName;
        item.m_icon = m_icons.macroIcon();
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

bool CppCodeCompletion::completeQtMethod(CPlusPlus::FullySpecifiedType,
                                         const QList<TypeOfExpression::Result> &results,
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
    foreach (TypeOfExpression::Result p, results) {
        FullySpecifiedType ty = p.first;
        if (ReferenceType *refTy = ty->asReferenceType())
            ty = refTy->elementType();
        if (PointerType *ptrTy = ty->asPointerType())
            ty = ptrTy->elementType();
        else
            continue; // not a pointer or a reference to a pointer.

        NamedType *namedTy = ty->asNamedType();
        if (! namedTy) // not a class name.
            continue;

        const QList<Symbol *> classObjects =
                resolveClass(namedTy, p, context);

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
                Function *fun = member->type()->asFunction();
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

                            ci.m_text = signature; // fix the completion item.
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
                } else if (m_caseSensitivity == Qt::CaseInsensitive && c.isLower()) {
                    keyRegExp += QLatin1Char('[');
                    keyRegExp += c;
                    keyRegExp += c.toUpper();
                    keyRegExp += QLatin1Char(']');
                } else {
                    keyRegExp += QRegExp::escape(c);
                }
                first = false;
            }
            const QRegExp regExp(keyRegExp, Qt::CaseSensitive);

            foreach (TextEditor::CompletionItem item, m_completions) {
                if (regExp.indexIn(item.m_text) == 0) {
                    item.m_relevance = (key.length() > 0 &&
                                         item.m_text.startsWith(key, Qt::CaseInsensitive)) ? 1 : 0;
                    (*completions) << item;
                }
            }
        } else if (m_completionOperator == T_LPAREN ||
                   m_completionOperator == T_SIGNAL ||
                   m_completionOperator == T_SLOT) {
            foreach (TextEditor::CompletionItem item, m_completions) {
                if (item.m_text.startsWith(key, Qt::CaseInsensitive)) {
                    (*completions) << item;
                }
            }
        }
    }
}

void CppCodeCompletion::complete(const TextEditor::CompletionItem &item)
{
    Symbol *symbol = 0;

    if (item.m_data.isValid())
        symbol = item.m_data.value<Symbol *>();

    // qDebug() << "*** complete symbol:" << symbol->fileName() << symbol->line();

    if (m_completionOperator == T_LPAREN) {
        if (symbol) {
            Function *function = symbol->type()->asFunction();
            QTC_ASSERT(function, return);

            // Recreate if necessary
            if (!m_functionArgumentWidget)
                m_functionArgumentWidget = new FunctionArgumentWidget;

            m_functionArgumentWidget->showFunctionHint(function, typeOfExpression.lookupContext());
        }
    } else if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        QString toInsert = item.m_text;
        toInsert += QLatin1Char(')');
        // Insert the remainder of the name
        int length = m_editor->position() - m_startPosition;
        m_editor->setCurPos(m_startPosition);
        m_editor->replace(length, toInsert);
    } else {
        QString toInsert = item.m_text;
        int extraLength = 0;

        //qDebug() << "current symbol:" << overview.prettyName(symbol->name())
        //<< overview.prettyType(symbol->type());

        if (m_autoInsertBraces && symbol) {
            QString extraChars;

            if (Function *function = symbol->type()->asFunction()) {
                // If the member is a function, automatically place the opening parenthesis,
                // except when it might take template parameters.
                const bool hasReturnType = function->returnType().isValid()  ||
                                           function->returnType().isSigned() ||
                                           function->returnType().isUnsigned();
                if (! hasReturnType && (function->identity() && !function->identity()->isDestructorNameId())) {
                    // Don't insert any magic, since the user might have just wanted to select the class

                } else if (function->templateParameterCount() != 0) {
                    // If there are no arguments, then we need the template specification
                    if (function->argumentCount() == 0) {
                        extraChars += QLatin1Char('<');
                    }
                } else {
                    extraChars += QLatin1Char('(');

                    // If the function takes no arguments, automatically place the closing parenthesis
                    if (function->argumentCount() == 0 || (function->argumentCount() == 1 &&
                                                           function->argumentAt(0)->type()->isVoidType())) {
                        extraChars += QLatin1Char(')');

                        // If the function doesn't return anything, automatically place the semicolon,
                        // unless we're doing a scope completion (then it might be function definition).
                        if (function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON) {
                            extraChars += QLatin1Char(';');
                        }
                    }
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
        }

        // Insert the remainder of the name
        int length = m_editor->position() - m_startPosition + extraLength;
        m_editor->setCurPos(m_startPosition);
        m_editor->replace(length, toInsert);
    }
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
        QString firstKey = completionItems.first().m_text;
        QString lastKey = completionItems.last().m_text;
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
