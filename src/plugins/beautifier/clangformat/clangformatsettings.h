// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../abstractsettings.h"

namespace Beautifier::Internal {

class ClangFormatSettings : public AbstractSettings
{
public:
    explicit ClangFormatSettings();

    void createDocumentationFile() const override;

    QStringList completerWords() override;

    Utils::BoolAspect usePredefinedStyle;
    Utils::SelectionAspect predefinedStyle;
    Utils::SelectionAspect fallbackStyle;
    Utils::StringAspect customStyle;

    QString styleFileName(const QString &key) const override;

private:
    void readStyles() override;
};

class ClangFormatOptionsPage final : public Core::IOptionsPage
{
public:
    explicit ClangFormatOptionsPage(ClangFormatSettings *settings);
};

} // Beautifier::Internal
