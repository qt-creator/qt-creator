// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/id.h>

QT_BEGIN_NAMESPACE
template <typename Key, typename T>
class QMap;
class QTextDocument;
class QWidget;
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
    explicit ICodeStylePreferencesFactory(Utils::Id languageId = {});
    virtual ~ICodeStylePreferencesFactory();

    virtual TextEditor::Indenter *createIndenter(QTextDocument *doc) const = 0;
    virtual QString snippetGroupId() const { return {}; }
    virtual QString previewText() const { return {}; }
    Utils::Id languageId() const;
    virtual QString displayName() = 0;
    virtual ICodeStylePreferences *createCodeStyle() const = 0;
    virtual CodeStyleEditor *createSettingsEditor(
        ICodeStylePreferences *codeStyle, QWidget *parent = nullptr) const = 0;
    virtual CodeStyleEditor *createProjectEditor(
        const Utils::FilePath &projectFile,
        ICodeStylePreferences *codeStyle,
        QWidget *parent = nullptr) const;

private:
    Utils::Id m_languageId;
};

TEXTEDITOR_EXPORT ICodeStylePreferencesFactory *codeStyleFactory(Utils::Id languageId);
TEXTEDITOR_EXPORT const QMap<Utils::Id, ICodeStylePreferencesFactory *> &codeStyleFactories();

} // namespace TextEditor
