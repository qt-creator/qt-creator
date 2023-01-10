// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "virtualfilesystemoverlay.h"

#include <coreplugin/documentmanager.h>
#include <coreplugin/editormanager/documentmodel.h>
#include <texteditor/textdocument.h>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLoggingCategory>
#include <QTextDocument>

static Q_LOGGING_CATEGORY(LOG, "qtc.clangtools.vfso", QtWarningMsg)

namespace ClangTools {
namespace Internal {

VirtualFileSystemOverlay::VirtualFileSystemOverlay(const QString &rootPattern)
    : m_root(rootPattern)
    , m_overlayFilePath(m_root.filePath("vfso.yaml"))
{ }

void VirtualFileSystemOverlay::update()
{
    overlayFilePath().removeRecursively();
    QFile overlayFile(m_overlayFilePath.toString());
    if (!overlayFile.open(QFile::ReadWrite))
        return;
    std::map<Utils::FilePath, QList<Core::IDocument *>> documentRoots;
    const QList<Core::IDocument *> &modifiedDocuments = Core::DocumentManager::modifiedDocuments();
    QHash<Core::IDocument *, AutoSavedPath> newSaved;
    for (Core::IDocument *doc : modifiedDocuments) {
        auto document = qobject_cast<TextEditor::TextDocument *>(doc);
        if (!document)
            continue;
        documentRoots[doc->filePath().absolutePath()] << doc;
        AutoSavedPath saved = m_saved.take(document);
        if (saved.revision != document->document()->revision()) {
            saved.path.removeRecursively();
            saved.revision = document->document()->revision();
            QString error;
            saved.path = m_root.filePath(doc->filePath().fileName() + ".auto");
            while (saved.path.exists())
                saved.path = saved.path.stringAppended(".1");
            if (!doc->save(&error, saved.path, true)) {
                qCDebug(LOG) << error;
                continue;
            }
        }
        newSaved[doc] = saved;
    }

    for (const AutoSavedPath &path : std::as_const(m_saved)) {
        QString error;
        if (!path.path.removeRecursively(&error))
            qCDebug(LOG) << error;
    }
    m_saved = newSaved;
    m_mapping.clear();
    for (auto it = m_saved.constBegin(), end = m_saved.constEnd(); it != end; ++it)
        m_mapping[it.value().path] = it.key()->filePath();

    auto toContent = [this](Core::IDocument *document) {
        QJsonObject content;
        content["name"] = document->filePath().fileName();
        content["type"] = "file";
        content["external-contents"] = m_saved[document].path.toUserOutput();
        return content;
    };

    QJsonObject main;
    main["version"] = 0;
    QJsonArray jsonRoots;
    for (auto [root, documents] : documentRoots) {
        QJsonObject jsonRoot;
        jsonRoot["type"] = "directory";
        jsonRoot["name"] = root.toUserOutput();
        QJsonArray contents;
        for (auto doc : std::as_const(documents))
            contents << toContent(doc);
        jsonRoot["contents"] = contents;
        jsonRoots << jsonRoot;
    }
    main["roots"] = jsonRoots;

    QJsonDocument overlay(main);
    if (!overlayFile.write(overlay.toJson(QJsonDocument::Compact)))
        qCDebug(LOG) << "failed to write vfso to " << m_overlayFilePath;
    overlayFile.close();
}

Utils::FilePath VirtualFileSystemOverlay::overlayFilePath() const { return m_overlayFilePath; }

Utils::FilePath VirtualFileSystemOverlay::autoSavedFilePath(Core::IDocument *doc) const
{
    const auto it = m_saved.constFind(doc);
    if (it != m_saved.constEnd())
        return it.value().path;
    return doc->filePath();
}

Utils::FilePath VirtualFileSystemOverlay::originalFilePath(const Utils::FilePath &file) const
{
    return m_mapping.value(file, file);
}

} // namespace Internal
} // namespace ClangTools
