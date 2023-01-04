// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QString>

namespace Utils { class FilePath; }

namespace Squish {
namespace Internal {

enum class Language;

class ScriptHelper
{
public:
    explicit ScriptHelper(Language lang);
    ~ScriptHelper() = default;

    bool writeScriptFile(const Utils::FilePath &outScriptFile,
                         const Utils::FilePath &snippetFile,
                         const QString &application,
                         const QString &arguments) const;
private:
    Language m_language;
};

} // namespace Internal
} // namespace Squish
