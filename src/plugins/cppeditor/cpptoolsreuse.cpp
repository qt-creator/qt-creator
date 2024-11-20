// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cpptoolsreuse.h"

#include "clangdsettings.h"
#include "cppautocompleter.h"
#include "cppcanonicalsymbol.h"
#include "cppcodemodelsettings.h"
#include "cppcompletionassist.h"
#include "cppeditorwidget.h"
#include "cppeditortr.h"
#include "cppfilesettingspage.h"
#include "cpphighlighter.h"
#include "cppqtstyleindenter.h"
#include "quickfixes/cppquickfixassistant.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/idocument.h>
#include <coreplugin/messagemanager.h>

#include <projectexplorer/projectmanager.h>

#include <texteditor/codeassist/assistinterface.h>
#include <texteditor/textdocument.h>

#include <cplusplus/BackwardsScanner.h>
#include <cplusplus/declarationcomments.h>
#include <cplusplus/LookupContext.h>
#include <cplusplus/Overview.h>
#include <cplusplus/SimpleLexer.h>

#include <utils/algorithm.h>
#include <utils/qtcassert.h>
#include <utils/textfileformat.h>
#include <utils/textutils.h>

#include <QDebug>
#include <QElapsedTimer>
#include <QHash>
#include <QRegularExpression>
#include <QSet>
#include <QStringView>
#include <QTextCursor>
#include <QTextDocument>

#include <optional>
#include <vector>

using namespace CPlusPlus;
using namespace Utils;

namespace CppEditor {

static int skipChars(QTextCursor *tc,
                      QTextCursor::MoveOperation op,
                      int offset,
                      std::function<bool(const QChar &)> skip)
{
    const QTextDocument *doc = tc->document();
    if (!doc)
        return 0;
    QChar ch = doc->characterAt(tc->position() + offset);
    if (ch.isNull())
        return 0;
    int count = 0;
    while (skip(ch)) {
        if (tc->movePosition(op))
            ++count;
        else
            break;
        ch = doc->characterAt(tc->position() + offset);
    }
    return count;
}

static int skipCharsForward(QTextCursor *tc, const std::function<bool(const QChar &)> &skip)
{
    return skipChars(tc, QTextCursor::NextCharacter, 0, skip);
}

static int skipCharsBackward(QTextCursor *tc, const std::function<bool(const QChar &)> &skip)
{
    return skipChars(tc, QTextCursor::PreviousCharacter, -1, skip);
}

QStringList identifierWordsUnderCursor(const QTextCursor &tc)
{
    const QTextDocument *document = tc.document();
    if (!document)
        return {};
    const auto isSpace = [](const QChar &c) { return c.isSpace(); };
    const auto isColon = [](const QChar &c) { return c == ':'; };
    const auto isValidIdentifierCharAt = [document](const QTextCursor &tc) {
        return isValidIdentifierChar(document->characterAt(tc.position()));
    };
    // move to the end
    QTextCursor endCursor(tc);
    do {
        moveCursorToEndOfIdentifier(&endCursor);
        // possibly skip ::
        QTextCursor temp(endCursor);
        skipCharsForward(&temp, isSpace);
        const int colons = skipCharsForward(&temp, isColon);
        skipCharsForward(&temp, isSpace);
        if (colons == 2 && isValidIdentifierCharAt(temp))
            endCursor = temp;
    } while (isValidIdentifierCharAt(endCursor));

    QStringList results;
    QTextCursor startCursor(endCursor);
    do {
        moveCursorToStartOfIdentifier(&startCursor);
        if (startCursor.position() == endCursor.position())
            break;
        QTextCursor temp(endCursor);
        temp.setPosition(startCursor.position(), QTextCursor::KeepAnchor);
        static const QRegularExpression rexgexp("\\s");
        results.append(temp.selectedText().remove(rexgexp));
        // possibly skip ::
        temp = startCursor;
        skipCharsBackward(&temp, isSpace);
        const int colons = skipCharsBackward(&temp, isColon);
        skipCharsBackward(&temp, isSpace);
        if (colons == 2
                && isValidIdentifierChar(document->characterAt(temp.position() - 1))) {
            startCursor = temp;
        }
    } while (!isValidIdentifierCharAt(startCursor));
    return results;
}

void moveCursorToEndOfIdentifier(QTextCursor *tc)
{
    skipCharsForward(tc, isValidIdentifierChar);
}

void moveCursorToStartOfIdentifier(QTextCursor *tc)
{
    skipCharsBackward(tc, isValidIdentifierChar);
}

bool isValidAsciiIdentifierChar(const QChar &ch)
{
    return ch.isLetterOrNumber() || ch == QLatin1Char('_');
}

bool isValidFirstIdentifierChar(const QChar &ch)
{
    return ch.isLetter() || ch == QLatin1Char('_') || ch.isHighSurrogate() || ch.isLowSurrogate();
}

bool isValidIdentifierChar(const QChar &ch)
{
    return isValidFirstIdentifierChar(ch) || ch.isNumber();
}

bool isValidIdentifier(const QString &s)
{
    const int length = s.length();
    for (int i = 0; i < length; ++i) {
        const QChar &c = s.at(i);
        if (i == 0) {
            if (!isValidFirstIdentifierChar(c))
                return false;
        } else {
            if (!isValidIdentifierChar(c))
                return false;
        }
    }
    return true;
}

int activeArgumenForPrefix(const QString &prefix)
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
        else if (!parcount && tk.is(T_COMMA))
            ++argnr;
    }

    if (parcount < 0)
        return -1;

    return argnr;
}

