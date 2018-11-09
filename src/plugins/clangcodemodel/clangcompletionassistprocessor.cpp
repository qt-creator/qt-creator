/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#include "clangassistproposalitem.h"

#include "clangactivationsequenceprocessor.h"
#include "clangassistproposalmodel.h"
#include "clangcompletionassistprocessor.h"
#include "clangcompletioncontextanalyzer.h"
#include "clangfixitoperation.h"
#include "clangfunctionhintmodel.h"
#include "clangcompletionchunkstotextconverter.h"
#include "clangpreprocessorassistproposalitem.h"

#include <cpptools/cppdoxygen.h>
#include <cpptools/cppmodelmanager.h>
#include <cpptools/cpptoolsbridge.h>
#include <cpptools/editordocumenthandle.h>

#include <texteditor/codeassist/assistproposalitem.h>
#include <texteditor/codeassist/functionhintproposal.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/ifunctionhintproposalmodel.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/ExpressionUnderCursor.h>
#include <cplusplus/Icons.h>
#include <cplusplus/SimpleLexer.h>

#include <clangsupport/filecontainer.h>

#include <utils/algorithm.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/optional.h>
#include <utils/textutils.h>
#include <utils/qtcassert.h>

#include <QDirIterator>
#include <QTextDocument>

namespace ClangCodeModel {
namespace Internal {

using ClangBackEnd::CodeCompletion;
using TextEditor::AssistProposalItemInterface;

static void addAssistProposalItem(QList<AssistProposalItemInterface *> &items,
                                  const CodeCompletion &codeCompletion,
                                  const QString &name)
{
    ClangAssistProposalItem *item = new ClangAssistProposalItem;
    items.push_back(item);

    item->setText(name);
    item->setOrder(int(codeCompletion.priority));
    item->appendCodeCompletion(codeCompletion);
}

// Add the next CXXMethod or CXXConstructor which is the overload for another existing item.
static void addFunctionOverloadAssistProposalItem(QList<AssistProposalItemInterface *> &items,
                                                  AssistProposalItemInterface *sameItem,
                                                  const ClangCompletionAssistInterface *interface,
                                                  const CodeCompletion &codeCompletion,
                                                  const QString &name)
{
    auto *item = static_cast<ClangAssistProposalItem *>(sameItem);
    item->setHasOverloadsWithParameters(true);
    if (codeCompletion.completionKind == CodeCompletion::ConstructorCompletionKind) {
        // It's the constructor, currently constructor definitions do not lead here.
        // CLANG-UPGRADE-CHECK: Can we get here with constructor definition?
        item->appendCodeCompletion(codeCompletion);
        return;
    }

    QTextCursor cursor = interface->textEditorWidget()->textCursor();
    cursor.setPosition(interface->position());
    cursor.movePosition(QTextCursor::StartOfWord);

    const ClangBackEnd::CodeCompletionChunk resultType = codeCompletion.chunks.first();
    if (Utils::Text::matchPreviousWord(*interface->textEditorWidget(),
                                       cursor,
                                       resultType.text.toString())) {
        // Function definition completion - do not merge completions together.
        addAssistProposalItem(items, codeCompletion, name);
    } else {
        item->appendCodeCompletion(codeCompletion);
    }
}

// Check if they are both CXXMethod or CXXConstructor.
static bool isTheSameFunctionOverload(const CodeCompletion &completion,
                                      const QString &name,
                                      ClangAssistProposalItem *lastItem)
{
    return completion.completionKind == lastItem->firstCodeCompletion().completionKind
            && lastItem->text() == name;
}

static QList<AssistProposalItemInterface *> toAssistProposalItems(
        const CodeCompletions &completions,
        const ClangCompletionAssistInterface *interface)
{
    bool signalCompletion = false; // TODO
    bool slotCompletion = false; // TODO

    QList<AssistProposalItemInterface *> items;
    items.reserve(completions.size());
    for (const CodeCompletion &codeCompletion : completions) {
        if (codeCompletion.text.isEmpty())
            continue; // It's an OverloadCandidate which has text but no typedText.

        if (signalCompletion && codeCompletion.completionKind != CodeCompletion::SignalCompletionKind)
            continue;
        if (slotCompletion && codeCompletion.completionKind != CodeCompletion::SlotCompletionKind)
            continue;

        const QString name = codeCompletion.completionKind == CodeCompletion::KeywordCompletionKind
                ? CompletionChunksToTextConverter::convertToName(codeCompletion.chunks)
                : codeCompletion.text.toString();

        if (items.empty()) {
            addAssistProposalItem(items, codeCompletion, name);
        } else {
            auto *lastItem = static_cast<ClangAssistProposalItem *>(items.last());
            if (isTheSameFunctionOverload(codeCompletion, name, lastItem)) {
                addFunctionOverloadAssistProposalItem(items, items.back(), interface,
                                                      codeCompletion, name);
            } else {
                addAssistProposalItem(items, codeCompletion, name);
            }
        }
    }

    return items;
}

using namespace CPlusPlus;
using namespace TextEditor;

ClangCompletionAssistProcessor::ClangCompletionAssistProcessor()
    : CppCompletionAssistProcessor(100)
    , m_completionOperator(T_EOF_SYMBOL)
{
}

ClangCompletionAssistProcessor::~ClangCompletionAssistProcessor()
{
}

IAssistProposal *ClangCompletionAssistProcessor::perform(const AssistInterface *interface)
{
    m_interface.reset(static_cast<const ClangCompletionAssistInterface *>(interface));

    if (interface->reason() != ExplicitlyInvoked && !accepts()) {
        m_requestSent = false;
        return nullptr;
    }

    return startCompletionHelper(); // == 0 if results are calculated asynchronously
}

// All completions require fix-it, apply this fix-it now.
CodeCompletions ClangCompletionAssistProcessor::applyCompletionFixIt(const CodeCompletions &completions)
{
    // CLANG-UPGRADE-CHECK: Currently we rely on fact that there are only 2 possible fix-it types:
    // 'dot to arrow' and 'arrow to dot' and they can't appear for the same item.
    // However if we get multiple options which trigger for the same code we need to improve this
    // algorithm. Check comments to FixIts field of CodeCompletionResult and which fix-its are used
    // to construct results in SemaCodeComplete.cpp.
    const CodeCompletion &completion = completions.front();
    const ClangBackEnd::FixItContainer &fixIt = completion.requiredFixIts.front();

    ClangFixItOperation fixItOperation(Utf8String(), completion.requiredFixIts);
    fixItOperation.perform();

    const int fixItLength = static_cast<int>(fixIt.range.end.column - fixIt.range.start.column);
    const QString fixItText = fixIt.text.toString();
    m_positionForProposal += fixItText.length() - fixItLength;

    CodeCompletions completionsWithoutFixIts;
    completionsWithoutFixIts.reserve(completions.size());
    for (const CodeCompletion &completion : completions) {
        CodeCompletion completionCopy = completion;
        completionCopy.requiredFixIts.clear();
        completionsWithoutFixIts.push_back(completionCopy);
    }

    return completionsWithoutFixIts;
}

void ClangCompletionAssistProcessor::handleAvailableCompletions(const CodeCompletions &completions)
{
    QTC_CHECK(m_completions.isEmpty());

    if (m_sentRequestType == FunctionHintCompletion && !completions.isEmpty()) {
        const CodeCompletion &firstCompletion = completions.front();
        if (firstCompletion.completionKind == CodeCompletion::FunctionOverloadCompletionKind) {
            setAsyncProposalAvailable(createFunctionHintProposal(completions));
            return;
        }
        // else: Proceed with a normal completion in case:
        // 1) it was not a function call, but e.g. a function declaration like "void f("
        // 2) '{' meant not a constructor call.
    }

    //m_sentRequestType == NormalCompletion or function signatures were empty

    // Completions are sorted the way that all items with fix-its come after all items without them
    // therefore it's enough to check only the first one.
    if (!completions.isEmpty() && !completions.front().requiredFixIts.isEmpty())
        m_completions = toAssistProposalItems(applyCompletionFixIt(completions), m_interface.data());
    else
        m_completions = toAssistProposalItems(completions, m_interface.data());

    if (m_addSnippets && !m_completions.isEmpty())
        addSnippets();

    setAsyncProposalAvailable(createProposal());
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
    m_addSnippets = analyzer.addSnippets();

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
        Q_FALLTHROUGH();
    case ClangCompletionContextAnalyzer::PassThroughToLibClang: {
        m_sentRequestType = NormalCompletion;
        m_requestSent = sendCompletionRequest(analyzer.positionForClang(),
                                              modifiedFileContent);
        break;
    }
    case ClangCompletionContextAnalyzer::PassThroughToLibClangAfterLeftParen: {
        m_sentRequestType = FunctionHintCompletion;
        m_requestSent = sendCompletionRequest(analyzer.positionForClang(), QByteArray(),
                                              analyzer.functionNameStart());
        break;
    }
    default:
        break;
    }

