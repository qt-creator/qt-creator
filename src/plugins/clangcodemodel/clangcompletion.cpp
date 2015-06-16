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
#include "clangcompletioncontextanalyzer.h"
#include "clangutils.h"
#include "completionchunkstotextconverter.h"
#include "pchmanager.h"

#include <coreplugin/icore.h>
#include <coreplugin/idocument.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Token.h>
#include <cplusplus/MatchingText.h>

#include <cppeditor/cppeditorconstants.h>

#include <cpptools/baseeditordocumentparser.h>
#include <cpptools/cppdoxygen.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cppworkingcopy.h>

#include <texteditor/texteditor.h>
#include <texteditor/convenience.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/texteditorsettings.h>
#include <texteditor/completionsettings.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/qtcassert.h>

#include <QCoreApplication>
#include <QDirIterator>
#include <QLoggingCategory>
#include <QTextCursor>
#include <QTextDocument>

using namespace ClangBackEnd;
using namespace ClangCodeModel;
using namespace ClangCodeModel::Internal;
using namespace CPlusPlus;
using namespace CppTools;
using namespace TextEditor;

namespace {

const char SNIPPET_ICON_PATH[] = ":/texteditor/images/snippet.png";

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

QList<AssistProposalItem *> toAssistProposalItems(const CodeCompletions &completions)
{
    static CPlusPlus::Icons m_icons; // de-deduplicate

    QList<AssistProposalItem *> result;

    bool signalCompletion = false; // TODO
    bool slotCompletion = false; // TODO

    const QIcon snippetIcon = QIcon(QLatin1String(SNIPPET_ICON_PATH));
    QHash<QString, ClangAssistProposalItem *> items;
    foreach (const CodeCompletion &ccr, completions) {
        if (ccr.text().isEmpty()) // TODO: Make isValid()?
            continue;
        if (signalCompletion && ccr.completionKind() != CodeCompletion::SignalCompletionKind)
            continue;
        if (slotCompletion && ccr.completionKind() != CodeCompletion::SlotCompletionKind)
            continue;

        const QString txt(ccr.text().toString());
        ClangAssistProposalItem *item = items.value(txt, 0);
        if (item) {
            item->addOverload(ccr);
        } else {
            item = new ClangAssistProposalItem;
            items.insert(txt, item);
            item->setText(txt);
            item->setDetail(ccr.hint().toString());
            item->setOrder(ccr.priority());

            const QString snippet = ccr.snippet().toString();
            if (!snippet.isEmpty())
                item->setData(snippet);
            else
                item->setData(qVariantFromValue(ccr));
        }

        // FIXME: show the effective accessebility instead of availability
        switch (ccr.completionKind()) {
        case CodeCompletion::ClassCompletionKind:
        case CodeCompletion::TemplateClassCompletionKind:
            item->setIcon(m_icons.iconForType(Icons::ClassIconType)); break;
        case CodeCompletion::EnumerationCompletionKind: item->setIcon(m_icons.iconForType(Icons::EnumIconType)); break;
        case CodeCompletion::EnumeratorCompletionKind: item->setIcon(m_icons.iconForType(Icons::EnumeratorIconType)); break;

        case CodeCompletion::ConstructorCompletionKind: // fall through
        case CodeCompletion::DestructorCompletionKind: // fall through
        case CodeCompletion::FunctionCompletionKind:
        case CodeCompletion::TemplateFunctionCompletionKind:
        case CodeCompletion::ObjCMessageCompletionKind:
            switch (ccr.availability()) {
            case CodeCompletion::Available:
            case CodeCompletion::Deprecated:
                item->setIcon(m_icons.iconForType(Icons::FuncPublicIconType));
                break;
            default:
                item->setIcon(m_icons.iconForType(Icons::FuncPrivateIconType));
                break;
            }
            break;

        case CodeCompletion::SignalCompletionKind:
            item->setIcon(m_icons.iconForType(Icons::SignalIconType));
            break;

        case CodeCompletion::SlotCompletionKind:
            switch (ccr.availability()) {
            case CodeCompletion::Available:
            case CodeCompletion::Deprecated:
                item->setIcon(m_icons.iconForType(Icons::SlotPublicIconType));
                break;
            case CodeCompletion::NotAccessible:
            case CodeCompletion::NotAvailable:
                item->setIcon(m_icons.iconForType(Icons::SlotPrivateIconType));
                break;
            }
            break;

        case CodeCompletion::NamespaceCompletionKind: item->setIcon(m_icons.iconForType(Icons::NamespaceIconType)); break;
        case CodeCompletion::PreProcessorCompletionKind: item->setIcon(m_icons.iconForType(Icons::MacroIconType)); break;
        case CodeCompletion::VariableCompletionKind:
            switch (ccr.availability()) {
            case CodeCompletion::Available:
            case CodeCompletion::Deprecated:
                item->setIcon(m_icons.iconForType(Icons::VarPublicIconType));
                break;
            default:
                item->setIcon(m_icons.iconForType(Icons::VarPrivateIconType));
                break;
            }
            break;

        case CodeCompletion::KeywordCompletionKind:
            item->setIcon(m_icons.iconForType(Icons::KeywordIconType));
            break;

        case CodeCompletion::ClangSnippetKind:
            item->setIcon(snippetIcon);
            break;

        case CodeCompletion::Other:
            break;
        }
    }

    foreach (ClangAssistProposalItem *item, items.values())
        result.append(item);

    return result;
}

bool isFunctionHintLikeCompletion(CodeCompletion::Kind kind)
{
    return kind == CodeCompletion::FunctionCompletionKind
        || kind == CodeCompletion::ConstructorCompletionKind
        || kind == CodeCompletion::DestructorCompletionKind
        || kind == CodeCompletion::SignalCompletionKind
        || kind == CodeCompletion::SlotCompletionKind;
}

QVector<CodeCompletion> matchingFunctionCompletions(const QVector<CodeCompletion> completions,
                                                    const QString &functionName)
{
    QVector<CodeCompletion> matching;

    foreach (const CodeCompletion &completion, completions) {
        if (isFunctionHintLikeCompletion(completion.completionKind())
                && completion.text().toString() == functionName) {
            matching.append(completion);
        }
    }

    return matching;
}

} // Anonymous

