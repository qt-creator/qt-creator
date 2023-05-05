// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "displaysettingspage.h"

#include "displaysettings.h"
#include "fontsettings.h"
#include "marginsettings.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QApplication>
#include <QCheckBox>
#include <QGroupBox>
#include <QLabel>
#include <QRadioButton>
#include <QSpinBox>

namespace TextEditor {

class DisplaySettingsPagePrivate
{
public:
    DisplaySettingsPagePrivate();

    DisplaySettings m_displaySettings;
    MarginSettings m_marginSettings;
};

DisplaySettingsPagePrivate::DisplaySettingsPagePrivate()
{
    m_displaySettings.fromSettings(Core::ICore::settings());
    m_marginSettings.fromSettings(Core::ICore::settings());
}

class DisplaySettingsWidget final : public Core::IOptionsPageWidget
{
public:
    DisplaySettingsWidget(DisplaySettingsPagePrivate *data)
        : m_data(data)
    {
        enableTextWrapping = new QCheckBox(Tr::tr("Enable text &wrapping"));

        enableTextWrappingHintLabel = new QLabel(Tr::tr("<i>Set <a href=\"font zoom\">font line spacing</a> "
                                                    "to 100% to enable text wrapping option.</i>"));

        auto updateWrapping = [this] {
            const bool normalLineSpacing = TextEditorSettings::fontSettings().relativeLineSpacing() == 100;
            if (!normalLineSpacing)
                enableTextWrapping->setChecked(false);
            enableTextWrapping->setEnabled(normalLineSpacing);
            enableTextWrappingHintLabel->setVisible(!normalLineSpacing);
        };

        updateWrapping();

        connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
                this, updateWrapping);

        connect(enableTextWrappingHintLabel, &QLabel::linkActivated, [] {
            Core::ICore::showOptionsDialog(Constants::TEXT_EDITOR_FONT_SETTINGS); } );


        showWrapColumn = new QCheckBox(Tr::tr("Display right &margin at column:"));
        tintMarginArea = new QCheckBox(Tr::tr("Tint whole margin area"));

        wrapColumn = new QSpinBox;
        wrapColumn->setMaximum(999);

        connect(showWrapColumn, &QAbstractButton::toggled, wrapColumn, &QWidget::setEnabled);
        connect(showWrapColumn, &QAbstractButton::toggled, tintMarginArea, &QWidget::setEnabled);

        useIndenter = new QCheckBox(Tr::tr("Use context-specific margin"));
        useIndenter->setToolTip(Tr::tr("If available, use a different margin. "
           "For example, the ColumnLimit from the ClangFormat plugin."));

        animateMatchingParentheses = new QCheckBox(Tr::tr("&Animate matching parentheses"));
        scrollBarHighlights = new QCheckBox(Tr::tr("Highlight search results on the scrollbar"));
        displayLineNumbers = new QCheckBox(Tr::tr("Display line &numbers"));
        animateNavigationWithinFile = new QCheckBox(Tr::tr("Animate navigation within file"));
        highlightCurrentLine = new QCheckBox(Tr::tr("Highlight current &line"));
        highlightBlocks = new QCheckBox(Tr::tr("Highlight &blocks"));
        markTextChanges = new QCheckBox(Tr::tr("Mark &text changes"));
        autoFoldFirstComment = new QCheckBox(Tr::tr("Auto-fold first &comment"));
        displayFoldingMarkers = new QCheckBox(Tr::tr("Display &folding markers"));
        centerOnScroll = new QCheckBox(Tr::tr("Center &cursor on scroll"));
        visualizeIndent = new QCheckBox(Tr::tr("Visualize indent"));
        displayFileLineEnding = new QCheckBox(Tr::tr("Display file line ending"));
        displayFileEncoding = new QCheckBox(Tr::tr("Display file encoding"));
        openLinksInNextSplit = new QCheckBox(Tr::tr("Always open links in another split"));
        highlightMatchingParentheses = new QCheckBox(Tr::tr("&Highlight matching parentheses"));

        visualizeWhitespace = new QCheckBox(Tr::tr("&Visualize whitespace"));
        visualizeWhitespace->setToolTip(Tr::tr("Shows tabs and spaces."));

        leftAligned = new QRadioButton(Tr::tr("Next to editor content"));
        atMargin = new QRadioButton(Tr::tr("Next to right margin"));
        rightAligned = new QRadioButton(Tr::tr("Aligned at right side"));
        rightAligned->setChecked(true);
        betweenLines = new QRadioButton(Tr::tr("Between lines"));

