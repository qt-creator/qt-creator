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

#include "cppcompletionassist.h"
#include "cppmodelmanager.h"
#include "cpptoolsconstants.h"
#include "cppdoxygen.h"

#include <coreplugin/icore.h>
#include <cppeditor/cppeditorconstants.h>
#include <texteditor/codeassist/basicproposalitem.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/convenience.h>
#include <texteditor/snippets/snippet.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>

#include <utils/qtcassert.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/CppRewriter.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/MatchingText.h>
#include <cplusplus/Overview.h>
#include <cplusplus/ResolveExpression.h>

#include <QLatin1String>
#include <QTextCursor>
#include <QTextDocument>
#include <QIcon>

using namespace CPlusPlus;
using namespace CppEditor;
using namespace CppTools;
using namespace CppTools::Internal;
using namespace TextEditor;

namespace CppTools {
namespace Internal {

struct CompleteFunctionDeclaration
{
    explicit CompleteFunctionDeclaration(Function *f = 0)
        : function(f)
    {}

    Function *function;
};

// ---------------------
// CppAssistProposalItem
// ---------------------
class CppAssistProposalItem : public TextEditor::BasicProposalItem
{
public:
    CppAssistProposalItem() :
        m_isOverloaded(false) {}

    virtual bool prematurelyApplies(const QChar &c) const;
    virtual void applyContextualContent(TextEditor::BaseTextEditor *editor,
                                        int basePosition) const;

    bool isOverloaded() const { return m_isOverloaded; }
    void markAsOverloaded() { m_isOverloaded = true; }
    void keepCompletionOperator(unsigned compOp) { m_completionOperator = compOp; }
    void keepTypeOfExpression(const QSharedPointer<TypeOfExpression> &typeOfExp)
    { m_typeOfExpression = typeOfExp; }

private:
    bool m_isOverloaded;
    mutable QChar m_typedChar;
    unsigned m_completionOperator;
    QSharedPointer<TypeOfExpression> m_typeOfExpression;
};

} // Internal
} // CppTools

Q_DECLARE_METATYPE(CppTools::Internal::CompleteFunctionDeclaration)

bool CppAssistProposalModel::isSortable(const QString &prefix) const
{
    if (m_completionOperator != T_EOF_SYMBOL)
        return true;

    return !prefix.isEmpty();
}

IAssistProposalItem *CppAssistProposalModel::proposalItem(int index) const
{
    BasicProposalItem *item =
        static_cast<BasicProposalItem *>(BasicProposalItemListModel::proposalItem(index));
    if (!item->data().canConvert<QString>()) {
        CppAssistProposalItem *cppItem = static_cast<CppAssistProposalItem *>(item);
        cppItem->keepCompletionOperator(m_completionOperator);
        cppItem->keepTypeOfExpression(m_typeOfExpression);
    }
    return item;
}

bool CppAssistProposalItem::prematurelyApplies(const QChar &typedChar) const
{
    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        if (typedChar == QLatin1Char('(') || typedChar == QLatin1Char(',')) {
            m_typedChar = typedChar;
            return true;
        }
    } else if (m_completionOperator == T_STRING_LITERAL
               || m_completionOperator == T_ANGLE_STRING_LITERAL) {
        if (typedChar == QLatin1Char('/') && text().endsWith(QLatin1Char('/'))) {
            m_typedChar = typedChar;
            return true;
        }
    } else if (data().value<Symbol *>()) {
        if (typedChar == QLatin1Char(':')
                || typedChar == QLatin1Char(';')
                || typedChar == QLatin1Char('.')
                || typedChar == QLatin1Char(',')
                || typedChar == QLatin1Char('(')) {
            m_typedChar = typedChar;
            return true;
        }
    } else if (data().canConvert<CompleteFunctionDeclaration>()) {
        if (typedChar == QLatin1Char('(')) {
            m_typedChar = typedChar;
            return true;
        }
    }

    return false;
}

static bool isDereferenced(TextEditor::BaseTextEditor *editor, int basePosition)
{
    QTextCursor cursor = editor->editorWidget()->textCursor();
    cursor.setPosition(basePosition);

    BackwardsScanner scanner(cursor);
    for (int pos = scanner.startToken()-1; pos >= 0; pos--) {
        switch (scanner[pos].kind()) {
        case T_COLON_COLON:
        case T_IDENTIFIER:
            //Ignore scope specifiers
            break;

        case T_AMPER: return true;
        default:      return false;
        }
    }
    return false;
}

