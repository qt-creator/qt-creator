/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#include "clangcompletion.h"
#include "clangutils.h"
#include "pchmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Token.h>
#include <cplusplus/MatchingText.h>

#include <cppeditor/cppeditorconstants.h>

#include <cpptools/cppdoxygen.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>

#include <texteditor/texteditor.h>
#include <texteditor/convenience.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>

#include <QCoreApplication>
#include <QDirIterator>
#include <QLoggingCategory>
#include <QTextCursor>
#include <QTextDocument>

using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;

static const char SNIPPET_ICON_PATH[] = ":/texteditor/images/snippet.png";
static Q_LOGGING_CATEGORY(log, "qtc.clangcodemodel.completion")

namespace {

int activationSequenceChar(const QChar &ch,
                           const QChar &ch2,
                           const QChar &ch3,
                           unsigned *kind,
                           bool wantFunctionCall)
{
    int referencePosition = 0;
    int completionKind = T_EOF_SYMBOL;
    switch (ch.toLatin1()) {
    case '.':
        if (ch2 != QLatin1Char('.')) {
            completionKind = T_DOT;
            referencePosition = 1;
        }
        break;
    case ',':
        completionKind = T_COMMA;
        referencePosition = 1;
        break;
    case '(':
        if (wantFunctionCall) {
            completionKind = T_LPAREN;
            referencePosition = 1;
        }
        break;
    case ':':
        if (ch3 != QLatin1Char(':') && ch2 == QLatin1Char(':')) {
            completionKind = T_COLON_COLON;
            referencePosition = 2;
        }
        break;
    case '>':
        if (ch2 == QLatin1Char('-')) {
            completionKind = T_ARROW;
            referencePosition = 2;
        }
        break;
    case '*':
        if (ch2 == QLatin1Char('.')) {
            completionKind = T_DOT_STAR;
            referencePosition = 2;
        } else if (ch3 == QLatin1Char('-') && ch2 == QLatin1Char('>')) {
            completionKind = T_ARROW_STAR;
            referencePosition = 3;
        }
        break;
    case '\\':
    case '@':
        if (ch2.isNull() || ch2.isSpace()) {
            completionKind = T_DOXY_COMMENT;
            referencePosition = 1;
        }
        break;
    case '<':
        completionKind = T_ANGLE_STRING_LITERAL;
        referencePosition = 1;
        break;
    case '"':
        completionKind = T_STRING_LITERAL;
        referencePosition = 1;
        break;
    case '/':
        completionKind = T_SLASH;
        referencePosition = 1;
        break;
    case '#':
        completionKind = T_POUND;
        referencePosition = 1;
        break;
    }

    if (kind)
        *kind = completionKind;

    return referencePosition;
}

static QList<CodeCompletionResult> unfilteredCompletion(const ClangCompletionAssistInterface* interface,
                                                        const QString &fileName,
                                                        unsigned line, unsigned column,
                                                        QByteArray modifiedInput = QByteArray(),
                                                        bool isSignalSlotCompletion = false)
{
    ClangCompleter::Ptr wrapper = interface->clangWrapper();
    QMutexLocker lock(wrapper->mutex());
    //### TODO: check if we're cancelled after we managed to acquire the mutex

    wrapper->setFileName(fileName);
    wrapper->setOptions(interface->options());
    wrapper->setSignalSlotCompletion(isSignalSlotCompletion);
    UnsavedFiles unsavedFiles = interface->unsavedFiles();
    if (!modifiedInput.isEmpty())
        unsavedFiles.insert(fileName, modifiedInput);

    qCDebug(log) << "Starting completion...";
    QTime t; t.start();

    QList<CodeCompletionResult> result = wrapper->codeCompleteAt(line, column + 1, unsavedFiles);
    ::Utils::sort(result);

    qCDebug(log) << "Completion done in" << t.elapsed() << "ms, with" << result.count() << "items.";

    return result;
}

} // Anonymous