bool isQtKeyword(QStringView text)
{
    switch (text.length()) {
    case 4:
        switch (text.at(0).toLatin1()) {
        case 'e':
            if (text == QLatin1String("emit"))
                return true;
            break;
        case 'S':
            if (text == QLatin1String("SLOT"))
                return true;
            break;
        }
        break;

    case 5:
        if (text.at(0) == QLatin1Char('s') && text == QLatin1String("slots"))
            return true;
        break;

    case 6:
        if (text.at(0) == QLatin1Char('S') && text == QLatin1String("SIGNAL"))
            return true;
        break;

    case 7:
        switch (text.at(0).toLatin1()) {
        case 's':
            if (text == QLatin1String("signals"))
                return true;
            break;
        case 'f':
            if (text == QLatin1String("foreach") || text ==  QLatin1String("forever"))
                return true;
            break;
        }
        break;

    default:
        break;
    }
    return false;
}

QString identifierUnderCursor(QTextCursor *cursor)
{
    cursor->movePosition(QTextCursor::StartOfWord);
    cursor->movePosition(QTextCursor::EndOfWord, QTextCursor::KeepAnchor);
    return cursor->selectedText();
}

const Macro *findCanonicalMacro(const QTextCursor &cursor, Document::Ptr document)
{
    QTC_ASSERT(document, return nullptr);

    if (const Macro *macro = document->findMacroDefinitionAt(cursor.blockNumber() + 1)) {
        QTextCursor macroCursor = cursor;
        const QByteArray name = identifierUnderCursor(&macroCursor).toUtf8();
        if (macro->name() == name)
            return macro;
    } else if (const Document::MacroUse *use = document->findMacroUseAt(cursor.position())) {
        return &use->macro();
    }

    return nullptr;
}

bool isInCommentOrString(const TextEditor::AssistInterface *interface,
                         CPlusPlus::LanguageFeatures features)
{
    QTextCursor tc(interface->textDocument());
    tc.setPosition(interface->position());
    return isInCommentOrString(tc, features);
}

