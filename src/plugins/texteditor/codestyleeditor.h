// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/filepath.h>

#include <QString>
#include <QWidget>

#include <functional>

QT_BEGIN_NAMESPACE
class QVBoxLayout;
QT_END_NAMESPACE

namespace TextEditor {
class CodeStyleSelectorWidget;
class ICodeStylePreferencesFactory;
class ICodeStylePreferences;
class SnippetEditorWidget;

class TEXTEDITOR_EXPORT ProjectWrapper
{
public:
    using PathRetriever = std::function<Utils::FilePath(const void *)>;
    ProjectWrapper() : ProjectWrapper({}, {}) {}
    ProjectWrapper(void *project, const PathRetriever &pathRetriever)
        : m_project(project)
        , m_pathRetriever(pathRetriever)
    {}

    void *project() const { return m_project; }
    operator bool() const { return m_project; }
    Utils::FilePath projectFilePath() const { return m_pathRetriever(m_project); }

private:
    void * const m_project;
    const PathRetriever m_pathRetriever;
};

class TEXTEDITOR_EXPORT CodeStyleEditorWidget : public QWidget
{
public:
    CodeStyleEditorWidget(QWidget *parent = nullptr)
        : QWidget(parent)
    {}
    virtual void apply() {}
    virtual void finish() {}
};

class TEXTEDITOR_EXPORT CodeStyleEditor : public CodeStyleEditorWidget
{
    Q_OBJECT
public:
    void apply() override;
    void finish() override;

protected:
    CodeStyleEditor(QWidget *parent = nullptr);
    virtual void init(
        const ICodeStylePreferencesFactory *factory,
        const ProjectWrapper &project,
        ICodeStylePreferences *codeStyle);

    QVBoxLayout *m_layout = nullptr;
    CodeStyleSelectorWidget *m_selector = nullptr;
    SnippetEditorWidget *m_preview = nullptr;
    CodeStyleEditorWidget *m_editor = nullptr;

private:
    virtual CodeStyleSelectorWidget *createCodeStyleSelectorWidget(
        ICodeStylePreferences *codeStyle, QWidget *parent = nullptr) const;
    virtual SnippetEditorWidget *createPreviewWidget(
        const ICodeStylePreferencesFactory *factory,
        const ProjectWrapper &project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const;
    virtual CodeStyleEditorWidget *createEditorWidget(
        const void *project,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const
        = 0;
    virtual QString previewText() const = 0;
    virtual QString snippetProviderGroupId() const = 0;
};

} // namespace TextEditor