namespace ClangCodeModel {
namespace Internal {

// -----------------------------
// ClangCompletionAssistProvider
// -----------------------------
ClangCompletionAssistProvider::ClangCompletionAssistProvider()
    : m_clangCompletionWrapper(new ClangCompleter)
{
}

IAssistProcessor *ClangCompletionAssistProvider::createProcessor() const
{
    return new ClangCompletionAssistProcessor;
}

AssistInterface *ClangCompletionAssistProvider::createAssistInterface(
        const QString &filePath, QTextDocument *document,
        const LanguageFeatures &languageFeatures, int position, AssistReason reason) const
{
    CppModelManager *modelManager = CppModelManager::instance();
    QList<ProjectPart::Ptr> parts = modelManager->projectPart(filePath);
    if (parts.isEmpty())
        parts += modelManager->fallbackProjectPart();
    LanguageFeatures features = languageFeatures;
    ProjectPart::HeaderPaths headerPaths;
    QStringList options;
    PchInfo::Ptr pchInfo;
    foreach (ProjectPart::Ptr part, parts) {
        if (part.isNull())
            continue;
        options = Utils::createClangOptions(part, filePath);
        pchInfo = PchManager::instance()->pchInfo(part);
        if (!pchInfo.isNull())
            options.append(Utils::createPCHInclusionOptions(pchInfo->fileName()));
        headerPaths = part->headerPaths;
        features = part->languageFeatures;
        break;
    }

    return new ClangCompletionAssistInterface(
                m_clangCompletionWrapper,
                document, position, filePath, reason,
                options, headerPaths, pchInfo, features);
}

// ------------------------
// ClangAssistProposalModel
// ------------------------
class ClangAssistProposalModel : public GenericProposalModel
{
public:
    ClangAssistProposalModel()
        : m_sortable(false)
        , m_completionOperator(T_EOF_SYMBOL)
        , m_replaceDotForArrow(false)
    {}

    virtual bool isSortable(const QString &prefix) const;
    bool m_sortable;
    unsigned m_completionOperator;
    bool m_replaceDotForArrow;
};

// -------------------
// ClangAssistProposal
// -------------------
class ClangAssistProposal : public GenericProposal
{
public:
    ClangAssistProposal(int cursorPos, GenericProposalModel *model)
        : GenericProposal(cursorPos, model)
        , m_replaceDotForArrow(static_cast<ClangAssistProposalModel *>(model)->m_replaceDotForArrow)
    {}

    virtual bool isCorrective() const { return m_replaceDotForArrow; }
    virtual void makeCorrection(TextEditorWidget *editorWidget)
    {
        editorWidget->setCursorPosition(basePosition() - 1);
        editorWidget->replace(1, QLatin1String("->"));
        moveBasePosition(1);
    }

private:
    bool m_replaceDotForArrow;
};

// ----------------------
// ClangFunctionHintModel
// ----------------------
class ClangFunctionHintModel : public IFunctionHintProposalModel
{
public:
    ClangFunctionHintModel(const QList<CodeCompletionResult> functionSymbols)
        : m_functionSymbols(functionSymbols)
        , m_currentArg(-1)
    {}

    virtual void reset() {}
    virtual int size() const { return m_functionSymbols.size(); }
    virtual QString text(int index) const;
    virtual int activeArgument(const QString &prefix) const;

private:
    QList<CodeCompletionResult> m_functionSymbols;
    mutable int m_currentArg;
};

QString ClangFunctionHintModel::text(int index) const
{
#if 0
    // TODO: add the boldening to the result
    Overview overview;
    overview.setShowReturnTypes(true);
    overview.setShowArgumentNames(true);
    overview.setMarkedArgument(m_currentArg + 1);
    Function *f = m_functionSymbols.at(index);

    const QString prettyMethod = overview(f->type(), f->name());
    const int begin = overview.markedArgumentBegin();
    const int end = overview.markedArgumentEnd();

    QString hintText;
    hintText += prettyMethod.left(begin).toHtmlEscaped());
    hintText += "<b>";
    hintText += prettyMethod.mid(begin, end - begin).toHtmlEscaped());
    hintText += "</b>";
    hintText += prettyMethod.mid(end).toHtmlEscaped());
    return hintText;
#endif
    return m_functionSymbols.at(index).hint();
}

