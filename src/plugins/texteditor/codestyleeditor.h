// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>
#include <utils/id.h>

#include <QPointer>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace TextEditor {
class CodeStylePool;
class ICodeStylePreferences;
class ICodeStylePreferencesFactory;
class SnippetEditorWidget;

// Base for a self-managed code style value editor whose deferred state lives
// outside the ICodeStylePreferences (e.g. ClangFormat's .clang-format files and
// global settings). When a value editor derives from this, the hosting
// CodeStyleAspect routes apply/cancel/isDirty to it and listens to changed(),
// and leaves it to lay out its own selector and preview. Plain value editors
// that only edit the preferences do not need it.
class TEXTEDITOR_EXPORT CodeStyleEditor : public QWidget
{
    Q_OBJECT

public:
    using QWidget::QWidget;

    virtual void apply();
    virtual void cancel();
    virtual bool isDirty() const;

signals:
    void changed();
};

// Creates a snippet preview editor bound to codeStyle: decorated with the
// factory's snippet group, filled with its preview text, and re-indented live
// by the factory's indenter as the style changes. Pass a project file for a
// per-project preview, or an empty path for the global one.
TEXTEDITOR_EXPORT SnippetEditorWidget *createCodeStylePreview(
    const ICodeStylePreferencesFactory *factory,
    const Utils::FilePath &projectFile,
    ICodeStylePreferences *codeStyle,
    QWidget *parent = nullptr);

// The standard explanatory note shown beneath a code style preview.
TEXTEDITOR_EXPORT QLabel *createCodeStylePreviewNote();

// The "take effect immediately" hint, for the per-project code style pages
// (which apply live). Global pages defer to Apply/OK and must not use it.
TEXTEDITOR_EXPORT QWidget *createTakeEffectImmediatelyLabel();

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
    void ensurePageCopy(ICodeStylePreferencesFactory *factory);
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
