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

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/mimetypes/mimedatabase.h>

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

Highlighter::Definition Highlighter::definitionForDocument(const TextDocument *document)
{
    const Utils::MimeType mimeType = Utils::mimeTypeForName(document->mimeType());
    Definition definition;
    if (mimeType.isValid())
        definition = Highlighter::definitionForMimeType(mimeType.name());
    if (!definition.isValid())
        definition = Highlighter::definitionForFilePath(document->filePath());
    return definition;
}

Highlighter::Definition Highlighter::definitionForMimeType(const QString &mimeType)
{
    const Definitions definitions = definitionsForMimeType(mimeType);
    if (definitions.size() == 1)
        return definitions.first();
    return highlightRepository()->definitionForMimeType(mimeType);
}

Highlighter::Definition Highlighter::definitionForFilePath(const Utils::FileName &fileName)
{
    const Definitions definitions = definitionsForFileName(fileName);
    if (definitions.size() == 1)
        return definitions.first();
    return highlightRepository()->definitionForFileName(fileName.fileName());
}

Highlighter::Definition Highlighter::definitionForName(const QString &name)
{
    return highlightRepository()->definitionForName(name);
}

Highlighter::Definitions Highlighter::definitionsForDocument(const TextDocument *document)
{
    const Utils::MimeType mimeType = Utils::mimeTypeForName(document->mimeType());
    Definitions definitions;
    if (mimeType.isValid())
        definitions = Highlighter::definitionsForMimeType(mimeType.name());
    if (definitions.isEmpty())
        definitions = Highlighter::definitionsForFileName(document->filePath());
    return definitions;
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

Highlighter::Definitions Highlighter::definitionsForFileName(const Utils::FileName &fileName)
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

void Highlighter::rememberDefintionForDocument(const Highlighter::Definition &definition,
                                               const TextDocument *document)
{
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

void Highlighter::clearDefintionForDocumentCache()
{
    QSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    settings->remove(kDefinitionForMimeType);
    settings->remove(kDefinitionForExtension);
    settings->remove(kDefinitionForFilePath);
    settings->endGroup();
}

void Highlighter::addCustomHighlighterPath(const Utils::FileName &path)
{
    highlightRepository()->addCustomSearchPath(path.toString());
}

void Highlighter::updateDefinitions(std::function<void()> callback) {
    auto downloader =
        new KSyntaxHighlighting::DefinitionDownloader(highlightRepository());
    connect(downloader, &KSyntaxHighlighting::DefinitionDownloader::done,
            [downloader, callback]() {
                Core::MessageManager::write(tr("Highlighter updates: done"),
                                            Core::MessageManager::ModeSwitch);
                downloader->deleteLater();
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
    if (!definition().isValid())
        return;
    const QTextBlock block = currentBlock();
    KSyntaxHighlighting::State state;
    setCurrentBlockState(qMax(0, previousBlockState()));
    if (TextBlockUserData *data = TextDocumentLayout::testUserData(block)) {
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
        data->setSyntaxState(state);
        data->setFoldingIndent(currentBlockState());
    }

    formatSpaces(text);
}

void Highlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    setFormat(offset, length, formatForCategory(format.textStyle()));
}

void Highlighter::applyFolding(int offset,
                               int length,
                               KSyntaxHighlighting::FoldingRegion region)
{
    if (!region.isValid())
        return;
    const QTextBlock &block = currentBlock();
    const QString &text = block.text();
    TextBlockUserData *data = TextDocumentLayout::userData(currentBlock());
    const bool fromStart = TabSettings::firstNonSpace(text) == offset;
    const bool toEnd = (offset + length) == (text.length() - TabSettings::trailingWhitespaces(text));
    if (region.type() == KSyntaxHighlighting::FoldingRegion::Begin) {
        setCurrentBlockState(currentBlockState() + 1);
        // if there is only a folding begin in the line move the current block into the fold
        if (fromStart && toEnd) {
            data->setFoldingIndent(currentBlockState());
            data->setFoldingStartIncluded(true);
        }
    } else if (region.type() == KSyntaxHighlighting::FoldingRegion::End) {
        setCurrentBlockState(qMax(0, currentBlockState() - 1));
        // if the folding end is at the end of the line move the current block into the fold
        if (toEnd)
            data->setFoldingEndIncluded(true);
        else
            data->setFoldingIndent(currentBlockState());
    }
}