int ClangFunctionHintModel::activeArgument(const QString &prefix) const
{
    int argnr = 0;
    int parcount = 0;
    SimpleLexer tokenize;
    Tokens tokens = tokenize(prefix);
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

class ClangAssistProposalItem : public AssistProposalItem
{
public:
    ClangAssistProposalItem() {}

    virtual bool prematurelyApplies(const QChar &c) const;
    virtual void applyContextualContent(TextEditorWidget *editorWidget,
                                        int basePosition) const;

    void keepCompletionOperator(unsigned compOp) { m_completionOperator = compOp; }

    bool isOverloaded() const
    { return !m_overloads.isEmpty(); }
    void addOverload(const CodeCompletionResult &ccr)
    { m_overloads.append(ccr); }

    CodeCompletionResult originalItem() const
    {
        const QVariant &v = data();
        if (v.canConvert<CodeCompletionResult>())
            return v.value<CodeCompletionResult>();
        else
            return CodeCompletionResult();
    }

    bool isCodeCompletionResult() const
    { return data().canConvert<CodeCompletionResult>(); }

private:
    unsigned m_completionOperator;
    mutable QChar m_typedChar;
    QList<CodeCompletionResult> m_overloads;
};

/// @return True, because clang always returns priorities for sorting
bool ClangAssistProposalModel::isSortable(const QString &prefix) const
{
    Q_UNUSED(prefix)
    return true;
}

} // namespace Internal
} // namespace ClangCodeModel

bool ClangAssistProposalItem::prematurelyApplies(const QChar &typedChar) const
{
    bool ok = false;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT)
        ok = QString::fromLatin1("(,").contains(typedChar);
    else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL)
        ok = (typedChar == QLatin1Char('/')) && text().endsWith(QLatin1Char('/'));
    else if (!isCodeCompletionResult())
        ok = (typedChar == QLatin1Char('(')); /* && data().canConvert<CompleteFunctionDeclaration>()*/ //###
    else if (originalItem().completionKind() == CodeCompletionResult::ObjCMessageCompletionKind)
        ok = QString::fromLatin1(";.,").contains(typedChar);
    else
        ok = QString::fromLatin1(";.,:(").contains(typedChar);

    if (ok)
        m_typedChar = typedChar;

    return ok;
}

void ClangAssistProposalItem::applyContextualContent(TextEditorWidget *editorWidget,
                                                     int basePosition) const
{
    const CodeCompletionResult ccr = originalItem();

    QString toInsert = text();
    QString extraChars;
    int extraLength = 0;
    int cursorOffset = 0;

    bool autoParenthesesEnabled = true;
    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT) {
        extraChars += QLatin1Char(')');
        if (m_typedChar == QLatin1Char('(')) // Eat the opening parenthesis
            m_typedChar = QChar();
    } else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL) {
        if (!toInsert.endsWith(QLatin1Char('/'))) {
            extraChars += QLatin1Char((m_completionOperator == T_ANGLE_STRING_LITERAL) ? '>' : '"');
        } else {
            if (m_typedChar == QLatin1Char('/')) // Eat the slash
                m_typedChar = QChar();
        }
    } else if (ccr.isValid()) {
        const CompletionSettings &completionSettings =
                TextEditorSettings::instance()->completionSettings();
        const bool autoInsertBrackets = completionSettings.m_autoInsertBrackets;

        if (autoInsertBrackets &&
                (ccr.completionKind() == CodeCompletionResult::FunctionCompletionKind
                 || ccr.completionKind() == CodeCompletionResult::DestructorCompletionKind
                 || ccr.completionKind() == CodeCompletionResult::SignalCompletionKind
                 || ccr.completionKind() == CodeCompletionResult::SlotCompletionKind)) {
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
            const QChar characterAtCursor = editorWidget->characterAt(editorWidget->position());
            bool endWithSemicolon = m_typedChar == QLatin1Char(';')/*
                                            || (function->returnType()->isVoidType() && m_completionOperator != T_COLON_COLON)*/; //###
            const QChar semicolon = m_typedChar.isNull() ? QLatin1Char(';') : m_typedChar;

            if (endWithSemicolon && characterAtCursor == semicolon) {
                endWithSemicolon = false;
                m_typedChar = QChar();
            }

            // If the function takes no arguments, automatically place the closing parenthesis
            if (!isOverloaded() && !ccr.hasParameters() && skipClosingParenthesis) {
                extraChars += QLatin1Char(')');
                if (endWithSemicolon) {
                    extraChars += semicolon;
                    m_typedChar = QChar();
                }
            } else if (autoParenthesesEnabled) {
                const QChar lookAhead = editorWidget->characterAt(editorWidget->position() + 1);
                if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                    extraChars += QLatin1Char(')');
                    --cursorOffset;
                    if (endWithSemicolon) {
                        extraChars += semicolon;
                        --cursorOffset;
                        m_typedChar = QChar();
                    }
                }
            }
        }

