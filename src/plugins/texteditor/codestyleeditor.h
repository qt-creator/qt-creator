// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "icodestylepreferencesfactory.h"

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace ProjectExplorer { class Project; }
namespace TextEditor {

class ICodeStylePreferencesFactory;
class ICodeStylePreferences;
class SnippetEditorWidget;

class TEXTEDITOR_EXPORT CodeStyleEditor : public CodeStyleEditorWidget
{
    Q_OBJECT
public:
    CodeStyleEditor(ICodeStylePreferencesFactory *factory,
                    ICodeStylePreferences *codeStyle,
                    ProjectExplorer::Project *project = nullptr,
                    QWidget *parent = nullptr);

    void apply() override;
    void finish() override;
private:
    void updatePreview();

    QVBoxLayout *m_layout;
    ICodeStylePreferencesFactory *m_factory;
    ICodeStylePreferences *m_codeStyle;
    SnippetEditorWidget *m_preview;
    CodeStyleEditorWidget *m_additionalGlobalSettingsWidget;
    CodeStyleEditorWidget *m_widget;
};

} // namespace TextEditor
