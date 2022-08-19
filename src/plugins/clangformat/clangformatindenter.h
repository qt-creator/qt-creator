// Copyright (C) 2019 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0+ OR GPL-3.0 WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatbaseindenter.h"

#include <texteditor/tabsettings.h>

namespace ClangFormat {

class ClangFormatIndenter final : public ClangFormatBaseIndenter
{
public:
    ClangFormatIndenter(QTextDocument *doc);
    Utils::optional<TextEditor::TabSettings> tabSettings() const override;
    bool formatOnSave() const override;

private:
    bool formatCodeInsteadOfIndent() const override;
    bool formatWhileTyping() const override;
    int lastSaveRevision() const override;
};

} // namespace ClangFormat
