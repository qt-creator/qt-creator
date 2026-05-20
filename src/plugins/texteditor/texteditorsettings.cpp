// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "texteditorsettings.h"

#include "behaviorsettings.h"
#include "codestylepool.h"
#include "commentssettings.h"
#include "completionsettings.h"
#include "displaysettings.h"
#include "fontsettings.h"
#include "fontsettingspage.h"
#include "highlightersettings.h"
#include "icodestylepreferences.h"
#include "icodestylepreferencesfactory.h"
#include "marginsettings.h"
#include "simplecodestylepreferences.h"
#include "texteditorconstants.h"
#include "texteditortr.h"

#include <coreplugin/find/searchresultwindow.h>
#include <coreplugin/icore.h>
#include <coreplugin/messagemanager.h>

#include <extensionsystem/pluginmanager.h>

#include <utils/fancylineedit.h>
#include <utils/qtcassert.h>

using namespace TextEditor::Constants;
using namespace TextEditor::Internal;
using namespace Utils;

namespace TextEditor {
namespace Internal {

class TextEditorSettingsPrivate
{
public:
    TextEditorSettingsPrivate()
    {
        m_globalCodeStyle.setDisplayName(Tr::tr("Global", "Settings"));
        m_globalCodeStyle.setId(Constants::GLOBAL_SETTINGS_ID);
        m_globalCodeStyle.fromSettings(Constants::CODE_STYLE_SETTINGS_PREFIX);
        m_defaultCodeStylePool.addCodeStyle(&m_globalCodeStyle);
    }

    CommentsSettingsPage m_commentsSettingsPage;

    QMap<Utils::Id, ICodeStylePreferencesFactory *> m_languageToFactory;

    QMap<Utils::Id, ICodeStylePreferences *> m_languageToCodeStyle;
    QMap<Utils::Id, CodeStylePool *> m_languageToCodeStylePool;
    QMap<QString, Utils::Id> m_mimeTypeToLanguage;

    std::function<CommentsSettings::Data(const Utils::FilePath &)> m_retrieveCommentsSettings;

    SimpleCodeStylePreferences m_globalCodeStyle;
    CodeStylePool m_defaultCodeStylePool{nullptr};
};

} // namespace Internal


static TextEditorSettingsPrivate *d = nullptr;

TextEditorSettings::TextEditorSettings()
{
    d = new Internal::TextEditorSettingsPrivate;

    Internal::setupFontSettingsPage();
    setupCompletionSettings();
    setupDisplaySettings();

    // Note: default background colors are coming from FormatDescription::background()

    auto updateGeneralMessagesFontSettings = []() {
        Core::MessageManager::setFont(globalFontSettings().data().font());
    };
    connect(this, &TextEditorSettings::fontSettingsChanged,
            this, updateGeneralMessagesFontSettings);
    updateGeneralMessagesFontSettings();
    auto updateBehaviorSettings = [](const BehaviorSettingsData &bs) {
        Core::MessageManager::setWheelZoomEnabled(bs.m_scrollWheelZooming);
        FancyLineEdit::setCamelCaseNavigationEnabled(bs.m_camelCaseNavigation);
    };
    connect(this, &TextEditorSettings::behaviorSettingsChanged,
            this, updateBehaviorSettings);
    updateBehaviorSettings(globalBehaviorSettings().data());
}

TextEditorSettings::~TextEditorSettings()
{
    delete d;
}

TextEditorSettings *TextEditorSettings::instance()
{
    return &textEditorSettings();
}

FontSettings TextEditorSettings::fontSettings()
{
    return globalFontSettings().data();
}

void TextEditorSettings::setCommentsSettingsRetriever(
    const std::function<CommentsSettings::Data(const Utils::FilePath &)> &retrieve)
{
    d->m_retrieveCommentsSettings = retrieve;
}

CommentsSettings::Data TextEditorSettings::commentsSettings(const Utils::FilePath &filePath)
{
    QTC_ASSERT(d->m_retrieveCommentsSettings, return CommentsSettings::instance().data());
    return d->m_retrieveCommentsSettings(filePath);
}

void TextEditorSettings::registerCodeStyleFactory(ICodeStylePreferencesFactory *factory)
{
    d->m_languageToFactory.insert(factory->languageId(), factory);
}

void TextEditorSettings::unregisterCodeStyleFactory(Utils::Id languageId)
{
    d->m_languageToFactory.remove(languageId);
}

const QMap<Utils::Id, ICodeStylePreferencesFactory *> &TextEditorSettings::codeStyleFactories()
{
    return d->m_languageToFactory;
}

ICodeStylePreferencesFactory *TextEditorSettings::codeStyleFactory(Utils::Id languageId)
{
    return d->m_languageToFactory.value(languageId);
}

ICodeStylePreferences *TextEditorSettings::codeStyle()
{
    return &d->m_globalCodeStyle;
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

CodeStylePool *TextEditorSettings::codeStylePool()
{
    return &d->m_defaultCodeStylePool;
}

CodeStylePool *TextEditorSettings::codeStylePool(Utils::Id languageId)
{
    return d->m_languageToCodeStylePool.value(languageId);
}

void TextEditorSettings::registerCodeStylePool(Utils::Id languageId, CodeStylePool *pool)
{
    d->m_languageToCodeStylePool.insert(languageId, pool);
}

void TextEditorSettings::unregisterCodeStylePool(Utils::Id languageId)
{
    d->m_languageToCodeStylePool.remove(languageId);
}

void TextEditorSettings::registerMimeTypeForLanguageId(const char *mimeType, Utils::Id languageId)
{
    d->m_mimeTypeToLanguage.insert(QString::fromLatin1(mimeType), languageId);
}

Utils::Id TextEditorSettings::languageId(const QString &mimeType)
{
    return d->m_mimeTypeToLanguage.value(mimeType);
}

static int setFontZoom(int zoom)
{
    zoom = qMax(10, zoom);
    FontSettings fs = globalFontSettings().data();
    if (fs.fontZoom() != zoom) {
        fs.setFontZoom(zoom);
        globalFontSettings().setData(fs);
        globalFontSettings().writeSettings();
        emit textEditorSettings().fontSettingsChanged(fs);
    }
    return zoom;
}

int TextEditorSettings::increaseFontZoom()
{
    const int previousZoom = globalFontSettings().data().fontZoom();
    return setFontZoom(previousZoom + 10 - previousZoom % 10);
}

int TextEditorSettings::decreaseFontZoom()
{
    const int previousZoom = globalFontSettings().data().fontZoom();
    const int delta = previousZoom % 10;
    return setFontZoom(previousZoom - (delta == 0 ? 10 : delta));
}

int TextEditorSettings::increaseFontZoom(int step)
{
    return setFontZoom(globalFontSettings().data().fontZoom() + step);
}

void TextEditorSettings::resetFontZoom()
{
    setFontZoom(100);
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
