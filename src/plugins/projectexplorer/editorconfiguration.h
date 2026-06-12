// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"
#include "useglobalaspect.h"

#include <texteditor/behaviorsettings.h>
#include <texteditor/extraencodingsettings.h>
#include <texteditor/marginsettings.h>
#include <texteditor/storagesettings.h>
#include <texteditor/typingsettings.h>

#include <memory>

namespace TextEditor {
class TextEditorWidget;
class TextDocument;
class TabSettingsData;
class ICodeStylePreferences;
} // namespace TextEditor

namespace Core { class IEditor; }

namespace Utils {
class FilePath;
class TextEncoding;
};

namespace ProjectExplorer {

class Project;
struct EditorConfigurationPrivate;

class PROJECTEXPLORER_EXPORT EditorConfiguration final : public Utils::AspectContainer
{
public:
    EditorConfiguration();
    ~EditorConfiguration() final;

    void cloneGlobalSettings();

    // The default codec is returned in the case the project doesn't override it.
    Utils::TextEncoding textEncoding() const;

    UseGlobalAspect useGlobalSettings; // not {this}: excluded from container
    TextEditor::StorageSettings storageSettings;
    TextEditor::BehaviorSettings behaviorSettings;
    TextEditor::ExtraEncodingSettings extraEncodingSettings;
    TextEditor::MarginSettings marginSettings;
    TextEditor::TypingSettings typingSettings;

    TextEditor::ICodeStylePreferences *codeStyle() const;
    TextEditor::ICodeStylePreferences *codeStyle(Utils::Id languageId) const;

    void configureEditor(Core::IEditor *editor) const;

    void toMap(Utils::Store &map) const final;
    void fromMap(const Utils::Store &map) final;

    void setTextEncoding(const Utils::TextEncoding &textEncoding);

private:
    void slotAboutToRemoveProject(ProjectExplorer::Project *project);
    void switchSettings(TextEditor::TextEditorWidget *baseTextEditor) const;

    const std::unique_ptr<EditorConfigurationPrivate> d;
};

// Return the editor settings in the case it's not null. Otherwise, try to find the project
// the file belongs to and return the project settings. If the file doesn't belong to any
// project return the global settings.
PROJECTEXPLORER_EXPORT TextEditor::TabSettingsData actualTabSettings(
    const Utils::FilePath &file, const TextEditor::TextDocument *baseTextDocument);

} // namespace ProjectExplorer