void CppAssistProposalItem::applyContextualContent(TextEditor::BaseTextEditor *editor,
                                                    int basePosition) const
{
    Symbol *symbol = 0;

    if (data().isValid())
        symbol = data().value<Symbol *>();

    QString toInsert;
    QString extraChars;
    int extraLength = 0;
    int cursorOffset = 0;

    bool autoParenthesesEnabled = true;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        toInsert = text();
        extraChars += QLatin1Char(')');

        if (m_typedChar == QLatin1Char('(')) // Eat the opening parenthesis
            m_typedChar = QChar();
    } else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL) {
        toInsert = text();
        if (!toInsert.endsWith(QLatin1Char('/'))) {
            extraChars += QLatin1Char((m_completionOperator == T_ANGLE_STRING_LITERAL) ? '>' : '"');
        } else {
            if (m_typedChar == QLatin1Char('/')) // Eat the slash
                m_typedChar = QChar();
        }
    } else {
        toInsert = text();

        const CompletionSettings &completionSettings =
                TextEditorSettings::instance()->completionSettings();
        const bool autoInsertBrackets = completionSettings.m_autoInsertBrackets;

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
                    if (function->argumentCount() == 0)
                        extraChars += QLatin1Char('<');
#endif
                } else if (!isDereferenced(editor, basePosition) && ! function->isAmbiguous()) {
                    // When the user typed the opening parenthesis, he'll likely also type the closing one,
                    // in which case it would be annoying if we put the cursor after the already automatically
                    // inserted closing parenthesis.
                    const bool skipClosingParenthesis = m_typedChar != QLatin1Char('(');

                    if (completionSettings.m_spaceAfterFunctionName)
                        extraChars += QLatin1Char(' ');
                    extraChars += QLatin1Char('(');
                    if (m_typedChar == QLatin1Char('('))
                        m_typedChar = QChar();

                    // If the function doesn't return anything, automatically place the semicolon,
                    // unless we're doing a scope completion (then it might be function definition).
                    const QChar characterAtCursor = editor->textDocument()->characterAt(editor->position());
                    bool endWithSemicolon = m_typedChar == QLatin1Char(';')
                            || (function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON);
                    const QChar semicolon = m_typedChar.isNull() ? QLatin1Char(';') : m_typedChar;

                    if (endWithSemicolon && characterAtCursor == semicolon) {
                        endWithSemicolon = false;
                        m_typedChar = QChar();
                    }

                    // If the function takes no arguments, automatically place the closing parenthesis
                    if (!isOverloaded() && ! function->hasArguments() && skipClosingParenthesis) {
                        extraChars += QLatin1Char(')');
                        if (endWithSemicolon) {
                            extraChars += semicolon;
                            m_typedChar = QChar();
                        }
                    } else if (autoParenthesesEnabled) {
                        const QChar lookAhead = editor->textDocument()->characterAt(editor->position() + 1);
                        if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                            extraChars += QLatin1Char(')');
                            --cursorOffset;
                            if (endWithSemicolon) {
                                extraChars += semicolon;
                                --cursorOffset;
                                m_typedChar = QChar();
                            }
                        }
                        // TODO: When an opening parenthesis exists, the "semicolon" should really be
                        // inserted after the matching closing parenthesis.
                    }
                }
            }
        }

        if (autoInsertBrackets && data().canConvert<CompleteFunctionDeclaration>()) {
            if (m_typedChar == QLatin1Char('('))
                m_typedChar = QChar();

            // everything from the closing parenthesis on are extra chars, to
            // make sure an auto-inserted ")" gets replaced by ") const" if necessary
            int closingParen = toInsert.lastIndexOf(QLatin1Char(')'));
            extraChars = toInsert.mid(closingParen);
            toInsert.truncate(closingParen);
        }
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!m_typedChar.isNull()) {
        extraChars += m_typedChar;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    // Determine the length of characters that should just be kept on the editor, but do
    // not consider content that ends as an identifier (which could be undesired).
    const int lineEnd = editor->position(TextEditor::ITextEditor::EndOfLine);
    const QString inEditor = editor->textDocument()->textAt(editor->position(),
                                                            lineEnd - editor->position());
    int preserveLength = 0;
    if (!inEditor.isEmpty()) {
        preserveLength = toInsert.length() - (editor->position() - basePosition);
        const int inEditorLength = inEditor.length();
        while (preserveLength > 0) {
            if (inEditor.startsWith(toInsert.right(preserveLength))
                    && (inEditorLength == preserveLength
                        || (!inEditor.at(preserveLength).isLetterOrNumber()
                            && inEditor.at(preserveLength) != QLatin1Char('_')))) {
                break;
            }
            --preserveLength;
        }
    }

    for (int i = 0; i < extraChars.length(); ++i) {
        const QChar a = extraChars.at(i);
        const QChar b = editor->textDocument()->characterAt(editor->position() + i + preserveLength);
        if (a == b)
            ++extraLength;
        else
            break;
    }

    toInsert += extraChars;

    // Insert the remainder of the name
    const int length = editor->position() - basePosition + preserveLength + extraLength;
    editor->setCursorPosition(basePosition);
    editor->replace(length, toInsert);
    if (cursorOffset)
        editor->setCursorPosition(editor->position() + cursorOffset);
}

// --------------------
// CppFunctionHintModel
// --------------------
class CppFunctionHintModel : public TextEditor::IFunctionHintProposalModel
{
public:
    CppFunctionHintModel(QList<Function *> functionSymbols,
                         const QSharedPointer<TypeOfExpression> &typeOfExp)
        : m_functionSymbols(functionSymbols)
        , m_currentArg(-1)
        , m_typeOfExpression(typeOfExp)
    {}

    virtual void reset() {}
    virtual int size() const { return m_functionSymbols.size(); }
    virtual QString text(int index) const;
    virtual int activeArgument(const QString &prefix) const;

private:
    QList<Function *> m_functionSymbols;
    mutable int m_currentArg;
    QSharedPointer<TypeOfExpression> m_typeOfExpression;
};

QString CppFunctionHintModel::text(int index) const
{
    Overview overview;
    overview.showReturnTypes = true;
    overview.showArgumentNames = true;
    overview.markedArgument = m_currentArg + 1;
    Function *f = m_functionSymbols.at(index);

    const QString prettyMethod = overview.prettyType(f->type(), f->name());
    const int begin = overview.markedArgumentBegin;
    const int end = overview.markedArgumentEnd;

    QString hintText;
    hintText += Qt::escape(prettyMethod.left(begin));
    hintText += QLatin1String("<b>");
    hintText += Qt::escape(prettyMethod.mid(begin, end - begin));
    hintText += QLatin1String("</b>");
    hintText += Qt::escape(prettyMethod.mid(end));
    return hintText;
}

