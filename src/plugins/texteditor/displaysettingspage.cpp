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

#include "displaysettingspage.h"

#include "displaysettings.h"
#include "marginsettings.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "ui_displaysettingspage.h"

#include <coreplugin/icore.h>

namespace TextEditor {

class DisplaySettingsPagePrivate
{
public:
    DisplaySettingsPagePrivate();

    DisplaySettings m_displaySettings;
    MarginSettings m_marginSettings;
    QString m_settingsPrefix;
};

DisplaySettingsPagePrivate::DisplaySettingsPagePrivate()
{
    m_settingsPrefix = QLatin1String("text");
    m_displaySettings.fromSettings(m_settingsPrefix, Core::ICore::settings());
    m_marginSettings.fromSettings(m_settingsPrefix, Core::ICore::settings());
}

class DisplaySettingsWidget final : public Core::IOptionsPageWidget
{
    Q_DECLARE_TR_FUNCTIONS(TextEditor::DisplaySettingsPage)

public:
    DisplaySettingsWidget(DisplaySettingsPagePrivate *data)
        : m_data(data)
    {
        m_ui.setupUi(this);
        settingsToUI();
    }

    void apply() final;

    void settingsFromUI(DisplaySettings &displaySettings, MarginSettings &marginSettings) const;
    void settingsToUI();
    void setDisplaySettings(const DisplaySettings &, const MarginSettings &newMarginSettings);

