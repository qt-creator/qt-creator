// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "behaviorsettings.h"

#include "codestylepool.h"
#include "extraencodingsettings.h"
#include "simplecodestylepreferences.h"
#include "storagesettings.h"
#include "tabsettings.h"
#include "tabsettingswidget.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"
#include "typingsettings.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>
#include <coreplugin/coreconstants.h>
#include <coreplugin/editormanager/editormanager.h>

#include <utils/hostosinfo.h>
#include <utils/layoutbuilder.h>
#include <utils/qtcassert.h>
#include <utils/qtcsettings.h>

// for opening the respective coding style preferences
#include <cppeditor/cppeditorconstants.h>
#include <qmljseditor/qmljseditorconstants.h>
#include <qmljstools/qmljstoolsconstants.h>


using namespace Utils;

namespace TextEditor {

BehaviorSettings::BehaviorSettings(const Key &keyPrefix)
{
    setSettingsGroup("textBehaviorSettings");

    mouseHiding.setSettingsKey(keyPrefix + "MouseHiding");
    mouseHiding.setDefaultValue(true);
    mouseHiding.setVisible(!HostOsInfo::isMacHost());
    mouseHiding.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    mouseHiding.setLabelText(Tr::tr("Hide mouse cursor while typing"));

    mouseNavigation.setSettingsKey(keyPrefix + "MouseNavigation");
    mouseNavigation.setDefaultValue(true);
    mouseNavigation.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    mouseNavigation.setLabelText(Tr::tr("Enable &mouse navigation"));

    scrollWheelZooming.setSettingsKey(keyPrefix + "ScrollWheelZooming");
    scrollWheelZooming.setDefaultValue(true);
    scrollWheelZooming.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    scrollWheelZooming.setLabelText(Tr::tr("Enable scroll &wheel zooming"));

    constrainHoverTooltips.setSettingsKey(keyPrefix + "ConstrainTooltips");
    constrainHoverTooltips.setDisplayStyle(SelectionAspect::DisplayStyle::ComboBox);
    constrainHoverTooltips.setUseDataAsSavedValue();
    constrainHoverTooltips.setLabelText(Tr::tr("Show help tooltips using the mouse:"));
    constrainHoverTooltips.addOption({
        Tr::tr("On Mouseover"),
        Tr::tr("Displays context-sensitive help or type information on mouseover."),
        QVariant(false)
    });
    constrainHoverTooltips.addOption({
        Tr::tr("On Shift+Mouseover"),
        Tr::tr("Displays context-sensitive help or type information on Shift+Mouseover."),
        QVariant(true)
    });
    constrainHoverTooltips.setLabelText(Tr::tr("Show help tooltips using the mouse:"));

    camelCaseNavigation.setSettingsKey(keyPrefix + "CamelCaseNavigation");
    camelCaseNavigation.setDefaultValue(true);
    camelCaseNavigation.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    camelCaseNavigation.setLabelText(Tr::tr("Enable built-in camel case &navigation"));

    keyboardTooltips.setSettingsKey(keyPrefix + "KeyboardTooltips");
    keyboardTooltips.setDefaultValue(false);
    keyboardTooltips.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    keyboardTooltips.setLabelText(Tr::tr("Show help tooltips using keyboard shortcut (Alt)"));
    keyboardTooltips.setToolTip(Tr::tr("Pressing Alt displays context-sensitive help or type information as tooltips."));

    smartSelectionChanging.setSettingsKey(keyPrefix + "SmartSelectionChanging");
    smartSelectionChanging.setDefaultValue(true);
    smartSelectionChanging.setLabelPlacement(BoolAspect::LabelPlacement::Compact);
    smartSelectionChanging.setLabelText(Tr::tr("Enable smart selection changing"));
    smartSelectionChanging.setToolTip(Tr::tr("Using Select Block Up / Down actions will now provide smarter selections."));

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Mouse and Keyboard")),
                Row {
                    Form {
                        mouseHiding, br,
                        mouseNavigation, br,
                        scrollWheelZooming, br,
                        camelCaseNavigation, br,
                        smartSelectionChanging, br,
                        keyboardTooltips, br,
                        constrainHoverTooltips, br
                    }, st
                }
            }
        };
    });
}

void BehaviorSettings::setData(const BehaviorSettingsData &data)
{
    mouseHiding.setValue(data.m_mouseHiding);
    mouseNavigation.setValue(data.m_mouseNavigation);
    scrollWheelZooming.setValue(data.m_scrollWheelZooming);
    constrainHoverTooltips.setValue(data.m_constrainHoverTooltips);
    camelCaseNavigation.setValue(data.m_camelCaseNavigation);
    keyboardTooltips.setValue(data.m_keyboardTooltips);
    smartSelectionChanging.setValue(data.m_smartSelectionChanging);
}

void BehaviorSettings::apply()
{
    AspectContainer::apply();
    AspectContainer::writeSettings();
}

BehaviorSettingsData BehaviorSettings::data() const
{
    BehaviorSettingsData d;
    d.m_mouseHiding = mouseHiding();
    d.m_mouseNavigation = mouseNavigation();
    d.m_scrollWheelZooming = scrollWheelZooming();
    d.m_constrainHoverTooltips = constrainHoverTooltips();
    d.m_camelCaseNavigation = camelCaseNavigation();
    d.m_keyboardTooltips = keyboardTooltips();
    d.m_smartSelectionChanging = smartSelectionChanging();
    return d;
}

