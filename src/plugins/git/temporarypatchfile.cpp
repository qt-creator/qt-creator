// Copyright (C) 2025 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "temporarypatchfile.h"

#include <coreplugin/editormanager/editormanager.h>

#include <QTextCodec>

using namespace Utils;

TemporaryPatchFile::TemporaryPatchFile(const QString &patch)
    : patchFile(new Utils::TemporaryFile("git-patchfile"))
{
    if (!patchFile->open())
        return;

    QString normalized = patch;
    normalized.replace("\r\n", "\n").replace('\r', '\n');
    QTextCodec *codec = Core::EditorManager::defaultTextCodec();
    const QByteArray patchData = codec ? codec->fromUnicode(normalized) : normalized.toLocal8Bit();
    patchFile->write(patchData);
    patchFile->close();
}

Utils::FilePath TemporaryPatchFile::filePath() const
{
    return Utils::FilePath::fromString(patchFile->fileName());
}