        displayAnnotations = new QGroupBox(Tr::tr("Line annotations")),
        displayAnnotations->setCheckable(true);

        using namespace Layouting;

        Column {
            leftAligned,
            atMargin,
            rightAligned,
            betweenLines,
        }.attachTo(displayAnnotations);

        Column {
            Group {
                title(Tr::tr("Margin")),
                Column {
                    Row { showWrapColumn, wrapColumn, st },
                    Row { useIndenter, tintMarginArea, st }
                }
            },
            Group {
                title(Tr::tr("Wrapping")),
                Column {
                    enableTextWrapping,
                    Row { enableTextWrappingHintLabel, st}
                }
            },
            Group {
                title(Tr::tr("Display")),
                Row {
                    Column {
                        displayLineNumbers,
                        displayFoldingMarkers,
                        markTextChanges,
                        visualizeWhitespace,
                        centerOnScroll,
                        autoFoldFirstComment,
                        scrollBarHighlights,
                        animateNavigationWithinFile,
                    },
                    Column {
                        highlightCurrentLine,
                        highlightBlocks,
                        animateMatchingParentheses,
                        visualizeIndent,
                        highlightMatchingParentheses,
                        openLinksInNextSplit,
                        displayFileEncoding,
                        displayFileLineEnding,
                        st
                    }
                }
            },

            displayAnnotations,
            st
        }.attachTo(this);

        settingsToUI();
    }

    void apply() final;

    void settingsFromUI(DisplaySettings &displaySettings, MarginSettings &marginSettings) const;
    void settingsToUI();
    void setDisplaySettings(const DisplaySettings &, const MarginSettings &newMarginSettings);

    DisplaySettingsPagePrivate *m_data = nullptr;

