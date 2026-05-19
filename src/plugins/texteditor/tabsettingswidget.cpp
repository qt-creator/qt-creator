// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tabsettings.h"

#include "icodestylepreferences.h"
#include "texteditortr.h"

#include <QGuiApplication>
#include <QLabel>

#include <utils/layoutbuilder.h>

namespace TextEditor {

static QString continuationTooltip()
{
    // FIXME: This is unfair towards translators.
    return Tr::tr(
        "<html><head/><body>\n"
        "Influences the indentation of continuation lines.\n"
        "\n"
        "<ul>\n"
        "<li>Not At All: Do not align at all. Lines will only be indented to the current logical indentation depth.\n"
        "<pre>\n"
        "(tab)int i = foo(a, b\n"
        "(tab)c, d);\n"
        "</pre>\n"
        "</li>\n"
        "\n"
        "<li>With Spaces: Always use spaces for alignment, regardless of the other indentation settings.\n"
        "<pre>\n"
        "(tab)int i = foo(a, b\n"
        "(tab)            c, d);\n"
        "</pre>\n"
        "</li>\n"
        "\n"
        "<li>With Regular Indent: Use tabs and/or spaces for alignment, as configured above.\n"
        "<pre>\n"
        "(tab)int i = foo(a, b\n"
        "(tab)(tab)(tab)  c, d);\n"
        "</pre>\n"
        "</li>\n"
        "</ul></body></html>");
}

TabSettings::TabSettings()
{
    m_codingStyleWarning = new QLabel(
        Tr::tr("<i>Code indentation is configured in <a href=\"C++\">C++</a> "
           "and <a href=\"QtQuick\">Qt Quick</a> settings.</i>"));
    m_codingStyleWarning->setVisible(false);
    m_codingStyleWarning->setToolTip(
        Tr::tr("The text editor indentation setting is used for non-code files only. See the C++ "
           "and Qt Quick coding style settings to configure indentation for code files."));

    autoDetect.setLabel(Tr::tr("Auto detect"));
    autoDetect.setToolTip(
        Tr::tr("%1 tries to detect the indentation settings based on the file contents. It "
               "will fallback to the settings below if the detection fails.")
            .arg(QGuiApplication::applicationDisplayName()));

    tabPolicy.setLabelText(Tr::tr("Default tab policy:"));
    tabPolicy.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    tabPolicy.addOption(Tr::tr("Spaces Only"));
    tabPolicy.addOption(Tr::tr("Tabs Only"));

    tabSize.setLabel(Tr::tr("Ta&b size:"));
    tabSize.setRange(1, 20);

    indentSize.setLabel(Tr::tr("Default &indent size:"));
    indentSize.setRange(1, 20);

    continuationAlignBehavior.setLabelText(Tr::tr("Align continuation lines:"));
    continuationAlignBehavior.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    continuationAlignBehavior.addOption(Tr::tr("Not At All"));
    continuationAlignBehavior.addOption(Tr::tr("With Spaces"));
    continuationAlignBehavior.addOption(Tr::tr("With Regular Indent"));
    continuationAlignBehavior.setToolTip(continuationTooltip());

    connect(m_codingStyleWarning, &QLabel::linkActivated,
            this, &TabSettings::codingStyleLinkActivated);
    connect(this, &Utils::AspectContainer::changed, this, [this] {
            emit settingsChanged(data());
    });

    setLayouter([this] {
        using namespace Layouting;
        return Column {
            Group {
                title(Tr::tr("Tabs And Indentation")),
                Row {
                    Form {
                        m_codingStyleWarning, br,
                        autoDetect, br,
                        tabPolicy, br,
                        indentSize, br,
                        tabSize, br,
                        continuationAlignBehavior, br
                    }
                }
            }
        };
    });
}

TabSettings::~TabSettings() = default;

void TabSettings::setPreferences(ICodeStylePreferences *preferences)
{
    if (m_preferences == preferences)
        return;

    slotCurrentPreferencesChanged(preferences);

    if (m_preferences) {
        disconnect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                   this, &TabSettings::setData);
        disconnect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
                   this, &TabSettings::slotCurrentPreferencesChanged);
        disconnect(this, &TabSettings::settingsChanged,
                   this, &TabSettings::slotTabSettingsChanged);
    }
    m_preferences = preferences;
    if (m_preferences) {
        setData(m_preferences->currentTabSettings());

        connect(m_preferences, &ICodeStylePreferences::currentTabSettingsChanged,
                this, &TabSettings::setData);
        connect(m_preferences, &ICodeStylePreferences::currentPreferencesChanged,
                this, &TabSettings::slotCurrentPreferencesChanged);
        connect(this, &TabSettings::settingsChanged,
                this, &TabSettings::slotTabSettingsChanged);
    }
}

void TabSettings::slotCurrentPreferencesChanged(ICodeStylePreferences *preferences)
{
    setEnabled(preferences && preferences->currentPreferences()
               && !preferences->currentPreferences()->isReadOnly());
}

void TabSettings::slotTabSettingsChanged(const TabSettingsData &settings)
{
    if (!m_preferences)
        return;
    ICodeStylePreferences *current = m_preferences->currentPreferences();
    if (!current)
        return;
    current->setTabSettings(settings);
}

void TabSettings::setData(const TabSettingsData &s)
{
    QSignalBlocker blocker(this);
    autoDetect.setValue(s.m_autoDetect);
    tabPolicy.setValue(int(s.m_tabPolicy));
    tabSize.setValue(s.m_tabSize);
    indentSize.setValue(s.m_indentSize);
    continuationAlignBehavior.setValue(s.m_continuationAlignBehavior);
}

TabSettingsData TabSettings::data() const
{
    TabSettingsData set;

    set.m_autoDetect = autoDetect();
    set.m_tabPolicy = TabSettingsData::TabPolicy(tabPolicy());
    set.m_tabSize = tabSize();
    set.m_indentSize = indentSize();
    set.m_continuationAlignBehavior =
        TabSettingsData::ContinuationAlignBehavior(continuationAlignBehavior());

    return set;
}

void TabSettings::codingStyleLinkActivated(const QString &linkString)
{
    if (linkString == QLatin1String("C++"))
        emit codingStyleLinkClicked(CppLink);
    else if (linkString == QLatin1String("QtQuick"))
        emit codingStyleLinkClicked(QtQuickLink);
}

void TabSettings::setCodingStyleWarningVisible(bool visible)
{
    m_codingStyleWarning->setVisible(visible);
}

} // namespace TextEditor