    return 0;
}

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

    CppCompletionAssistProcessor::startOfOperator(m_interface->textDocument(),
                                                  positionInDocument,
                                                  kind,
                                                  start,
                                                  m_interface->languageFeatures());

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
    ProjectExplorer::HeaderPaths headerPaths = m_interface->headerPaths();
    const ProjectExplorer::HeaderPath currentFilePath(QFileInfo(m_interface->fileName()).path(),
                                                          ProjectExplorer::HeaderPathType::User);
    if (!headerPaths.contains(currentFilePath))
        headerPaths.append(currentFilePath);

    const ::Utils::MimeType mimeType = ::Utils::mimeTypeForName("text/x-c++hdr");
    const QStringList suffixes = mimeType.suffixes();

    foreach (const ProjectExplorer::HeaderPath &headerPath, headerPaths) {
        QString realPath = headerPath.path;
        if (!directoryPrefix.isEmpty()) {
            realPath += QLatin1Char('/');
            realPath += directoryPrefix;
            if (headerPath.type == ProjectExplorer::HeaderPathType::Framework)
                realPath += QLatin1String(".framework/Headers");
        }
        completeIncludePath(realPath, suffixes);
    }

    auto includesCompare = [](AssistProposalItemInterface *first,
                              AssistProposalItemInterface *second) {
        return first->text() < second->text();
    };
    std::sort(m_completions.begin(), m_completions.end(), includesCompare);

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

