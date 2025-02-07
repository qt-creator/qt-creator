// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/icodestylepreferencesfactory.h>
#include <utils/id.h>

#include <QString>

class QTextDocument;
class QWidget;

namespace TextEditor {
class CodeStyleEditorWidget;
class Indenter;
} // namespace TextEditor

namespace QmlJSTools {
class QmlJSCodeStylePreferencesFactory final : public TextEditor::ICodeStylePreferencesFactory
{
public:
    TextEditor::CodeStyleEditorWidget *createCodeStyleEditor(
        const TextEditor::ProjectWrapper &project,
        TextEditor::ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const override;

private:
    Utils::Id languageId() override;
    QString displayName() override;
    TextEditor::ICodeStylePreferences *createCodeStyle() const override;
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override;
};
} // namespace QmlJSTools
