// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "highlighterhelper.h"

#include "highlighter.h"
#include "highlightersettings.h"
#include "textdocument.h"
#include "texteditor.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/editormanager/documentmodel.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <utils/mimeutils.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>
#include <utils/stylehelper.h>

#include <KSyntaxHighlighting/DefinitionDownloader>
#include <KSyntaxHighlighting/FoldingRegion>
#include <KSyntaxHighlighting/Format>
#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/SyntaxHighlighter>

#include <QLoggingCategory>
#include <QMetaEnum>

using namespace Utils;

namespace TextEditor::HighlighterHelper {

const char kDefinitionForMimeType[] = "definitionForMimeType";
const char kDefinitionForExtension[] = "definitionForExtension";
const char kDefinitionForFilePath[] = "definitionForFilePath";

static KSyntaxHighlighting::Repository *highlightRepository()
{
    static KSyntaxHighlighting::Repository *repository = nullptr;
    if (!repository) {
        repository = new KSyntaxHighlighting::Repository();
        repository->addCustomSearchPath(
            TextEditorSettings::highlighterSettings().definitionFilesPath().toString());
        const FilePath dir = Core::ICore::resourcePath("generic-highlighter/syntax");
        if (dir.exists())
            repository->addCustomSearchPath(dir.parentDir().path());
    }
    return repository;
}

Definition definitionForName(const QString &name)
{
    return highlightRepository()->definitionForName(name);
}

Definitions definitionsForDocument(const TextEditor::TextDocument *document)
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

static Definition definitionForSetting(const Key &settingsKey, const QString &mapKey)
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    const QString &definitionName = settings->value(settingsKey).toMap().value(mapKey).toString();
    settings->endGroup();
    return HighlighterHelper::definitionForName(definitionName);
}

Definitions definitionsForMimeType(const QString &mimeType)
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

Definitions definitionsForFileName(const FilePath &fileName)
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

void rememberDefinitionForDocument(const Definition &definition,
                                   const TextEditor::TextDocument *document)
{
    QTC_ASSERT(document, return);
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

void clearDefinitionForDocumentCache()
{
    QtcSettings *settings = Core::ICore::settings();
    settings->beginGroup(Constants::HIGHLIGHTER_SETTINGS_CATEGORY);
    settings->remove(kDefinitionForMimeType);
    settings->remove(kDefinitionForExtension);
    settings->remove(kDefinitionForFilePath);
    settings->endGroup();
}

void addCustomHighlighterPath(const FilePath &path)
{
    highlightRepository()->addCustomSearchPath(path.toString());
}

void downloadDefinitions(std::function<void()> callback)
{
    auto downloader = new KSyntaxHighlighting::DefinitionDownloader(highlightRepository());
    QObject::connect(downloader,
                     &KSyntaxHighlighting::DefinitionDownloader::done,
                     [downloader, callback]() {
                         Core::MessageManager::writeFlashing(Tr::tr("Highlighter updates: done"));
                         downloader->deleteLater();
                         reload();
                         if (callback)
                             callback();
                     });
    QObject::connect(downloader,
                     &KSyntaxHighlighting::DefinitionDownloader::informationMessage,
                     [](const QString &message) {
                         Core::MessageManager::writeSilently(Tr::tr("Highlighter updates:") + ' '
                                                             + message);
                     });
    Core::MessageManager::writeDisrupting(Tr::tr("Highlighter updates: starting"));
    downloader->start();
}

void reload()
{
    highlightRepository()->reload();
    for (auto editor : Core::DocumentModel::editorsForOpenedDocuments()) {
        if (auto textEditor = qobject_cast<BaseTextEditor *>(editor)) {
            if (auto highlighter = textEditor->textDocument()->syntaxHighlighter();
                highlighter && qobject_cast<SyntaxHighlighter*>(highlighter)) {
                textEditor->editorWidget()->configureGenericHighlighter();
            }
        }
    }
}

void handleShutdown()
{
    delete highlightRepository();
}

QFuture<QTextDocument *> highlightCode(const QString &code, const QString &mimeType)
{
    QTextDocument *document = new QTextDocument;
    document->setPlainText(code);

    const HighlighterHelper::Definitions definitions = HighlighterHelper::definitionsForMimeType(
        mimeType);

    std::shared_ptr<QPromise<QTextDocument *>> promise
        = std::make_shared<QPromise<QTextDocument *>>();

    promise->start();

    if (definitions.isEmpty()) {
        promise->addResult(document);
        promise->finish();
        return promise->future();
    }

    auto definition = definitions.first();

    const QString definitionFilesPath
        = TextEditorSettings::highlighterSettings().definitionFilesPath().toString();

    Highlighter *highlighter = new Highlighter(definitionFilesPath);
    QObject::connect(highlighter, &Highlighter::finished, document, [document, promise]() {
        promise->addResult(document);
        promise->finish();
    });

    QFutureWatcher<QTextDocument *> *watcher = new QFutureWatcher<QTextDocument *>(document);
    QObject::connect(watcher, &QFutureWatcher<QTextDocument *>::canceled, document, [document]() {
        document->deleteLater();
    });
    watcher->setFuture(promise->future());

    highlighter->setDefinition(definition);
    highlighter->setParent(document);
    highlighter->setFontSettings(TextEditorSettings::fontSettings());
    highlighter->setMimeType(mimeType);
    highlighter->setDocument(document);

    return promise->future();
}

} // namespace TextEditor::HighlighterHelper