int CppFunctionHintModel::activeArgument(const QString &prefix) const
{
    int argnr = 0;
    int parcount = 0;
    SimpleLexer tokenize;
    QList<Token> tokens = tokenize(prefix);
    for (int i = 0; i < tokens.count(); ++i) {
        const Token &tk = tokens.at(i);
        if (tk.is(T_LPAREN))
            ++parcount;
        else if (tk.is(T_RPAREN))
            --parcount;
        else if (! parcount && tk.is(T_COMMA))
            ++argnr;
    }

    if (parcount < 0)
        return -1;

    if (argnr != m_currentArg)
        m_currentArg = argnr;

    return argnr;
}

// ---------------------------
// InternalCompletionAssistProvider
// ---------------------------

IAssistProcessor *InternalCompletionAssistProvider::createProcessor() const
{
    return new CppCompletionAssistProcessor;
}

namespace {
class CppCompletionSupportInternal: public CppCompletionSupport
{
public:
    CppCompletionSupportInternal(TextEditor::ITextEditor *editor)
        : CppCompletionSupport(editor)
    {}

    virtual ~CppCompletionSupportInternal()
    {}

    virtual TextEditor::IAssistInterface *createAssistInterface(ProjectExplorer::Project *project,
                                                                QTextDocument *document,
                                                                int position,
                                                                TextEditor::AssistReason reason) const
    {
        CppModelManagerInterface *modelManager = CppModelManagerInterface::instance();
        QStringList includePaths;
        QStringList frameworkPaths;
        if (project) {
            includePaths = modelManager->projectInfo(project).includePaths();
            frameworkPaths = modelManager->projectInfo(project).frameworkPaths();
        }
        return new CppTools::Internal::CppCompletionAssistInterface(
                    document,
                    position,
                    editor()->document()->fileName(),
                    reason,
                    modelManager->snapshot(),
                    includePaths,
                    frameworkPaths);
    }
};
}

CppCompletionSupport *InternalCompletionAssistProvider::completionSupport(ITextEditor *editor)
{
    return new CppCompletionSupportInternal(editor);
}

// -----------------
// CppAssistProposal
// -----------------
class CppAssistProposal : public TextEditor::GenericProposal
{
public:
    CppAssistProposal(int cursorPos, TextEditor::IGenericProposalModel *model)
        : TextEditor::GenericProposal(cursorPos, model)
        , m_replaceDotForArrow(static_cast<CppAssistProposalModel *>(model)->m_replaceDotForArrow)
    {}

    virtual bool isCorrective() const { return m_replaceDotForArrow; }
    virtual void makeCorrection(BaseTextEditor *editor);

private:
    bool m_replaceDotForArrow;
};

void CppAssistProposal::makeCorrection(BaseTextEditor *editor)
{
    editor->setCursorPosition(basePosition() - 1);
    editor->replace(1, QLatin1String("->"));
    moveBasePosition(1);
}

namespace {

class ConvertToCompletionItem: protected NameVisitor
{
    // The completion item.
    BasicProposalItem *_item;

    // The current symbol.
    Symbol *_symbol;

    // The pretty printer.
    Overview overview;

public:
    ConvertToCompletionItem()
        : _item(0)
        , _symbol(0)
    {
        overview.showReturnTypes = true;
        overview.showArgumentNames = true;
    }

