/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
****************************************************************************/

#pragma once

#include "texteditor_global.h"

#include <coreplugin/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
template <typename Key, typename T>
class QMap;
QT_END_NAMESPACE

namespace TextEditor {

class FontSettings;
class TabSettings;
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
    explicit TextEditorSettings(QObject *parent);
    ~TextEditorSettings();

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
    static const CommentsSettings &commentsSettings();

    static ICodeStylePreferencesFactory *codeStyleFactory(Core::Id languageId);
    static QMap<Core::Id, ICodeStylePreferencesFactory *> codeStyleFactories();
    static void registerCodeStyleFactory(ICodeStylePreferencesFactory *codeStyleFactory);
    static void unregisterCodeStyleFactory(Core::Id languageId);

    static CodeStylePool *codeStylePool();
    static CodeStylePool *codeStylePool(Core::Id languageId);
    static void registerCodeStylePool(Core::Id languageId, CodeStylePool *pool);
    static void unregisterCodeStylePool(Core::Id languageId);

    static ICodeStylePreferences *codeStyle();
    static ICodeStylePreferences *codeStyle(Core::Id languageId);
    static QMap<Core::Id, ICodeStylePreferences *> codeStyles();
    static void registerCodeStyle(Core::Id languageId, ICodeStylePreferences *prefs);
    static void unregisterCodeStyle(Core::Id languageId);

    static void registerMimeTypeForLanguageId(const char *mimeType, Core::Id languageId);
    static Core::Id languageId(const QString &mimeType);
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
    void commentsSettingsChanged(const TextEditor::CommentsSettings &);
};

} // namespace TextEditor
