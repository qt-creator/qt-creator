/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
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
    , m_overlayFilePath(Utils::FilePath::fromString(m_root.filePath("vfso.yaml")))
{ }

void VirtualFileSystemOverlay::update()
{
    Utils::FileUtils::removeRecursively(overlayFilePath());
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
            if (saved.path.exists())
                Utils::FileUtils::removeRecursively(saved.path);
            saved.revision = document->document()->revision();
            QString error;
            saved.path = Utils::FilePath::fromString(m_root.path())
                    .pathAppended(doc->filePath().fileName() + ".auto");
            while (saved.path.exists())
                saved.path = saved.path + ".1";
            if (!doc->save(&error, saved.path.toString(), true)) {
                qCDebug(LOG) << error;
                continue;
            }
        }
        newSaved[doc] = saved;
    }

    for (const AutoSavedPath &path : qAsConst(m_saved)) {
        QString error;
        if (!Utils::FileUtils::removeRecursively(path.path, &error))
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
        for (auto doc : qAsConst(documents))
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

Utils::FilePath VirtualFileSystemOverlay::overlayFilePath() { return m_overlayFilePath; }

Utils::FilePath VirtualFileSystemOverlay::autoSavedFilePath(Core::IDocument *doc)
{
    auto it = m_saved.find(doc);
    if (it != m_saved.end())
        return it.value().path;
    return doc->filePath();
}

Utils::FilePath VirtualFileSystemOverlay::originalFilePath(const Utils::FilePath &file)
{
    return m_mapping.value(file, file);
}

} // namespace Internal
} // namespace ClangTools
