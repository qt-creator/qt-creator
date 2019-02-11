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

#include "definitiondownloader.h"
#include "highlightersettings.h"
#include "textdocumentlayout.h"
#include "texteditorsettings.h"

#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>
#include <utils/mimetypes/mimedatabase.h>

#include <Format>
#include <Repository>
#include <SyntaxHighlighter>

#include <QDir>
#include <QMetaEnum>

using namespace TextEditor;

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
        definition = Highlighter::definitionForFileName(document->filePath().fileName());
    return definition;
}

Highlighter::Definition Highlighter::definitionForMimeType(const QString &mimeType)
{
    return highlightRepository()->definitionForMimeType(mimeType);
}

Highlighter::Definition Highlighter::definitionForFileName(const QString &fileName)
{
    return highlightRepository()->definitionForFileName(fileName);
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
        definitions = Highlighter::definitionsForFileName(document->filePath().fileName());
    return definitions;
}

Highlighter::Definitions Highlighter::definitionsForMimeType(const QString &mimeType)
{
    return highlightRepository()->definitionsForMimeType(mimeType).toList();
}

Highlighter::Definitions Highlighter::definitionsForFileName(const QString &fileName)
{
    return highlightRepository()->definitionsForFileName(fileName).toList();
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

void Highlighter::highlightBlock(const QString &text)
{
    if (!definition().isValid())
        return;
    QTextBlock block = currentBlock();
    KSyntaxHighlighting::State state = TextDocumentLayout::userData(block)->syntaxState();
    state = highlightLine(text, state);
    block = block.next();
    if (block.isValid())
        TextDocumentLayout::userData(block)->setSyntaxState(state);
}

void Highlighter::applyFormat(int offset, int length, const KSyntaxHighlighting::Format &format)
{
    setFormat(offset, length, formatForCategory(format.textStyle()));
}
