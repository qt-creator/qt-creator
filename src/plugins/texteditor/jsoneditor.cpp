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
    bool contextAllowsElectricCharacters(const QTextCursor &cursor) const override { return true; }
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
    setDisplayName(Tr::tr("Json Editor"));
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