    BasicProposalItem *operator()(Symbol *symbol)
    {
        //using declaration can be qualified
        if (! symbol || ! symbol->name() || (symbol->name()->isQualifiedNameId()
                                             && ! symbol->asUsingDeclaration()))
            return 0;

        BasicProposalItem *previousItem = switchCompletionItem(0);
        Symbol *previousSymbol = switchSymbol(symbol);
        accept(symbol->unqualifiedName());
        if (_item)
            _item->setData(QVariant::fromValue(symbol));
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

    BasicProposalItem *switchCompletionItem(BasicProposalItem *item)
    {
        BasicProposalItem *previousItem = _item;
        _item = item;
        return previousItem;
    }

    BasicProposalItem *newCompletionItem(const Name *name)
    {
        BasicProposalItem *item = new CppAssistProposalItem;
        item->setText(overview.prettyName(name));
        return item;
    }

    virtual void visit(const Identifier *name)
    {
        _item = newCompletionItem(name);
        if (!_symbol->isScope() || _symbol->isFunction())
            _item->setDetail(overview.prettyType(_symbol->type(), name));
    }

    virtual void visit(const TemplateNameId *name)
    {
        _item = newCompletionItem(name);
        _item->setText(QLatin1String(name->identifier()->chars()));
    }

    virtual void visit(const DestructorNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(const OperatorNameId *name)
    {
        _item = newCompletionItem(name);
        _item->setDetail(overview.prettyType(_symbol->type(), name));
    }

    virtual void visit(const ConversionNameId *name)
    { _item = newCompletionItem(name); }

    virtual void visit(const QualifiedNameId *name)
    { _item = newCompletionItem(name->name()); }
};

Class *asClassOrTemplateClassType(FullySpecifiedType ty)
{
    if (Class *classTy = ty->asClassType())
        return classTy;
    else if (Template *templ = ty->asTemplateType()) {
        if (Symbol *decl = templ->declaration())
            return decl->asClass();
    }
    return 0;
}

Scope *enclosingNonTemplateScope(Symbol *symbol)
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

Function *asFunctionOrTemplateFunctionType(FullySpecifiedType ty)
{
    if (Function *funTy = ty->asFunctionType())
        return funTy;
    else if (Template *templ = ty->asTemplateType()) {
        if (Symbol *decl = templ->declaration())
            return decl->asFunction();
    }
    return 0;
}

} // Anonymous

// ----------------------------
// CppCompletionAssistProcessor
// ----------------------------
CppCompletionAssistProcessor::CppCompletionAssistProcessor()
    : m_startPosition(-1)
    , m_objcEnabled(true)
    , m_snippetCollector(QLatin1String(CppEditor::Constants::CPP_SNIPPETS_GROUP_ID),
                         QIcon(QLatin1String(":/texteditor/images/snippet.png")))
    , preprocessorCompletions(QStringList()
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
          << QLatin1String("endif"))
    , m_model(new CppAssistProposalModel)
    , m_hintProposal(0)
{}

CppCompletionAssistProcessor::~CppCompletionAssistProcessor()
{}

IAssistProposal * CppCompletionAssistProcessor::perform(const IAssistInterface *interface)
{
    m_interface.reset(static_cast<const CppCompletionAssistInterface *>(interface));

    if (interface->reason() != ExplicitlyInvoked && !accepts())
        return 0;

    int index = startCompletionHelper();
    if (index != -1) {
        if (m_hintProposal)
            return m_hintProposal;

        return createContentProposal();
    }

    return 0;
}

bool CppCompletionAssistProcessor::accepts() const
{
    const int pos = m_interface->position();
    unsigned token = T_EOF_SYMBOL;

    const int start = startOfOperator(pos, &token, /*want function call=*/ true);
    if (start != pos) {
        if (token == T_POUND) {
            const int column = pos - m_interface->textDocument()->findBlock(start).position();
            if (column != 1)
                return false;
        }

        return true;
    } else {
        // Trigger completion after three characters of a name have been typed, when not editing an existing name
        QChar characterUnderCursor = m_interface->characterAt(pos);
        if (!characterUnderCursor.isLetterOrNumber() && characterUnderCursor != QLatin1Char('_')) {
            const int startOfName = findStartOfName(pos);
            if (pos - startOfName >= 3) {
                const QChar firstCharacter = m_interface->characterAt(startOfName);
                if (firstCharacter.isLetter() || firstCharacter == QLatin1Char('_')) {
                    // Finally check that we're not inside a comment or string (code copied from startOfOperator)
                    QTextCursor tc(m_interface->textDocument());
                    tc.setPosition(pos);

                    SimpleLexer tokenize;
                    tokenize.setQtMocRunEnabled(true);
                    tokenize.setObjCEnabled(true);
                    tokenize.setSkipComments(false);
                    const QList<Token> &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
                    const int tokenIdx = SimpleLexer::tokenBefore(tokens, qMax(0, tc.positionInBlock() - 1));
                    const Token tk = (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);

                    if (!tk.isComment() && !tk.isLiteral()) {
                        return true;
                    } else if (tk.isLiteral()
                               && tokens.size() == 3
                               && tokens.at(0).kind() == T_POUND
                               && tokens.at(1).kind() == T_IDENTIFIER) {
                        const QString &line = tc.block().text();
                        const Token &idToken = tokens.at(1);
                        const QStringRef &identifier =
                                line.midRef(idToken.begin(), idToken.end() - idToken.begin());
                        if (identifier == QLatin1String("include")
                                || identifier == QLatin1String("include_next")
                                || (m_objcEnabled && identifier == QLatin1String("import"))) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

IAssistProposal *CppCompletionAssistProcessor::createContentProposal()
{
    // Duplicates are kept only if they are snippets.
    QSet<QString> processed;
    QList<BasicProposalItem *>::iterator it = m_completions.begin();
    while (it != m_completions.end()) {
        CppAssistProposalItem *item = static_cast<CppAssistProposalItem *>(*it);
        if (!processed.contains(item->text()) || item->data().canConvert<QString>()) {
            ++it;
            if (!item->data().canConvert<QString>()) {
                processed.insert(item->text());
                if (!item->isOverloaded()) {
                    if (Symbol *symbol = qvariant_cast<Symbol *>(item->data())) {
                        if (Function *funTy = symbol->type()->asFunctionType()) {
                            if (funTy->hasArguments())
                                item->markAsOverloaded();
                        }
                    }
                }
            }
        } else {
            delete *it;
            it = m_completions.erase(it);
        }
    }

    m_model->loadContent(m_completions);
    return new CppAssistProposal(m_startPosition, m_model.take());
}

IAssistProposal *CppCompletionAssistProcessor::createHintProposal(
    QList<CPlusPlus::Function *> functionSymbols) const
{
    IFunctionHintProposalModel *model =
            new CppFunctionHintModel(functionSymbols, m_model->m_typeOfExpression);
    IAssistProposal *proposal = new FunctionHintProposal(m_startPosition, model);
    return proposal;
}

int CppCompletionAssistProcessor::startOfOperator(int pos,
                                                  unsigned *kind,
                                                  bool wantFunctionCall) const
{
    const QChar ch  = pos > -1 ? m_interface->characterAt(pos - 1) : QChar();
    const QChar ch2 = pos >  0 ? m_interface->characterAt(pos - 2) : QChar();
    const QChar ch3 = pos >  1 ? m_interface->characterAt(pos - 3) : QChar();

    int start = pos - CppCompletionAssistProvider::activationSequenceChar(ch, ch2, ch3, kind, wantFunctionCall);
    if (start != pos) {
        QTextCursor tc(m_interface->textDocument());
        tc.setPosition(pos);

        // Include completion: make sure the quote character is the first one on the line
        if (*kind == T_STRING_LITERAL) {
            QTextCursor s = tc;
            s.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString sel = s.selectedText();
            if (sel.indexOf(QLatin1Char('"')) < sel.length() - 1) {
                *kind = T_EOF_SYMBOL;
                start = pos;
            }
        }

        if (*kind == T_COMMA) {
            ExpressionUnderCursor expressionUnderCursor;
            if (expressionUnderCursor.startOfFunctionCall(tc) == -1) {
                *kind = T_EOF_SYMBOL;
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

        if (*kind == T_DOXY_COMMENT && !(tk.is(T_DOXY_COMMENT) || tk.is(T_CPP_DOXY_COMMENT))) {
            *kind = T_EOF_SYMBOL;
            start = pos;
        }
        // Don't complete in comments or strings, but still check for include completion
        else if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT) ||
                 (tk.isLiteral() && (*kind != T_STRING_LITERAL
                                     && *kind != T_ANGLE_STRING_LITERAL
                                     && *kind != T_SLASH
                                     && *kind != T_DOT))) {
            *kind = T_EOF_SYMBOL;
            start = pos;
        }
        // Include completion: can be triggered by slash, but only in a string
        else if (*kind == T_SLASH && (tk.isNot(T_STRING_LITERAL) && tk.isNot(T_ANGLE_STRING_LITERAL))) {
            *kind = T_EOF_SYMBOL;
            start = pos;
        }
        else if (*kind == T_LPAREN) {
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
                    *kind = T_EOF_SYMBOL;
                    start = pos;
                }
            }
        }
        // Check for include preprocessor directive
        else if (*kind == T_STRING_LITERAL || *kind == T_ANGLE_STRING_LITERAL|| *kind == T_SLASH
                 || (*kind == T_DOT && (tk.is(T_STRING_LITERAL) || tk.is(T_ANGLE_STRING_LITERAL)))) {
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
                *kind = T_EOF_SYMBOL;
                start = pos;
            } else {
                if (*kind == T_DOT) {
                    start = findStartOfName(start);
                    const QChar ch4  = start > -1 ? m_interface->characterAt(start - 1) : QChar();
                    const QChar ch5 = start >  0 ? m_interface->characterAt(start - 2) : QChar();
                    const QChar ch6 = start >  1 ? m_interface->characterAt(start - 3) : QChar();
                    start = start - CppCompletionAssistProvider::activationSequenceChar(ch4, ch5, ch6, kind, wantFunctionCall);
                }
            }
        }
    }

    return start;
}

int CppCompletionAssistProcessor::findStartOfName(int pos) const
{
    if (pos == -1)
        pos = m_interface->position();
    QChar chr;

    // Skip to the start of a name
    do {
        chr = m_interface->characterAt(--pos);
    } while (chr.isLetterOrNumber() || chr == QLatin1Char('_'));

    return pos + 1;
}

int CppCompletionAssistProcessor::startCompletionHelper()
{
    if (m_objcEnabled) {
        if (tryObjCCompletion())
            return m_startPosition;
    }

    const int startOfName = findStartOfName();
    m_startPosition = startOfName;
    m_model->m_completionOperator = T_EOF_SYMBOL;

    int endOfOperator = m_startPosition;

    // Skip whitespace preceding this position
    while (m_interface->characterAt(endOfOperator - 1).isSpace())
        --endOfOperator;

    int endOfExpression = startOfOperator(endOfOperator,
                                          &m_model->m_completionOperator,
                                          /*want function call =*/ true);

    if (m_model->m_completionOperator == T_DOXY_COMMENT) {
        for (int i = 1; i < T_DOXY_LAST_TAG; ++i)
            addCompletionItem(QString::fromLatin1(doxygenTagSpell(i)), m_icons.keywordIcon());
        return m_startPosition;
    }

    // Pre-processor completion
    if (m_model->m_completionOperator == T_POUND) {
        completePreprocessor();
        m_startPosition = startOfName;
        return m_startPosition;
    }

    // Include completion
    if (m_model->m_completionOperator == T_STRING_LITERAL
        || m_model->m_completionOperator == T_ANGLE_STRING_LITERAL
        || m_model->m_completionOperator == T_SLASH) {

        QTextCursor c(m_interface->textDocument());
        c.setPosition(endOfExpression);
        if (completeInclude(c))
            m_startPosition = endOfExpression + 1;
        return m_startPosition;
    }

    ExpressionUnderCursor expressionUnderCursor;
    QTextCursor tc(m_interface->textDocument());

    if (m_model->m_completionOperator == T_COMMA) {
        tc.setPosition(endOfExpression);
        const int start = expressionUnderCursor.startOfFunctionCall(tc);
        if (start == -1) {
            m_model->m_completionOperator = T_EOF_SYMBOL;
            return -1;
        }

        endOfExpression = start;
        m_startPosition = start + 1;
        m_model->m_completionOperator = T_LPAREN;
    }

    QString expression;
    int startOfExpression = m_interface->position();
    tc.setPosition(endOfExpression);

    if (m_model->m_completionOperator) {
        expression = expressionUnderCursor(tc);
        startOfExpression = endOfExpression - expression.length();

        if (m_model->m_completionOperator == T_LPAREN) {
            if (expression.endsWith(QLatin1String("SIGNAL")))
                m_model->m_completionOperator = T_SIGNAL;

            else if (expression.endsWith(QLatin1String("SLOT")))
                m_model->m_completionOperator = T_SLOT;

            else if (m_interface->position() != endOfOperator) {
                // We don't want a function completion when the cursor isn't at the opening brace
                expression.clear();
                m_model->m_completionOperator = T_EOF_SYMBOL;
                m_startPosition = startOfName;
                startOfExpression = m_interface->position();
            }
        }
    } else if (expression.isEmpty()) {
        while (startOfExpression > 0 && m_interface->characterAt(startOfExpression).isSpace())
            --startOfExpression;
    }

    int line = 0, column = 0;
    Convenience::convertPosition(m_interface->textDocument(), startOfExpression, &line, &column);
    const QString fileName = m_interface->fileName();
    return startCompletionInternal(fileName, line, column, expression, endOfExpression);
}

bool CppCompletionAssistProcessor::tryObjCCompletion()
{
    int end = m_interface->position();
    while (m_interface->characterAt(end).isSpace())
        ++end;
    if (m_interface->characterAt(end) != QLatin1Char(']'))
        return false;

    QTextCursor tc(m_interface->textDocument());
    tc.setPosition(end);
    BackwardsScanner tokens(tc);
    if (tokens[tokens.startToken() - 1].isNot(T_RBRACKET))
        return false;

    const int start = tokens.startOfMatchingBrace(tokens.startToken());
    if (start == tokens.startToken())
        return false;

    const int startPos = tokens[start].begin() + tokens.startPosition();
    const QString expr = m_interface->textAt(startPos, m_interface->position() - startPos);

    Document::Ptr thisDocument = m_interface->snapshot().document(m_interface->fileName());
    if (! thisDocument)
        return false;

    m_model->m_typeOfExpression->init(thisDocument, m_interface->snapshot());

    int line = 0, column = 0;
    Convenience::convertPosition(m_interface->textDocument(), m_interface->position(), &line, &column);
    Scope *scope = thisDocument->scopeAt(line, column);
    if (!scope)
        return false;

    const QList<LookupItem> items = (*m_model->m_typeOfExpression)(expr.toUtf8(), scope);
    LookupContext lookupContext(thisDocument, m_interface->snapshot());

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

    m_startPosition = m_interface->position();
    return true;
}

namespace {
enum CompletionOrder {
    // default order is 0
    FunctionArgumentsOrder = 2,
    FunctionLocalsOrder = 2, // includes local types
    PublicClassMemberOrder = 1,
    InjectedClassNameOrder = -1,
    MacrosOrder = -2,
    KeywordsOrder = -2
};
}

void CppCompletionAssistProcessor::addCompletionItem(const QString &text,
                                                     const QIcon &icon,
                                                     int order,
                                                     const QVariant &data)
{
    BasicProposalItem *item = new CppAssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    item->setData(data);
    m_completions.append(item);
}

void CppCompletionAssistProcessor::addCompletionItem(CPlusPlus::Symbol *symbol,
                                                     int order)
{
    ConvertToCompletionItem toCompletionItem;
    BasicProposalItem *item = toCompletionItem(symbol);
    if (item) {
        item->setIcon(m_icons.iconForSymbol(symbol));
        item->setOrder(order);
        m_completions.append(item);
    }
}

void CppCompletionAssistProcessor::completeObjCMsgSend(CPlusPlus::ClassOrNamespace *binding,
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
                            text += QString::fromUtf8(selectorName->nameAt(i)->identifier()->chars());
                            text += QLatin1Char(':');
                            text += TextEditor::Snippet::kVariableDelimiter;
                            text += QLatin1Char('(');
                            text += oo.prettyType(arg->type());
                            text += QLatin1Char(')');
                            text += oo.prettyName(arg->name());
                            text += TextEditor::Snippet::kVariableDelimiter;
                        }
                    } else {
                        text = QString::fromUtf8(selectorName->identifier()->chars());
                    }
                    data = text;

                    if (!text.isEmpty())
                        addCompletionItem(text, QIcon(), 0, QVariant::fromValue(data));
                }
            }
        }
    }
}

