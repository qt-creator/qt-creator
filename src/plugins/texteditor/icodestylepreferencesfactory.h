// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/id.h>

#include <QString>

#include <functional>

QT_BEGIN_NAMESPACE
template <typename Key, typename T>
class QMap;
class QTextDocument;
class QWidget;
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace TextEditor {
class CodeStyleEditor;
class CodeStylePool;
class ICodeStylePreferences;
class Indenter;

class TEXTEDITOR_EXPORT ICodeStylePreferencesFactory
{
    ICodeStylePreferencesFactory(const ICodeStylePreferencesFactory &) = delete;
    ICodeStylePreferencesFactory(const ICodeStylePreferencesFactory &&) = delete;
    ICodeStylePreferencesFactory &operator=(const ICodeStylePreferencesFactory &) = delete;
    ICodeStylePreferencesFactory &operator=(const ICodeStylePreferencesFactory &&) = delete;

public:
    using IndenterCreator = std::function<Indenter *(QTextDocument *)>;
    using CodeStyleCreator = std::function<ICodeStylePreferences *()>;
    using SettingsEditorCreator = std::function<CodeStyleEditor *(ICodeStylePreferences *)>;
    using ValueEditorCreator = std::function<QWidget *(ICodeStylePreferences *)>;
    using ProjectEditorCreator
        = std::function<QWidget *(const Utils::FilePath &, ICodeStylePreferences *)>;

    explicit ICodeStylePreferencesFactory(Utils::Id languageId = {});
    virtual ~ICodeStylePreferencesFactory();

    Utils::Id languageId() const;

    QString displayName() const;
    QString snippetGroupId() const;
    QString previewText() const;
    Indenter *createIndenter(QTextDocument *doc) const;
    ICodeStylePreferences *createCodeStyle() const;
    CodeStyleEditor *createSettingsEditor(ICodeStylePreferences *codeStyle) const;
    // The language's value editor: just the widgets that edit codeStyle's own
    // settings, editing it live. The selector and preview are provided by the
    // hosting CodeStyleAspect. Returns nullptr for languages that instead
    // supply a full settings editor via setSettingsEditorCreator().
    QWidget *createValueEditor(ICodeStylePreferences *codeStyle) const;
    // Whether the value editor already contains its own preview, so the hosting
    // CodeStyleAspect should not add the standard one below it.
    bool valueEditorHasPreview() const;
    QWidget *createProjectEditor(const Utils::FilePath &projectFile,
                                 ICodeStylePreferences *codeStyle) const;

    void setDisplayName(const QString &displayName);
    void setSnippetGroupId(const QString &snippetGroupId);
    void setPreviewText(const QString &previewText);
    void setIndenterCreator(const IndenterCreator &creator);
    void setCodeStyleCreator(const CodeStyleCreator &creator);
    void setSettingsEditorCreator(const SettingsEditorCreator &creator);
    void setValueEditorCreator(const ValueEditorCreator &creator);
    void setValueEditorHasPreview(bool hasPreview);
    void setProjectEditorCreator(const ProjectEditorCreator &creator);

    // Builds and owns the language's code style pool and its editable global
    // style. setupCodeStyles() must be called once the creators above are set;
    // it adds the global (delegating to the default built-in), the built-ins,
    // and the custom styles in the right order, loads the saved settings, and
    // registers the global for the language.
    void setGlobalCodeStyleId(const QByteArray &id);
    void setDefaultCodeStyleId(const QByteArray &id);
    void setBuiltInCodeStyles(const std::function<void(CodeStylePool *)> &adder);
    void setupCodeStyles();
    ICodeStylePreferences *globalCodeStyle() const;
    CodeStylePool *codeStylePool() const;

private:
    Utils::Id m_languageId;
    QString m_displayName;
    QString m_snippetGroupId;
    QString m_previewText;
    IndenterCreator m_indenterCreator;
    CodeStyleCreator m_codeStyleCreator;
    SettingsEditorCreator m_settingsEditorCreator;
    ValueEditorCreator m_valueEditorCreator;
    bool m_valueEditorHasPreview = false;
    ProjectEditorCreator m_projectEditorCreator;
    std::function<void(CodeStylePool *)> m_builtInCodeStyles;
    QByteArray m_globalCodeStyleId;
    QByteArray m_defaultCodeStyleId;
    CodeStylePool *m_pool = nullptr;
    ICodeStylePreferences *m_globalCodeStyle = nullptr;
};

TEXTEDITOR_EXPORT ICodeStylePreferencesFactory *codeStyleFactory(Utils::Id languageId);
TEXTEDITOR_EXPORT const QMap<Utils::Id, ICodeStylePreferencesFactory *> &codeStyleFactories();

} // namespace TextEditor
