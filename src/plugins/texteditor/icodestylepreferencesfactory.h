// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"
#include <utils/id.h>

#include <QString>

QT_BEGIN_NAMESPACE
class QTextDocument;
class QWidget;
QT_END_NAMESPACE

namespace TextEditor {
class CodeStyleEditorWidget;
class ICodeStylePreferences;
class Indenter;
class ProjectWrapper;

class TEXTEDITOR_EXPORT ICodeStylePreferencesFactory
{
    ICodeStylePreferencesFactory(const ICodeStylePreferencesFactory &) = delete;
    ICodeStylePreferencesFactory(const ICodeStylePreferencesFactory &&) = delete;
    ICodeStylePreferencesFactory &operator=(const ICodeStylePreferencesFactory &) = delete;
    ICodeStylePreferencesFactory &operator=(const ICodeStylePreferencesFactory &&) = delete;

public:
    ICodeStylePreferencesFactory() = default;
    virtual ~ICodeStylePreferencesFactory() = default;

    virtual CodeStyleEditorWidget *createCodeStyleEditor(
        const ProjectWrapper &project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const
        = 0;
    virtual TextEditor::Indenter *createIndenter(QTextDocument *doc) const = 0;
    virtual Utils::Id languageId() = 0;
    virtual QString displayName() = 0;
    virtual ICodeStylePreferences *createCodeStyle() const = 0;
};

} // namespace TextEditor
