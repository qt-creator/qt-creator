// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsoneditor.h"

#include "autocompleter.h"
#include "textdocument.h"
#include "texteditortr.h"
#include "textindenter.h"

#include <utils/mimeconstants.h>

#include <QTextBlock>

namespace TextEditor::Internal {

const char JSON_EDITOR_ID[] = "Editors.Json";

static int startsWith(const QString &line, const QString &closingChars)
{
    int count = 0;
    for (const QChar &lineChar : line) {
        if (closingChars.contains(lineChar))
            ++count;
        else if (!lineChar.isSpace())
            break;
    }
    return count;
}

class JsonAutoCompleter : public AutoCompleter
{
    bool contextAllowsElectricCharacters(const QTextCursor &cursor) const override
    {
        return !isInString(cursor);
    }

    bool contextAllowAutoInsertion(const QTextCursor &cursor) const
    {
        if (isInString(cursor))
            return false;
        if (cursor.atBlockEnd())
            return true;
        return cursor.block().text().at(cursor.positionInBlock()).isSpace();
    }

    bool contextAllowsAutoBrackets(const QTextCursor &cursor, const QString &) const override
    {
        return contextAllowAutoInsertion(cursor);
    }

    QString insertMatchingBrace(const QTextCursor &cursor,
                                const QString &text,
                                QChar lookAhead,
                                bool skipChars,
                                int *skippedChars) const override
    {
        Q_UNUSED(cursor)
        if (text.isEmpty())
            return QString();
        const QChar current = text.at(0);
        switch (current.unicode()) {
        case '{':
            return QStringLiteral("}");
        case '[':
            return QStringLiteral("]");
        case ']':
        case '}':
            if (current == lookAhead && skipChars)
                ++*skippedChars;
            break;
        default:
            break;
        }

        return QString();
    }

    bool contextAllowsAutoQuotes(const QTextCursor &cursor, const QString &) const override
    {
        return contextAllowAutoInsertion(cursor);
    }

    QString insertMatchingQuote(const QTextCursor &cursor,
                                const QString &text,
                                QChar lookAhead,
                                bool skipChars,
                                int *skippedChars) const override
    {
        Q_UNUSED(cursor)
        static const QChar quote('"');
        if (text.isEmpty() || text != quote)
            return QString();
        if (lookAhead == quote && skipChars) {
            ++*skippedChars;
            return QString();
        }
        return quote;
    }

    bool isInString(const QTextCursor &cursor) const override
    {
        bool result = false;
        const QString text = cursor.block().text();
        const int position = qMin(cursor.positionInBlock(), text.size());

        for (int i = 0; i < position; ++i) {
            if (text.at(i) == '"') {
                if (!result || text[i - 1] != '\\')
                    result = !result;
            }
        }
        return result;
    }
};

class JsonIndenter : public TextIndenter
{
public:
    JsonIndenter(QTextDocument *doc) : TextIndenter(doc)
    {
        setElectricCharacters("{}[]");
    }

    int indentFor(const QTextBlock &block,
                  const TabSettingsData &tabSettings,
                  int /*cursorPositionInEditor*/) override
    {
        QTextBlock previous = block.previous();
        if (!previous.isValid())
            return 0;

        QString previousText = previous.text();
        while (previousText.trimmed().isEmpty()) {
            previous = previous.previous();
            if (!previous.isValid())
                return 0;
            previousText = previous.text();
        }

        int indent = tabSettings.indentationColumn(previousText);

        int adjust = previousText.count('{') + previousText.count('[');
        adjust -= previousText.count('}') + previousText.count(']');
        adjust += startsWith(previousText, "}]") - startsWith(block.text(), "}]");
        adjust *= tabSettings.m_indentSize;

        return qMax(0, indent + adjust);
    }

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TabSettingsData &tabSettings,
                     int cursorPositionInEditor) override
    {
        Q_UNUSED(typedChar)
        tabSettings.indentLine(block, indentFor(block, tabSettings, cursorPositionInEditor));
    }
};

class JsonToken
{
public:
    enum Kind { Punctuation, String, Value };

    Kind kind;
    int position;
    int length;
};

