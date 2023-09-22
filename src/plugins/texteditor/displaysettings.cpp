// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "displaysettings.h"

#include "texteditorconstants.h"

#include <coreplugin/icore.h>

#include <utils/qtcsettings.h>
#include <utils/tooltip/tooltip.h>

#include <QLabel>

using namespace Utils;

namespace TextEditor {

const char displayLineNumbersKey[] = "DisplayLineNumbers";
const char textWrappingKey[] = "TextWrapping";
const char visualizeWhitespaceKey[] = "VisualizeWhitespace";
const char visualizeIndentKey[] = "VisualizeIndent";
const char displayFoldingMarkersKey[] = "DisplayFoldingMarkers";
const char highlightCurrentLineKey[] = "HighlightCurrentLine2Key";
const char highlightBlocksKey[] = "HighlightBlocksKey";
const char animateMatchingParenthesesKey[] = "AnimateMatchingParenthesesKey";
const char highlightMatchingParenthesesKey[] = "HightlightMatchingParenthesesKey";
const char markTextChangesKey[] = "MarkTextChanges";
const char autoFoldFirstCommentKey[] = "AutoFoldFirstComment";
const char centerCursorOnScrollKey[] = "CenterCursorOnScroll";
const char openLinksInNextSplitKey[] = "OpenLinksInNextSplitKey";
const char displayFileEncodingKey[] = "DisplayFileEncoding";
const char displayFileLineEndingKey[] = "DisplayFileLineEnding";
const char scrollBarHighlightsKey[] = "ScrollBarHighlights";
const char animateNavigationWithinFileKey[] = "AnimateNavigationWithinFile";
const char animateWithinFileTimeMaxKey[] = "AnimateWithinFileTimeMax";
const char displayAnnotationsKey[] = "DisplayAnnotations";
const char annotationAlignmentKey[] = "AnnotationAlignment";
const char minimalAnnotationContentKey[] = "MinimalAnnotationContent";
const char groupPostfix[] = "textDisplaySettings";

void DisplaySettings::toSettings(QtcSettings *s) const
{
    s->beginGroup(groupPostfix);
    s->setValue(displayLineNumbersKey, m_displayLineNumbers);
    s->setValue(textWrappingKey, m_textWrapping);
    s->setValue(visualizeWhitespaceKey, m_visualizeWhitespace);
    s->setValue(visualizeIndentKey, m_visualizeIndent);
    s->setValue(displayFoldingMarkersKey, m_displayFoldingMarkers);
    s->setValue(highlightCurrentLineKey, m_highlightCurrentLine);
    s->setValue(highlightBlocksKey, m_highlightBlocks);
    s->setValue(animateMatchingParenthesesKey, m_animateMatchingParentheses);
    s->setValue(highlightMatchingParenthesesKey, m_highlightMatchingParentheses);
    s->setValue(markTextChangesKey, m_markTextChanges);
    s->setValue(autoFoldFirstCommentKey, m_autoFoldFirstComment);
    s->setValue(centerCursorOnScrollKey, m_centerCursorOnScroll);
    s->setValue(openLinksInNextSplitKey, m_openLinksInNextSplit);
    s->setValue(displayFileEncodingKey, m_displayFileEncoding);
    s->setValue(displayFileLineEndingKey, m_displayFileLineEnding);
    s->setValue(scrollBarHighlightsKey, m_scrollBarHighlights);
    s->setValue(animateNavigationWithinFileKey, m_animateNavigationWithinFile);
    s->setValue(displayAnnotationsKey, m_displayAnnotations);
    s->setValue(annotationAlignmentKey, static_cast<int>(m_annotationAlignment));
    s->endGroup();
}

void DisplaySettings::fromSettings(QtcSettings *s)
{
    s->beginGroup(groupPostfix);
    *this = DisplaySettings(); // Assign defaults

    m_displayLineNumbers = s->value(displayLineNumbersKey, m_displayLineNumbers).toBool();
    m_textWrapping = s->value(textWrappingKey, m_textWrapping).toBool();
    m_visualizeWhitespace = s->value(visualizeWhitespaceKey, m_visualizeWhitespace).toBool();
    m_visualizeIndent = s->value(visualizeIndentKey, m_visualizeIndent).toBool();
    m_displayFoldingMarkers = s->value(displayFoldingMarkersKey, m_displayFoldingMarkers).toBool();
    m_highlightCurrentLine = s->value(highlightCurrentLineKey, m_highlightCurrentLine).toBool();
    m_highlightBlocks = s->value(highlightBlocksKey, m_highlightBlocks).toBool();
    m_animateMatchingParentheses = s->value(animateMatchingParenthesesKey, m_animateMatchingParentheses).toBool();
    m_highlightMatchingParentheses = s->value(highlightMatchingParenthesesKey, m_highlightMatchingParentheses).toBool();
    m_markTextChanges = s->value(markTextChangesKey, m_markTextChanges).toBool();
    m_autoFoldFirstComment = s->value(autoFoldFirstCommentKey, m_autoFoldFirstComment).toBool();
    m_centerCursorOnScroll = s->value(centerCursorOnScrollKey, m_centerCursorOnScroll).toBool();
    m_openLinksInNextSplit = s->value(openLinksInNextSplitKey, m_openLinksInNextSplit).toBool();
    m_displayFileEncoding = s->value(displayFileEncodingKey, m_displayFileEncoding).toBool();
    m_displayFileLineEnding = s->value(displayFileLineEndingKey, m_displayFileLineEnding).toBool();
    m_scrollBarHighlights = s->value(scrollBarHighlightsKey, m_scrollBarHighlights).toBool();
    m_animateNavigationWithinFile = s->value(animateNavigationWithinFileKey, m_animateNavigationWithinFile).toBool();
    m_animateWithinFileTimeMax = s->value(animateWithinFileTimeMaxKey, m_animateWithinFileTimeMax).toInt();
    m_displayAnnotations = s->value(displayAnnotationsKey, m_displayAnnotations).toBool();
    m_annotationAlignment = static_cast<TextEditor::AnnotationAlignment>(
                s->value(annotationAlignmentKey,
                         static_cast<int>(m_annotationAlignment)).toInt());
    m_minimalAnnotationContent = s->value(minimalAnnotationContentKey, m_minimalAnnotationContent).toInt();
    s->endGroup();
}

bool DisplaySettings::equals(const DisplaySettings &ds) const
{
    return m_displayLineNumbers == ds.m_displayLineNumbers
        && m_textWrapping == ds.m_textWrapping
        && m_visualizeWhitespace == ds.m_visualizeWhitespace
        && m_visualizeIndent == ds.m_visualizeIndent
        && m_displayFoldingMarkers == ds.m_displayFoldingMarkers
        && m_highlightCurrentLine == ds.m_highlightCurrentLine
        && m_highlightBlocks == ds.m_highlightBlocks
        && m_animateMatchingParentheses == ds.m_animateMatchingParentheses
        && m_highlightMatchingParentheses == ds.m_highlightMatchingParentheses
        && m_markTextChanges == ds.m_markTextChanges
        && m_autoFoldFirstComment== ds.m_autoFoldFirstComment
        && m_centerCursorOnScroll == ds.m_centerCursorOnScroll
        && m_openLinksInNextSplit == ds.m_openLinksInNextSplit
        && m_forceOpenLinksInNextSplit == ds.m_forceOpenLinksInNextSplit
        && m_displayFileEncoding == ds.m_displayFileEncoding
        && m_displayFileLineEnding == ds.m_displayFileLineEnding
        && m_scrollBarHighlights == ds.m_scrollBarHighlights
        && m_animateNavigationWithinFile == ds.m_animateNavigationWithinFile
        && m_animateWithinFileTimeMax == ds.m_animateWithinFileTimeMax
        && m_displayAnnotations == ds.m_displayAnnotations
        && m_annotationAlignment == ds.m_annotationAlignment
        && m_minimalAnnotationContent == ds.m_minimalAnnotationContent
            ;
}

QLabel *DisplaySettings::createAnnotationSettingsLink()
{
    auto label = new QLabel("<small><i><a href>Annotation Settings</a></i></small>");
    QObject::connect(label, &QLabel::linkActivated, []() {
        Utils::ToolTip::hideImmediately();
        Core::ICore::showOptionsDialog(Constants::TEXT_EDITOR_DISPLAY_SETTINGS);
    });
    return label;
}

} // namespace TextEditor
