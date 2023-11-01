// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "clangdcompletion.h"

#include "clangcodemodeltr.h"
#include "clangcompletioncontextanalyzer.h"
#include "clangdclient.h"
#include "clangpreprocessorassistproposalitem.h"
#include "clangutils.h"
#include "tasktimers.h"

#include <cppeditor/cppcompletionassistprocessor.h>
#include <cppeditor/cppcompletionassistprovider.h>
#include <cppeditor/cppdoxygen.h>
#include <cppeditor/cppeditorconstants.h>
#include <cppeditor/cppmodelmanager.h>
#include <cppeditor/cppprojectfile.h>
#include <cppeditor/projectpart.h>
#include <cplusplus/Icons.h>
#include <cplusplus/MatchingText.h>

#include <languageclient/languageclientfunctionhint.h>

#include <projectexplorer/headerpath.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/codeassist/genericproposal.h>
#include <texteditor/codeassist/genericproposalmodel.h>
#include <texteditor/texteditorsettings.h>

#include <utils/utilsicons.h>

using namespace CppEditor;
using namespace CPlusPlus;
using namespace LanguageClient;
using namespace LanguageServerProtocol;
using namespace ProjectExplorer;
using namespace TextEditor;
using namespace Utils;

namespace ClangCodeModel::Internal {

static Q_LOGGING_CATEGORY(clangdLogCompletion, "qtc.clangcodemodel.clangd.completion",
                          QtWarningMsg);

enum class CustomAssistMode { Doxygen, Preprocessor, IncludePath };

class CustomAssistProcessor : public IAssistProcessor
{
public:
    CustomAssistProcessor(ClangdClient *client, int position, int endPos,
                          unsigned completionOperator, CustomAssistMode mode);

private:
    IAssistProposal *perform() override;

    AssistProposalItemInterface *createItem(const QString &text, const QIcon &icon) const;

    static QList<AssistProposalItemInterface *> completeInclude(
            int position, unsigned completionOperator, const AssistInterface *interface,
            const HeaderPaths &headerPaths);
    static QList<AssistProposalItemInterface *> completeIncludePath(
            const QString &realPath, const QStringList &suffixes, unsigned completionOperator);

    ClangdClient * const m_client;
    const int m_position;
    const int m_endPos;
    const unsigned m_completionOperator;
    const CustomAssistMode m_mode;
};

class ClangdCompletionItem : public LanguageClientCompletionItem
{
public:
    using LanguageClientCompletionItem::LanguageClientCompletionItem;
    void apply(TextDocumentManipulatorInterface &manipulator,
               int basePosition) const override;

    enum class SpecialQtType { Signal, Slot, None };
    static SpecialQtType getQtType(const CompletionItem &item);

private:
    QIcon icon() const override;
    QString text() const override;
};

class ClangdCompletionAssistProcessor : public LanguageClientCompletionAssistProcessor
{
public:
    ClangdCompletionAssistProcessor(ClangdClient *client,
                                    const IAssistProvider *provider,
                                    const QString &snippetsGroup);
    ~ClangdCompletionAssistProcessor();

private:
    IAssistProposal *perform() override;
    QList<AssistProposalItemInterface *> generateCompletionItems(
            const QList<LanguageServerProtocol::CompletionItem> &items) const override;

    ClangdClient * const m_client;
    QElapsedTimer m_timer;
};

class ClangdFunctionHintProcessor : public FunctionHintProcessor
{
public:
    ClangdFunctionHintProcessor(ClangdClient *client);

private:
    IAssistProposal *perform() override;

