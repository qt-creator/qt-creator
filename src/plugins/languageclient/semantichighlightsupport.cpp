/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include "semantichighlightsupport.h"

#include <QTextDocument>

using namespace LanguageServerProtocol;

namespace LanguageClient {
namespace SemanticHighligtingSupport {

static Q_LOGGING_CATEGORY(LOGLSPHIGHLIGHT, "qtc.languageclient.highlight", QtWarningMsg);

static const QList<QList<QString>> highlightScopes(const ServerCapabilities &capabilities)
{
    return capabilities.semanticHighlighting()
        .value_or(ServerCapabilities::SemanticHighlightingServerCapabilities())
        .scopes().value_or(QList<QList<QString>>());
}

static Utils::optional<TextEditor::TextStyle> styleForScopes(const QList<QString> &scopes)
{
    // missing "Minimal Scope Coverage" scopes

    // entity.other.inherited-class
    // entity.name.section
    // entity.name.tag
    // entity.other.attribute-name
    // variable.language
    // variable.parameter
    // variable.function
    // constant.numeric
    // constant.language
    // constant.character.escape
    // support
    // storage.modifier
    // keyword.control
    // keyword.operator
    // keyword.declaration
    // invalid
    // invalid.deprecated

    static const QMap<QString, TextEditor::TextStyle> styleForScopes = {
        {"entity.name", TextEditor::C_TYPE},
        {"entity.name.function", TextEditor::C_FUNCTION},
        {"entity.name.function.method.static", TextEditor::C_GLOBAL},
        {"entity.name.function.preprocessor", TextEditor::C_PREPROCESSOR},
        {"entity.name.label", TextEditor::C_LABEL},
        {"keyword", TextEditor::C_KEYWORD},
        {"storage.type", TextEditor::C_KEYWORD},
        {"constant.numeric", TextEditor::C_NUMBER},
        {"string", TextEditor::C_STRING},
        {"comment", TextEditor::C_COMMENT},
        {"comment.block.documentation", TextEditor::C_DOXYGEN_COMMENT},
        {"variable.function", TextEditor::C_FUNCTION},
        {"variable.other", TextEditor::C_LOCAL},
        {"variable.other.member", TextEditor::C_FIELD},
        {"variable.other.field", TextEditor::C_FIELD},
        {"variable.other.field.static", TextEditor::C_GLOBAL},
        {"variable.parameter", TextEditor::C_LOCAL},
    };

    for (QString scope : scopes) {
        while (!scope.isEmpty()) {
            auto style = styleForScopes.find(scope);
            if (style != styleForScopes.end())
                return style.value();
            const int index = scope.lastIndexOf('.');
            if (index <= 0)
                break;
            scope = scope.left(index);
        }
    }
    return Utils::nullopt;
}

static QHash<int, QTextCharFormat> scopesToFormatHash(QList<QList<QString>> scopes,
                                                      const TextEditor::FontSettings &fontSettings)
{
    QHash<int, QTextCharFormat> scopesToFormat;
    for (int i = 0; i < scopes.size(); ++i) {
        if (Utils::optional<TextEditor::TextStyle> style = styleForScopes(scopes[i]))
            scopesToFormat[i] = fontSettings.toTextCharFormat(style.value());
    }
    return scopesToFormat;
}

TextEditor::HighlightingResult tokenToHighlightingResult(int line,
                                                         const SemanticHighlightToken &token)
{
    return TextEditor::HighlightingResult(unsigned(line) + 1,
                                          unsigned(token.character) + 1,
                                          token.length,
                                          int(token.scope));
}

TextEditor::HighlightingResults generateResults(const QList<SemanticHighlightingInformation> &lines)
{
    TextEditor::HighlightingResults results;

    for (const SemanticHighlightingInformation &info : lines) {
        const int line = info.line();
        for (const SemanticHighlightToken &token :
             info.tokens().value_or(QList<SemanticHighlightToken>())) {
            results << tokenToHighlightingResult(line, token);
        }
    }

    return results;
}

void applyHighlight(TextEditor::TextDocument *doc,
                    const TextEditor::HighlightingResults &results,
                    const ServerCapabilities &capabilities)
{
    if (!doc->syntaxHighlighter())
        return;
    if (LOGLSPHIGHLIGHT().isDebugEnabled()) {
        auto scopes = highlightScopes(capabilities);
        qCDebug(LOGLSPHIGHLIGHT) << "semantic highlight for" << doc->filePath();
        for (auto result : results) {
            auto b = doc->document()->findBlockByNumber(int(result.line - 1));
            const QString &text = b.text().mid(int(result.column - 1), int(result.length));
            auto resultScupes = scopes[result.kind];
            auto style = styleForScopes(resultScupes).value_or(TextEditor::C_TEXT);
            qCDebug(LOGLSPHIGHLIGHT) << result.line - 1 << '\t'
                                     << result.column - 1 << '\t'
                                     << result.length << '\t'
                                     << TextEditor::Constants::nameForStyle(style) << '\t'
                                     << text
                                     << resultScupes;
        }
    }

    if (capabilities.semanticHighlighting().has_value()) {
        TextEditor::SemanticHighlighter::setExtraAdditionalFormats(
            doc->syntaxHighlighter(),
            results,
            scopesToFormatHash(highlightScopes(capabilities), doc->fontSettings()));
    }
}

} // namespace SemanticHighligtingSupport
} // namespace LanguageClient
