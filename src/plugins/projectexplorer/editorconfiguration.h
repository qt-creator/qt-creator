// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/typingsettings.h>

#include <memory>

namespace TextEditor {
class BaseTextEditor;
class TextEditorWidget;
class TextDocument;
class TabSettings;
class ICodeStylePreferences;
class TypingSettingsData;
} // namespace TextEditor

namespace Core { class IEditor; }

namespace Utils {
class FilePath;
class TextEncoding;
};

namespace ProjectExplorer {

class Project;
struct EditorConfigurationPrivate;

class PROJECTEXPLORER_EXPORT EditorConfiguration : public QObject
{
    Q_OBJECT

public:
    EditorConfiguration();
    ~EditorConfiguration() override;

    void setUseGlobalSettings(bool use);
    bool useGlobalSettings() const;
    void cloneGlobalSettings();

    // The default codec is returned in the case the project doesn't override it.
    Utils::TextEncoding textEncoding() const;

    TextEditor::StorageSettings storageSettings;
    TextEditor::BehaviorSettings behaviorSettings;
    TextEditor::ExtraEncodingSettings extraEncodingSettings;
    TextEditor::MarginSettings marginSettings;
    TextEditor::TypingSettings typingSettings;

    TextEditor::ICodeStylePreferences *codeStyle() const;
    TextEditor::ICodeStylePreferences *codeStyle(Utils::Id languageId) const;
    QMap<Utils::Id, TextEditor::ICodeStylePreferences *> codeStyles() const;

    void configureEditor(Core::IEditor *editor) const;
    void deconfigureEditor(Core::IEditor *editor) const;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    void setTextEncoding(const Utils::TextEncoding &textEncoding);

    void slotAboutToRemoveProject(ProjectExplorer::Project *project);

signals:
    void typingSettingsChanged(const TextEditor::TypingSettingsData &);
    void storageSettingsChanged(const TextEditor::StorageSettingsData &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettingsData &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettingsData &);

private:
    void switchSettings(TextEditor::TextEditorWidget *baseTextEditor) const;

    const std::unique_ptr<EditorConfigurationPrivate> d;
};

// Return the editor settings in the case it's not null. Otherwise, try to find the project
// the file belongs to and return the project settings. If the file doesn't belong to any
// project return the global settings.
PROJECTEXPLORER_EXPORT TextEditor::TabSettings actualTabSettings(
    const Utils::FilePath &file, const TextEditor::TextDocument *baseTextDocument);

} // namespace ProjectExplorer
