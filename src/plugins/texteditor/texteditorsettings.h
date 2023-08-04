// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "commentssettings.h"
#include "texteditor_global.h"

#include <utils/id.h>

#include <QObject>

#include <functional>

QT_BEGIN_NAMESPACE
template <typename Key, typename T>
class QMap;
QT_END_NAMESPACE

namespace TextEditor {

class FontSettings;
class TypingSettings;
class StorageSettings;
class BehaviorSettings;
class MarginSettings;
class DisplaySettings;
class CompletionSettings;
class HighlighterSettings;
class ExtraEncodingSettings;
class ICodeStylePreferences;
class ICodeStylePreferencesFactory;
class CodeStylePool;
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

    static const FontSettings &fontSettings();
    static const TypingSettings &typingSettings();
    static const StorageSettings &storageSettings();
    static const BehaviorSettings &behaviorSettings();
    static const MarginSettings &marginSettings();
    static const DisplaySettings &displaySettings();
    static const CompletionSettings &completionSettings();
    static const HighlighterSettings &highlighterSettings();
    static const ExtraEncodingSettings &extraEncodingSettings();

    static void setCommentsSettingsRetriever(
        const std::function<CommentsSettings::Data(const Utils::FilePath &)> &);
    static CommentsSettings::Data commentsSettings(const Utils::FilePath &filePath);

    static ICodeStylePreferencesFactory *codeStyleFactory(Utils::Id languageId);
    static const QMap<Utils::Id, ICodeStylePreferencesFactory *> &codeStyleFactories();
    static void registerCodeStyleFactory(ICodeStylePreferencesFactory *codeStyleFactory);
    static void unregisterCodeStyleFactory(Utils::Id languageId);

    static CodeStylePool *codeStylePool();
    static CodeStylePool *codeStylePool(Utils::Id languageId);
    static void registerCodeStylePool(Utils::Id languageId, CodeStylePool *pool);
    static void unregisterCodeStylePool(Utils::Id languageId);

    static ICodeStylePreferences *codeStyle();
    static ICodeStylePreferences *codeStyle(Utils::Id languageId);
    static QMap<Utils::Id, ICodeStylePreferences *> codeStyles();
    static void registerCodeStyle(Utils::Id languageId, ICodeStylePreferences *prefs);
    static void unregisterCodeStyle(Utils::Id languageId);

    static void registerMimeTypeForLanguageId(const char *mimeType, Utils::Id languageId);
    static Utils::Id languageId(const QString &mimeType);
    static int increaseFontZoom(int step);
    static void resetFontZoom();

signals:
    void fontSettingsChanged(const TextEditor::FontSettings &);
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void marginSettingsChanged(const TextEditor::MarginSettings &);
    void displaySettingsChanged(const TextEditor::DisplaySettings &);
    void completionSettingsChanged(const TextEditor::CompletionSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);
    void commentsSettingsChanged();
};

} // namespace TextEditor
