// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include "indenter.h"

#include <utils/id.h>

#include <QWidget>

namespace  ProjectExplorer { class Project; }

namespace TextEditor {

class ICodeStylePreferences;

class TEXTEDITOR_EXPORT CodeStyleEditorWidget : public QWidget
{
    Q_OBJECT
public:
    CodeStyleEditorWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {}
    virtual void apply() {}
    virtual void finish() {}
};

class TEXTEDITOR_EXPORT ICodeStylePreferencesFactory
{
    ICodeStylePreferencesFactory(const ICodeStylePreferencesFactory &) = delete;
    ICodeStylePreferencesFactory &operator=(const ICodeStylePreferencesFactory &) = delete;

public:
    ICodeStylePreferencesFactory();
    virtual ~ICodeStylePreferencesFactory() = default;

    virtual CodeStyleEditorWidget *createCodeStyleEditor(ICodeStylePreferences *codeStyle,
                                                         ProjectExplorer::Project *project = nullptr,
                                                         QWidget *parent = nullptr);
    virtual CodeStyleEditorWidget *createAdditionalGlobalSettings(ICodeStylePreferences *codeStyle,
                                                                  ProjectExplorer::Project *project
                                                                  = nullptr,
                                                                  QWidget *parent = nullptr);
    virtual Utils::Id languageId() = 0;
    virtual QString displayName() = 0;
    virtual ICodeStylePreferences *createCodeStyle() const = 0;
    virtual CodeStyleEditorWidget *createEditor(ICodeStylePreferences *preferences,
                                                ProjectExplorer::Project *project = nullptr,
                                                QWidget *parent = nullptr) const = 0;
    virtual TextEditor::Indenter *createIndenter(QTextDocument *doc) const = 0;
    virtual QString snippetProviderGroupId() const = 0;
    virtual QString previewText() const = 0;
};

} // namespace TextEditor
