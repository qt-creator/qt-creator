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

#include "clangassistproposalitem.h"

#include "activationsequenceprocessor.h"
#include "clangassistproposal.h"
#include "clangassistproposalmodel.h"
#include "clangcompletionassistprocessor.h"
#include "clangcompletioncontextanalyzer.h"
#include "clangfunctionhintmodel.h"
#include "clangutils.h"
#include "completionchunkstotextconverter.h"

#include <utils/qtcassert.h>

#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>
#include <texteditor/convenience.h>

#include <cpptools/cppdoxygen.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/SimpleLexer.h>

#include <QDirIterator>
#include <QTextBlock>
#include <filecontainer.h>

#include <utils/mimetypes/mimedatabase.h>

namespace ClangCodeModel {
namespace Internal {

using ClangBackEnd::CodeCompletion;
using TextEditor::AssistProposalItem;

namespace {

const char SNIPPET_ICON_PATH[] = ":/texteditor/images/snippet.png";


QList<AssistProposalItem *> toAssistProposalItems(const CodeCompletions &completions)
{
    static CPlusPlus::Icons m_icons; // de-deduplicate

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

        QString name;
        if (ccr.completionKind() == CodeCompletion::KeywordCompletionKind)
            name = CompletionChunksToTextConverter::convertToName(ccr.chunks());
        else
            name = ccr.text().toString();

        ClangAssistProposalItem *item = items.value(name, 0);
        if (item) {
            item->addOverload(ccr);
        } else {
            item = new ClangAssistProposalItem;
            items.insert(name, item);
            item->setText(name);
            item->setOrder(ccr.priority());

            if (ccr.completionKind() == CodeCompletion::KeywordCompletionKind)
                item->setDetail(CompletionChunksToTextConverter::convertToToolTip(ccr.chunks()));

            item->setCodeCompletion(ccr);
        }

        // FIXME: show the effective accessebility instead of availability
        using CPlusPlus::Icons;

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

    QList<AssistProposalItem *> results;
    results.reserve(items.size());
    std::copy(items.cbegin(), items.cend(), std::back_inserter(results));

    return results;
}

bool isFunctionHintLikeCompletion(CodeCompletion::Kind kind)
{
    return kind == CodeCompletion::FunctionCompletionKind
        || kind == CodeCompletion::ConstructorCompletionKind
        || kind == CodeCompletion::DestructorCompletionKind
        || kind == CodeCompletion::SignalCompletionKind
        || kind == CodeCompletion::SlotCompletionKind;
}

CodeCompletions matchingFunctionCompletions(const CodeCompletions completions,
                                            const QString &functionName)
{
    CodeCompletions matching;

    foreach (const CodeCompletion &completion, completions) {
        if (isFunctionHintLikeCompletion(completion.completionKind())
                && completion.text().toString() == functionName) {
            matching.append(completion);
        }
    }

    return matching;
}

} // Anonymous

using namespace CPlusPlus;
using namespace TextEditor;

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

    if (interface->reason() != ExplicitlyInvoked && !accepts()) {
        setPerformWasApplicable(false);
        return 0;
    }