// JSON has no strings that continue over several lines, so each line can be split on its own.
static QList<JsonToken> jsonTokens(const QString &text)
{
    static const QString punctuation = "{}[]:,";
    QList<JsonToken> tokens;
    int i = 0;
    while (i < text.size()) {
        const QChar c = text.at(i);
        const int start = i;
        if (c.isSpace()) {
            ++i;
        } else if (punctuation.contains(c)) {
            tokens.append({JsonToken::Punctuation, start, 1});
            ++i;
        } else if (c == '"') {
            for (++i; i < text.size() && text.at(i) != '"'; ++i) {
                if (text.at(i) == '\\')
                    ++i;
            }
            i = qMin(i + 1, int(text.size()));
            tokens.append({JsonToken::String, start, i - start});
        } else {
            while (i < text.size() && !text.at(i).isSpace() && text.at(i) != '"'
                   && !punctuation.contains(text.at(i))) {
                ++i;
            }
            tokens.append({JsonToken::Value, start, i - start});
        }
    }
    return tokens;
}

static bool isPunctuation(const QString &text, const JsonToken &token, QChar c)
{
    return token.kind == JsonToken::Punctuation && text.at(token.position) == c;
}

// The number of lines in front of a line that are searched for the start of its scope. Splitting
// a line into tokens allocates, and the scope ends are searched for while painting, so a long
// scope, like the outermost array of a compile_commands.json, would otherwise be searched to the
// beginning of the file in every repaint of its last line. As with the "#endif" lines of C++, a
// scope longer than this gets no label. It is well above the largest minimum length of a
// labeled scope that the settings allow, 1000 lines.
const int maximumScopeLines = 10000;

// Calls visit with the tokens in front of position in block, the closest first, until it
// returns false, or until it has gone back maximumScopeLines lines.
template<typename Visitor>
static void visitTokensBackwards(QTextBlock block, int position, const Visitor &visit)
{
    for (int lines = 0; block.isValid() && lines <= maximumScopeLines;
         block = block.previous(), position = -1, ++lines) {
        const QString text = block.text();
        const QList<JsonToken> tokens = jsonTokens(text);
        for (auto token = tokens.crbegin(); token != tokens.crend(); ++token) {
            if (position >= 0 && token->position >= position)
                continue;
            if (!visit(block, text, *token))
                return;
        }
    }
}

// Calls visit with the tokens after position in block, in order, until it returns false, or
// until it has gone forward maximumScopeLines lines.
template<typename Visitor>
static void visitTokensForwards(QTextBlock block, int position, const Visitor &visit)
{
    for (int lines = 0; block.isValid() && lines <= maximumScopeLines;
         block = block.next(), position = -1, ++lines) {
        const QString text = block.text();
        for (const JsonToken &token : jsonTokens(text)) {
            if (token.position <= position)
                continue;
            if (!visit(block, text, token))
                return;
        }
    }
}

JsonEditorWidget::JsonEditorWidget()
{
    // Also emitted when the document is replaced.
    connect(this, &TextEditorWidget::textChanged, this, [this] {
        m_scopeStartCache.clear();
        m_scopeLabelCache.clear();
        m_elementIndexCache.clear();
        m_indexedArrays.clear();
    });
}

int JsonEditorWidget::scopeStartPosition(const QTextBlock &endBlock, int bracePosition) const
{
    const int position = endBlock.position() + bracePosition;
    auto cached = m_scopeStartCache.constFind(position);
    if (cached == m_scopeStartCache.cend())
        cached = m_scopeStartCache.insert(position,
                                          findScopeStartPosition(endBlock, bracePosition));
    return *cached;
}

// The end of an object or array is a closing brace or bracket. The base widget only asks for
// the ones that are the only code in their line, like those in "}],".
int JsonEditorWidget::findScopeStartPosition(const QTextBlock &endBlock, int bracePosition) const
{
    const bool closesObject = endBlock.text().at(bracePosition) == '}';
    int result = -1;
    int depth = 0;
    visitTokensBackwards(endBlock, bracePosition,
                         [&](const QTextBlock &block, const QString &line, const JsonToken &token) {
        if (token.kind != JsonToken::Punctuation)
            return true;
        const QChar c = line.at(token.position);
        if (c == '}' || c == ']') {
            ++depth;
        } else if (c == '{' || c == '[') {
            if (depth-- > 0)
                return true;
            if ((c == '{') == closesObject)
                result = block.position() + token.position;
            return false;
        }
        return true;
    });
    return result;
}

