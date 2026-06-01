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
    Q_OBJECT

public:
    CodeStyleEditor(QWidget *parent = nullptr);

    virtual void apply();
    virtual void finish();

protected:
    void addHeaderWidget(QWidget *widget);
    void addSelector(CodeStyleSelectorWidget *selector);
    void addInfoLabel();
    void addEditorWidget(QWidget *editor);
    void setupPreview(SnippetEditorWidget *preview,
                      Indenter *indenter,
                      const Utils::FilePath &projectFile,
                      ICodeStylePreferences *codeStyle);

private:
    QVBoxLayout *m_layout = nullptr;
};

} // namespace TextEditor