#if 0
        if (autoInsertBrackets && data().canConvert<CompleteFunctionDeclaration>()) {
            if (m_typedChar == QLatin1Char('('))
                m_typedChar = QChar();

            // everything from the closing parenthesis on are extra chars, to
            // make sure an auto-inserted ")" gets replaced by ") const" if necessary
            int closingParen = toInsert.lastIndexOf(QLatin1Char(')'));
            extraChars = toInsert.mid(closingParen);
            toInsert.truncate(closingParen);
        }
#endif
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!m_typedChar.isNull()) {
        extraChars += m_typedChar;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    // Avoid inserting characters that are already there
    const int endsPosition = editorWidget->position(EndOfLinePosition);
    const QString existingText = editorWidget->textAt(editorWidget->position(), endsPosition - editorWidget->position());
    int existLength = 0;
    if (!existingText.isEmpty()) {
        // Calculate the exist length in front of the extra chars
        existLength = toInsert.length() - (editorWidget->position() - basePosition);
        while (!existingText.startsWith(toInsert.right(existLength))) {
            if (--existLength == 0)
                break;
        }
    }
    for (int i = 0; i < extraChars.length(); ++i) {
        const QChar a = extraChars.at(i);
        const QChar b = editorWidget->characterAt(editorWidget->position() + i + existLength);
        if (a == b)
            ++extraLength;
        else
            break;
    }
    toInsert += extraChars;

    // Insert the remainder of the name
    const int length = editorWidget->position() - basePosition + existLength + extraLength;
    editorWidget->setCursorPosition(basePosition);
    editorWidget->replace(length, toInsert);
    if (cursorOffset)
        editorWidget->setCursorPosition(editorWidget->position() + cursorOffset);
}

bool ClangCompletionAssistInterface::objcEnabled() const
{
    return m_clangWrapper->objcEnabled();
}

ClangCompletionAssistInterface::ClangCompletionAssistInterface(ClangCompleter::Ptr clangWrapper,
        QTextDocument *document,
        int position,
        const QString &fileName,
        AssistReason reason,
        const QStringList &options,
        const QList<ProjectPart::HeaderPath> &headerPaths,
        const PchInfo::Ptr &pchInfo,
        const LanguageFeatures &features)
    : AssistInterface(document, position, fileName, reason)
    , m_clangWrapper(clangWrapper)
    , m_options(options)
    , m_headerPaths(headerPaths)
    , m_savedPchPointer(pchInfo)
    , m_languageFeatures(features)
{
    Q_ASSERT(!clangWrapper.isNull());

    CppModelManager *mmi = CppModelManager::instance();
    m_unsavedFiles = Utils::createUnsavedFiles(mmi->workingCopy());
}

ClangCompletionAssistProcessor::ClangCompletionAssistProcessor()
    : m_model(new ClangAssistProposalModel)
{
}

ClangCompletionAssistProcessor::~ClangCompletionAssistProcessor()
{
}

IAssistProposal *ClangCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    m_interface.reset(static_cast<const ClangCompletionAssistInterface *>(interface));

    if (interface->reason() != ExplicitlyInvoked && !accepts())
        return 0;

    int index = startCompletionHelper();
    if (index != -1) {
        if (m_hintProposal)
            return m_hintProposal;

        m_model->m_sortable = (m_model->m_completionOperator != T_EOF_SYMBOL);
        return createContentProposal();
    }

    return 0;
}