BehaviorSettings &globalBehaviorSettings()
{
    static BehaviorSettings theGlobalBehaviorSettings;
    return theGlobalBehaviorSettings;
}

// BehaviorSettingsPage

class BehaviorSettingsPage : public Core::IOptionsPage
{
public:
    BehaviorSettingsPage();

    ICodeStylePreferences *codeStyle() { return &m_codeStyle; }
    CodeStylePool *codeStylePool() { return &m_defaultCodeStylePool; }

    const Key m_settingsPrefix{"text"};
    CodeStylePool m_defaultCodeStylePool{nullptr};
    SimpleCodeStylePreferences m_codeStyle;
};

class BehaviorSettingsWidgetImpl : public Core::IOptionsPageWidget
{
public:
    BehaviorSettingsWidgetImpl(BehaviorSettingsPage *page)
        : m_page(page)
    {
        m_pageCodeStyle.setDelegatingPool(page->m_codeStyle.delegatingPool());
        m_pageCodeStyle.setTabSettings(page->m_codeStyle.tabSettings());
        m_pageCodeStyle.setCurrentDelegate(page->m_codeStyle.currentDelegate());
        m_tabSettingsWidget.setPreferences(&m_pageCodeStyle);

        using namespace Layouting;
        Column {
            &m_tabSettingsWidget,
            &globalTypingSettings(),
            &globalStorageSettings(),
            &globalExtraEncodingSettings(),
            &globalBehaviorSettings(),
            st,
        }.attachTo(this);

        m_tabSettingsWidget.setCodingStyleWarningVisible(true);
        connect(&m_tabSettingsWidget, &TabSettings::codingStyleLinkClicked,
                this, [] (TabSettings::CodingStyleLink link) {
            switch (link) {
            case TabSettings::CppLink:
                Core::ICore::showSettings(CppEditor::Constants::CPP_CODE_STYLE_SETTINGS_ID);
                break;
            case TabSettings::QtQuickLink:
                Core::ICore::showSettings(QmlJSTools::Constants::QML_JS_CODE_STYLE_SETTINGS_ID);
                break;
            }
        });

        installMarkSettingsDirtyTriggerRecursively(&m_pageCodeStyle);
        installCheckSettingsDirtyTrigger(&globalTypingSettings());
        installCheckSettingsDirtyTrigger(&globalStorageSettings());
        installCheckSettingsDirtyTrigger(&globalBehaviorSettings());
        installCheckSettingsDirtyTrigger(&globalExtraEncodingSettings());
    }

    bool isDirty() const;
    void apply() final;

    BehaviorSettingsPage *m_page;
    TabSettings m_tabSettingsWidget;
    SimpleCodeStylePreferences m_pageCodeStyle;
};

BehaviorSettingsPage::BehaviorSettingsPage()
{
    setId(Constants::TEXT_EDITOR_BEHAVIOR_SETTINGS);
    setDisplayName(Tr::tr("Behavior"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setWidgetCreator([this] { return new BehaviorSettingsWidgetImpl(this); });

    m_codeStyle.setDisplayName(Tr::tr("Global", "Settings"));
    m_codeStyle.setId(Constants::GLOBAL_SETTINGS_ID);
    m_codeStyle.fromSettings(m_settingsPrefix);

    m_defaultCodeStylePool.addCodeStyle(&m_codeStyle);
}

bool BehaviorSettingsWidgetImpl::isDirty() const
{
    if (globalTypingSettings().isDirty())
        return true;
    if (globalStorageSettings().isDirty())
        return true;
    if (globalBehaviorSettings().isDirty())
        return true;
    if (globalExtraEncodingSettings().isDirty())
        return true;

    if (m_page->m_codeStyle.tabSettings() != m_pageCodeStyle.tabSettings())
        return true;
    if (m_page->m_codeStyle.currentDelegate() != m_pageCodeStyle.currentDelegate())
        return true;

    return false;
}

void BehaviorSettingsWidgetImpl::apply()
{
    globalTypingSettings().apply();
    globalBehaviorSettings().apply();
    globalStorageSettings().apply();
    globalExtraEncodingSettings().apply();

    if (m_page->m_codeStyle.tabSettings() != m_pageCodeStyle.tabSettings()) {
        m_page->m_codeStyle.setTabSettings(m_pageCodeStyle.tabSettings());
        m_page->m_codeStyle.toSettings(m_page->m_settingsPrefix);
    }

    if (m_page->m_codeStyle.currentDelegate() != m_pageCodeStyle.currentDelegate()) {
        m_page->m_codeStyle.setCurrentDelegate(m_pageCodeStyle.currentDelegate());
        m_page->m_codeStyle.toSettings(m_page->m_settingsPrefix);
    }
}

static BehaviorSettingsPage &behaviorSettingsPage()
{
    static BehaviorSettingsPage theBehaviorSettingsPage;
    return theBehaviorSettingsPage;
}

ICodeStylePreferences *globalCodeStyle()
{
    return behaviorSettingsPage().codeStyle();
}

CodeStylePool *globalCodeStylePool()
{
    return behaviorSettingsPage().codeStylePool();
}

void Internal::setupBehaviorSettings()
{
    globalBehaviorSettings().readSettings();
    (void) behaviorSettingsPage();
}

} // namespace TextEditor
