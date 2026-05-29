// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/aspects.h>

namespace TextEditor {

class HighlighterSettings final : public Utils::AspectContainer
{
public:
    HighlighterSettings();

    bool skipUpdateCheck(const QString &fileName) const;
    bool skipHighlighting(const QString &fileName) const;

    Utils::FilePathAspect definitionFilesPath{this};
    Utils::StringListAspect skipUpdateCheckForFilesPattern{this};
    Utils::StringListAspect skipFilesPattern{this};
};

HighlighterSettings &globalHighlighterSettings();

} // namespace TextEditor