int ClangCompletionAssistProcessor::startCompletionHelper()
{
    //### TODO: clean-up this method, some calculated values might not be used anymore.

    Q_ASSERT(m_model);

    const int startOfName = findStartOfName();
    m_startPosition = startOfName;
    m_model->m_completionOperator = T_EOF_SYMBOL;

    int endOfOperator = m_startPosition;

    // Skip whitespace preceding this position
    while (m_interface->characterAt(endOfOperator - 1).isSpace())
        --endOfOperator;

    const QString fileName = m_interface->fileName();

    int endOfExpression = startOfOperator(endOfOperator,
                                          &m_model->m_completionOperator,
                                          /*want function call =*/ true);

    if (m_model->m_completionOperator == T_EOF_SYMBOL) {
        endOfOperator = m_startPosition;
    } else if (m_model->m_completionOperator == T_DOXY_COMMENT) {
        for (int i = 1; i < T_DOXY_LAST_TAG; ++i)
            addCompletionItem(QString::fromLatin1(doxygenTagSpell(i)),
                              m_icons.keywordIcon());
        return m_startPosition;
    }

    // Pre-processor completion
    //### TODO: check if clang can do pp completion
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
            m_startPosition = startOfName;
        return m_startPosition;
    }

    ExpressionUnderCursor expressionUnderCursor(m_interface->languageFeatures());
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

    tc.setPosition(endOfExpression);

    if (m_model->m_completionOperator) {
        const QString expression = expressionUnderCursor(tc);

        if (m_model->m_completionOperator == T_LPAREN) {
            if (expression.endsWith(QLatin1String("SIGNAL")))
                m_model->m_completionOperator = T_SIGNAL;

            else if (expression.endsWith(QLatin1String("SLOT")))
                m_model->m_completionOperator = T_SLOT;

            else if (m_interface->position() != endOfOperator) {
                // We don't want a function completion when the cursor isn't at the opening brace
                m_model->m_completionOperator = T_EOF_SYMBOL;
                m_startPosition = startOfName;
            }
        }
    }

    int line = 0, column = 0;
    Convenience::convertPosition(m_interface->textDocument(), endOfOperator, &line, &column);
    return startCompletionInternal(fileName, line, column, endOfOperator);
}

int ClangCompletionAssistProcessor::startOfOperator(int pos,
                                                    unsigned *kind,
                                                    bool wantFunctionCall) const
{
    const QChar ch  = pos > -1 ? m_interface->characterAt(pos - 1) : QChar();
    const QChar ch2 = pos >  0 ? m_interface->characterAt(pos - 2) : QChar();
    const QChar ch3 = pos >  1 ? m_interface->characterAt(pos - 3) : QChar();

    int start = pos - activationSequenceChar(ch, ch2, ch3, kind, wantFunctionCall);
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
            ExpressionUnderCursor expressionUnderCursor(m_interface->languageFeatures());
            if (expressionUnderCursor.startOfFunctionCall(tc) == -1) {
                *kind = T_EOF_SYMBOL;
                start = pos;
            }
        }

        SimpleLexer tokenize;
        tokenize.setLanguageFeatures(m_interface->languageFeatures());
        tokenize.setSkipComments(false);
        const Tokens &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
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
                                     && *kind != T_SLASH))) {
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
        else if (*kind == T_STRING_LITERAL || *kind == T_ANGLE_STRING_LITERAL || *kind == T_SLASH) {
            bool include = false;
            if (tokens.size() >= 3) {
                if (tokens.at(0).is(T_POUND) && tokens.at(1).is(T_IDENTIFIER) && (tokens.at(2).is(T_STRING_LITERAL) ||
                                                                                  tokens.at(2).is(T_ANGLE_STRING_LITERAL))) {
                    const Token &directiveToken = tokens.at(1);
                    QString directive = tc.block().text().mid(directiveToken.bytesBegin(),
                                                              directiveToken.bytes());
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
            }
        }
    }

    return start;
}

int ClangCompletionAssistProcessor::findStartOfName(int pos) const
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