QString JsonEditorWidget::scopeLabelText(const QTextBlock &openBlock, int openBracePosition) const
{
    const int position = openBlock.position() + openBracePosition;
    auto cached = m_scopeLabelCache.constFind(position);
    if (cached == m_scopeLabelCache.cend())
        cached = m_scopeLabelCache.insert(position,
                                          findScopeLabelText(openBlock, openBracePosition));
    return *cached;
}

// An object or array is labeled with its key, and an element of an array with its index in
// the array, like "[2]". The outermost value has no label.
QString JsonEditorWidget::findScopeLabelText(const QTextBlock &openBlock,
                                             int openBracePosition) const
{
    const int position = openBlock.position() + openBracePosition;
    // The elements of an array are indexed all at once, so that the labels of its other
    // elements do not search for its start again.
    if (const auto index = m_elementIndexCache.constFind(position);
        index != m_elementIndexCache.cend()) {
        return QString("[%1]").arg(*index);
    }

    QString label;
    bool first = true;
    bool afterColon = false;
    int depth = 0;
    int arrayPosition = -1;
    visitTokensBackwards(openBlock, openBracePosition,
                         [&](const QTextBlock &block, const QString &line, const JsonToken &token) {
        if (std::exchange(first, false) && isPunctuation(line, token, ':')) {
            afterColon = true;
            return true;
        }
        if (afterColon) {
            if (token.kind == JsonToken::String)
                label = line.mid(token.position, token.length);
            return false;
        }
        if (token.kind != JsonToken::Punctuation)
            return true;
        const QChar c = line.at(token.position);
        if (c == '}' || c == ']') {
            ++depth;
        } else if (c == '{' || c == '[') {
            if (depth-- > 0)
                return true;
            if (c == '[')
                arrayPosition = block.position() + token.position;
            return false;
        }
        return true;
    });
    if (afterColon || arrayPosition < 0)
        return label;

    indexArrayElements(arrayPosition);
    const auto index = m_elementIndexCache.constFind(position);
    return index == m_elementIndexCache.cend() ? QString() : QString("[%1]").arg(*index);
}

// Stores the index of each element of the array that starts at arrayPosition by the position
// of the element's first token. Elements further than maximumScopeLines lines away from the
// start of the array are not indexed, as the search back to the start does not reach them.
void JsonEditorWidget::indexArrayElements(int arrayPosition) const
{
    if (m_indexedArrays.contains(arrayPosition))
        return;
    m_indexedArrays.insert(arrayPosition);
    const QTextBlock arrayBlock = document()->findBlock(arrayPosition);
    int index = 0;
    int depth = 0;
    bool elementNext = true;
    visitTokensForwards(arrayBlock, arrayPosition - arrayBlock.position(),
                        [&](const QTextBlock &block, const QString &line, const JsonToken &token) {
        const QChar c = token.kind == JsonToken::Punctuation ? line.at(token.position) : QChar();
        if (depth == 0) {
            if (c == ']' || c == '}')
                return false;
            if (c == ',') {
                elementNext = true;
                return true;
            }
            if (std::exchange(elementNext, false))
                m_elementIndexCache.insert(block.position() + token.position, index++);
        }
        if (c == '{' || c == '[')
            ++depth;
        else if (c == '}' || c == ']')
            --depth;
        return true;
    });
}

class JsonEditorFactory final : public TextEditorFactory
{
public:
    JsonEditorFactory()
    {
        setId(JSON_EDITOR_ID);
        setDisplayName(Tr::tr("JSON Editor"));
        addMimeType(Utils::Constants::JSON_MIMETYPE);

        setEditorCreator([] { return new BaseTextEditor; });
        setEditorWidgetCreator([] { return new JsonEditorWidget; });
        setDocumentCreator([] { return new TextDocument(JSON_EDITOR_ID); });
        setAutoCompleterCreator([] { return new JsonAutoCompleter; });
        setIndenterCreator([](QTextDocument *doc) { return new JsonIndenter(doc); });
        setOptionalActionMask(OptionalActions::Format);
        setUseGenericHighlighter(true);
    }
};

void setupJsonEditor()
{
    static JsonEditorFactory theJsonEditorFactory;
}

} // TextEditor::Internal
