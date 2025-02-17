// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tabsettingswidget.h"

#include "tabsettings.h"
#include "texteditortr.h"

#include <QApplication>
#include <QCheckBox>
#include <QComboBox>
#include <QLabel>
#include <QSpinBox>
#include <QTextStream>

#include <utils/layoutbuilder.h>

namespace TextEditor {

QString continuationTooltip()
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

TabSettingsWidget::TabSettingsWidget()
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

    tabPolicy.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    tabPolicy.addOption(Tr::tr("Spaces Only"));
    tabPolicy.addOption(Tr::tr("Tabs Only"));

    tabSize.setRange(1, 20);

    indentSize.setRange(1, 20);

    continuationAlignBehavior.setDisplayStyle(Utils::SelectionAspect::DisplayStyle::ComboBox);
    continuationAlignBehavior.addOption(Tr::tr("Not At All"));
    continuationAlignBehavior.addOption(Tr::tr("With Spaces"));
    continuationAlignBehavior.addOption(Tr::tr("With Regular Indent"));
    continuationAlignBehavior.setToolTip(continuationTooltip());

    connect(m_codingStyleWarning, &QLabel::linkActivated,
            this, &TabSettingsWidget::codingStyleLinkActivated);
    connect(this, &Utils::AspectContainer::changed, this, [this] {
            emit settingsChanged(tabSettings());
    });
}

TabSettingsWidget::~TabSettingsWidget() = default;

void TabSettingsWidget::setTabSettings(const TabSettings &s)
{
    QSignalBlocker blocker(this);
    autoDetect.setValue(s.m_autoDetect);
    tabPolicy.setValue(int(s.m_tabPolicy));
    tabSize.setValue(s.m_tabSize);
    indentSize.setValue(s.m_indentSize);
    continuationAlignBehavior.setValue(s.m_continuationAlignBehavior);
}

void TabSettingsWidget::addToLayoutImpl(Layouting::Layout &parent)
{
    using namespace Layouting;
    parent.addItem(Group {
        title(Tr::tr("Tabs And Indentation")),
        Row {
            Form {
                m_codingStyleWarning, br,
                autoDetect, br,
                Tr::tr("Default tab policy:"), tabPolicy, br,
                Tr::tr("Default &indent size:"), indentSize, br,
                Tr::tr("Ta&b size:"), tabSize, br,
                Tr::tr("Align continuation lines:"), continuationAlignBehavior, br
            }
        }
    });
}

TabSettings TabSettingsWidget::tabSettings() const
{
    TabSettings set;

    set.m_autoDetect = autoDetect();
    set.m_tabPolicy = TabSettings::TabPolicy(tabPolicy());
    set.m_tabSize = tabSize();
    set.m_indentSize = indentSize();
    set.m_continuationAlignBehavior =
        TabSettings::ContinuationAlignBehavior(continuationAlignBehavior());

    return set;
}

void TabSettingsWidget::codingStyleLinkActivated(const QString &linkString)
{
    if (linkString == QLatin1String("C++"))
        emit codingStyleLinkClicked(CppLink);
    else if (linkString == QLatin1String("QtQuick"))
        emit codingStyleLinkClicked(QtQuickLink);
}

void TabSettingsWidget::setCodingStyleWarningVisible(bool visible)
{
    m_codingStyleWarning->setVisible(visible);
}

} // TextEditor