bool ClangCompletionAssistProcessor::accepts() const
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
                    LanguageFeatures lf = tokenize.languageFeatures();
                    lf.qtMocRunEnabled = true;
                    lf.objCEnabled = true;
                    tokenize.setLanguageFeatures(lf);
                    tokenize.setSkipComments(false);
                    const Tokens &tokens = tokenize(tc.block().text(), BackwardsScanner::previousBlockState(tc.block()));
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
                                line.midRef(idToken.bytesBegin(),
                                            idToken.bytesEnd() - idToken.bytesBegin());
                        if (identifier == QLatin1String("include")
                                || identifier == QLatin1String("include_next")
                                || (m_interface->objcEnabled() && identifier == QLatin1String("import"))) {
                            return true;
                        }
                    }
                }
            }
        }
    }

    return false;
}

IAssistProposal *ClangCompletionAssistProcessor::createContentProposal()
{
    m_model->loadContent(m_completions);
    return new ClangAssistProposal(m_startPosition, m_model.take());
}

/// Seach backwards in the document starting from pos to find the first opening
/// parenthesis. Nested parenthesis are skipped.
static int findOpenParen(QTextDocument *doc, int start)
{
    unsigned parenCount = 1;
    for (int pos = start; pos >= 0; --pos) {
        const QChar ch = doc->characterAt(pos);
        if (ch == QLatin1Char('(')) {
            --parenCount;
            if (parenCount == 0)
                return pos;
        } else if (ch == QLatin1Char(')')) {
            ++parenCount;
        }
    }
    return -1;
}

static QByteArray modifyInput(QTextDocument *doc, int endOfExpression) {
    int comma = endOfExpression;
    while (comma > 0) {
        const QChar ch = doc->characterAt(comma);
        if (ch == QLatin1Char(','))
            break;
        if (ch == QLatin1Char(';') || ch == QLatin1Char('{') || ch == QLatin1Char('}')) {
            // Safety net: we don't seem to have "connect(pointer, SIGNAL(" as
            // input, so stop searching.
            comma = -1;
            break;
        }
        --comma;
    }
    if (comma < 0)
        return QByteArray();
    const int openBrace = findOpenParen(doc, comma);
    if (openBrace < 0)
        return QByteArray();

    QByteArray modifiedInput = doc->toPlainText().toUtf8();
    const int len = endOfExpression - comma;
    QByteArray replacement(len - 4, ' ');
    replacement.append(")->");
    modifiedInput.replace(comma, len, replacement);
    modifiedInput.insert(openBrace, '(');
    return modifiedInput;
}

