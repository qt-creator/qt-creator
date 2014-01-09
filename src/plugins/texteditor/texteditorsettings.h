/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef TEXTEDITORSETTINGS_H
#define TEXTEDITORSETTINGS_H

#include "texteditor_global.h"

#include <coreplugin/id.h>

#include <QObject>

QT_BEGIN_NAMESPACE
template <class Key, class T>
class QMap;
QT_END_NAMESPACE

namespace TextEditor {

class BaseTextEditorWidget;
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

    static void initializeEditor(BaseTextEditorWidget *editor);

    static const FontSettings &fontSettings();
    static const TypingSettings &typingSettings();
    static const StorageSettings &storageSettings();
    static const BehaviorSettings &behaviorSettings();
    static const MarginSettings &marginSettings();
    static const DisplaySettings &displaySettings();
    static const CompletionSettings &completionSettings();
    static const HighlighterSettings &highlighterSettings();
    static const ExtraEncodingSettings &extraEncodingSettings();

    static void setCompletionSettings(const TextEditor::CompletionSettings &);

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

signals:
    void fontSettingsChanged(const TextEditor::FontSettings &);
    void typingSettingsChanged(const TextEditor::TypingSettings &);
    void storageSettingsChanged(const TextEditor::StorageSettings &);
    void behaviorSettingsChanged(const TextEditor::BehaviorSettings &);
    void marginSettingsChanged(const TextEditor::MarginSettings &);
    void displaySettingsChanged(const TextEditor::DisplaySettings &);
    void completionSettingsChanged(const TextEditor::CompletionSettings &);
    void extraEncodingSettingsChanged(const TextEditor::ExtraEncodingSettings &);

private slots:
    void fontZoomRequested(int zoom);
    void zoomResetRequested();
};

} // namespace TextEditor

#endif // TEXTEDITORSETTINGS_H
