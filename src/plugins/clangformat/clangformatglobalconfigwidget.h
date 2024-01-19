// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppcodestylesettingspage.h>

#include <utils/guard.h>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }
namespace TextEditor {
class ICodeStylePreferences;
class CodeStyleEditorWidget;
} // namespace TextEditor

namespace ClangFormat {

class ClangFormatGlobalConfigWidget : public TextEditor::CodeStyleEditorWidget
{
    Q_OBJECT

public:
    explicit ClangFormatGlobalConfigWidget(TextEditor::ICodeStylePreferences *codeStyle,
                                           ProjectExplorer::Project *project = nullptr,
                                           QWidget *parent = nullptr);
    ~ClangFormatGlobalConfigWidget() override;
    void apply() override;
    void finish() override;

private:
    void initCheckBoxes();
    void initIndentationOrFormattingCombobox();
    void initCustomSettingsCheckBox();
    void initUseGlobalSettingsCheckBox();
    void initFileSizeThresholdSpinBox();
    void initCurrentProjectLabel();

    bool projectClangFormatFileExists();

    ProjectExplorer::Project *m_project;
    TextEditor::ICodeStylePreferences *m_codeStyle;
    Utils::Guard m_ignoreChanges;
    bool m_useCustomSettings;

    QLabel *m_projectHasClangFormat;
    QLabel *m_formattingModeLabel;
    QLabel *m_fileSizeThresholdLabel;
    QSpinBox *m_fileSizeThresholdSpinBox;
    QComboBox *m_indentingOrFormatting;
    QCheckBox *m_formatWhileTyping;
    QCheckBox *m_formatOnSave;
    QCheckBox *m_useCustomSettingsCheckBox;
    QCheckBox *m_useGlobalSettings;
    QLabel *m_currentProjectLabel;
};

void setupClangFormatStyleFactory(QObject *guard);

} // namespace ClangFormat
