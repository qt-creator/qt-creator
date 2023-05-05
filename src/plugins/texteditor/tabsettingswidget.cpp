// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "tabsettingswidget.h"

#include "tabsettings.h"
#include "texteditortr.h"

#include <QApplication>
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

TabSettingsWidget::TabSettingsWidget(QWidget *parent) :
    QGroupBox(parent)
{
    setTitle(Tr::tr("Tabs And Indentation"));

    m_codingStyleWarning = new QLabel(
        Tr::tr("<i>Code indentation is configured in <a href=\"C++\">C++</a> "
           "and <a href=\"QtQuick\">Qt Quick</a> settings.</i>"));
    m_codingStyleWarning->setVisible(false);
    m_codingStyleWarning->setWordWrap(true);
    m_codingStyleWarning->setToolTip(
        Tr::tr("The text editor indentation setting is used for non-code files only. See the C++ "
           "and Qt Quick coding style settings to configure indentation for code files."));

    m_tabPolicy = new QComboBox(this);
    m_tabPolicy->setMinimumContentsLength(28);
    m_tabPolicy->addItem(Tr::tr("Spaces Only"));
    m_tabPolicy->addItem(Tr::tr("Tabs Only"));
    m_tabPolicy->addItem(Tr::tr("Mixed"));

    auto tabSizeLabel = new QLabel(Tr::tr("Ta&b size:"));

    m_tabSize = new QSpinBox(this);
    m_tabSize->setRange(1, 20);

    auto indentSizeLabel = new QLabel(Tr::tr("&Indent size:"));

    m_indentSize = new QSpinBox(this);
    m_indentSize->setRange(1, 20);

    m_continuationAlignBehavior = new QComboBox;
    m_continuationAlignBehavior->addItem(Tr::tr("Not At All"));
    m_continuationAlignBehavior->addItem(Tr::tr("With Spaces"));
    m_continuationAlignBehavior->addItem(Tr::tr("With Regular Indent"));
    m_continuationAlignBehavior->setToolTip(continuationTooltip());

    tabSizeLabel->setBuddy(m_tabSize);
    indentSizeLabel->setBuddy(m_indentSize);

    using namespace Layouting;
    const auto indent = [](QWidget *inner) { return Row { Space(30), inner }; };

    Column {
        m_codingStyleWarning,
        Tr::tr("Tab policy:"),
        indent(m_tabPolicy),
        Row { tabSizeLabel, m_tabSize, indentSizeLabel, m_indentSize, st },
        Tr::tr("Align continuation lines:"),
        indent(m_continuationAlignBehavior)
    }.attachTo(this);

    connect(m_codingStyleWarning, &QLabel::linkActivated,
            this, &TabSettingsWidget::codingStyleLinkActivated);
    connect(m_tabPolicy, &QComboBox::currentIndexChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
    connect(m_tabSize, &QSpinBox::valueChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
    connect(m_indentSize, &QSpinBox::valueChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
    connect(m_continuationAlignBehavior, &QComboBox::currentIndexChanged,
            this, &TabSettingsWidget::slotSettingsChanged);
}

TabSettingsWidget::~TabSettingsWidget() = default;

void TabSettingsWidget::setTabSettings(const TabSettings &s)
{
    QSignalBlocker blocker(this);
    m_tabPolicy->setCurrentIndex(s.m_tabPolicy);
    m_tabSize->setValue(s.m_tabSize);
    m_indentSize->setValue(s.m_indentSize);
    m_continuationAlignBehavior->setCurrentIndex(s.m_continuationAlignBehavior);
}

TabSettings TabSettingsWidget::tabSettings() const
{
    TabSettings set;

    set.m_tabPolicy = TabSettings::TabPolicy(m_tabPolicy->currentIndex());
    set.m_tabSize = m_tabSize->value();
    set.m_indentSize = m_indentSize->value();
    set.m_continuationAlignBehavior =
        TabSettings::ContinuationAlignBehavior(m_continuationAlignBehavior->currentIndex());

    return set;
}

void TabSettingsWidget::slotSettingsChanged()
{
    emit settingsChanged(tabSettings());
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