    return startCompletionHelper(); // == 0 if results are calculated asynchronously
}

bool ClangCompletionAssistProcessor::handleAvailableAsyncCompletions(
        const CodeCompletions &completions)
{
    bool handled = true;

    switch (m_sentRequestType) {
    case CompletionRequestType::NormalCompletion:
        handleAvailableCompletions(completions);
        break;
    case CompletionRequestType::FunctionHintCompletion:
        handled = handleAvailableFunctionHintCompletions(completions);
        break;
    default:
        QTC_CHECK(!"Unhandled ClangCompletionAssistProcessor::CompletionRequestType");
        break;
    }

    return handled;
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
int ClangCompletionAssistProcessor::startOfOperator(int positionInDocument,
                                                    unsigned *kind,
                                                    bool wantFunctionCall) const
{
    auto activationSequence = m_interface->textAt(positionInDocument - 3, 3);
    ActivationSequenceProcessor activationSequenceProcessor(activationSequence,
                                                            positionInDocument,
                                                            wantFunctionCall);

    *kind = activationSequenceProcessor.completionKind();

    int start = activationSequenceProcessor.operatorStartPosition();
    if (start != positionInDocument) {
        QTextCursor tc(m_interface->textDocument());
        tc.setPosition(positionInDocument);

        // Include completion: make sure the quote character is the first one on the line
        if (*kind == T_STRING_LITERAL) {
            QTextCursor s = tc;
            s.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
            QString sel = s.selectedText();
            if (sel.indexOf(QLatin1Char('"')) < sel.length() - 1) {
                *kind = T_EOF_SYMBOL;
                start = positionInDocument;
            }
        }

        if (*kind == T_COMMA) {
            ExpressionUnderCursor expressionUnderCursor(m_interface->languageFeatures());
            if (expressionUnderCursor.startOfFunctionCall(tc) == -1) {
                *kind = T_EOF_SYMBOL;
                start = positionInDocument;
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
            start = positionInDocument;
        }
        // Don't complete in comments or strings, but still check for include completion
        else if (tk.is(T_COMMENT) || tk.is(T_CPP_COMMENT)
                 || tk.is(T_CPP_DOXY_COMMENT) || tk.is(T_DOXY_COMMENT)
                 || (tk.isLiteral() && (*kind != T_STRING_LITERAL
                                     && *kind != T_ANGLE_STRING_LITERAL
                                     && *kind != T_SLASH))) {
            *kind = T_EOF_SYMBOL;
            start = positionInDocument;
        }
        // Include completion: can be triggered by slash, but only in a string
        else if (*kind == T_SLASH && (tk.isNot(T_STRING_LITERAL) && tk.isNot(T_ANGLE_STRING_LITERAL))) {
            *kind = T_EOF_SYMBOL;
            start = positionInDocument;
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
                    start = positionInDocument;
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
                start = positionInDocument;
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
    CppTools::ProjectPart::HeaderPaths headerPaths = m_interface->headerPaths();
    const CppTools::ProjectPart::HeaderPath currentFilePath(QFileInfo(m_interface->fileName()).path(),
                                                            CppTools::ProjectPart::HeaderPath::IncludePath);
    if (!headerPaths.contains(currentFilePath))
        headerPaths.append(currentFilePath);

    ::Utils::MimeDatabase mdb;
    const ::Utils::MimeType mimeType = mdb.mimeTypeForName(QLatin1String("text/x-c++hdr"));
    const QStringList suffixes = mimeType.suffixes();

    foreach (const CppTools::ProjectPart::HeaderPath &headerPath, headerPaths) {
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
    for (int i = 1; i < CppTools::T_DOXY_LAST_TAG; ++i)
        addCompletionItem(QString::fromLatin1(CppTools::doxygenTagSpell(i)), m_icons.keywordIcon());
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

ClangCompletionAssistProcessor::UnsavedFileContentInfo
ClangCompletionAssistProcessor::unsavedFileContent(const QByteArray &customFileContent) const
{
    const bool hasCustomModification = !customFileContent.isEmpty();

    UnsavedFileContentInfo info;
    info.isDocumentModified = hasCustomModification || m_interface->textDocument()->isModified();
    info.unsavedContent = hasCustomModification
                        ? customFileContent
                        : m_interface->textDocument()->toPlainText().toUtf8();
    return info;
}

void ClangCompletionAssistProcessor::sendFileContent(const QString &projectPartId,
                                                     const QByteArray &customFileContent)
{
    // TODO: Revert custom modification after the completions
    const UnsavedFileContentInfo info = unsavedFileContent(customFileContent);

    IpcCommunicator &ipcCommunicator = m_interface->ipcCommunicator();
    ipcCommunicator.registerFilesForCodeCompletion(
        {ClangBackEnd::FileContainer(m_interface->fileName(),
                       projectPartId,
                       Utf8String::fromByteArray(info.unsavedContent),
                       info.isDocumentModified)});
}

void ClangCompletionAssistProcessor::sendCompletionRequest(int position,
                                                           const QByteArray &customFileContent)
{
    int line, column;
    TextEditor::Convenience::convertPosition(m_interface->textDocument(), position, &line, &column);
    ++column;

    const QString filePath = m_interface->fileName();
    const QString projectPartId = Utils::projectPartIdForFile(filePath);
    sendFileContent(projectPartId, customFileContent);
    m_interface->ipcCommunicator().completeCode(this, filePath, line, column, projectPartId);
}

TextEditor::IAssistProposal *ClangCompletionAssistProcessor::createProposal() const
{
    ClangAssistProposalModel *model = new ClangAssistProposalModel;
    model->loadContent(m_completions);
    return new ClangAssistProposal(m_positionForProposal, model);
}

void ClangCompletionAssistProcessor::handleAvailableCompletions(const CodeCompletions &completions)
{
    QTC_CHECK(m_completions.isEmpty());

    m_completions = toAssistProposalItems(completions);
    if (m_addSnippets)
        addSnippets();

    setAsyncProposalAvailable(createProposal());
}

bool ClangCompletionAssistProcessor::handleAvailableFunctionHintCompletions(
        const CodeCompletions &completions)
{
    QTC_CHECK(!m_functionName.isEmpty());
    const auto relevantCompletions = matchingFunctionCompletions(completions, m_functionName);

    if (!relevantCompletions.isEmpty()) {
        auto *model = new ClangFunctionHintModel(relevantCompletions);
        auto *proposal = new FunctionHintProposal(m_positionForProposal, model);

        setAsyncProposalAvailable(proposal);
        return true;
    } else {
        m_addSnippets = false;
        m_functionName.clear();
        m_sentRequestType = NormalCompletion;
        sendCompletionRequest(m_interface->position(), QByteArray());
        return false; // We are not yet finished.
    }
}

} // namespace Internal
} // namespace ClangCodeModel