    ClangdClient * const m_client;
};

ClangdCompletionAssistProvider::ClangdCompletionAssistProvider(ClangdClient *client)
    : LanguageClientCompletionAssistProvider(client)
    , m_client(client)
{}

IAssistProcessor *ClangdCompletionAssistProvider::createProcessor(
    const AssistInterface *interface) const
{
    qCDebug(clangdLogCompletion) << "completion processor requested for" << interface->filePath();
    qCDebug(clangdLogCompletion) << "text before cursor is"
                                 << interface->textAt(interface->position(), -10);
    qCDebug(clangdLogCompletion) << "text after cursor is"
                                 << interface->textAt(interface->position(), 10);
    ClangCompletionContextAnalyzer contextAnalyzer(interface->textDocument(),
                                                   interface->position(), false, {});
    contextAnalyzer.analyze();
    switch (contextAnalyzer.completionAction()) {
    case ClangCompletionContextAnalyzer::PassThroughToLibClangAfterLeftParen:
        qCDebug(clangdLogCompletion) << "creating function hint processor";
        return new ClangdFunctionHintProcessor(m_client);
    case ClangCompletionContextAnalyzer::CompleteDoxygenKeyword:
        qCDebug(clangdLogCompletion) << "creating doxygen processor";
        return new CustomAssistProcessor(m_client,
                                         contextAnalyzer.positionForProposal(),
                                         contextAnalyzer.positionEndOfExpression(),
                                         contextAnalyzer.completionOperator(),
                                         CustomAssistMode::Doxygen);
    case ClangCompletionContextAnalyzer::CompletePreprocessorDirective:
        qCDebug(clangdLogCompletion) << "creating macro processor";
        return new CustomAssistProcessor(m_client,
                                         contextAnalyzer.positionForProposal(),
                                         contextAnalyzer.positionEndOfExpression(),
                                         contextAnalyzer.completionOperator(),
                                         CustomAssistMode::Preprocessor);
    case ClangCompletionContextAnalyzer::CompleteSignal:
    case ClangCompletionContextAnalyzer::CompleteSlot:
        if (!interface->isBaseObject())
            return CppEditor::getCppCompletionAssistProcessor();
    default:
        break;
    }

    if (interface->reason() == ActivationCharacter) {
        switch (interface->characterAt(interface->position() - 1).toLatin1()) {
        case '"': case '<': case '/':
            if (contextAnalyzer.completionAction()
                    != ClangCompletionContextAnalyzer::CompleteIncludePath) {
                class NoOpProcessor : public IAssistProcessor {
                    IAssistProposal *perform() override { return nullptr; }
                };
                return new NoOpProcessor;
            }
        }
    }

    const QString snippetsGroup = contextAnalyzer.addSnippets() && !isInCommentOrString(interface)
                                      ? CppEditor::Constants::CPP_SNIPPETS_GROUP_ID
                                      : QString();
    qCDebug(clangdLogCompletion) << "creating proper completion processor"
                                 << (snippetsGroup.isEmpty() ? "without" : "with") << "snippets";
    return new ClangdCompletionAssistProcessor(m_client, this, snippetsGroup);
}

bool ClangdCompletionAssistProvider::isActivationCharSequence(const QString &sequence) const
{
    const QChar &ch  = sequence.at(2);
    const QChar &ch2 = sequence.at(1);
    const QChar &ch3 = sequence.at(0);
    unsigned kind = T_EOF_SYMBOL;
    const int pos = CppCompletionAssistProvider::activationSequenceChar(
                ch, ch2, ch3, &kind, false, false);
    if (pos == 0)
        return false;

    // We want to minimize unneeded completion requests, as those trigger document updates,
    // which trigger re-highlighting and diagnostics, which we try to delay.
    // Therefore, for '"', '<', and '/', a follow-up check will verify whether we are in
    // an include completion context and otherwise not start the LSP completion procedure.
    switch (kind) {
    case T_DOT: case T_COLON_COLON: case T_ARROW: case T_DOT_STAR: case T_ARROW_STAR: case T_POUND:
    case T_STRING_LITERAL: case T_ANGLE_STRING_LITERAL: case T_SLASH:
        qCDebug(clangdLogCompletion) << "detected" << sequence << "as activation char sequence";
        return true;
    }
    return false;
}

bool ClangdCompletionAssistProvider::isContinuationChar(const QChar &c) const
{
    return isValidIdentifierChar(c);
}

bool ClangdCompletionAssistProvider::isInCommentOrString(const AssistInterface *interface) const
{
    LanguageFeatures features = LanguageFeatures::defaultFeatures();
    features.objCEnabled = ProjectFile::isObjC(interface->filePath().toString());
    return CppEditor::isInCommentOrString(interface, features);
}

void ClangdCompletionItem::apply(TextDocumentManipulatorInterface &manipulator,
                                 int /*basePosition*/) const
{
    const CompletionItem item = this->item();
    QChar typedChar = triggeredCommitCharacter();
    const auto edit = item.textEdit();
    if (!edit)
        return;

    const int labelOpenParenOffset = item.label().indexOf('(');
    const int labelClosingParenOffset = item.label().indexOf(')');
    const auto kind = static_cast<CompletionItemKind::Kind>(
                item.kind().value_or(CompletionItemKind::Text));
    const bool isMacroCall = kind == CompletionItemKind::Text && labelOpenParenOffset != -1
            && labelClosingParenOffset > labelOpenParenOffset; // Heuristic
    const bool isFunctionLike = kind == CompletionItemKind::Function
            || kind == CompletionItemKind::Method || kind == CompletionItemKind::Constructor
            || isMacroCall;

    QString rawInsertText = edit->newText();

    // Some preparation for our magic involving (non-)insertion of parentheses and
    // cursor placement.
    if (isFunctionLike && !rawInsertText.contains('(')) {
        if (labelOpenParenOffset != -1) {
            if (labelClosingParenOffset == labelOpenParenOffset + 1) // function takes no arguments
                rawInsertText += "()";
            else                                                     // function takes arguments
                rawInsertText += "( )";
        }
    }

    const int firstParenOffset = rawInsertText.indexOf('(');
    const int lastParenOffset = rawInsertText.lastIndexOf(')');
    const QString detail = item.detail().value_or(QString());
    const CompletionSettings &completionSettings = TextEditorSettings::completionSettings();
    QString textToBeInserted = rawInsertText.left(firstParenOffset);
    QString extraCharacters;
    int extraLength = 0;
    int cursorOffset = 0;
    bool setAutoCompleteSkipPos = false;
    int currentPos = manipulator.currentPosition();
    const QTextDocument * const doc = manipulator.textCursorAt(currentPos).document();
    const Range range = edit->range();
    const int rangeStart = range.start().toPositionInDocument(doc);
    if (isFunctionLike && completionSettings.m_autoInsertBrackets) {
        // If the user typed the opening parenthesis, they'll likely also type the closing one,
        // in which case it would be annoying if we put the cursor after the already automatically
        // inserted closing parenthesis.
        const bool skipClosingParenthesis = typedChar != '(';
        QTextCursor cursor = manipulator.textCursorAt(rangeStart);

        bool abandonParen = false;
        if (matchPreviousWord(manipulator, cursor, "&")) {
            moveToPreviousWord(manipulator, cursor);
            moveToPreviousChar(manipulator, cursor);
            const QChar prevChar = manipulator.characterAt(cursor.position());
            cursor.setPosition(rangeStart);
            abandonParen = QString("(;,{}=").contains(prevChar);
        }
        if (!abandonParen)
            abandonParen = isAtUsingDeclaration(manipulator, rangeStart);
        if (!abandonParen && !isMacroCall && matchPreviousWord(manipulator, cursor, detail))
            abandonParen = true; // function definition
        if (!abandonParen) {
            if (completionSettings.m_spaceAfterFunctionName)
                extraCharacters += ' ';
            extraCharacters += '(';
            if (typedChar == '(')
                typedChar = {};

            // If the function doesn't return anything, automatically place the semicolon,
            // unless we're doing a scope completion (then it might be function definition).
            const QChar characterAtCursor = manipulator.characterAt(currentPos);
            bool endWithSemicolon = typedChar == ';';
            const QChar semicolon = typedChar.isNull() ? QLatin1Char(';') : typedChar;
            if (endWithSemicolon && characterAtCursor == semicolon) {
                endWithSemicolon = false;
                typedChar = {};
            }

            // If the function takes no arguments, automatically place the closing parenthesis
            if (firstParenOffset + 1 == lastParenOffset && skipClosingParenthesis) {
                extraCharacters += QLatin1Char(')');
                if (endWithSemicolon) {
                    extraCharacters += semicolon;
                    typedChar = {};
                }
            } else {
                const QChar lookAhead = manipulator.characterAt(currentPos + 1);
                if (MatchingText::shouldInsertMatchingText(lookAhead)) {
                    extraCharacters += ')';
                    --cursorOffset;
                    setAutoCompleteSkipPos = true;
                    if (endWithSemicolon) {
                        extraCharacters += semicolon;
                        --cursorOffset;
                        typedChar = {};
                    }
                }
            }
        }
    }

    // Append an unhandled typed character, adjusting cursor offset when it had been adjusted before
    if (!typedChar.isNull()) {
        extraCharacters += typedChar;
        if (cursorOffset != 0)
            --cursorOffset;
    }

    // Avoid inserting characters that are already there
    QTextCursor cursor = manipulator.textCursorAt(rangeStart);
    cursor.movePosition(QTextCursor::EndOfWord);
    const QString textAfterCursor = manipulator.textAt(currentPos, cursor.position() - currentPos);
    if (currentPos < cursor.position()
            && textToBeInserted != textAfterCursor
            && textToBeInserted.indexOf(textAfterCursor, currentPos - rangeStart) >= 0) {
        currentPos = cursor.position();
    }
    for (int i = 0; i < extraCharacters.length(); ++i) {
        const QChar a = extraCharacters.at(i);
        const QChar b = manipulator.characterAt(currentPos + i);
        if (a == b)
            ++extraLength;
        else
            break;
    }

    textToBeInserted += extraCharacters;
    const int length = currentPos - rangeStart + extraLength;
    const bool isReplaced = manipulator.replace(rangeStart, length, textToBeInserted);
    manipulator.setCursorPosition(rangeStart + textToBeInserted.length());
    if (isReplaced) {
        if (cursorOffset)
            manipulator.setCursorPosition(manipulator.currentPosition() + cursorOffset);
        if (setAutoCompleteSkipPos)
            manipulator.setAutoCompleteSkipPosition(manipulator.currentPosition());
    }

    if (auto additionalEdits = item.additionalTextEdits()) {
        for (const auto &edit : *additionalEdits)
            applyTextEdit(manipulator, edit);
    }
}

ClangdCompletionItem::SpecialQtType ClangdCompletionItem::getQtType(const CompletionItem &item)
{
    const std::optional<MarkupOrString> doc = item.documentation();
    if (!doc)
        return SpecialQtType::None;
    QString docText;
    if (std::holds_alternative<QString>(*doc))
        docText = std::get<QString>(*doc);
    else if (std::holds_alternative<MarkupContent>(*doc))
        docText = std::get<MarkupContent>(*doc).content();
    if (docText.contains("Annotation: qt_signal"))
        return SpecialQtType::Signal;
    if (docText.contains("Annotation: qt_slot"))
        return SpecialQtType::Slot;
    return SpecialQtType::None;
}

QIcon ClangdCompletionItem::icon() const
{
    if (isDeprecated())
        return Utils::Icons::WARNING.icon();
    const SpecialQtType qtType = getQtType(item());
    switch (qtType) {
    case SpecialQtType::Signal:
        return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::Signal);
    case SpecialQtType::Slot:
         // FIXME: Add visibility info to completion item tags in clangd?
        return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::SlotPublic);
    case SpecialQtType::None:
        break;
    }
    if (item().kind().value_or(CompletionItemKind::Text) == CompletionItemKind::Property)
        return Utils::CodeModelIcon::iconForType(Utils::CodeModelIcon::VarPublicStatic);
    return LanguageClientCompletionItem::icon();
}

