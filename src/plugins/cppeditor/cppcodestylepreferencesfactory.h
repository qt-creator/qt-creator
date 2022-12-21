// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <texteditor/icodestylepreferencesfactory.h>

namespace CppEditor {
class CppCodeStyleWidget;

class CPPEDITOR_EXPORT CppCodeStylePreferencesFactory : public TextEditor::ICodeStylePreferencesFactory
{
public:
    CppCodeStylePreferencesFactory();

    Utils::Id languageId() override;
    QString displayName() override;
    TextEditor::ICodeStylePreferences *createCodeStyle() const override;
    TextEditor::CodeStyleEditorWidget *createEditor(TextEditor::ICodeStylePreferences *settings,
                                                    ProjectExplorer::Project *project,
                                                    QWidget *parent) const override;
    TextEditor::Indenter *createIndenter(QTextDocument *doc) const override;
    QString snippetProviderGroupId() const override;
    QString previewText() const override;
    virtual std::pair<CppCodeStyleWidget *, QString> additionalTab(
        TextEditor::ICodeStylePreferences *codeStyle,
        ProjectExplorer::Project *project,
        QWidget *parent) const;
};

} // namespace CppEditor
