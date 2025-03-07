// Copyright (C) 2025 Andre Hartmann <aha_1980@gmx.de>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>
#include <utils/temporaryfile.h>

class TemporaryPatchFile
{
public:
    TemporaryPatchFile(const QString &patch);

    Utils::FilePath filePath() const;

private:
    std::unique_ptr<Utils::TemporaryFile> patchFile;
};