QString ClangdCompletionItem::text() const
{
    const QString clangdValue = LanguageClientCompletionItem::text();
    if (isDeprecated())
        return "[[deprecated]]" + clangdValue;
    return clangdValue;
}

CustomAssistProcessor::CustomAssistProcessor(ClangdClient *client, int position, int endPos,
                                             unsigned completionOperator, CustomAssistMode mode)
    : m_client(client)
    , m_position(position)
    , m_endPos(endPos)
    , m_completionOperator(completionOperator)
    , m_mode(mode)
{}

IAssistProposal *CustomAssistProcessor::perform()
{
    QList<AssistProposalItemInterface *> completions;
    switch (m_mode) {
    case CustomAssistMode::Doxygen:
        for (int i = 1; i < T_DOXY_LAST_TAG; ++i) {
            completions << createItem(QLatin1String(doxygenTagSpell(i)),
                                      CPlusPlus::Icons::keywordIcon());
        }
        break;
    case CustomAssistMode::Preprocessor: {
        static QIcon macroIcon = Utils::CodeModelIcon::iconForType(CodeModelIcon::Macro);
        for (const QString &completion
             : CppCompletionAssistProcessor::preprocessorCompletions()) {
            completions << createItem(completion, macroIcon);
        }
        if (ProjectFile::isObjC(interface()->filePath().toString()))
            completions << createItem("import", macroIcon);
        break;
    }
    case CustomAssistMode::IncludePath: {
        HeaderPaths headerPaths;
        const ProjectPart::ConstPtr projectPart = projectPartForFile(interface()->filePath());
        if (projectPart)
            headerPaths = projectPart->headerPaths;
        completions = completeInclude(m_endPos, m_completionOperator, interface(), headerPaths);
        break;
    }
    }
    GenericProposalModelPtr model(new GenericProposalModel);
    model->loadContent(completions);
    const auto proposal = new GenericProposal(m_position, model);
    if (m_client->testingEnabled()) {
        emit m_client->proposalReady(proposal);
        return nullptr;
    }
    return proposal;
}

