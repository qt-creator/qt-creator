// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "clangformatfile.h"
#include "clangformatindenter.h"

#include <coreplugin/editormanager/ieditor.h>
#include <texteditor/codestyleeditor.h>
#include <utils/filepath.h>
#include <utils/guard.h>

#include <memory>

QT_BEGIN_NAMESPACE
class QLabel;
class QScrollArea;
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {
class ICodeStylePreferences;
class TextEditorWidget;
} // namespace TextEditor

namespace Utils {
class InfoLabel;
}

namespace ProjectExplorer { class Project; }

namespace ClangFormat {
class ClangFormatConfigWidget final : public TextEditor::CodeStyleEditorWidget
{
public:
    ClangFormatConfigWidget(
        const ProjectExplorer::Project *project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent);
    ~ClangFormatConfigWidget() override;

    void apply() override;

    void onUseCustomSettingsChanged(bool doUse);

private:
    bool eventFilter(QObject *object, QEvent *event) override;

    Utils::FilePath globalPath();
    Utils::FilePath projectPath();
    void createStyleFileIfNeeded(bool isGlobal);
    void initPreview(TextEditor::ICodeStylePreferences *codeStyle);
    void initEditor();
    void reopenClangFormatDocument();
    void updatePreview();
    void slotCodeStyleChanged(TextEditor::ICodeStylePreferences *currentPreferences);
    void updateReadOnlyState();
    TextEditor::TextEditorWidget *editorWidget() const;

    const ProjectExplorer::Project *m_project = nullptr;
    QScrollArea *m_editorScrollArea = nullptr;
    TextEditor::SnippetEditorWidget * const m_preview;
    std::unique_ptr<Core::IEditor> m_editor;
    std::unique_ptr<ClangFormatFile> m_config;
    Utils::Guard m_ignoreChanges;
    QLabel *m_clangVersion;
    Utils::InfoLabel *m_clangFileIsCorrectText;
    ClangFormatIndenter *m_indenter;

    bool m_useCustomSettings = false;
};
} // ClangFormat