bool CppCompletionAssistProcessor::completeInclude(const QTextCursor &cursor)
{
    QString directoryPrefix;
    if (m_model->m_completionOperator == T_SLASH) {
        QTextCursor c = cursor;
        c.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString sel = c.selectedText();
        int startCharPos = sel.indexOf(QLatin1Char('"'));
        if (startCharPos == -1) {
            startCharPos = sel.indexOf(QLatin1Char('<'));
            m_model->m_completionOperator = T_ANGLE_STRING_LITERAL;
        } else {
            m_model->m_completionOperator = T_STRING_LITERAL;
        }
        if (startCharPos != -1)
            directoryPrefix = sel.mid(startCharPos + 1, sel.length() - 1);
    }

    // Make completion for all relevant includes
    QStringList includePaths = m_interface->includePaths();
    const QString &currentFilePath = QFileInfo(m_interface->fileName()).path();
    if (!includePaths.contains(currentFilePath))
        includePaths.append(currentFilePath);

    const Core::MimeType mimeType =
            Core::ICore::mimeDatabase()->findByType(QLatin1String("text/x-c++hdr"));
    const QStringList suffixes = mimeType.suffixes();

    foreach (const QString &includePath, includePaths) {
        QString realPath = includePath;
        if (!directoryPrefix.isEmpty()) {
            realPath += QLatin1Char('/');
            realPath += directoryPrefix;
        }
        completeInclude(realPath, suffixes);
    }

    foreach (const QString &frameworkPath, m_interface->frameworkPaths()) {
        QString realPath = frameworkPath;
        if (!directoryPrefix.isEmpty()) {
            realPath += QLatin1Char('/');
            realPath += directoryPrefix;
            realPath += QLatin1String(".framework/Headers");
        }
        completeInclude(realPath, suffixes);
    }

    return !m_completions.isEmpty();
}

