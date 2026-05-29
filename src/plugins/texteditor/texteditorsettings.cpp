// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorsettings.h"

#include "codestylepool.h"
#include "commentssettings.h"
#include "completionsettings.h"
#include "displaysettings.h"
#include "fontsettingspage.h"
#include "icodestylepreferences.h"

using namespace TextEditor::Internal;
using namespace Utils;

namespace TextEditor {
namespace Internal {

class TextEditorSettingsPrivate
{
public:
    QMap<Utils::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<QString, Utils::Id> m_mimeTypeToLanguage;
};

} // namespace Internal


static TextEditorSettingsPrivate *d = nullptr;

TextEditorSettings::TextEditorSettings()
{
    d = new Internal::TextEditorSettingsPrivate;

    Internal::setupFontSettingsPage();
    setupCompletionSettings();
    setupDisplaySettings();
    setupCommentsSettings();
    Internal::setupGlobalCodeStyle();
}

TextEditorSettings::~TextEditorSettings()
{
    delete d;
}

ICodeStylePreferences *TextEditorSettings::codeStyle()
{
    return &globalCodeStyle();
}

ICodeStylePreferences *TextEditorSettings::codeStyle(Utils::Id languageId)
{
    return d->m_languageToCodeStyle.value(languageId, codeStyle());
}

QMap<Utils::Id, ICodeStylePreferences *> TextEditorSettings::codeStyles()
{
    return d->m_languageToCodeStyle;
}

void TextEditorSettings::registerCodeStyle(Utils::Id languageId, ICodeStylePreferences *prefs)
{
    d->m_languageToCodeStyle.insert(languageId, prefs);
}

void TextEditorSettings::unregisterCodeStyle(Utils::Id languageId)
{
    d->m_languageToCodeStyle.remove(languageId);
}

void TextEditorSettings::registerMimeTypeForLanguageId(const char *mimeType, Utils::Id languageId)
{
    d->m_mimeTypeToLanguage.insert(QString::fromLatin1(mimeType), languageId);
}

Utils::Id TextEditorSettings::languageId(const QString &mimeType)
{
    return d->m_mimeTypeToLanguage.value(mimeType);
}

TextEditorSettings &Internal::textEditorSettings()
{
    static TextEditorSettings theTextEditorSettings;
    return theTextEditorSettings;
}

void Internal::setupTextEditorSettings()
{
    (void) textEditorSettings(); // Trigger instantiation.
}

} // TextEditor