AssistProposalItemInterface *CustomAssistProcessor::createItem(const QString &text,
                                                               const QIcon &icon) const
{
    const auto item = new ClangPreprocessorAssistProposalItem;
    item->setText(text);
    item->setIcon(icon);
    item->setCompletionOperator(m_completionOperator);
    return item;
}

/**
 * @brief Creates completion proposals for #include and given cursor
 * @param position - cursor placed after opening bracked or quote
 * @param completionOperator - the type of token
 * @param interface - relevant document data
 * @param headerPaths - the include paths
 * @return the list of completion items
 */
QList<AssistProposalItemInterface *> CustomAssistProcessor::completeInclude(
        int position, unsigned completionOperator, const AssistInterface *interface,
        const HeaderPaths &headerPaths)
{
    QTextCursor cursor(interface->textDocument());
    cursor.setPosition(position);
    QString directoryPrefix;
    if (completionOperator == T_SLASH) {
        QTextCursor c = cursor;
        c.movePosition(QTextCursor::StartOfLine, QTextCursor::KeepAnchor);
        QString sel = c.selectedText();
        int startCharPos = sel.indexOf(QLatin1Char('"'));
        if (startCharPos == -1) {
            startCharPos = sel.indexOf(QLatin1Char('<'));
            completionOperator = T_ANGLE_STRING_LITERAL;
        } else {
            completionOperator = T_STRING_LITERAL;
        }
        if (startCharPos != -1)
            directoryPrefix = sel.mid(startCharPos + 1, sel.length() - 1);
    }

    // Make completion for all relevant includes
    HeaderPaths allHeaderPaths = headerPaths;
    const auto currentFilePath = HeaderPath::makeUser(
                interface->filePath().toFileInfo().path());
    if (!allHeaderPaths.contains(currentFilePath))
        allHeaderPaths.append(currentFilePath);

    const MimeType mimeType = mimeTypeForName("text/x-c++hdr");
    const QStringList suffixes = mimeType.suffixes();

    QList<AssistProposalItemInterface *> completions;
    for (const HeaderPath &headerPath : std::as_const(allHeaderPaths)) {
        QString realPath = headerPath.path;
        if (!directoryPrefix.isEmpty()) {
            realPath += QLatin1Char('/');
            realPath += directoryPrefix;
            if (headerPath.type == HeaderPathType::Framework)
                realPath += QLatin1String(".framework/Headers");
        }
        completions << completeIncludePath(realPath, suffixes, completionOperator);
    }

    QList<QPair<AssistProposalItemInterface *, QString>> completionsForSorting;
    for (AssistProposalItemInterface * const item : std::as_const(completions)) {
        QString s = item->text();
        s.replace('/', QChar(0)); // The dir separator should compare less than anything else.
        completionsForSorting.push_back({item, s});
    }
    Utils::sort(completionsForSorting, [](const auto &left, const auto &right) {
        return left.second < right.second;
    });
    for (int i = 0; i < completionsForSorting.count(); ++i)
        completions[i] = completionsForSorting[i].first;

    return completions;
}

