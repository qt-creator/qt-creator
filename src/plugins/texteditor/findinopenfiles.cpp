// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "findinopenfiles.h"

#include "basefilefind.h"
#include "textdocument.h"
#include "texteditortr.h"

#include <coreplugin/editormanager/editormanager.h>
#include <coreplugin/editormanager/documentmodel.h>

#include <utils/qtcsettings.h>

#include <QTextCodec>

using namespace Utils;

namespace TextEditor::Internal {

class FindInOpenFiles : public BaseFileFind
{
public:
    FindInOpenFiles();

private:
    QString id() const final;
    QString displayName() const final;
    bool isEnabled() const final;
    Utils::Store save() const final;
    void restore(const Utils::Store &s) final;

    QString label() const final;
    QString toolTip() const final;

    FileContainerProvider fileContainerProvider() const final;
    void updateEnabledState() { emit enabledChanged(isEnabled()); }

    // deprecated
    QByteArray settingsKey() const final;
};

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
        const QMap<FilePath, QByteArray> encodings = TextDocument::openedTextDocumentEncodings();
        FilePaths fileNames;
        QList<QTextCodec *> codecs;
        const QList<Core::DocumentModel::Entry *> entries = Core::DocumentModel::entries();
        for (Core::DocumentModel::Entry *entry : entries) {
            const FilePath fileName = entry->filePath();
            if (!fileName.isEmpty()) {
                fileNames.append(fileName);
                QByteArray codec = encodings.value(fileName);
                if (codec.isEmpty())
                    codec = Core::EditorManager::defaultTextCodecName();
                codecs.append(QTextCodec::codecForName(codec));
            }
        }
        return FileListContainer(fileNames, codecs);
    };
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

const char kDefaultInclusion[] = "*";
const char kDefaultExclusion[] = "";

Store FindInOpenFiles::save() const
{
    Store s;
    writeCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
    return s;
}

void FindInOpenFiles::restore(const Store &s)
{
    readCommonSettings(s, kDefaultInclusion, kDefaultExclusion);
}

QByteArray FindInOpenFiles::settingsKey() const
{
    return "FindInOpenFiles";
}

void setupFindInOpenFiles()
{
    static FindInOpenFiles theFindInOpenFiles;
}

} // TextEditor::Internal