    QCheckBox *enableTextWrapping;
    QLabel *enableTextWrappingHintLabel;
    QCheckBox *showWrapColumn;
    QCheckBox *tintMarginArea;
    QSpinBox *wrapColumn;
    QCheckBox *useIndenter;
    QCheckBox *animateMatchingParentheses;
    QCheckBox *scrollBarHighlights;
    QCheckBox *displayLineNumbers;
    QCheckBox *animateNavigationWithinFile;
    QCheckBox *highlightCurrentLine;
    QCheckBox *highlightBlocks;
    QCheckBox *markTextChanges;
    QCheckBox *autoFoldFirstComment;
    QCheckBox *displayFoldingMarkers;
    QCheckBox *centerOnScroll;
    QCheckBox *visualizeIndent;
    QCheckBox *displayFileLineEnding;
    QCheckBox *displayFileEncoding;
    QCheckBox *openLinksInNextSplit;
    QCheckBox *highlightMatchingParentheses;
    QCheckBox *visualizeWhitespace;
    QGroupBox *displayAnnotations;
    QRadioButton *leftAligned;
    QRadioButton *atMargin;
    QRadioButton *rightAligned;
    QRadioButton *betweenLines;
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
    displaySettings.m_displayLineNumbers = displayLineNumbers->isChecked();
    displaySettings.m_textWrapping = enableTextWrapping->isChecked();
    if (TextEditorSettings::fontSettings().relativeLineSpacing() != 100)
        displaySettings.m_textWrapping = false;
    marginSettings.m_showMargin = showWrapColumn->isChecked();
    marginSettings.m_tintMarginArea = tintMarginArea->isChecked();
    marginSettings.m_useIndenter = useIndenter->isChecked();
    marginSettings.m_marginColumn = wrapColumn->value();
    displaySettings.m_visualizeWhitespace = visualizeWhitespace->isChecked();
    displaySettings.m_visualizeIndent = visualizeIndent->isChecked();
    displaySettings.m_displayFoldingMarkers = displayFoldingMarkers->isChecked();
    displaySettings.m_highlightCurrentLine = highlightCurrentLine->isChecked();
    displaySettings.m_highlightBlocks = highlightBlocks->isChecked();
    displaySettings.m_animateMatchingParentheses = animateMatchingParentheses->isChecked();
    displaySettings.m_highlightMatchingParentheses = highlightMatchingParentheses->isChecked();
    displaySettings.m_markTextChanges = markTextChanges->isChecked();
    displaySettings.m_autoFoldFirstComment = autoFoldFirstComment->isChecked();
    displaySettings.m_centerCursorOnScroll = centerOnScroll->isChecked();
    displaySettings.m_openLinksInNextSplit = openLinksInNextSplit->isChecked();
    displaySettings.m_displayFileEncoding = displayFileEncoding->isChecked();
    displaySettings.m_displayFileLineEnding = displayFileLineEnding->isChecked();
    displaySettings.m_scrollBarHighlights = scrollBarHighlights->isChecked();
    displaySettings.m_animateNavigationWithinFile = animateNavigationWithinFile->isChecked();
    displaySettings.m_displayAnnotations = displayAnnotations->isChecked();
    if (leftAligned->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::NextToContent;
    else if (atMargin->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::NextToMargin;
    else if (rightAligned->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::RightSide;
    else if (betweenLines->isChecked())
        displaySettings.m_annotationAlignment = AnnotationAlignment::BetweenLines;
}

void DisplaySettingsWidget::settingsToUI()
{
    const DisplaySettings &displaySettings = m_data->m_displaySettings;
    const MarginSettings &marginSettings = m_data->m_marginSettings;
    displayLineNumbers->setChecked(displaySettings.m_displayLineNumbers);
    enableTextWrapping->setChecked(displaySettings.m_textWrapping);
    showWrapColumn->setChecked(marginSettings.m_showMargin);
    tintMarginArea->setChecked(marginSettings.m_tintMarginArea);
    tintMarginArea->setEnabled(marginSettings.m_showMargin);
    useIndenter->setChecked(marginSettings.m_useIndenter);
    wrapColumn->setValue(marginSettings.m_marginColumn);
    wrapColumn->setEnabled(marginSettings.m_showMargin);
    visualizeWhitespace->setChecked(displaySettings.m_visualizeWhitespace);
    visualizeIndent->setChecked(displaySettings.m_visualizeIndent);
    displayFoldingMarkers->setChecked(displaySettings.m_displayFoldingMarkers);
    highlightCurrentLine->setChecked(displaySettings.m_highlightCurrentLine);
    highlightBlocks->setChecked(displaySettings.m_highlightBlocks);
    animateMatchingParentheses->setChecked(displaySettings.m_animateMatchingParentheses);
    highlightMatchingParentheses->setChecked(displaySettings.m_highlightMatchingParentheses);
    markTextChanges->setChecked(displaySettings.m_markTextChanges);
    autoFoldFirstComment->setChecked(displaySettings.m_autoFoldFirstComment);
    centerOnScroll->setChecked(displaySettings.m_centerCursorOnScroll);
    openLinksInNextSplit->setChecked(displaySettings.m_openLinksInNextSplit);
    displayFileEncoding->setChecked(displaySettings.m_displayFileEncoding);
    displayFileLineEnding->setChecked(displaySettings.m_displayFileLineEnding);
    scrollBarHighlights->setChecked(displaySettings.m_scrollBarHighlights);
    animateNavigationWithinFile->setChecked(displaySettings.m_animateNavigationWithinFile);
    displayAnnotations->setChecked(displaySettings.m_displayAnnotations);
    switch (displaySettings.m_annotationAlignment) {
    case AnnotationAlignment::NextToContent: leftAligned->setChecked(true); break;
    case AnnotationAlignment::NextToMargin: atMargin->setChecked(true); break;
    case AnnotationAlignment::RightSide: rightAligned->setChecked(true); break;
    case AnnotationAlignment::BetweenLines: betweenLines->setChecked(true); break;
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
        m_data->m_displaySettings.toSettings(Core::ICore::settings());

        emit TextEditorSettings::instance()->displaySettingsChanged(newDisplaySettings);
    }

    if (newMarginSettings != m_data->m_marginSettings) {
        m_data->m_marginSettings = newMarginSettings;
        m_data->m_marginSettings.toSettings(Core::ICore::settings());

        emit TextEditorSettings::instance()->marginSettingsChanged(newMarginSettings);
    }
}

DisplaySettingsPage::DisplaySettingsPage()
  : d(new DisplaySettingsPagePrivate)
{
    setId(Constants::TEXT_EDITOR_DISPLAY_SETTINGS);
    setDisplayName(Tr::tr("Display"));
    setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
    setDisplayCategory(Tr::tr("Text Editor"));
    setCategoryIconPath(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY_ICON_PATH);
    setWidgetCreator([this] { return new DisplaySettingsWidget(d); });
}

DisplaySettingsPage::~DisplaySettingsPage()
{
    delete d;
}

} // TextEditor
