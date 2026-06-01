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

protected:
    void addSelector(CodeStyleSelectorWidget *selector);
    void addInfoLabel();
    void addEditorWidget(CodeStyleWidget *editor);
    void setupPreview(const ICodeStylePreferencesFactory *factory,
                      const Utils::FilePath &projectFile,
                      ICodeStylePreferences *codeStyle,
                      const QString &previewText,
                      const QString &snippetGroupId);

    QVBoxLayout *m_layout = nullptr;
    CodeStyleSelectorWidget *m_selector = nullptr;
    SnippetEditorWidget *m_preview = nullptr;
    CodeStyleWidget *m_editor = nullptr;
};

} // namespace TextEditor
