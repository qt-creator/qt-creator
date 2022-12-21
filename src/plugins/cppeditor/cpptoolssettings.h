// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "cppeditor_global.h"

#include <QObject>

namespace TextEditor { class CommentsSettings; }

namespace CppEditor
{
class CppCodeStylePreferences;

namespace Internal { class CppToolsSettingsPrivate; }

/**
 * This class provides a central place for cpp tools settings.
 */
class CPPEDITOR_EXPORT CppToolsSettings : public QObject
{
    Q_OBJECT

public:
    CppToolsSettings();
    ~CppToolsSettings() override;

    static CppToolsSettings *instance();

    CppCodeStylePreferences *cppCodeStyle() const;

    const TextEditor::CommentsSettings &commentsSettings() const;
    void setCommentsSettings(const TextEditor::CommentsSettings &commentsSettings);

    bool sortedEditorDocumentOutline() const;
    void setSortedEditorDocumentOutline(bool sorted);

signals:
    void editorDocumentOutlineSortingChanged(bool isSorted);

private:
    Internal::CppToolsSettingsPrivate *d;

    static CppToolsSettings *m_instance;
};

} // namespace CppEditor
