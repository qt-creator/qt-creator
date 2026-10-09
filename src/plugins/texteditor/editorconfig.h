// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>
#include <utils/textcodec.h>
#include <utils/textfileformat.h>

#include <QMap>

#include <optional>

namespace Utils { class FilePath; }

namespace TextEditor {

class ExtraEncodingSettingsData;
class MarginSettingsData;
class StorageSettingsData;
class TabSettingsData;

class TEXTEDITOR_EXPORT EditorConfigProperties
{
public:
    static EditorConfigProperties forFile(const Utils::FilePath &filePath);
    static EditorConfigProperties fromProperties(const QMap<QString, QString> &properties);

    bool isEmpty() const;

    void applyTo(TabSettingsData &tabSettings) const;
    void applyTo(StorageSettingsData &storageSettings) const;
    void applyTo(ExtraEncodingSettingsData &extraEncodingSettings) const;
    void applyTo(MarginSettingsData &marginSettings) const;

    std::optional<bool> indentWithTabs;
    std::optional<int> indentSize;
    bool indentSizeIsTabWidth = false;
    std::optional<int> tabWidth;
    std::optional<Utils::TextFileFormat::LineTerminationMode> lineTerminationMode;
    std::optional<Utils::TextEncoding> encoding;
    std::optional<bool> utf8Bom;
    std::optional<bool> trimTrailingWhitespace;
    std::optional<bool> insertFinalNewline;
    std::optional<int> maxLineLength;
};

class TEXTEDITOR_EXPORT EditorConfigSettings final : public Utils::AspectContainer
{
public:
    EditorConfigSettings();

    void apply() final;

    Utils::BoolAspect enabled{this};
};

TEXTEDITOR_EXPORT EditorConfigSettings &globalEditorConfigSettings();

namespace Internal {

const int kMaxCachedEditorConfigDirectories = 1000;

int editorConfigCacheSize();
bool editorConfigGlobMatches(const QString &glob, const QString &relativeFilePath);
QMap<QString, QString> editorConfigPropertiesForFile(const Utils::FilePath &filePath);

} // namespace Internal

} // namespace TextEditor
