// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>
#include <utils/id.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace TextEditor {
class CodeStylePool;
class CodeStyleSelectorWidget;
class ICodeStylePreferences;
class Indenter;
class SnippetEditorWidget;

class TEXTEDITOR_EXPORT CodeStyleEditor : public QWidget
{
    Q_OBJECT

public:
    CodeStyleEditor();

    virtual void apply();
    virtual void cancel();
    virtual bool isDirty() const;

signals:
    void changed();

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

// Reusable settings-page container for editing an ICodeStylePreferences with
// deferred apply/cancel. A page-local copy of the style is its volatile state;
// the language's code style editor edits that copy, and apply() commits it to
// the real style. Lets a code style page be a plain setSettingsProvider().
class TEXTEDITOR_EXPORT CodeStyleAspect : public Utils::AspectContainer
{
public:
    CodeStyleAspect(ICodeStylePreferences *codeStyle, Utils::Id languageId);
    ~CodeStyleAspect() override;

    void apply() override;
    void cancel() override;
    bool isDirty() const override;

private:
    void syncFromReal();
    bool poolsDiffer() const;
    ICodeStylePreferences *addPageCopy(ICodeStylePreferences *realStyle);

    ICodeStylePreferences *m_codeStyle = nullptr;
    Utils::Id m_languageId;
    CodeStylePool *m_pagePool = nullptr;
    ICodeStylePreferences *m_pageCodeStyle = nullptr;
    QPointer<CodeStyleEditor> m_editor;
    bool m_syncing = false;
};

} // namespace TextEditor
