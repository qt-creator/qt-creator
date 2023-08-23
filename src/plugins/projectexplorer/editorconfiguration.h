// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "projectexplorer_export.h"

#include <utils/id.h>
#include <utils/store.h>

#include <QObject>

#include <memory>

QT_BEGIN_NAMESPACE
class QTextCodec;
QT_END_NAMESPACE

namespace TextEditor {
class BaseTextEditor;
class TextEditorWidget;
class TextDocument;
class TabSettings;
class ICodeStylePreferences;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class ExtraEncodingSettings;
class MarginSettings;
} // namespace TextEditor

namespace Utils { class FilePath; }

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
    QTextCodec *textCodec() const;

    const TextEditor::TypingSettings &typingSettings() const;
    const TextEditor::StorageSettings &storageSettings() const;
    const TextEditor::BehaviorSettings &behaviorSettings() const;
    const TextEditor::ExtraEncodingSettings &extraEncodingSettings() const;
    const TextEditor::MarginSettings &marginSettings() const;

    TextEditor::ICodeStylePreferences *codeStyle() const;
    TextEditor::ICodeStylePreferences *codeStyle(Utils::Id languageId) const;
    QMap<Utils::Id, TextEditor::ICodeStylePreferences *> codeStyles() const;

    void configureEditor(TextEditor::BaseTextEditor *textEditor) const;
    void deconfigureEditor(TextEditor::BaseTextEditor *textEditor) const;

    Utils::Store toMap() const;
    void fromMap(const Utils::Store &map);

    void setTypingSettings(const TextEditor::TypingSettings &settings);
    void setStorageSettings(const TextEditor::StorageSettings &settings);
    void setBehaviorSettings(const TextEditor::BehaviorSettings &settings);
    void setExtraEncodingSettings(const TextEditor::ExtraEncodingSettings &settings);
    void setMarginSettings(const TextEditor::MarginSettings &settings);

    void setShowWrapColumn(bool onoff);
    void setTintMarginArea(bool onoff);
    void setUseIndenter(bool onoff);
    void setWrapColumn(int column);

    void setTextCodec(QTextCodec *textCodec);

    void slotAboutToRemoveProject(ProjectExplorer::Project *project);

signals:
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);
    void marginSettingsChanged(const TextEditor::MarginSettings &);

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