            auto *item = new ClangPreprocessorAssistProposalItem;
            item->setText(text);
            item->setDetail(hint);
            item->setIcon(Icons::keywordIcon());
            item->setCompletionOperator(m_completionOperator);
            m_completions.append(item);
        }
    }
}

bool ClangCompletionAssistProcessor::completePreprocessorDirectives()
{
    foreach (const QString &preprocessorCompletion, m_preprocessorCompletions)
        addCompletionItem(preprocessorCompletion,
                          ::Utils::CodeModelIcon::iconForType(::Utils::CodeModelIcon::Macro));

    if (m_interface->objcEnabled())
        addCompletionItem(QLatin1String("import"),
                          ::Utils::CodeModelIcon::iconForType(::Utils::CodeModelIcon::Macro));

    return !m_completions.isEmpty();
}

bool ClangCompletionAssistProcessor::completeDoxygenKeywords()
{
    for (int i = 1; i < CppTools::T_DOXY_LAST_TAG; ++i)
        addCompletionItem(QString::fromLatin1(CppTools::doxygenTagSpell(i)), Icons::keywordIcon());
    return !m_completions.isEmpty();
}

void ClangCompletionAssistProcessor::addCompletionItem(const QString &text,
                                                       const QIcon &icon,
                                                       int order)
{
    auto *item = new ClangPreprocessorAssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setOrder(order);
    item->setCompletionOperator(m_completionOperator);
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

void ClangCompletionAssistProcessor::sendFileContent(const QByteArray &customFileContent)
{
    // TODO: Revert custom modification after the completions
    const UnsavedFileContentInfo info = unsavedFileContent(customFileContent);

    BackendCommunicator &communicator = m_interface->communicator();
    communicator.documentsChanged({{m_interface->fileName(),
                                    Utf8String::fromByteArray(info.unsavedContent),
                                    info.isDocumentModified,
                                    uint(m_interface->textDocument()->revision())}});
}
namespace {
bool shouldSendDocumentForCompletion(const QString &filePath,
                                     int completionPosition)
{
    CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    if (document) {
        auto &sendTracker = document->sendTracker();
        return sendTracker.shouldSendRevisionWithCompletionPosition(int(document->revision()),
                                                                    completionPosition);
    }

    return true;
}

bool shouldSendCodeCompletion(const QString &filePath,
                              int completionPosition)
{
    CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    if (document) {
        auto &sendTracker = document->sendTracker();
        return sendTracker.shouldSendCompletion(completionPosition);
    }

    return true;
}

void setLastDocumentRevision(const QString &filePath)
{
    CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    if (document)
        document->sendTracker().setLastSentRevision(int(document->revision()));
}

void setLastCompletionPosition(const QString &filePath,
                               int completionPosition)
{
    CppTools::CppEditorDocumentHandle *document = ClangCodeModel::Utils::cppDocument(filePath);

    if (document)
        document->sendTracker().setLastCompletionPosition(completionPosition);
}

}

ClangCompletionAssistProcessor::Position
ClangCompletionAssistProcessor::extractLineColumn(int position)
{
    if (position < 0)
        return {-1, -1};

    int line = -1, column = -1;
    ::Utils::Text::convertPosition(m_interface->textDocument(), position, &line, &column);

    column = Utils::clangColumn(m_interface->textDocument()->findBlock(position), column);
    return {line, column};
}

bool ClangCompletionAssistProcessor::sendCompletionRequest(int position,
                                                           const QByteArray &customFileContent,
                                                           int functionNameStartPosition)
{
    const QString filePath = m_interface->fileName();

    auto &communicator = m_interface->communicator();

    if (shouldSendCodeCompletion(filePath, position) || communicator.isNotWaitingForCompletion()) {
        if (shouldSendDocumentForCompletion(filePath, position)) {
            sendFileContent(customFileContent);
            setLastDocumentRevision(filePath);
        }

        const Position cursorPosition = extractLineColumn(position);
        const Position functionNameStart = extractLineColumn(functionNameStartPosition);
        communicator.requestCompletions(this,
                                        filePath,
                                        uint(cursorPosition.line),
                                        uint(cursorPosition.column),
                                        functionNameStart.line,
                                        functionNameStart.column);
        setLastCompletionPosition(filePath, position);
        return true;
    }

    return false;
}

TextEditor::IAssistProposal *ClangCompletionAssistProcessor::createProposal()
{
    m_requestSent = false;
    TextEditor::GenericProposalModelPtr model(new ClangAssistProposalModel());
    model->loadContent(m_completions);
    return new GenericProposal(m_positionForProposal, model);
}

IAssistProposal *ClangCompletionAssistProcessor::createFunctionHintProposal(
        const ClangBackEnd::CodeCompletions &completions)
{
    m_requestSent = false;
    TextEditor::FunctionHintProposalModelPtr model(new ClangFunctionHintModel(completions));
    return new FunctionHintProposal(m_positionForProposal, model);
}

} // namespace Internal
} // namespace ClangCodeModel