int ClangCompletionAssistProcessor::startCompletionInternal(const QString fileName,
                                                            unsigned line,
                                                            unsigned column,
                                                            int endOfExpression)
{
    bool signalCompletion = false;
    bool slotCompletion = false;
    QByteArray modifiedInput;

    if (m_model->m_completionOperator == T_SIGNAL) {
        signalCompletion = true;
        modifiedInput = modifyInput(m_interface->textDocument(), endOfExpression);
    } else if (m_model->m_completionOperator == T_SLOT) {
        slotCompletion = true;
        modifiedInput = modifyInput(m_interface->textDocument(), endOfExpression);
    } else if (m_model->m_completionOperator == T_LPAREN) {
        // Find the expression that precedes the current name
        int index = endOfExpression;
        while (m_interface->characterAt(index - 1).isSpace())
            --index;

        QTextCursor tc(m_interface->textDocument());
        tc.setPosition(index);
        ExpressionUnderCursor euc(m_interface->languageFeatures());
        index = euc.startOfFunctionCall(tc);
        int nameStart = findStartOfName(index);
        QTextCursor tc2(m_interface->textDocument());
        tc2.setPosition(nameStart);
        tc2.setPosition(index, QTextCursor::KeepAnchor);
        const QString functionName = tc2.selectedText().trimmed();
        int l = line, c = column;
        Convenience::convertPosition(m_interface->textDocument(), nameStart, &l, &c);

        qCDebug(log)<<"complete constructor or function @" << line<<":"<<column << "->"<<l<<":"<<c;

        const QList<CodeCompletionResult> completions = unfilteredCompletion(
                    m_interface.data(), fileName, l, c, QByteArray(), signalCompletion || slotCompletion);
        QList<CodeCompletionResult> functionCompletions;
        foreach (const CodeCompletionResult &ccr, completions) {
            if (ccr.completionKind() == CodeCompletionResult::FunctionCompletionKind
                    || ccr.completionKind() == CodeCompletionResult::ConstructorCompletionKind
                    || ccr.completionKind() == CodeCompletionResult::DestructorCompletionKind
                    || ccr.completionKind() == CodeCompletionResult::SignalCompletionKind
                    || ccr.completionKind() == CodeCompletionResult::SlotCompletionKind)
                if (ccr.text() == functionName)
                    functionCompletions.append(ccr);
        }

        if (!functionCompletions.isEmpty()) {
            IFunctionHintProposalModel *model = new ClangFunctionHintModel(functionCompletions);
            m_hintProposal = new FunctionHintProposal(m_startPosition, model);
            return m_startPosition;
        }
    }

    const QIcon snippetIcon = QIcon(QLatin1String(SNIPPET_ICON_PATH));
    QList<CodeCompletionResult> completions = unfilteredCompletion(
                m_interface.data(), fileName, line, column, modifiedInput, signalCompletion || slotCompletion);
    QHash<QString, ClangAssistProposalItem *> items;
    foreach (const CodeCompletionResult &ccr, completions) {
        if (!ccr.isValid())
            continue;
        if (signalCompletion && ccr.completionKind() != CodeCompletionResult::SignalCompletionKind)
            continue;
        if (slotCompletion && ccr.completionKind() != CodeCompletionResult::SlotCompletionKind)
            continue;

        const QString txt(ccr.text());
        ClangAssistProposalItem *item = items.value(txt, 0);
        if (item) {
            item->addOverload(ccr);
        } else {
            item = new ClangAssistProposalItem;
            items.insert(txt, item);
            item->setText(txt);
            item->setDetail(ccr.hint());
            item->setOrder(ccr.priority());

            const QString snippet = ccr.snippet();
            if (!snippet.isEmpty())
                item->setData(snippet);
            else
                item->setData(qVariantFromValue(ccr));
        }

        // FIXME: show the effective accessebility instead of availability
        switch (ccr.completionKind()) {
        case CodeCompletionResult::ClassCompletionKind: item->setIcon(m_icons.iconForType(Icons::ClassIconType)); break;
        case CodeCompletionResult::EnumCompletionKind: item->setIcon(m_icons.iconForType(Icons::EnumIconType)); break;
        case CodeCompletionResult::EnumeratorCompletionKind: item->setIcon(m_icons.iconForType(Icons::EnumeratorIconType)); break;

        case CodeCompletionResult::ConstructorCompletionKind: // fall through
        case CodeCompletionResult::DestructorCompletionKind: // fall through
        case CodeCompletionResult::FunctionCompletionKind:
        case CodeCompletionResult::ObjCMessageCompletionKind:
            switch (ccr.availability()) {
            case CodeCompletionResult::Available:
            case CodeCompletionResult::Deprecated:
                item->setIcon(m_icons.iconForType(Icons::FuncPublicIconType));
                break;
            default:
                item->setIcon(m_icons.iconForType(Icons::FuncPrivateIconType));
                break;
            }
            break;

        case CodeCompletionResult::SignalCompletionKind:
            item->setIcon(m_icons.iconForType(Icons::SignalIconType));
            break;

        case CodeCompletionResult::SlotCompletionKind:
            switch (ccr.availability()) {
            case CodeCompletionResult::Available:
            case CodeCompletionResult::Deprecated:
                item->setIcon(m_icons.iconForType(Icons::SlotPublicIconType));
                break;
            case CodeCompletionResult::NotAccessible:
            case CodeCompletionResult::NotAvailable:
                item->setIcon(m_icons.iconForType(Icons::SlotPrivateIconType));
                break;
            }
            break;

        case CodeCompletionResult::NamespaceCompletionKind: item->setIcon(m_icons.iconForType(Icons::NamespaceIconType)); break;
        case CodeCompletionResult::PreProcessorCompletionKind: item->setIcon(m_icons.iconForType(Icons::MacroIconType)); break;
        case CodeCompletionResult::VariableCompletionKind:
            switch (ccr.availability()) {
            case CodeCompletionResult::Available:
            case CodeCompletionResult::Deprecated:
                item->setIcon(m_icons.iconForType(Icons::VarPublicIconType));
                break;
            default:
                item->setIcon(m_icons.iconForType(Icons::VarPrivateIconType));
                break;
            }
            break;

        case CodeCompletionResult::KeywordCompletionKind:
            item->setIcon(m_icons.iconForType(Icons::KeywordIconType));
            break;

        case CodeCompletionResult::ClangSnippetKind:
            item->setIcon(snippetIcon);
            break;

        default:
            break;
        }
    }

    foreach (ClangAssistProposalItem *item, items.values())
        m_completions.append(item);

    if (m_model->m_completionOperator == T_EOF_SYMBOL)
        addSnippets();

    return m_startPosition;
}