bool isInCommentOrString(const QTextCursor &cursor, CPlusPlus::LanguageFeatures features)
{
    SimpleLexer tokenize;
    features.qtMocRunEnabled = true;
    tokenize.setLanguageFeatures(features);
    tokenize.setSkipComments(false);
    const Tokens &tokens = tokenize(cursor.block().text(),
                                    BackwardsScanner::previousBlockState(cursor.block()));
    const int tokenIdx = SimpleLexer::tokenBefore(tokens, qMax(0, cursor.positionInBlock() - 1));
    const Token tk = (tokenIdx == -1) ? Token() : tokens.at(tokenIdx);

    if (tk.isComment())
        return true;
    if (!tk.isStringLiteral())
        return false;
    if (tokens.size() == 3 && tokens.at(0).kind() == T_POUND
        && tokens.at(1).kind() == T_IDENTIFIER) {
        const QString &line = cursor.block().text();
        const Token &idToken = tokens.at(1);
        QStringView identifier = QStringView(line).mid(idToken.utf16charsBegin(),
                                                       idToken.utf16chars());
        if (identifier == QLatin1String("include")
            || identifier == QLatin1String("include_next")
            || (features.objCEnabled && identifier == QLatin1String("import"))) {
            return false;
        }
    }
    return true;
}

TextEditor::QuickFixOperations quickFixOperations(const TextEditor::AssistInterface *interface)
{
    return Internal::quickFixOperations(interface);
}

CppCompletionAssistProcessor *getCppCompletionAssistProcessor()
{
    return new Internal::InternalCppCompletionAssistProcessor();
}

QString deriveHeaderGuard(const Utils::FilePath &filePath, ProjectExplorer::Project *project)
{
    return Internal::cppFileSettingsForProject(project).headerGuard(filePath);
}

bool fileSizeExceedsLimit(const FilePath &filePath, int sizeLimitInMb)
{
    if (sizeLimitInMb <= 0)
        return false;

    const qint64 fileSizeInMB = filePath.fileSize() / (1000 * 1000);
    if (fileSizeInMB > sizeLimitInMb) {
        Core::MessageManager::writeSilently(Tr::tr("C++ Indexer: Skipping file \"%1\" because "
                                                   "it is too big.").arg(filePath.displayName()));
        return true;
    }
    return false;
}

void openEditor(const Utils::FilePath &filePath, bool inNextSplit, Utils::Id editorId)
{
    using Core::EditorManager;
    EditorManager::openEditor(filePath, editorId, inNextSplit ? EditorManager::OpenInOtherSplit
                                                              : EditorManager::NoFlags);
}

bool preferLowerCaseFileNames(ProjectExplorer::Project *project)
{
    return Internal::cppFileSettingsForProject(project).lowerCaseFiles;
}

QString preferredCxxHeaderSuffix(ProjectExplorer::Project *project)
{
    return Internal::cppFileSettingsForProject(project).headerSuffix;
}

QString preferredCxxSourceSuffix(ProjectExplorer::Project *project)
{
    return Internal::cppFileSettingsForProject(project).sourceSuffix;
}

