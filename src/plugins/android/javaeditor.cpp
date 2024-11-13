// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "javaeditor.h"
#include "androidconstants.h"

#include <coreplugin/coreplugintr.h>
#include <coreplugin/editormanager/ieditorfactory.h>

#include <texteditor/codeassist/keywordscompletionassist.h>
#include <texteditor/textdocument.h>
#include <texteditor/texteditorconstants.h>
#include <texteditor/texteditor.h>
#include <texteditor/textindenter.h>

#include <utils/mimeconstants.h>
#include <utils/uncommentselection.h>

namespace Android::Internal {

class JavaIndenter final : public TextEditor::TextIndenter
{
public:
    explicit JavaIndenter(QTextDocument *doc) : TextEditor::TextIndenter(doc) {}

    bool isElectricCharacter(const QChar &ch) const final
    {
        return ch == QLatin1Char('{') || ch == QLatin1Char('}');
    }

    void indentBlock(const QTextBlock &block,
                     const QChar &typedChar,
                     const TextEditor::TabSettings &tabSettings,
                     int cursorPositionInEditor = -1) final;

    int indentFor(const QTextBlock &block,
                  const TextEditor::TabSettings &tabSettings,
                  int cursorPositionInEditor = -1) final;
};

void JavaIndenter::indentBlock(const QTextBlock &block,
                               const QChar &typedChar,
                               const TextEditor::TabSettings &tabSettings,
                               int /*cursorPositionInEditor*/)
{
    int indent = indentFor(block, tabSettings);
    if (typedChar == QLatin1Char('}'))
        indent -= tabSettings.m_indentSize;
    tabSettings.indentLine(block, qMax(0, indent));
}

int JavaIndenter::indentFor(const QTextBlock &block,
                            const TextEditor::TabSettings &tabSettings,
                            int /*cursorPositionInEditor*/)
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

    int adjust = previousText.count(QLatin1Char('{')) - previousText.count(QLatin1Char('}'));
    adjust *= tabSettings.m_indentSize;

    return qMax(0, indent + adjust);
}

static TextEditor::TextDocument *createJavaDocument()
{
    auto doc = new TextEditor::TextDocument;
    doc->setId(Constants::JAVA_EDITOR_ID);
    doc->setMimeType(Utils::Constants::JAVA_MIMETYPE);
    doc->setIndenter(new JavaIndenter(doc->document()));
    return doc;
}

class JavaEditorFactory : public TextEditor::TextEditorFactory
{
public:
    JavaEditorFactory()
    {
        static QStringList keywords = {
            "abstract", "assert", "boolean", "break", "byte", "case", "catch", "char", "class", "const",
            "continue", "default", "do", "double", "else", "enum", "extends", "final", "finally",
            "float", "for", "goto", "if", "implements", "import", "instanceof", "int", "interface",
            "long", "native", "new", "package", "private", "protected", "public", "return", "short",
            "static", "strictfp", "super", "switch", "synchronized", "this", "throw", "throws",
            "transient", "try", "void", "volatile", "while"
        };
        setId(Constants::JAVA_EDITOR_ID);
        setDisplayName(::Core::Tr::tr("Java Editor"));
        addMimeType(Utils::Constants::JAVA_MIMETYPE);

        setDocumentCreator(createJavaDocument);
        setUseGenericHighlighter(true);
        setCommentDefinition(Utils::CommentDefinition::CppStyle);
        setOptionalActionMask(TextEditor::OptionalActions::UnCommentSelection);
        setCompletionAssistProvider(new TextEditor::KeywordsCompletionAssistProvider(keywords));
    }
};

void setupJavaEditor()
{
    static JavaEditorFactory theJavaEditorFactory;
}

} // Android::Internal
