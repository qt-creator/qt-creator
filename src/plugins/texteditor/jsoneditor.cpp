// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "jsoneditor.h"

#include "autocompleter.h"
#include "textdocument.h"
#include "texteditoractionhandler.h"
#include "texteditortr.h"
#include "textindenter.h"

namespace TextEditor::Internal {

const char JSON_EDITOR_ID[] = "Editors.Json";
const char JSON_MIME_TYPE[] = "application/json";

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

    bool contextAllowsAutoBrackets(const QTextCursor &cursor, const QString &) const override
    {
        return !isInString(cursor);
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
        return !isInString(cursor);
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
    JsonIndenter(QTextDocument *doc) : TextIndenter(doc) {}

    bool isElectricCharacter(const QChar &c) const override
    {
        static QString echars("{}[]");
        return echars.contains(c);
    }

    int indentFor(const QTextBlock &block,
                  const TabSettings &tabSettings,
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
                     const TabSettings &tabSettings,
                     int cursorPositionInEditor) override
    {
        Q_UNUSED(typedChar)
        tabSettings.indentLine(block, indentFor(block, tabSettings, cursorPositionInEditor));
    }
};

JsonEditorFactory::JsonEditorFactory()
{
    setId(JSON_EDITOR_ID);
    setDisplayName(Tr::tr("JSON Editor"));
    addMimeType(JSON_MIME_TYPE);

    setEditorCreator([] { return new BaseTextEditor; });
    setEditorWidgetCreator([] { return new TextEditorWidget; });
    setDocumentCreator([] { return new TextDocument(JSON_EDITOR_ID); });
    setAutoCompleterCreator([] { return new JsonAutoCompleter; });
    setIndenterCreator([](QTextDocument *doc) { return new JsonIndenter(doc); });
    setEditorActionHandlers(TextEditorActionHandler::Format);
    setUseGenericHighlighter(true);
}

} // namespace TextEditor::Internal