void CppCompletionAssistProcessor::completeInclude(const QString &realPath,
                                                   const QStringList &suffixes)
{
    QDirIterator i(realPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    while (i.hasNext()) {
        const QString fileName = i.next();
        const QFileInfo fileInfo = i.fileInfo();
        const QString suffix = fileInfo.suffix();
        if (suffix.isEmpty() || suffixes.contains(suffix)) {
            QString text = fileName.mid(realPath.length() + 1);
            if (fileInfo.isDir())
                text += QLatin1Char('/');
            addCompletionItem(text, m_icons.keywordIcon());
        }
    }
}

void CppCompletionAssistProcessor::completePreprocessor()
{
    foreach (const QString &preprocessorCompletion, preprocessorCompletions)
        addCompletionItem(preprocessorCompletion);

    if (objcKeywordsWanted())
        addCompletionItem(QLatin1String("import"));
}

bool CppCompletionAssistProcessor::objcKeywordsWanted() const
{
    if (!m_objcEnabled)
        return false;

    const QString fileName = m_interface->fileName();

    const Core::MimeDatabase *mdb = Core::ICore::mimeDatabase();
    const QString mt = mdb->findByFile(fileName).type();
    return mt == QLatin1String(CppTools::Constants::OBJECTIVE_CPP_SOURCE_MIMETYPE);
}

int CppCompletionAssistProcessor::startCompletionInternal(const QString fileName,
                                                          unsigned line, unsigned column,
                                                          const QString &expr,
                                                          int endOfExpression)
{
    QString expression = expr.trimmed();

    Document::Ptr thisDocument = m_interface->snapshot().document(fileName);
    if (! thisDocument)
        return -1;

    m_model->m_typeOfExpression->init(thisDocument, m_interface->snapshot());

    Scope *scope = thisDocument->scopeAt(line, column);
    QTC_ASSERT(scope != 0, return -1);

    if (expression.isEmpty()) {
        if (m_model->m_completionOperator == T_EOF_SYMBOL || m_model->m_completionOperator == T_COLON_COLON) {
            (void) (*m_model->m_typeOfExpression)(expression.toUtf8(), scope);
            globalCompletion(scope);
            if (m_completions.isEmpty())
                return -1;
            return m_startPosition;
        }

        else if (m_model->m_completionOperator == T_SIGNAL || m_model->m_completionOperator == T_SLOT) {
            // Apply signal/slot completion on 'this'
            expression = QLatin1String("this");
        }
    }

    QByteArray utf8Exp = expression.toUtf8();
    QList<LookupItem> results =
            (*m_model->m_typeOfExpression)(utf8Exp, scope, TypeOfExpression::Preprocess);

    if (results.isEmpty()) {
        if (m_model->m_completionOperator == T_SIGNAL || m_model->m_completionOperator == T_SLOT) {
            if (! (expression.isEmpty() || expression == QLatin1String("this"))) {
                expression = QLatin1String("this");
                results = (*m_model->m_typeOfExpression)(utf8Exp, scope);
            }

            if (results.isEmpty())
                return -1;

        } else if (m_model->m_completionOperator == T_LPAREN) {
            // Find the expression that precedes the current name
            int index = endOfExpression;
            while (m_interface->characterAt(index - 1).isSpace())
                --index;
            index = findStartOfName(index);

            QTextCursor tc(m_interface->textDocument());
            tc.setPosition(index);

            ExpressionUnderCursor expressionUnderCursor;
            const QString baseExpression = expressionUnderCursor(tc);

            // Resolve the type of this expression
            const QList<LookupItem> results =
                    (*m_model->m_typeOfExpression)(baseExpression.toUtf8(), scope,
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

    switch (m_model->m_completionOperator) {
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

void CppCompletionAssistProcessor::globalCompletion(CPlusPlus::Scope *currentScope)
{
    const LookupContext &context = m_model->m_typeOfExpression->context();

    if (m_model->m_completionOperator == T_COLON_COLON) {
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
                addCompletionItem(scope->memberAt(i), FunctionLocalsOrder);
            }
        } else if (scope->isFunction()) {
            Function *fun = scope->asFunction();
            for (unsigned i = 0, argc = fun->argumentCount(); i < argc; ++i) {
                addCompletionItem(fun->argumentAt(i), FunctionArgumentsOrder);
            }
        } else if (scope->isTemplate()) {
            Template *templ = scope->asTemplate();
            for (unsigned i = 0, argc = templ->templateParameterCount(); i < argc; ++i) {
                addCompletionItem(templ->templateParameterAt(i), FunctionArgumentsOrder);
            }
            break;
        }
    }

    for (; currentBinding; currentBinding = currentBinding->parent()) {
        const QList<Symbol *> symbols = currentBinding->symbols();

        if (! symbols.isEmpty()) {
            if (symbols.first()->isClass())
                completeClass(currentBinding);
            else
                completeNamespace(currentBinding);
        }
    }

    foreach (ClassOrNamespace *b, usingBindings)
        completeNamespace(b);

    addKeywords();
    addMacros(CppModelManagerInterface::configurationFileName(), context.snapshot());
    addMacros(context.thisDocument()->fileName(), context.snapshot());
    addSnippets();
}

bool CppCompletionAssistProcessor::completeMember(const QList<CPlusPlus::LookupItem> &baseResults)
{
    const LookupContext &context = m_model->m_typeOfExpression->context();

    if (baseResults.isEmpty())
        return false;

    ResolveExpression resolveExpression(context);

    if (ClassOrNamespace *binding =
            resolveExpression.baseExpression(baseResults,
                                             m_model->m_completionOperator,
                                             &m_model->m_replaceDotForArrow)) {
        if (binding)
            completeClass(binding, /*static lookup = */ true);

        return ! m_completions.isEmpty();
    }

    return false;
}

bool CppCompletionAssistProcessor::completeScope(const QList<CPlusPlus::LookupItem> &results)
{
    const LookupContext &context = m_model->m_typeOfExpression->context();
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

        } else if (Template *templ = ty->asTemplateType()) {
            if (!result.binding())
                continue;
            if (ClassOrNamespace *b = result.binding()->lookupType(templ->name())) {
                completeClass(b);
                break;
            }

        } else if (Enum *e = ty->asEnumType()) {
            if (ClassOrNamespace *b = context.lookupType(e)) {
                completeNamespace(b);
                break;
            }
        }
    }

    return ! m_completions.isEmpty();
}

void CppCompletionAssistProcessor::completeNamespace(CPlusPlus::ClassOrNamespace *b)
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
            if (Scope *scope = bb->asScope())
                scopesToVisit.append(scope);
        }

        foreach (Enum *e, binding->unscopedEnums()) {
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

void CppCompletionAssistProcessor::completeClass(CPlusPlus::ClassOrNamespace *b, bool staticLookup)
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

        foreach (Enum *e, binding->unscopedEnums())
            scopesToVisit.append(e);

        while (! scopesToVisit.isEmpty()) {
            Scope *scope = scopesToVisit.takeFirst();
            if (! scope || scopesVisited.contains(scope))
                continue;

            scopesVisited.insert(scope);

            if (staticLookup)
                addCompletionItem(scope, InjectedClassNameOrder); // add a completion item for the injected class name.

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

                if (member->isPublic())
                    addCompletionItem(member, PublicClassMemberOrder);
                else
                    addCompletionItem(member);
            }
        }
    }
}

