// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commentssettings.h"
#include "texteditor_global.h"

#include <utils/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
template <typename Key, typename T>
class QMap;
QT_END_NAMESPACE

namespace TextEditor {

class FontSettingsData;
class TypingSettingsData;
class StorageSettingsData;
class BehaviorSettingsData;
class MarginSettingsData;
class DisplaySettingsData;
class CompletionSettingsData;
class ExtraEncodingSettingsData;
class ICodeStylePreferences;
class CommentsSettings;

/**
 * This class provides a central place for basic text editor settings. These
 * settings include font settings, tab settings, storage settings, behavior
 * settings, display settings and completion settings.
 */
class TEXTEDITOR_EXPORT TextEditorSettings : public QObject
{
    Q_OBJECT

public:
    TextEditorSettings();
    ~TextEditorSettings() override;

    static TextEditorSettings *instance();

    static FontSettingsData fontSettings();

    static ICodeStylePreferences *codeStyle();
    static ICodeStylePreferences *codeStyle(Utils::Id languageId);
    static QMap<Utils::Id, ICodeStylePreferences *> codeStyles();
    static void registerCodeStyle(Utils::Id languageId, ICodeStylePreferences *prefs);
    static void unregisterCodeStyle(Utils::Id languageId);

    static void registerMimeTypeForLanguageId(const char *mimeType, Utils::Id languageId);
    static Utils::Id languageId(const QString &mimeType);
    static int increaseFontZoom();
    static int decreaseFontZoom();
    static int increaseFontZoom(int step);
    static void resetFontZoom();

signals:
    void fontSettingsChanged(const TextEditor::FontSettingsData &);
    void typingSettingsChanged(const TextEditor::TypingSettingsData &);
    void storageSettingsChanged(const TextEditor::StorageSettingsData &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettingsData &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettingsData &);
    void commentsSettingsChanged();
};

namespace Internal {
TextEditorSettings &textEditorSettings();
void setupTextEditorSettings();
} // Internal

} // TextEditor
