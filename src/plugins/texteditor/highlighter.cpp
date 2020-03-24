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

#include "highlighter.h"

#include "highlightersettings.h"
#include "textdocumentlayout.h"
#include "tabsettings.h"
#include "texteditorsettings.h"
#include "texteditor.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/mimetypes/mimedatabase.h>
#include <utils/stylehelper.h>
#include <utils/qtcassert.h>

#include <DefinitionDownloader>
#include <Format>
#include <FoldingRegion>
#include <Repository>
#include <SyntaxHighlighter>

#include <QDir>
#include <QMetaEnum>

using namespace TextEditor;

static const char kDefinitionForMimeType[] = "definitionForMimeType";
static const char kDefinitionForExtension[] = "definitionForExtension";
static const char kDefinitionForFilePath[] = "definitionForFilePath";

KSyntaxHighlighting::Repository *highlightRepository()
{
    static KSyntaxHighlighting::Repository *repository = nullptr;
    if (!repository) {
        repository = new KSyntaxHighlighting::Repository();
        repository->addCustomSearchPath(TextEditorSettings::highlighterSettings().definitionFilesPath());
        QDir dir(Core::ICore::resourcePath() + QLatin1String("/generic-highlighter/syntax"));
        if (dir.exists() && dir.cdUp())
            repository->addCustomSearchPath(dir.path());
    }
    return repository;
}

TextStyle categoryForTextStyle(int style)
{
    switch (style) {
    case KSyntaxHighlighting::Theme::Normal: return C_TEXT;
    case KSyntaxHighlighting::Theme::Keyword: return C_KEYWORD;
    case KSyntaxHighlighting::Theme::Function: return C_FUNCTION;
    case KSyntaxHighlighting::Theme::Variable: return C_LOCAL;
    case KSyntaxHighlighting::Theme::ControlFlow: return C_KEYWORD;
    case KSyntaxHighlighting::Theme::Operator: return C_OPERATOR;
    case KSyntaxHighlighting::Theme::BuiltIn: return C_PRIMITIVE_TYPE;
    case KSyntaxHighlighting::Theme::Extension: return C_GLOBAL;
    case KSyntaxHighlighting::Theme::Preprocessor: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::Attribute: return C_LOCAL;
    case KSyntaxHighlighting::Theme::Char: return C_STRING;
    case KSyntaxHighlighting::Theme::SpecialChar: return C_STRING;
    case KSyntaxHighlighting::Theme::String: return C_STRING;
    case KSyntaxHighlighting::Theme::VerbatimString: return C_STRING;
    case KSyntaxHighlighting::Theme::SpecialString: return C_STRING;
    case KSyntaxHighlighting::Theme::Import: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::DataType: return C_TYPE;
    case KSyntaxHighlighting::Theme::DecVal: return C_NUMBER;
    case KSyntaxHighlighting::Theme::BaseN: return C_NUMBER;
    case KSyntaxHighlighting::Theme::Float: return C_NUMBER;
    case KSyntaxHighlighting::Theme::Constant: return C_KEYWORD;
    case KSyntaxHighlighting::Theme::Comment: return C_COMMENT;
    case KSyntaxHighlighting::Theme::Documentation: return C_DOXYGEN_COMMENT;
    case KSyntaxHighlighting::Theme::Annotation: return C_DOXYGEN_TAG;
    case KSyntaxHighlighting::Theme::CommentVar: return C_DOXYGEN_TAG;
    case KSyntaxHighlighting::Theme::RegionMarker: return C_PREPROCESSOR;
    case KSyntaxHighlighting::Theme::Information: return C_WARNING;
    case KSyntaxHighlighting::Theme::Warning: return C_WARNING;
    case KSyntaxHighlighting::Theme::Alert: return C_ERROR;
    case KSyntaxHighlighting::Theme::Error: return C_ERROR;
    case KSyntaxHighlighting::Theme::Others: return C_TEXT;
    }
    return C_TEXT;
}

Highlighter::Highlighter()
{
    setTextFormatCategories(QMetaEnum::fromType<KSyntaxHighlighting::Theme::TextStyle>().keyCount(),
                            &categoryForTextStyle);
}

Highlighter::Definition Highlighter::definitionForName(const QString &name)
{
    return highlightRepository()->definitionForName(name);
}

