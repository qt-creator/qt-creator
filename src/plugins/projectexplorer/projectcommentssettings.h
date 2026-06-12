// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <texteditor/commentssettings.h>

namespace Utils { class FilePath; }

namespace ProjectExplorer {

PROJECTEXPLORER_EXPORT TextEditor::CommentsSettings::Data commentsSettingsForFile(
    const Utils::FilePath &filePath);

namespace Internal { void setupCommentsSettingsProjectPanel(); }

} // namespace ProjectExplorer
