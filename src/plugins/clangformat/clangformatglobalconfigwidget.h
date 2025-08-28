// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatsettings.h"

#include <texteditor/codestyleeditor.h>
#include <utils/guard.h>

class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;

namespace ProjectExplorer {
class Project;
}

namespace TextEditor {
class ICodeStylePreferences;
}

namespace Utils {
class InfoLabel;
}

namespace ClangFormat {
class ClangFormatGlobalConfigWidget final : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    ClangFormatGlobalConfigWidget(
        ProjectExplorer::Project *project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent);
    void apply() override;
    void finish() override;

    ClangFormatSettings::Mode mode() const;
    bool useCustomSettings() const;

signals:
    void modeChanged(ClangFormatSettings::Mode newMode);
    void useCustomSettingsChanged(bool doUse);

private:
    void initCheckBoxes();
    void initIndentationOrFormattingCombobox();
    void initCustomSettingsCheckBox();
    void initUseGlobalSettingsCheckBox();
    void initFileSizeThresholdSpinBox();
    void initCurrentProjectLabel();

    bool projectClangFormatFileExists();

    ProjectExplorer::Project *m_project = nullptr;
    TextEditor::ICodeStylePreferences *m_codeStyle = nullptr;
    Utils::Guard m_ignoreChanges;
    bool m_useCustomSettings = false;

    QLabel *m_projectHasClangFormat = nullptr;
    QLabel *m_formattingModeLabel = nullptr;
    QLabel *m_fileSizeThresholdLabel = nullptr;
    QSpinBox *m_fileSizeThresholdSpinBox = nullptr;
    QComboBox *m_indentingOrFormatting = nullptr;
    QCheckBox *m_formatWhileTyping = nullptr;
    QCheckBox *m_formatOnSave = nullptr;
    QCheckBox *m_useCustomSettingsCheckBox = nullptr;
    QCheckBox *m_useGlobalSettings = nullptr;
    Utils::InfoLabel *m_currentProjectLabel = nullptr;
};
} // namespace ClangFormat
