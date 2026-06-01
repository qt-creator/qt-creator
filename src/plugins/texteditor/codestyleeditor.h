// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/filepath.h>

#include <QString>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace TextEditor {
class CodeStyleSelectorWidget;
class ICodeStylePreferencesFactory;
class ICodeStylePreferences;
class SnippetEditorWidget;

class TEXTEDITOR_EXPORT CodeStyleWidget : public QWidget
{
public:
    CodeStyleWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {}
    virtual void apply() {}
    virtual void finish() {}
};

class TEXTEDITOR_EXPORT CodeStyleEditor : public QWidget
{
    Q_OBJECT

public:
    CodeStyleEditor(QWidget *parent = nullptr);

    virtual void apply();
    virtual void finish();

    virtual void init(
        const ICodeStylePreferencesFactory *factory,
        const Utils::FilePath &projectFile,
        ICodeStylePreferences *codeStyle);

protected:
    QVBoxLayout *m_layout = nullptr;
    CodeStyleSelectorWidget *m_selector = nullptr;
    SnippetEditorWidget *m_preview = nullptr;
    CodeStyleWidget *m_editor = nullptr;

private:
    virtual CodeStyleSelectorWidget *createCodeStyleSelectorWidget(
        ICodeStylePreferences *codeStyle,
        const Utils::FilePath &projectFile,
        QWidget *parent = nullptr) const;
    virtual SnippetEditorWidget *createPreviewWidget(
        const ICodeStylePreferencesFactory *factory,
        const Utils::FilePath &projectFile,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const;
    virtual CodeStyleWidget *createEditorWidget(
        const Utils::FilePath &projectFile,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const
        = 0;
    virtual QString previewText() const = 0;
    virtual QString snippetProviderGroupId() const = 0;
};

} // namespace TextEditor
