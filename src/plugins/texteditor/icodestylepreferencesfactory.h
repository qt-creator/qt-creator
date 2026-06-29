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
QT_END_NAMESPACE

namespace Utils { class FilePath; }

namespace TextEditor {
class CodeStyleEditor;
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
    using ProjectEditorCreator
        = std::function<CodeStyleEditor *(const Utils::FilePath &, ICodeStylePreferences *)>;

    explicit ICodeStylePreferencesFactory(Utils::Id languageId = {});
    virtual ~ICodeStylePreferencesFactory();

    Utils::Id languageId() const;

    QString displayName() const;
    QString snippetGroupId() const;
    QString previewText() const;
    Indenter *createIndenter(QTextDocument *doc) const;
    ICodeStylePreferences *createCodeStyle() const;
    CodeStyleEditor *createSettingsEditor(ICodeStylePreferences *codeStyle) const;
    CodeStyleEditor *createProjectEditor(const Utils::FilePath &projectFile,
                                         ICodeStylePreferences *codeStyle) const;

    void setDisplayName(const QString &displayName);
    void setSnippetGroupId(const QString &snippetGroupId);
    void setPreviewText(const QString &previewText);
    void setIndenterCreator(const IndenterCreator &creator);
    void setCodeStyleCreator(const CodeStyleCreator &creator);
    void setSettingsEditorCreator(const SettingsEditorCreator &creator);
    void setProjectEditorCreator(const ProjectEditorCreator &creator);

private:
    Utils::Id m_languageId;
    QString m_displayName;
    QString m_snippetGroupId;
    QString m_previewText;
    IndenterCreator m_indenterCreator;
    CodeStyleCreator m_codeStyleCreator;
    SettingsEditorCreator m_settingsEditorCreator;
    ProjectEditorCreator m_projectEditorCreator;
};

TEXTEDITOR_EXPORT ICodeStylePreferencesFactory *codeStyleFactory(Utils::Id languageId);
TEXTEDITOR_EXPORT const QMap<Utils::Id, ICodeStylePreferencesFactory *> &codeStyleFactories();

} // namespace TextEditor