SearchResultItems symbolOccurrencesInDeclarationComments(
    const Utils::SearchResultItems &symbolOccurrencesInCode)
{
    if (symbolOccurrencesInCode.isEmpty())
        return {};

    // When using clangd, this function gets called every time the replacement string changes,
    // so cache the results.
    static QHash<SearchResultItems, SearchResultItems> resultCache;
    if (const auto it = resultCache.constFind(symbolOccurrencesInCode);
        it != resultCache.constEnd())  {
        return it.value();
    }
    if (resultCache.size() > 5)
        resultCache.clear();

    QElapsedTimer timer;
    timer.start();
    Snapshot snapshot = CppModelManager::snapshot();
    std::vector<std::unique_ptr<QTextDocument>> docPool;
    using FileData = std::tuple<QTextDocument *, QString, Document::Ptr, QList<Token>>;
    QHash<FilePath, FileData> dataPerFile;
    QString symbolName;
    const auto fileData = [&](const FilePath &filePath) -> FileData & {
        auto &data = dataPerFile[filePath];
        auto &[doc, content, cppDoc, allCommentTokens] = data;
        if (!doc) {
            if (TextEditor::TextDocument * const textDoc
                = TextEditor::TextDocument::textDocumentForFilePath(filePath)) {
                doc = textDoc->document();
            } else {
                std::unique_ptr<QTextDocument> newDoc = std::make_unique<QTextDocument>();
                if (const auto content = TextFileFormat::readFile(
                        filePath, Core::EditorManager::defaultTextCodec())) {
                    newDoc->setPlainText(content.value());
                }
                doc = newDoc.get();
                docPool.push_back(std::move(newDoc));
            }
            content = doc->toPlainText();
            cppDoc = snapshot.preprocessedDocument(content.toUtf8(), filePath);
            cppDoc->check();
        }
        return data;
    };
    static const auto addToken = [](QList<Token> &tokens, const Token &tok) {
        if (!Utils::contains(tokens, [&tok](const Token &t) {
                return t.byteOffset == tok.byteOffset; })) {
            tokens << tok;
        }
    };

    struct ClassInfo {
        FilePath filePath;
        int startOffset = -1;
        int endOffset = -1;
    };
    std::optional<ClassInfo> classInfo;

    // Collect comment blocks associated with replace locations.
    for (const SearchResultItem &item : symbolOccurrencesInCode) {
        const FilePath filePath = FilePath::fromUserInput(item.path().last());
        auto &[doc, content, cppDoc, allCommentTokens] = fileData(filePath);
        const Text::Range &range = item.mainRange();
        if (symbolName.isEmpty()) {
            const int symbolStartPos = Utils::Text::positionInText(doc, range.begin.line,
                                                                   range.begin.column + 1);
            const int symbolEndPos = Utils::Text::positionInText(doc, range.end.line,
                                                                 range.end.column + 1);
            symbolName = content.mid(symbolStartPos, symbolEndPos - symbolStartPos);
        }
        const QList<Token> commentTokens = commentsForDeclaration(symbolName, range.begin,
                                                                  *doc, cppDoc);
        for (const Token &tok : commentTokens)
            addToken(allCommentTokens, tok);

        if (!classInfo) {
            QTextCursor cursor(doc);
            cursor.setPosition(Text::positionInText(doc, range.begin.line, range.begin.column + 1));
            Internal::CanonicalSymbol cs(cppDoc, snapshot);
            Symbol * const canonicalSymbol = cs(cursor);
            if (canonicalSymbol) {
                classInfo.emplace();
                if (Class * const klass = canonicalSymbol->asClass()) {
                    classInfo->filePath = canonicalSymbol->filePath();
                    classInfo->startOffset = klass->startOffset();
                    classInfo->endOffset = klass->endOffset();
                }
            }
        }

        // We hook in between the end of the "regular" search and (possibly non-interactive)
        // actions on it, so we must run synchronously in the UI thread and therefore be fast.
        // If we notice we are lagging, just abort, as renaming the comments is not
        // required for code correctness.
        if (timer.elapsed() > 1000) {
            resultCache.insert(symbolOccurrencesInCode, {});
            return {};
        }
    }

    // If the symbol is a class, collect all comment blocks in the class body.
    if (classInfo && !classInfo->filePath.isEmpty()) {
        auto &[_1, _2, symbolCppDoc, commentTokens] = fileData(classInfo->filePath);
        TranslationUnit * const tu = symbolCppDoc->translationUnit();
        for (int i = 0; i < tu->commentCount(); ++i) {
            const Token &tok = tu->commentAt(i);
            if (tok.bytesBegin() < classInfo->startOffset)
                continue;
            if (tok.bytesBegin() >= classInfo->endOffset)
                break;
            addToken(commentTokens, tok);
        }
    }

    // Create new replace items for occurrences of the symbol name in collected comment blocks.
    SearchResultItems commentItems;
    for (auto it = dataPerFile.cbegin(); it != dataPerFile.cend(); ++it) {
        const auto &[doc, content, cppDoc, commentTokens] = it.value();
        const QStringView docView(content);
        for (const Token &tok : commentTokens) {
            const int tokenStartPos = cppDoc->translationUnit()->getTokenPositionInDocument(
                tok, doc);
            const int tokenEndPos = cppDoc->translationUnit()->getTokenEndPositionInDocument(
                tok, doc);
            const QStringView tokenView = docView.mid(tokenStartPos, tokenEndPos - tokenStartPos);
            const QList<Text::Range> ranges = symbolOccurrencesInText(
                *doc, tokenView, tokenStartPos, symbolName);
            for (const Text::Range &range : ranges) {
                SearchResultItem item;
                item.setUseTextEditorFont(true);
                item.setFilePath(it.key());
                item.setMainRange(range);
                item.setLineText(doc->findBlockByNumber(range.begin.line - 1).text());
                commentItems << item;
            }
        }
    }

    resultCache.insert(symbolOccurrencesInCode, commentItems);
    return commentItems;
}

