// Copyright (C) 2016 Marc Reilly <marc.reilly+qtc@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <projectexplorer/projectexplorer_export.h>

namespace TextEditor { class BaseHoverHandler; }

namespace ProjectExplorer {

PROJECTEXPLORER_EXPORT TextEditor::BaseHoverHandler &resourcePreviewHoverHandler();

} // namespace ProjectExplorer