/**
 * @brief Finds #include completion proposals using given include path
 * @param realPath - one of directories where compiler searches includes
 * @param suffixes - file suffixes for C/C++ header files
 * @return a list of matching completion items
 */
QList<AssistProposalItemInterface *> CustomAssistProcessor::completeIncludePath(
        const QString &realPath, const QStringList &suffixes, unsigned completionOperator)
{
    QList<AssistProposalItemInterface *> completions;
    QDirIterator i(realPath, QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot);
    //: Parent folder for proposed #include completion
    const QString hint = Tr::tr("Location: %1")
            .arg(QDir::toNativeSeparators(QDir::cleanPath(realPath)));
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
            item->setIcon(CPlusPlus::Icons::keywordIcon());
            item->setCompletionOperator(completionOperator);
            completions.append(item);
        }
    }
    return completions;
}

ClangdCompletionAssistProcessor::ClangdCompletionAssistProcessor(ClangdClient *client,
                                                                 const IAssistProvider *provider,
                                                                 const QString &snippetsGroup)
    : LanguageClientCompletionAssistProcessor(client, provider, snippetsGroup)
    , m_client(client)
{
    m_timer.start();
}

ClangdCompletionAssistProcessor::~ClangdCompletionAssistProcessor()
{
    qCDebug(clangdLogTiming).noquote().nospace()
            << "ClangdCompletionAssistProcessor took: " << m_timer.elapsed() << " ms";
}