/**
 * @brief Creates completion proposals for #include and given cursor
 * @param cursor - cursor placed after opening bracked or quote
 * @return false if completions list is empty
 */
bool ClangCompletionAssistProcessor::completeInclude(const QTextCursor &cursor)
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
    ProjectPart::HeaderPaths headerPaths = m_interface->headerPaths();
    const ProjectPart::HeaderPath currentFilePath(QFileInfo(m_interface->fileName()).path(),
                                                  ProjectPart::HeaderPath::IncludePath);
    if (!headerPaths.contains(currentFilePath))
        headerPaths.append(currentFilePath);

    ::Utils::MimeDatabase mdb;
    const ::Utils::MimeType mimeType = mdb.mimeTypeForName(QLatin1String("text/x-c++hdr"));
    const QStringList suffixes = mimeType.suffixes();

    foreach (const ProjectPart::HeaderPath &headerPath, headerPaths) {
        QString realPath = headerPath.path;
        if (!directoryPrefix.isEmpty()) {
            realPath += QLatin1Char('/');
            realPath += directoryPrefix;
            if (headerPath.isFrameworkPath())
                realPath += QLatin1String(".framework/Headers");
        }
        completeIncludePath(realPath, suffixes);
    }

    return !m_completions.isEmpty();
}

/**
 * @brief Adds #include completion proposals using given include path
 * @param realPath - one of directories where compiler searches includes
 * @param suffixes - file suffixes for C/C++ header files
 */
void ClangCompletionAssistProcessor::completeIncludePath(const QString &realPath,
                                                         const QStringList &suffixes)
{
    QDirIterator i(realPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    //: Parent folder for proposed #include completion
    const QString hint = tr("Location: %1").arg(QDir::toNativeSeparators(QDir::cleanPath(realPath)));
    while (i.hasNext()) {
        const QString fileName = i.next();
        const QFileInfo fileInfo = i.fileInfo();
        const QString suffix = fileInfo.suffix();
        if (suffix.isEmpty() || suffixes.contains(suffix)) {
            QString text = fileName.mid(realPath.length() + 1);
            if (fileInfo.isDir())
                text += QLatin1Char('/');

            ClangAssistProposalItem *item = new ClangAssistProposalItem;
            item->setText(text);
            item->setDetail(hint);
            item->setIcon(m_icons.keywordIcon());
            item->keepCompletionOperator(m_model->m_completionOperator);
            m_completions.append(item);
        }
    }
}

void ClangCompletionAssistProcessor::completePreprocessor()
{
    foreach (const QString &preprocessorCompletion, m_preprocessorCompletions)
        addCompletionItem(preprocessorCompletion,
                          m_icons.iconForType(Icons::MacroIconType));

    if (m_interface->objcEnabled())
        addCompletionItem(QLatin1String("import"),
                          m_icons.iconForType(Icons::MacroIconType));
}

void ClangCompletionAssistProcessor::addCompletionItem(const QString &text,
                                                       const QIcon &icon,
                                                       int order,
                                                       const QVariant &data)
{
    ClangAssistProposalItem *item = new ClangAssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    item->setData(data);
    item->keepCompletionOperator(m_model->m_completionOperator);
    m_completions.append(item);
}
