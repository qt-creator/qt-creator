// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <utils/filepath.h>

#include <clang/Format/Format.h>

namespace CppEditor { class CppCodeStyleSettings; }
namespace ProjectExplorer { class Project; }
namespace TextEditor {
class ICodeStylePreferences;
class TabSettings;
}

namespace ClangFormat {

class ClangFormatFile
{
public:
    explicit ClangFormatFile(
        const TextEditor::ICodeStylePreferences *preferences, const Utils::FilePath &filePath = {});
    clang::format::FormatStyle style();

    Utils::FilePath filePath();
    void resetStyleToQtC(const TextEditor::ICodeStylePreferences *codeStyle);

    bool isReadOnly() const;
    void setIsReadOnly(bool isReadOnly);

    static void removeClangFormatFileForStylePreferences(
        const TextEditor::ICodeStylePreferences *preferences);
    static void copyClangFormatFileBasedOnStylePreferences(
        const TextEditor::ICodeStylePreferences *current,
        const TextEditor::ICodeStylePreferences *target);

private:
    void saveStyleToFile(clang::format::FormatStyle style, Utils::FilePath filePath);

private:
    Utils::FilePath m_filePath;
    clang::format::FormatStyle m_style;
    bool m_isReadOnly;
};

} // namespace ClangFormat
