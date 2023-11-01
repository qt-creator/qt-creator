// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <cppeditor/cppcodestylesettingspage.h>

#include <utils/guard.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QLabel;
class QSpinBox;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }
namespace TextEditor { class ICodeStylePreferences; }

namespace ClangFormat {

class ClangFormatGlobalConfigWidget : public CppEditor::CppCodeStyleWidget
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
    void initOverrideCheckBox();
    void initUseGlobalSettingsCheckBox();
    void initFileSizeThresholdSpinBox();

    bool projectClangFormatFileExists();

    ProjectExplorer::Project *m_project;
    TextEditor::ICodeStylePreferences *m_codeStyle;
    Utils::Guard m_ignoreChanges;
    bool m_overrideDefaultFile;

    QLabel *m_projectHasClangFormat;
    QLabel *m_formattingModeLabel;
    QLabel *m_fileSizeThresholdLabel;
    QSpinBox *m_fileSizeThresholdSpinBox;
    QComboBox *m_indentingOrFormatting;
    QCheckBox *m_formatWhileTyping;
    QCheckBox *m_formatOnSave;
    QCheckBox *m_overrideDefault;
    QCheckBox *m_useGlobalSettings;
};

} // namespace ClangFormat
