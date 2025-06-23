// Copyright (C) 2025 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "temporarypatchfile.h"

#include <coreplugin/editormanager/editormanager.h>

#include <utils/textcodec.h>

using namespace Utils;

namespace Git::Internal {

TemporaryPatchFile::TemporaryPatchFile(const QString &patch)
    : patchFile(new TemporaryFile("git-patchfile"))
{
    if (!patchFile->open())
        return;

    QString normalized = patch;
    normalized.replace("\r\n", "\n").replace('\r', '\n');
    const TextEncoding encoding = Core::EditorManager::defaultTextEncoding();
    const QByteArray patchData = encoding.isValid() ? encoding.encode(normalized) : normalized.toLocal8Bit();
    patchFile->write(patchData);
    patchFile->close();
}

FilePath TemporaryPatchFile::filePath() const
{
    return FilePath::fromString(patchFile->fileName());
}

} // Git::Internal
