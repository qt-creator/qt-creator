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
    explicit ClangFormatFile(const TextEditor::ICodeStylePreferences *preferences);
    clang::format::FormatStyle style();

    Utils::FilePath filePath();
    void resetStyleToQtC(const TextEditor::ICodeStylePreferences *codeStyle);
    void setBasedOnStyle(QString styleName);
    void setStyle(clang::format::FormatStyle style);
    QString setStyle(QString style);
    void clearBasedOnStyle();

    using Field = std::pair<QString, QString>;
    QString changeFields(QList<Field> fields);
    QString changeField(Field field);
    bool isReadOnly() const;
    void setIsReadOnly(bool isReadOnly);

private:
    void saveNewFormat();
    void saveNewFormat(QByteArray style);
    void saveStyleToFile(clang::format::FormatStyle style, Utils::FilePath filePath);

private:
    Utils::FilePath m_filePath;
    clang::format::FormatStyle m_style;
    bool m_isReadOnly;
};

} // namespace ClangFormat