    DisplaySettingsPagePrivate *m_data = nullptr;
    Internal::Ui::DisplaySettingsPage m_ui;
};

void DisplaySettingsWidget::apply()
{
    DisplaySettings newDisplaySettings;
    MarginSettings newMarginSettings;

    settingsFromUI(newDisplaySettings, newMarginSettings);
    setDisplaySettings(newDisplaySettings, newMarginSettings);
}

void DisplaySettingsWidget::settingsFromUI(DisplaySettings &displaySettings,
                                           MarginSettings &marginSettings) const
{
    displaySettings.m_displayLineNumbers = m_ui.displayLineNumbers->isChecked();
    displaySettings.m_textWrapping = m_ui.enableTextWrapping->isChecked();
    marginSettings.m_showMargin = m_ui.showWrapColumn->isChecked();
    marginSettings.m_marginColumn = m_ui.wrapColumn->value();
    displaySettings.m_visualizeWhitespace = m_ui.visualizeWhitespace->isChecked();
    displaySettings.m_displayFoldingMarkers = m_ui.displayFoldingMarkers->isChecked();
    displaySettings.m_highlightCurrentLine = m_ui.highlightCurrentLine->isChecked();
    displaySettings.m_highlightBlocks = m_ui.highlightBlocks->isChecked();
    displaySettings.m_animateMatchingParentheses = m_ui.animateMatchingParentheses->isChecked();
    displaySettings.m_highlightMatchingParentheses = m_ui.highlightMatchingParentheses->isChecked();
    displaySettings.m_markTextChanges = m_ui.markTextChanges->isChecked();
    displaySettings.m_autoFoldFirstComment = m_ui.autoFoldFirstComment->isChecked();
    displaySettings.m_centerCursorOnScroll = m_ui.centerOnScroll->isChecked();
    displaySettings.m_openLinksInNextSplit = m_ui.openLinksInNextSplit->isChecked();
    displaySettings.m_displayFileEncoding = m_ui.displayFileEncoding->isChecked();
    displaySettings.m_scrollBarHighlights = m_ui.scrollBarHighlights->isChecked();
    displaySettings.m_animateNavigationWithinFile = m_ui.animateNavigationWithinFile->isChecked();
    displaySettings.m_displayAnnotations = m_ui.displayAnnotations->isChecked();
    if (m_ui.leftAligned->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::NextToContent;
    else if (m_ui.atMargin->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::NextToMargin;
    else if (m_ui.rightAligned->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::RightSide;
    else if (m_ui.betweenLines->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::BetweenLines;
}

void DisplaySettingsWidget::settingsToUI()
{
    const DisplaySettings &displaySettings = m_data->m_displaySettings;
    const MarginSettings &marginSettings = m_data->m_marginSettings;
    m_ui.displayLineNumbers->setChecked(displaySettings.m_displayLineNumbers);
    m_ui.enableTextWrapping->setChecked(displaySettings.m_textWrapping);
    m_ui.showWrapColumn->setChecked(marginSettings.m_showMargin);
    m_ui.wrapColumn->setValue(marginSettings.m_marginColumn);
    m_ui.visualizeWhitespace->setChecked(displaySettings.m_visualizeWhitespace);
    m_ui.displayFoldingMarkers->setChecked(displaySettings.m_displayFoldingMarkers);
    m_ui.highlightCurrentLine->setChecked(displaySettings.m_highlightCurrentLine);
    m_ui.highlightBlocks->setChecked(displaySettings.m_highlightBlocks);
    m_ui.animateMatchingParentheses->setChecked(displaySettings.m_animateMatchingParentheses);
    m_ui.highlightMatchingParentheses->setChecked(displaySettings.m_highlightMatchingParentheses);
    m_ui.markTextChanges->setChecked(displaySettings.m_markTextChanges);
    m_ui.autoFoldFirstComment->setChecked(displaySettings.m_autoFoldFirstComment);
    m_ui.centerOnScroll->setChecked(displaySettings.m_centerCursorOnScroll);
    m_ui.openLinksInNextSplit->setChecked(displaySettings.m_openLinksInNextSplit);
    m_ui.displayFileEncoding->setChecked(displaySettings.m_displayFileEncoding);
    m_ui.scrollBarHighlights->setChecked(displaySettings.m_scrollBarHighlights);
    m_ui.animateNavigationWithinFile->setChecked(displaySettings.m_animateNavigationWithinFile);
    m_ui.displayAnnotations->setChecked(displaySettings.m_displayAnnotations);
    switch (displaySettings.m_annotationAlignment) {
    case AnnotationAlignment::NextToContent: m_ui.leftAligned->setChecked(true); break;
    case AnnotationAlignment::NextToMargin: m_ui.atMargin->setChecked(true); break;
    case AnnotationAlignment::RightSide: m_ui.rightAligned->setChecked(true); break;
    case AnnotationAlignment::BetweenLines: m_ui.betweenLines->setChecked(true); break;
    }
}

const DisplaySettings &DisplaySettingsPage::displaySettings() const
{
    return d->m_displaySettings;
}

const MarginSettings &DisplaySettingsPage::marginSettings() const
{
    return d->m_marginSettings;
}

void DisplaySettingsWidget::setDisplaySettings(const DisplaySettings &newDisplaySettings,
                                               const MarginSettings &newMarginSettings)
{
    if (newDisplaySettings != m_data->m_displaySettings) {
        m_data->m_displaySettings = newDisplaySettings;
        m_data->m_displaySettings.toSettings(m_data->m_settingsPrefix, Core::ICore::settings());

        emit TextEditorSettings::instance()->displaySettingsChanged(newDisplaySettings);
    }

    if (newMarginSettings != m_data->m_marginSettings) {
        m_data->m_marginSettings = newMarginSettings;
        m_data->m_marginSettings.toSettings(m_data->m_settingsPrefix, Core::ICore::settings());

        emit TextEditorSettings::instance()->marginSettingsChanged(newMarginSettings);
    }
}

DisplaySettingsPage::DisplaySettingsPage()
  : d(new DisplaySettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_DISPLAY_SETTINGS);
    setDisplayName(DisplaySettingsWidget::tr("Display"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(QCoreApplication::translate("TextEditor", "Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([this] { return new DisplaySettingsWidget(d); });
}

DisplaySettingsPage::~DisplaySettingsPage()
{
    delete d;
}

} // TextEditor
