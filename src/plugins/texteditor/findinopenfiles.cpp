// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinopenfiles.h"

#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>
#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <utils/filesearch.h>

#include <QSettings>

using namespace Utils;

namespace TextEditor::Internal {

FindInOpenFiles::FindInOpenFiles()
{
    connect(Core::EditorManager::instance(), &Core::EditorManager::editorOpened,
            this, &FindInOpenFiles::updateEnabledState);
    connect(Core::EditorManager::instance(), &Core::EditorManager::editorsClosed,
            this, &FindInOpenFiles::updateEnabledState);
}

QString FindInOpenFiles::id() const
{
    return QLatin1String("Open Files");
}

QString FindInOpenFiles::displayName() const
{
    return Tr::tr("Open Documents");
}

FileContainerProvider FindInOpenFiles::fileContainerProvider() const
{
    return [] {
        const QMap<FilePath, QTextCodec *> encodings = TextDocument::openedTextDocumentEncodings();
        FilePaths fileNames;
        QList<QTextCodec *> codecs;
        const QList<Core::DocumentModel::Entry *> entries = Core::DocumentModel::entries();
        for (Core::DocumentModel::Entry *entry : entries) {
            const FilePath fileName = entry->filePath();
            if (!fileName.isEmpty()) {
                fileNames.append(fileName);
                QTextCodec *codec = encodings.value(fileName);
                if (!codec)
                    codec = Core::EditorManager::defaultTextCodec();
                codecs.append(codec);
            }
        }
        return FileListContainer(fileNames, codecs);
    };
}

QVariant FindInOpenFiles::additionalParameters() const
{
    return {};
}

QString FindInOpenFiles::label() const
{
    return Tr::tr("Open documents:");
}

QString FindInOpenFiles::toolTip() const
{
    // %1 is filled by BaseFileFind::runNewSearch
    return Tr::tr("Open Documents\n%1");
}

bool FindInOpenFiles::isEnabled() const
{
    return Core::DocumentModel::entryCount() > 0;
}

void FindInOpenFiles::writeSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInOpenFiles"));
    writeCommonSettings(settings);
    settings->endGroup();
}

void FindInOpenFiles::readSettings(QSettings *settings)
{
    settings->beginGroup(QLatin1String("FindInOpenFiles"));
    readCommonSettings(settings, "*", "");
    settings->endGroup();
}

void FindInOpenFiles::updateEnabledState()
{
    emit enabledChanged(isEnabled());
}

} // TextEditor::Internal