QList<Text::Range> symbolOccurrencesInText(const QTextDocument &doc, QStringView text, int offset,
                                           const QString &symbolName)
{
    QTC_ASSERT(!symbolName.isEmpty(), return QList<Text::Range>());
    QList<Text::Range> ranges;
    int index = 0;
    while (true) {
        index = text.indexOf(symbolName, index);
        if (index == -1)
            break;

        // Prevent substring matching.
        const auto checkAdjacent = [&](int i) {
            if (i == -1 || i == text.size())
                return true;
            const QChar c = text.at(i);
            if (c.isLetterOrNumber() || c == '_') {
                index += symbolName.length();
                return false;
            }
            return true;
        };
        if (!checkAdjacent(index - 1))
            continue;
        if (!checkAdjacent(index + symbolName.length()))
            continue;

        const Text::Position startPos = Text::Position::fromPositionInDocument(&doc, offset + index);
        index += symbolName.length();
        const Text::Position endPos = Text::Position::fromPositionInDocument(&doc, offset + index);
        ranges << Text::Range{startPos, endPos};
    }
    return ranges;
}

QList<Text::Range> symbolOccurrencesInDeclarationComments(CppEditorWidget *editorWidget,
                                                          const QTextCursor &cursor)
{
    if (!editorWidget)
        return {};
    const SemanticInfo &semanticInfo = editorWidget->semanticInfo();
    const Document::Ptr &cppDoc = semanticInfo.doc;
    if (!cppDoc)
        return {};
    Internal::CanonicalSymbol cs(cppDoc, semanticInfo.snapshot);
    const Symbol * const symbol = cs(cursor);
    if (!symbol || !symbol->asArgument())
        return {};
    const QTextDocument * const textDoc = editorWidget->textDocument()->document();
    QTC_ASSERT(textDoc, return {});
    const QList<Token> comments = commentsForDeclaration(symbol, *textDoc, cppDoc);
    if (comments.isEmpty())
        return {};
    QList<Text::Range> ranges;
    const QString &content = textDoc->toPlainText();
    const QStringView docView = QStringView(content);
    const QString symbolName = Overview().prettyName(symbol->name());
    for (const Token &tok : comments) {
        const int tokenStartPos = cppDoc->translationUnit()->getTokenPositionInDocument(
            tok, textDoc);
        const int tokenEndPos = cppDoc->translationUnit()->getTokenEndPositionInDocument(
            tok, textDoc);
        const QStringView tokenView = docView.mid(tokenStartPos, tokenEndPos - tokenStartPos);
        ranges << symbolOccurrencesInText(*textDoc, tokenView, tokenStartPos, symbolName);
    }
    return ranges;
}

namespace Internal {

void decorateCppEditor(TextEditor::TextEditorWidget *editor)
{
    editor->textDocument()->resetSyntaxHighlighter([] { return new CppHighlighter(); });
    editor->textDocument()->setIndenter(createCppQtStyleIndenter(editor->textDocument()->document()));
    editor->setAutoCompleter(new CppAutoCompleter);
}

} // Internal
} // CppEditor
