// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "displaysettings.h"

#include "fontsettings.h"
#include "marginsettings.h"
#include "texteditorconstants.h"
#include "texteditorsettings.h"
#include "texteditortr.h"

#include <coreplugin/dialogs/ioptionspage.h>
#include <coreplugin/icore.h>

#include <utils/layoutbuilder.h>

#include <QLabel>

using namespace Utils;

namespace TextEditor {

DisplaySettings &displaySettings()
{
    static DisplaySettings theDisplaySettings;
    return theDisplaySettings;
}

DisplaySettings::DisplaySettings()
{
    setAutoApply(false);
    setSettingsGroup("textDisplaySettings");

    displayLineNumbers.setSettingsKey("DisplayLineNumbers");
    displayLineNumbers.setDefaultValue(true);
    displayLineNumbers.setLabelText(Tr::tr("Display line &numbers"));

    textWrapping.setSettingsKey("TextWrapping");
    textWrapping.setDefaultValue(false);
    textWrapping.setLabelText(Tr::tr("Enable text &wrapping"));

    visualizeWhitespace.setSettingsKey("VisualizeWhitespace");
    visualizeWhitespace.setDefaultValue(false);
    visualizeWhitespace.setLabelText(Tr::tr("&Visualize whitespace"));
    visualizeWhitespace.setToolTip(Tr::tr("Shows tabs and spaces."));

    visualizeIndent.setSettingsKey("VisualizeIndent");
    visualizeIndent.setDefaultValue(true);
    visualizeIndent.setLabelText(Tr::tr("Visualize indent"));

    displayFoldingMarkers.setSettingsKey("DisplayFoldingMarkers");
    displayFoldingMarkers.setDefaultValue(true);
    displayFoldingMarkers.setLabelText(Tr::tr("Display &folding markers"));

    highlightCurrentLine.setSettingsKey("HighlightCurrentLine2Key");
    highlightCurrentLine.setDefaultValue(false);
    highlightCurrentLine.setLabelText(Tr::tr("Highlight current &line"));

    highlightBlocks.setSettingsKey("HighlightBlocksKey");
    highlightBlocks.setDefaultValue(false);
    highlightBlocks.setLabelText(Tr::tr("Highlight &blocks"));

    animateMatchingParentheses.setSettingsKey("AnimateMatchingParenthesesKey");
    animateMatchingParentheses.setDefaultValue(true);
    animateMatchingParentheses.setLabelText(Tr::tr("&Animate matching parentheses"));

    highlightMatchingParentheses.setSettingsKey("HightlightMatchingParenthesesKey");
    highlightMatchingParentheses.setDefaultValue(true);
    highlightMatchingParentheses.setLabelText(Tr::tr("&Highlight matching parentheses"));

    markTextChanges.setSettingsKey("MarkTextChanges");
    markTextChanges.setDefaultValue(true);
    markTextChanges.setLabelText(Tr::tr("Mark &text changes"));

    autoFoldFirstComment.setSettingsKey("AutoFoldFirstComment");
    autoFoldFirstComment.setDefaultValue(true);
    autoFoldFirstComment.setLabelText(Tr::tr("Auto-fold first &comment"));

    centerCursorOnScroll.setSettingsKey("CenterCursorOnScroll");
    centerCursorOnScroll.setDefaultValue(false);
    centerCursorOnScroll.setLabelText(Tr::tr("Center &cursor on scroll"));

    openLinksInNextSplit.setSettingsKey("OpenLinksInNextSplitKey");
    openLinksInNextSplit.setDefaultValue(false);
    openLinksInNextSplit.setLabelText(Tr::tr("Always open links in another split"));

    forceOpenLinksInNextSplit.setDefaultValue(false);

    displayFileEncoding.setDefaultValue(false);
    displayFileEncoding.setSettingsKey("DisplayFileEncoding");
    displayFileEncoding.setLabelText(Tr::tr("Display file encoding"));

    displayFileLineEnding.setDefaultValue(true);
    displayFileLineEnding.setSettingsKey("DisplayFileLineEnding");
    displayFileLineEnding.setLabelText(Tr::tr("Display file line ending"));

    displayTabSettings.setDefaultValue(true);
    displayTabSettings.setSettingsKey("DisplayTabSettings");
    displayTabSettings.setLabelText(Tr::tr("Display tab settings"));

    scrollBarHighlights.setSettingsKey("ScrollBarHighlights");
    scrollBarHighlights.setDefaultValue(true);
    scrollBarHighlights.setLabelText(Tr::tr("Highlight search results on the scrollbar"));

    animateNavigationWithinFile.setSettingsKey("AnimateNavigationWithinFile");
    animateNavigationWithinFile.setDefaultValue(false);
    animateNavigationWithinFile.setLabelText(Tr::tr("Animate navigation within file"));

    highlightSelection.setSettingsKey("HighlightSelection");
    highlightSelection.setDefaultValue(true);
    highlightSelection.setLabelText(Tr::tr("&Highlight selection"));
    highlightSelection.setToolTip(Tr::tr("Adds a colored background and a marker to the "
                                         "scrollbar to occurrences of the selected text."));

    animateWithinFileTimeMax.setSettingsKey("AnimateWithinFileTimeMax");
    animateWithinFileTimeMax.setDefaultValue(333); // read only setting

    displayAnnotations.setSettingsKey("DisplayAnnotations");
    displayAnnotations.setDefaultValue(true);

    annotationAlignment.setSettingsKey("AnnotationAlignment");
    annotationAlignment.addOption(Tr::tr("Next to editor content"));
    annotationAlignment.addOption(Tr::tr("Next to right margin"));
    annotationAlignment.addOption(Tr::tr("Aligned at right side"));
    annotationAlignment.addOption(Tr::tr("Between lines"));
    annotationAlignment.setDefaultValue(AnnotationAlignment::RightSide);

    minimalAnnotationContent.setSettingsKey("MinimalAnnotationContent");
    minimalAnnotationContent.setDefaultValue(15);

    displayMinimap.setSettingsKey("DisplayMinimap");
    displayMinimap.setDefaultValue(false);
    displayMinimap.setLabelText(Tr::tr("Enable minimap"));

    readSettings();
}

void DisplaySettings::apply()
{
    AspectContainer::apply();
    AspectContainer::writeSettings();
}

class DisplaySettingsContainer final : public AspectContainer
{
public:
    DisplaySettingsContainer()
    {
        registerAspect(&displaySettings());
        registerAspect(&marginSettings());
        setLayouter([] {
            DisplaySettings &s = displaySettings();
            MarginSettings &m = marginSettings();
            auto *label =
                new QLabel(Tr::tr("<i>Set <a href=\"font zoom\">font line spacing</a> "
                                  "to 100% to enable text wrapping option.</i>"));
            const auto updateWrapping = [label] {
                const bool normalLineSpacing =
                    TextEditorSettings::fontSettings().relativeLineSpacing() == 100;
                if (!normalLineSpacing)
                    displaySettings().textWrapping.setVolatileValue(false);
                displaySettings().textWrapping.setEnabled(normalLineSpacing);
                label->setVisible(!normalLineSpacing);
            };
            updateWrapping();
            connect(TextEditorSettings::instance(), &TextEditorSettings::fontSettingsChanged,
                    label, updateWrapping);
            connect(label, &QLabel::linkActivated, [] {
                Core::ICore::showSettings(Constants::TEXT_EDITOR_FONT_SETTINGS); });
            using namespace Layouting;
            return Column {
                Group {
                    title(Tr::tr("Margin")),
                    Column {
                        Row { m.showMargin, m.marginColumn, st },
                        Row { m.useIndenter, m.tintMarginArea, st },
                        Row { m.centerEditorContentWidthPercent, st }
                    }
                },
                Group {
                    title(Tr::tr("Wrapping")),
                    Column {
                        s.textWrapping,
                        Row { label, st }
                    }
                },
                Group {
                    title(Tr::tr("Display")),
                    Row {
                        Column {
                            s.displayLineNumbers,
                            s.displayFoldingMarkers,
                            s.markTextChanges,
                            s.visualizeWhitespace,
                            s.centerCursorOnScroll,
                            s.autoFoldFirstComment,
                            s.scrollBarHighlights,
                            s.animateNavigationWithinFile,
                            s.highlightSelection,
                        },
                        Column {
                            s.highlightCurrentLine,
                            s.highlightBlocks,
                            s.animateMatchingParentheses,
                            s.visualizeIndent,
                            s.highlightMatchingParentheses,
                            s.openLinksInNextSplit,
                            s.displayFileEncoding,
                            s.displayFileLineEnding,
                            s.displayTabSettings,
                            s.displayMinimap,
                        }
                    }
                },
                Group {
                    title(Tr::tr("Line Annotations")),
                    groupChecker(s.displayAnnotations.groupChecker()),
                    Column {
                        s.annotationAlignment
                    }
                },
                st
            };
        });
    }
};

DisplaySettingsData DisplaySettings::data() const
{
    DisplaySettingsData d;

    d.m_displayLineNumbers = displayLineNumbers();
    d.m_textWrapping = textWrapping();
    d.m_visualizeWhitespace = visualizeWhitespace();
    d.m_visualizeIndent = visualizeIndent();
    d.m_displayFoldingMarkers = displayFoldingMarkers();
    d.m_highlightCurrentLine = highlightCurrentLine();
    d.m_highlightBlocks = highlightBlocks();
    d.m_animateMatchingParentheses = animateMatchingParentheses();
    d.m_highlightMatchingParentheses = highlightMatchingParentheses();
    d.m_markTextChanges = markTextChanges();
    d.m_autoFoldFirstComment = autoFoldFirstComment();
    d.m_centerCursorOnScroll = centerCursorOnScroll();
    d.m_openLinksInNextSplit = openLinksInNextSplit();
    d.m_displayFileEncoding = displayFileEncoding();
    d.m_displayFileLineEnding = displayFileLineEnding();
    d.m_displayTabSettings = displayTabSettings();
    d.m_scrollBarHighlights = scrollBarHighlights();
    d.m_animateNavigationWithinFile = animateNavigationWithinFile();
    d.m_animateWithinFileTimeMax = animateWithinFileTimeMax();
    d.m_displayAnnotations = displayAnnotations();
    d.m_annotationAlignment = annotationAlignment();
    d.m_minimalAnnotationContent = minimalAnnotationContent();
    d.m_highlightSelection = highlightSelection();
    d.m_displayMinimap = displayMinimap();

    return d;
}

class DisplaySettingsPage final : public Core::IOptionsPage
{
public:
    DisplaySettingsPage()
    {
        setId(Constants::TEXT_EDITOR_DISPLAY_SETTINGS);
        setDisplayName(Tr::tr("Display"));
        setCategory(TextEditor::Constants::TEXT_EDITOR_SETTINGS_CATEGORY);
        setSettingsProvider([this] { return &thePage; });
    }

    DisplaySettingsContainer thePage;
};

void Internal::setupDisplaySettings()
{
    static DisplaySettingsPage theDisplaySettings;
}

} // TextEditor