Highlighter::Definitions Highlighter::definitionsForDocument(const TextDocument *document)
{
    QTC_ASSERT(document, return {});
    // First try to find definitions for the file path, only afterwards try the MIME type.
    // An example where that is important is if there was a definition for "*.rb.xml", which
    // cannot be referred to with a MIME type (since there is none), but there is the definition
    // for XML files, which specifies a MIME type in addition to a glob pattern.
    // If we check the MIME type first and then skip the pattern, the definition for "*.rb.xml" is
    // never considered.
    // The KSyntaxHighlighting CLI also completely ignores MIME types.
    const Definitions &fileNameDefinitions = definitionsForFileName(document->filePath());
    if (!fileNameDefinitions.isEmpty())
        return fileNameDefinitions;
    const Utils::MimeType &mimeType = Utils::mimeTypeForName(document->mimeType());
    if (!mimeType.isValid())
        return fileNameDefinitions;
    return definitionsForMimeType(mimeType.name());
}

static Highlighter::Definition definitionForSetting(const QString &settingsKey,
                                                    const QString &mapKey)
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    const QString &definitionName = settings->value(settingsKey).toMap().value(mapKey).toString();
    settings->endGroup();
    return Highlighter::definitionForName(definitionName);
}

Highlighter::Definitions Highlighter::definitionsForMimeType(const QString &mimeType)
{
    Definitions definitions = highlightRepository()->definitionsForMimeType(mimeType).toList();
    if (definitions.size() > 1) {
        const Definition &rememberedDefinition = definitionForSetting(kDefinitionForMimeType,
                                                                      mimeType);
        if (rememberedDefinition.isValid() && definitions.contains(rememberedDefinition))
            definitions = {rememberedDefinition};
    }
    return definitions;
}

Highlighter::Definitions Highlighter::definitionsForFileName(const Utils::FilePath &fileName)
{
    Definitions definitions
        = highlightRepository()->definitionsForFileName(fileName.fileName()).toList();

    if (definitions.size() > 1) {
        const QString &fileExtension = fileName.toFileInfo().completeSuffix();
        const Definition &rememberedDefinition
            = fileExtension.isEmpty()
                  ? definitionForSetting(kDefinitionForFilePath,
                                         fileName.toFileInfo().canonicalFilePath())
                  : definitionForSetting(kDefinitionForExtension, fileExtension);
        if (rememberedDefinition.isValid() && definitions.contains(rememberedDefinition))
            definitions = {rememberedDefinition};
    }

    return definitions;
}

void Highlighter::rememberDefinitionForDocument(const Highlighter::Definition &definition,
                                                const TextDocument *document)
{
    QTC_ASSERT(document, return );
    if (!definition.isValid())
        return;
    const QString &mimeType = document->mimeType();
    const QString &fileExtension = document->filePath().toFileInfo().completeSuffix();
    const QString &path = document->filePath().toFileInfo().canonicalFilePath();
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    if (!mimeType.isEmpty()) {
        const QString id(kDefinitionForMimeType);
        QMap<QString, QVariant> map = settings->value(id).toMap();
        map.insert(mimeType, definition.name());
        settings->setValue(id, map);
    } else if (!fileExtension.isEmpty()) {
        const QString id(kDefinitionForExtension);
        QMap<QString, QVariant> map = settings->value(id).toMap();
        map.insert(fileExtension, definition.name());
        settings->setValue(id, map);
    } else if (!path.isEmpty()) {
        const QString id(kDefinitionForFilePath);
        QMap<QString, QVariant> map = settings->value(id).toMap();
        map.insert(document->filePath().toFileInfo().absoluteFilePath(), definition.name());
        settings->setValue(id, map);
    }
    settings->endGroup();
}

void Highlighter::clearDefinitionForDocumentCache()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    settings->remove(kDefinitionForMimeType);
    settings->remove(kDefinitionForExtension);
    settings->remove(kDefinitionForFilePath);
    settings->endGroup();
}

void Highlighter::addCustomHighlighterPath(const Utils::FilePath &path)
{
    highlightRepository()->addCustomSearchPath(path.toString());
}

void Highlighter::downloadDefinitions(std::function<void()> callback) {
    auto downloader =
        new KSyntaxHighlighting::DefinitionDownloader(highlightRepository());
    connect(downloader, &KSyntaxHighlighting::DefinitionDownloader::done,
            [downloader, callback]() {
                Core::MessageManager::write(tr("Highlighter updates: done"),
                                            Core::MessageManager::ModeSwitch);
                downloader->deleteLater();
                reload();
                if (callback)
                    callback();
            });
    connect(downloader,
            &KSyntaxHighlighting::DefinitionDownloader::informationMessage,
            [](const QString &message) {
                Core::MessageManager::write(tr("Highlighter updates:") + ' ' +
                                                message,
                                            Core::MessageManager::ModeSwitch);
            });
    downloader->start();
}

