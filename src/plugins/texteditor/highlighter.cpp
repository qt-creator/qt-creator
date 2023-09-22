// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlighter.h"

#include "highlightersettings.h"
#include "tabsettings.h"
#include "textdocumentlayout.h"
#include "texteditor.h"
#include "texteditortr.h"
#include "texteditorsettings.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/stylehelper.h>

#include <KSyntaxHighlighting/DefinitionDownloader>
#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QLoggingCategory>
#include <QMetaEnum>

using namespace Utils;

namespace TextEditor {

static Q_LOGGING_CATEGORY(highlighterLog, "qtc.editor.highlighter", QtWarningMsg)

const char kDefinitionForMimeType[] = "definitionForMimeType";
const char kDefinitionForExtension[] = "definitionForExtension";
const char kDefinitionForFilePath[] = "definitionForFilePath";

static KSyntaxHighlighting::Repository *highlightRepository()
{
    static KSyntaxHighlighting::Repository *repository = nullptr;
    if (!repository) {
        repository = new KSyntaxHighlighting::Repository();
        repository->addCustomSearchPath(TextEditorSettings::highlighterSettings().definitionFilesPath().toString());
        const FilePath dir = Core::ICore::resourcePath("generic-highlighter/syntax");
        if (dir.exists())
            repository->addCustomSearchPath(dir.parentDir().path());
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
    const FilePath &filePath = document->filePath();
    Definitions definitions = definitionsForFileName(filePath);
    if (definitions.isEmpty()) {
        // check for *.in filename since those are usually used for
        // cmake configure_file input filenames without the .in extension
        if (filePath.endsWith(".in"))
            definitions = definitionsForFileName(FilePath::fromString(filePath.completeBaseName()));
        if (filePath.fileName() == "qtquickcontrols2.conf")
            definitions = definitionsForFileName(filePath.stringAppended(".ini"));
    }
    if (definitions.isEmpty()) {
        const MimeType &mimeType = Utils::mimeTypeForName(document->mimeType());
        if (mimeType.isValid()) {
            Utils::visitMimeParents(mimeType, [&](const MimeType &mt) -> bool {
                // highlight definitions might not use the canonical name but an alias
                const QStringList names = QStringList(mt.name()) + mt.aliases();
                for (const QString &name : names) {
                    definitions = definitionsForMimeType(name);
                    if (!definitions.isEmpty())
                        return false; // stop
                }
                return true; // continue
            });
        }
    }

    return definitions;
}

static Highlighter::Definition definitionForSetting(const Key &settingsKey,
                                                    const QString &mapKey)
{
    QtcSettings *settings = Core::ICore::settings();
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

Highlighter::Definitions Highlighter::definitionsForFileName(const FilePath &fileName)
{
    Definitions definitions
        = highlightRepository()->definitionsForFileName(fileName.fileName()).toList();

    if (definitions.size() > 1) {
        const QString &fileExtension = fileName.completeSuffix();
        const Definition &rememberedDefinition
            = fileExtension.isEmpty()
                  ? definitionForSetting(kDefinitionForFilePath,
                                         fileName.absoluteFilePath().toString())
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
    const FilePath &path = document->filePath();
    const QString &fileExtension = path.completeSuffix();
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    const Definitions &fileNameDefinitions = definitionsForFileName(path);
    if (fileNameDefinitions.contains(definition)) {
        if (!fileExtension.isEmpty()) {
            const Key id(kDefinitionForExtension);
            QMap<QString, QVariant> map = settings->value(id).toMap();
            map.insert(fileExtension, definition.name());
            settings->setValue(id, map);
        } else if (!path.isEmpty()) {
            const Key id(kDefinitionForFilePath);
            QMap<QString, QVariant> map = settings->value(id).toMap();
            map.insert(path.absoluteFilePath().toString(), definition.name());
            settings->setValue(id, map);
        }
    } else if (!mimeType.isEmpty()) {
        const Key id(kDefinitionForMimeType);
        QMap<QString, QVariant> map = settings->value(id).toMap();
        map.insert(mimeType, definition.name());
        settings->setValue(id, map);
    }
    settings->endGroup();
}

void Highlighter::clearDefinitionForDocumentCache()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    settings->remove(kDefinitionForMimeType);
    settings->remove(kDefinitionForExtension);
    settings->remove(kDefinitionForFilePath);
    settings->endGroup();
}

void Highlighter::addCustomHighlighterPath(const FilePath &path)
{
    highlightRepository()->addCustomSearchPath(path.toString());
}

void Highlighter::downloadDefinitions(std::function<void()> callback)
{
    auto downloader =
        new KSyntaxHighlighting::DefinitionDownloader(highlightRepository());
    connect(downloader, &KSyntaxHighlighting::DefinitionDownloader::done, [downloader, callback]() {
        Core::MessageManager::writeFlashing(Tr::tr("Highlighter updates: done"));
        downloader->deleteLater();
        reload();
        if (callback)
            callback();
    });
    connect(downloader,
            &KSyntaxHighlighting::DefinitionDownloader::informationMessage,
            [](const QString &message) {
                Core::MessageManager::writeSilently(Tr::tr("Highlighter updates:") + ' ' + message);
            });
    Core::MessageManager::writeDisrupting(Tr::tr("Highlighter updates: starting"));
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
    const QTextBlock previousBlock = block.previous();
    TextDocumentLayout::setBraceDepth(block, TextDocumentLayout::braceDepth(previousBlock));
    KSyntaxHighlighting::State previousLineState;
    if (TextBlockUserData *data = TextDocumentLayout::textUserData(previousBlock))
        previousLineState = data->syntaxState();
    KSyntaxHighlighting::State oldState;
    if (TextBlockUserData *data = TextDocumentLayout::textUserData(block)) {
        oldState = data->syntaxState();
        data->setFoldingStartIncluded(false);
        data->setFoldingEndIncluded(false);
    }
    KSyntaxHighlighting::State state = highlightLine(text, previousLineState);
    if (oldState != state) {
        TextBlockUserData *data = TextDocumentLayout::userData(block);
        data->setSyntaxState(state);
        // Toggles the LSB of current block's userState. It forces rehighlight of next block.
        setCurrentBlockState(currentBlockState() ^ 1);
    }

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

    const QTextBlock nextBlock = block.next();
    if (nextBlock.isValid()) {
        TextBlockUserData *data = TextDocumentLayout::userData(nextBlock);
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
            if (StyleHelper::isReadableOn(backgroundColor, textColor)) {
                qformat.setForeground(textColor);
                qformat.setBackground(backgroundColor);
            } else if (StyleHelper::isReadableOn(qformat.background().color(), textColor)) {
                qformat.setForeground(textColor);
            }
        } else if (StyleHelper::isReadableOn(qformat.background().color(), textColor)) {
            qformat.setForeground(textColor);
        }
    } else if (format.hasBackgroundColor(defaultTheme)) {
        const QColor backgroundColor = format.hasBackgroundColor(defaultTheme);
        if (StyleHelper::isReadableOn(backgroundColor, qformat.foreground().color()))
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
        const int newBraceDepth = TextDocumentLayout::braceDepth(block) + 1;
        TextDocumentLayout::setBraceDepth(block, newBraceDepth);
        qCDebug(highlighterLog) << "Found folding start from '" << offset << "' to '" << length
                                << "' resulting in the bracedepth '" << newBraceDepth << "' in :";
        qCDebug(highlighterLog) << text;
        // if there is only a folding begin marker in the line move the current block into the fold
        if (fromStart && toEnd && length <= 1) {
            data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
            data->setFoldingStartIncluded(true);
        }
    } else if (region.type() == KSyntaxHighlighting::FoldingRegion::End) {
        const int newBraceDepth = qMax(0, TextDocumentLayout::braceDepth(block) - 1);
        qCDebug(highlighterLog) << "Found folding end from '" << offset << "' to '" << length
                                << "' resulting in the bracedepth '" << newBraceDepth << "' in :";
        qCDebug(highlighterLog) << text;
        TextDocumentLayout::setBraceDepth(block, newBraceDepth);
        // if the folding end is at the end of the line move the current block into the fold
        if (toEnd)
            data->setFoldingEndIncluded(true);
        else
            data->setFoldingIndent(TextDocumentLayout::braceDepth(block));
    }
}

} // TextEditor