IAssistProposal *ClangdCompletionAssistProcessor::perform()
{
    if (m_client->testingEnabled()) {
        setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal) {
            emit m_client->proposalReady(proposal);
        });
    }
    return LanguageClientCompletionAssistProcessor::perform();
}

QList<AssistProposalItemInterface *> ClangdCompletionAssistProcessor::generateCompletionItems(
        const QList<CompletionItem> &items) const
{
    qCDebug(clangdLog) << "received" << items.count() << "completions";

    auto itemGenerator = [](const QList<LanguageServerProtocol::CompletionItem> &items) {
        return Utils::transform<QList<AssistProposalItemInterface *>>(items,
            [](const LanguageServerProtocol::CompletionItem &item) {
                return new ClangdCompletionItem(item);
        });
    };

    // If there are signals among the candidates, we employ the built-in code model to find out
    // whether the cursor was on the second argument of a (dis)connect() call.
    // If so, we offer only signals, as nothing else makes sense in that context.
    static const auto criterion = [](const CompletionItem &ci) {
        return ClangdCompletionItem::getQtType(ci) == ClangdCompletionItem::SpecialQtType::Signal;
    };
    const QTextDocument *doc = document();
    const int pos = basePos();
    if (!doc || pos < 0 || !Utils::anyOf(items, criterion))
        return itemGenerator(items);
    const QString content = doc->toPlainText();
    const bool requiresSignal = CppModelManager::getSignalSlotType(
                filePath(), content.toUtf8(), pos)
            == SignalSlotType::NewStyleSignal;
    if (requiresSignal)
        return itemGenerator(Utils::filtered(items, criterion));
    return itemGenerator(items);
}

ClangdFunctionHintProcessor::ClangdFunctionHintProcessor(ClangdClient *client)
    : FunctionHintProcessor(client)
    , m_client(client)
{}

IAssistProposal *ClangdFunctionHintProcessor::perform()
{
    if (m_client->testingEnabled()) {
        setAsyncCompletionAvailableHandler([this](IAssistProposal *proposal) {
            emit m_client->proposalReady(proposal);
        });
    }
    return FunctionHintProcessor::perform();
}

ClangdCompletionCapabilities::ClangdCompletionCapabilities(const JsonObject &object)
    : TextDocumentClientCapabilities::CompletionCapabilities(object)
{
    insert("editsNearCursor", true); // For dot-to-arrow correction.
    if (std::optional<CompletionItemCapbilities> completionItemCaps = completionItem()) {
        completionItemCaps->setSnippetSupport(false);
        setCompletionItem(*completionItemCaps);
    }
}

} // namespace ClangCodeModel::Internal
