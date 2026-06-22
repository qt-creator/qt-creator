// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace TextEditor {
class CodeStyleSelectorWidget;
class ICodeStylePreferences;
class Indenter;
class SnippetEditorWidget;

class TEXTEDITOR_EXPORT CodeStyleEditor : public QWidget
{
public:
    CodeStyleEditor();

    virtual void apply();
    virtual void cancel();

protected:
    void addHeaderWidget(QWidget *widget);
    void addSelector(CodeStyleSelectorWidget *selector);
    QWidget *addInfoLabel();
    void addEditorWidget(QWidget *editor);
    // Adds a vertically expanding empty widget at the end of the layout. Toggle
    // its visibility opposite to the editor area so that the remaining widgets
    // stay packed at the top when the editor area is hidden.
    QWidget *addExpandingFiller();
    // Returns the trailing explanatory label, so callers that hide the preview
    // can hide it too.
    QWidget *setupPreview(SnippetEditorWidget *preview,
                          Indenter *indenter,
                          const Utils::FilePath &projectFile,
                          ICodeStylePreferences *codeStyle);

private:
    QVBoxLayout *m_layout = nullptr;
};

} // namespace TextEditor