namespace ClangCodeModel {
namespace Internal {

// -----------------------------
// ClangCompletionAssistProvider
// -----------------------------
ClangCompletionAssistProvider::ClangCompletionAssistProvider(IpcCommunicator::Ptr ipcCommunicator)
    : m_ipcCommunicator(ipcCommunicator)
{
    QTC_CHECK(m_ipcCommunicator);
}

IAssistProvider::RunType ClangCompletionAssistProvider::runType() const
{
    return Asynchronous;
}

IAssistProcessor *ClangCompletionAssistProvider::createProcessor() const
{
    return new ClangCompletionAssistProcessor;
}

AssistInterface *ClangCompletionAssistProvider::createAssistInterface(
        const QString &filePath,
        const TextEditorWidget *textEditorWidget,
        const LanguageFeatures &languageFeatures,
        int position,
        AssistReason reason) const
{
    Q_UNUSED(languageFeatures);
    const ProjectPart::Ptr part = Utils::projectPartForFile(filePath);
    QTC_ASSERT(!part.isNull(), return 0);

    const PchInfo::Ptr pchInfo = PchManager::instance()->pchInfo(part);
    return new ClangCompletionAssistInterface(m_ipcCommunicator,
                                              textEditorWidget,
                                              position,
                                              filePath,
                                              reason,
                                              part->headerPaths,
                                              pchInfo,
                                              part->languageFeatures);
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

ClangFunctionHintModel::ClangFunctionHintModel(const CodeCompletions &functionSymbols)
    : m_functionSymbols(functionSymbols)
    , m_currentArg(-1)
{}

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
    return CompletionChunksToTextConverter::convert(m_functionSymbols.at(index).chunks());
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

/// @return True, because clang always returns priorities for sorting
bool ClangAssistProposalModel::isSortable(const QString &prefix) const
{
    Q_UNUSED(prefix)
    return true;
}

bool ClangAssistProposalItem::prematurelyApplies(const QChar &typedChar) const
{
    bool ok = false;

    if (m_completionOperator == T_SIGNAL || m_completionOperator == T_SLOT)
        ok = QString::fromLatin1("(,").contains(typedChar);
    else if (m_completionOperator == T_STRING_LITERAL || m_completionOperator == T_ANGLE_STRING_LITERAL)
        ok = (typedChar == QLatin1Char('/')) && text().endsWith(QLatin1Char('/'));
    else if (!isCodeCompletion())
        ok = (typedChar == QLatin1Char('(')); /* && data().canConvert<CompleteFunctionDeclaration>()*/ //###
    else if (originalItem().completionKind() == CodeCompletion::ObjCMessageCompletionKind)
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
    const CodeCompletion ccr = originalItem();

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
    } else if (!ccr.text().isEmpty()) {
        const CompletionSettings &completionSettings =
                TextEditorSettings::instance()->completionSettings();
        const bool autoInsertBrackets = completionSettings.m_autoInsertBrackets;

        if (autoInsertBrackets &&
                (ccr.completionKind() == CodeCompletion::FunctionCompletionKind
                 || ccr.completionKind() == CodeCompletion::DestructorCompletionKind
                 || ccr.completionKind() == CodeCompletion::SignalCompletionKind
                 || ccr.completionKind() == CodeCompletion::SlotCompletionKind)) {
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

bool ClangAssistProposalItem::isOverloaded() const
{
    return !m_overloads.isEmpty();
}

void ClangAssistProposalItem::addOverload(const CodeCompletion &ccr)
{
    m_overloads.append(ccr);
}

CodeCompletion ClangAssistProposalItem::originalItem() const
{
    const QVariant &value = data();
    if (value.canConvert<CodeCompletion>())
        return value.value<CodeCompletion>();
    else
        return CodeCompletion();
}

bool ClangAssistProposalItem::isCodeCompletion() const
{
    return data().canConvert<CodeCompletion>();
}

bool ClangCompletionAssistInterface::objcEnabled() const
{
    return true; // TODO:
}

const ProjectPart::HeaderPaths &ClangCompletionAssistInterface::headerPaths() const
{
    return m_headerPaths;
}

LanguageFeatures ClangCompletionAssistInterface::languageFeatures() const
{
    return m_languageFeatures;
}

void ClangCompletionAssistInterface::setHeaderPaths(const ProjectPart::HeaderPaths &headerPaths)
{
    m_headerPaths = headerPaths;
}

const TextEditor::TextEditorWidget *ClangCompletionAssistInterface::textEditorWidget() const
{
    return m_textEditorWidget;
}

ClangCompletionAssistInterface::ClangCompletionAssistInterface(
        IpcCommunicator::Ptr ipcCommunicator,
        const TextEditorWidget *textEditorWidget,
        int position,
        const QString &fileName,
        AssistReason reason,
        const ProjectPart::HeaderPaths &headerPaths,
        const PchInfo::Ptr &pchInfo,
        const LanguageFeatures &features)
    : AssistInterface(textEditorWidget->document(), position, fileName, reason)
    , m_ipcCommunicator(ipcCommunicator)
    , m_headerPaths(headerPaths)
    , m_savedPchPointer(pchInfo)
    , m_languageFeatures(features)
    , m_textEditorWidget(textEditorWidget)
{
    CppModelManager *mmi = CppModelManager::instance();
    m_unsavedFiles = Utils::createUnsavedFiles(mmi->workingCopy());
}

IpcCommunicator::Ptr ClangCompletionAssistInterface::ipcCommunicator() const
{
    return m_ipcCommunicator;
}

const UnsavedFiles &ClangCompletionAssistInterface::unsavedFiles() const
{
    return m_unsavedFiles;
}

ClangCompletionAssistProcessor::ClangCompletionAssistProcessor()
    : m_completionOperator(T_EOF_SYMBOL)
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

    return startCompletionHelper(); // == 0 if results are calculated asynchronously
}

void ClangCompletionAssistProcessor::asyncCompletionsAvailable(const CodeCompletions &completions)
{
    switch (m_sentRequestType) {
    case CompletionRequestType::NormalCompletion:
        onCompletionsAvailable(completions);
        break;
    case CompletionRequestType::FunctionHintCompletion:
        onFunctionHintCompletionsAvailable(completions);
        break;
    default:
        QTC_CHECK(!"Unhandled ClangCompletionAssistProcessor::CompletionRequestType");
        break;
    }
}

const TextEditorWidget *ClangCompletionAssistProcessor::textEditorWidget() const
{
    return m_interface->textEditorWidget();
}

/// Seach backwards in the document starting from pos to find the first opening
/// parenthesis. Nested parenthesis are skipped.
static int findOpenParen(QTextDocument *document, int start)
{
    unsigned parenCount = 1;
    for (int position = start; position >= 0; --position) {
        const QChar ch = document->characterAt(position);
        if (ch == QLatin1Char('(')) {
            --parenCount;
            if (parenCount == 0)
                return position;
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

IAssistProposal *ClangCompletionAssistProcessor::startCompletionHelper()
{
    sendFileContent(Utils::projectFilePathForFile(m_interface->fileName()), QByteArray()); // TODO: Remoe

    ClangCompletionContextAnalyzer analyzer(m_interface.data(), m_interface->languageFeatures());
    analyzer.analyze();
    m_completionOperator = analyzer.completionOperator();
    m_positionForProposal = analyzer.positionForProposal();

    QByteArray modifiedFileContent;

    const ClangCompletionContextAnalyzer::CompletionAction action = analyzer.completionAction();
    switch (action) {
    case ClangCompletionContextAnalyzer::CompleteDoxygenKeyword:
        if (completeDoxygenKeywords())
            return createProposal();
        break;
    case ClangCompletionContextAnalyzer::CompleteIncludePath:
        if (completeInclude(analyzer.positionEndOfExpression()))
            return createProposal();
        break;
    case ClangCompletionContextAnalyzer::CompletePreprocessorDirective:
        if (completePreprocessorDirectives())
            return createProposal();
        break;
    case ClangCompletionContextAnalyzer::CompleteSignal:
    case ClangCompletionContextAnalyzer::CompleteSlot:
        modifiedFileContent = modifyInput(m_interface->textDocument(),
                                          analyzer.positionEndOfExpression());
        // Fall through!
    case ClangCompletionContextAnalyzer::PassThroughToLibClang: {
        m_addSnippets = m_completionOperator == T_EOF_SYMBOL;
        m_sentRequestType = NormalCompletion;
        sendCompletionRequest(analyzer.positionForClang(), modifiedFileContent);
        break;
    }
    case ClangCompletionContextAnalyzer::PassThroughToLibClangAfterLeftParen: {
        m_sentRequestType = FunctionHintCompletion;
        m_functionName = analyzer.functionName();
        sendCompletionRequest(analyzer.positionForClang(), QByteArray());
        break;
    }
    default:
        break;
    }

    return 0;
}

// TODO: Extract duplicated logic from InternalCppCompletionAssistProcessor::startOfOperator
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
        else if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT)
                 || tk.is(T_CPP_DOXY_COMMENT) || tk.is(T_DOXY_COMMENT)
                 || (tk.isLiteral() && (*kind != T_STRING_LITERAL
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

/**
 * @brief Creates completion proposals for #include and given cursor
 * @param cursor - cursor placed after opening bracked or quote
 * @return false if completions list is empty
 */
bool ClangCompletionAssistProcessor::completeInclude(const QTextCursor &cursor)
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

bool ClangCompletionAssistProcessor::completeInclude(int position)
{
    QTextCursor textCursor(m_interface->textDocument()); // TODO: Simplify, move into function
    textCursor.setPosition(position);
    return completeInclude(textCursor);
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
            item->keepCompletionOperator(m_completionOperator);
            m_completions.append(item);
        }
    }
}

bool ClangCompletionAssistProcessor::completePreprocessorDirectives()
{
    foreach (const QString &preprocessorCompletion, m_preprocessorCompletions)
        addCompletionItem(preprocessorCompletion,
                          m_icons.iconForType(Icons::MacroIconType));

    if (m_interface->objcEnabled())
        addCompletionItem(QLatin1String("import"),
                          m_icons.iconForType(Icons::MacroIconType));

    return !m_completions.isEmpty();
}

bool ClangCompletionAssistProcessor::completeDoxygenKeywords()
{
    for (int i = 1; i < T_DOXY_LAST_TAG; ++i)
        addCompletionItem(QString::fromLatin1(doxygenTagSpell(i)), m_icons.keywordIcon());
    return !m_completions.isEmpty();
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
    item->keepCompletionOperator(m_completionOperator);
    m_completions.append(item);
}

void ClangCompletionAssistProcessor::sendFileContent(const QString &projectFilePath,
                                                     const QByteArray &modifiedFileContent)
{
    const QString filePath = m_interface->fileName();
    const QByteArray unsavedContent = modifiedFileContent.isEmpty()
            ? m_interface->textDocument()->toPlainText().toUtf8()
            : modifiedFileContent;
    const bool hasUnsavedContent = true; // TODO

    IpcCommunicator::Ptr ipcCommunicator = m_interface->ipcCommunicator();
    ipcCommunicator->registerFilesForCodeCompletion(
        {FileContainer(filePath,
                       projectFilePath,
                       Utf8String::fromByteArray(unsavedContent),
                       hasUnsavedContent)});
}

void ClangCompletionAssistProcessor::sendCompletionRequest(int position,
                                                           const QByteArray &modifiedFileContent)
{
    int line, column;
    Convenience::convertPosition(m_interface->textDocument(), position, &line, &column);
    ++column;

    const QString filePath = m_interface->fileName();
    const QString projectFilePath = Utils::projectFilePathForFile(filePath);
    sendFileContent(projectFilePath, modifiedFileContent);
    m_interface->ipcCommunicator()->completeCode(this, filePath, line, column, projectFilePath);
}

TextEditor::IAssistProposal *ClangCompletionAssistProcessor::createProposal() const
{
    ClangAssistProposalModel *model = new ClangAssistProposalModel;
    model->loadContent(m_completions);
    return new ClangAssistProposal(m_positionForProposal, model);
}

void ClangCompletionAssistProcessor::onCompletionsAvailable(const CodeCompletions &completions)
{
    QTC_CHECK(m_completions.isEmpty());

    m_completions = toAssistProposalItems(completions);
    if (m_addSnippets)
        addSnippets();

    setAsyncProposalAvailable(createProposal());
}

void ClangCompletionAssistProcessor::onFunctionHintCompletionsAvailable(
        const CodeCompletions &completions)
{
    QTC_CHECK(!m_functionName.isEmpty());
    const auto relevantCompletions = matchingFunctionCompletions(completions, m_functionName);

    if (!relevantCompletions.isEmpty()) {
        IFunctionHintProposalModel *model = new ClangFunctionHintModel(relevantCompletions);
        FunctionHintProposal *proposal = new FunctionHintProposal(m_positionForProposal, model);

        setAsyncProposalAvailable(proposal);
    } else {
        QTC_CHECK(!"Function completion failed. Would fallback to global completion here...");
        // TODO: If we need this, the processor can't be deleted in IpcClient.
    }
}

} // namespace Internal
} // namespace ClangCodeModel
