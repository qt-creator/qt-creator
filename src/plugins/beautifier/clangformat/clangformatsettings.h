// Copyright (C) 2016 Lorenz Haas
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "../abstractsettings.h"

namespace Beautifier {
namespace Internal {

class ClangFormatSettings : public AbstractSettings
{
    Q_OBJECT

public:
    explicit ClangFormatSettings();

    QString documentationFilePath() const override;
    void createDocumentationFile() const override;
    QStringList completerWords() override;

    bool usePredefinedStyle() const;
    void setUsePredefinedStyle(bool usePredefinedStyle);

    QString predefinedStyle() const;
    void setPredefinedStyle(const QString &predefinedStyle);

    QString fallbackStyle() const;
    void setFallbackStyle(const QString &fallbackStyle);

    QString customStyle() const;
    void setCustomStyle(const QString &customStyle);

    QStringList predefinedStyles() const;
    QStringList fallbackStyles() const;

    QString styleFileName(const QString &key) const override;

private:
    void readStyles() override;
};

} // namespace Internal
} // namespace Beautifier