bool CppCompletionAssistProcessor::completeQtMethod(const QList<CPlusPlus::LookupItem> &results, bool wantSignals)
{
    if (results.isEmpty())
        return false;

    const LookupContext &context = m_model->m_typeOfExpression->context();

    ConvertToCompletionItem toCompletionItem;
    Overview o;
    o.showReturnTypes = false;
    o.showArgumentNames = false;
    o.showFunctionSignatures = true;

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

                unsigned count = fun->argumentCount();
                while (true) {
                    QString signature;
                    signature += Overview().prettyName(fun->name());
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
                        BasicProposalItem *ci = toCompletionItem(fun);
                        if (!ci)
                            break;
                        signatures.insert(signature);
                        ci->setText(signature); // fix the completion item.
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

    return ! m_completions.isEmpty();
}

void CppCompletionAssistProcessor::addSnippets()
{
    m_completions.append(m_snippetCollector.collect());
}

void CppCompletionAssistProcessor::addKeywords()
{
    int keywordLimit = T_FIRST_OBJC_AT_KEYWORD;
    if (objcKeywordsWanted())
        keywordLimit = T_LAST_OBJC_AT_KEYWORD + 1;

    // keyword completion items.
    for (int i = T_FIRST_KEYWORD; i < keywordLimit; ++i)
        addCompletionItem(QLatin1String(Token::name(i)), m_icons.keywordIcon(), KeywordsOrder);
}

void CppCompletionAssistProcessor::addMacros(const QString &fileName, const CPlusPlus::Snapshot &snapshot)
{
    QSet<QString> processed;
    QSet<QString> definedMacros;

    addMacros_helper(snapshot, fileName, &processed, &definedMacros);

    foreach (const QString &macroName, definedMacros)
        addCompletionItem(macroName, m_icons.macroIcon(), MacrosOrder);
}

void CppCompletionAssistProcessor::addMacros_helper(const CPlusPlus::Snapshot &snapshot,
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

bool CppCompletionAssistProcessor::completeConstructorOrFunction(const QList<CPlusPlus::LookupItem> &results,
                                                                 int endOfExpression,
                                                                 bool toolTipOnly)
{
    const LookupContext &context = m_model->m_typeOfExpression->context();
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
        const Name *functionCallOp = context.bindings()->control()->operatorNameId(OperatorNameId::FunctionCallOp);

        foreach (const LookupItem &result, results) {
            FullySpecifiedType ty = result.type().simplified();
            Scope *scope = result.scope();

            if (NamedType *namedTy = ty->asNamedType()) {
                if (ClassOrNamespace *b = context.lookupType(namedTy->name(), scope)) {
                    foreach (const LookupItem &r, b->lookup(functionCallOp)) {
                        Symbol *overload = r.declaration();
                        FullySpecifiedType overloadTy = overload->type().simplified();

                        if (Function *funTy = overloadTy->asFunctionType())
                            functions.append(funTy);
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
        int lineSigned = 0, columnSigned = 0;
        Convenience::convertPosition(m_interface->textDocument(), m_interface->position(), &lineSigned, &columnSigned);
        unsigned line = lineSigned, column = columnSigned;

        // find a scope that encloses the current location, starting from the lastVisibileSymbol
        // and moving outwards

        Scope *sc = context.thisDocument()->scopeAt(line, column);

        if (sc && (sc->isClass() || sc->isNamespace())) {
            // It may still be a function call. If the whole line parses as a function
            // declaration, we should be certain that it isn't.
            bool autocompleteSignature = false;

            QTextCursor tc(m_interface->textDocument());
            tc.setPosition(endOfExpression);
            BackwardsScanner bs(tc);
            const int startToken = bs.startToken();
            int lineStartToken = bs.startOfLine(startToken);
            // make sure the required tokens are actually available
            bs.LA(startToken - lineStartToken);
            QString possibleDecl = bs.mid(lineStartToken).trimmed().append(QLatin1String("();"));

            Document::Ptr doc = Document::create(QLatin1String("<completion>"));
            doc->setUtf8Source(possibleDecl.toLatin1());
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
                // set up for rewriting function types with minimally qualified names
                // to do it correctly we'd need the declaration's context and scope, but
                // that'd be too expensive to get here. instead, we just minimize locally
                SubstitutionEnvironment env;
                env.setContext(context);
                env.switchScope(sc);
                ClassOrNamespace *targetCoN = context.lookupType(sc);
                if (!targetCoN)
                    targetCoN = context.globalNamespace();
                UseMinimalNames q(targetCoN);
                env.enter(&q);
                Control *control = context.bindings()->control().data();

                // set up signature autocompletion
                foreach (Function *f, functions) {
                    Overview overview;
                    overview.showArgumentNames = true;
                    overview.showDefaultArguments = false;

                    const FullySpecifiedType localTy = rewriteType(f->type(), &env, control);

                    // gets: "parameter list) cv-spec",
                    QString completion = overview.prettyType(localTy).mid(1);

                    addCompletionItem(completion, QIcon(), 0,
                                      QVariant::fromValue(CompleteFunctionDeclaration(f)));
                }
                return true;
            }
        }
    }

    if (! functions.empty() && !isDestructor) {
        m_hintProposal = createHintProposal(functions);
        return true;
    }

    return false;
}