void Highlighter::reload()
{
    highlightRepository()->reload();
    for (auto editor : Core::DocumentModel::editorsForOpenedDocuments()) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
            if (qobject_cast<Highlighter *>(textEditor->textDocument()->syntaxHighlighter()))
                textEditor->editorWidget()->configureGenericHighlighter();
        }
    }
}

void Highlighter::handleShutdown()
{
    delete highlightRepository();
}

static bool isOpeningParenthesis(QChar c)
{
    return c == QLatin1Char('{') || c == QLatin1Char('[') || c == QLatin1Char('(');
}

static bool isClosingParenthesis(QChar c)
{
    return c == QLatin1Char('}') || c == QLatin1Char(']') || c == QLatin1Char(')');
}

void Highlighter::highlightBlock(const QString &text)
{
    if (!definition().isValid()) {
        formatSpaces(text);
        return;
    }
    QTextBlock block = currentBlock();
    KSyntaxHighlighting::State state;
    TextDocumentLayout::setBraceDepth(block, TextDocumentLayout::braceDepth(block.previous()));
    if (TextBlockUserData *data = TextDocumentLayout::textUserData(block)) {
        state = data->syntaxState();
        data->setFoldingStartIncluded(false);
        data->setFoldingEndIncluded(false);
    }
    state = highlightLine(text, state);
    const QTextBlock nextBlock = block.next();

    Parentheses parentheses;
    int pos = 0;
    for (const QChar &c : text) {
        if (isOpeningParenthesis(c))
            parentheses.push_back(Parenthesis(Parenthesis::Opened, c, pos));
        else if (isClosingParenthesis(c))
            parentheses.push_back(Parenthesis(Parenthesis::Closed, c, pos));
        pos++;
    }
    TextDocumentLayout::setParentheses(currentBlock(), parentheses);

    if (nextBlock.isValid()) {
        TextBlockUserData *data = TextDocumentLayout::userData(nextBlock);
        if (data->syntaxState() != state) {
            data->setSyntaxState(state);
            setCurrentBlockState(currentBlockState() ^ 1); // force rehighlight of next block
        }
        data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
    }

    formatSpaces(text);
}

void Highlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    const KSyntaxHighlighting::Theme defaultTheme;
    QTextCharFormat qformat = formatForCategory(format.textStyle());

    if (format.hasTextColor(defaultTheme)) {
        const QColor textColor = format.textColor(defaultTheme);
        if (format.hasBackgroundColor(defaultTheme)) {
            const QColor backgroundColor = format.hasBackgroundColor(defaultTheme);
            if (Utils::StyleHelper::isReadableOn(backgroundColor, textColor)) {
                qformat.setForeground(textColor);
                qformat.setBackground(backgroundColor);
            } else if (Utils::StyleHelper::isReadableOn(qformat.background().color(), textColor)) {
                qformat.setForeground(textColor);
            }
        } else if (Utils::StyleHelper::isReadableOn(qformat.background().color(), textColor)) {
            qformat.setForeground(textColor);
        }
    } else if (format.hasBackgroundColor(defaultTheme)) {
        const QColor backgroundColor = format.hasBackgroundColor(defaultTheme);
        if (Utils::StyleHelper::isReadableOn(backgroundColor, qformat.foreground().color()))
            qformat.setBackground(backgroundColor);
    }

    if (format.isBold(defaultTheme))
        qformat.setFontWeight(QFont::Bold);

    if (format.isItalic(defaultTheme))
        qformat.setFontItalic(true);

    if (format.isUnderline(defaultTheme))
        qformat.setFontUnderline(true);

    if (format.isStrikeThrough(defaultTheme))
        qformat.setFontStrikeOut(true);
    setFormat(offset, length, qformat);
}

void Highlighter::applyFolding(int offset,
                               int length,
                               KSyntaxHighlighting::FoldingRegion region)
{
    if (!region.isValid())
        return;
    QTextBlock block = currentBlock();
    const QString &text = block.text();
    TextBlockUserData *data = TextDocumentLayout::userData(currentBlock());
    const bool fromStart = TabSettings::firstNonSpace(text) == offset;
    const bool toEnd = (offset + length) == (text.length() - TabSettings::trailingWhitespaces(text));
    if (region.type() == KSyntaxHighlighting::FoldingRegion::Begin) {
        TextDocumentLayout::changeBraceDepth(block, 1);
        // if there is only a folding begin in the line move the current block into the fold
        if (fromStart && toEnd) {
            data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
            data->setFoldingStartIncluded(true);
        }
    } else if (region.type() == KSyntaxHighlighting::FoldingRegion::End) {
        TextDocumentLayout::changeBraceDepth(block, -1);
        // if the folding end is at the end of the line move the current block into the fold
        if (toEnd)
            data->setFoldingEndIncluded(true);
        else
            data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
    }
}
