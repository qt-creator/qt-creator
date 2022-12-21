// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "displaysettings.h"

#include "texteditorconstants.h"

#include <coreplugin/icore.h>
#include <utils/tooltip/tooltip.h>

#include <QLabel>
#include <QSettings>
#include <QString>

static const char displayLineNumbersKey[] = "DisplayLineNumbers";
static const char textWrappingKey[] = "TextWrapping";
static const char visualizeWhitespaceKey[] = "VisualizeWhitespace";
static const char visualizeIndentKey[] = "VisualizeIndent";
static const char displayFoldingMarkersKey[] = "DisplayFoldingMarkers";
static const char highlightCurrentLineKey[] = "HighlightCurrentLine2Key";
static const char highlightBlocksKey[] = "HighlightBlocksKey";
static const char animateMatchingParenthesesKey[] = "AnimateMatchingParenthesesKey";
static const char highlightMatchingParenthesesKey[] = "HightlightMatchingParenthesesKey";
static const char markTextChangesKey[] = "MarkTextChanges";
static const char autoFoldFirstCommentKey[] = "AutoFoldFirstComment";
static const char centerCursorOnScrollKey[] = "CenterCursorOnScroll";
static const char openLinksInNextSplitKey[] = "OpenLinksInNextSplitKey";
static const char displayFileEncodingKey[] = "DisplayFileEncoding";
static const char displayFileLineEndingKey[] = "DisplayFileLineEnding";
static const char scrollBarHighlightsKey[] = "ScrollBarHighlights";
static const char animateNavigationWithinFileKey[] = "AnimateNavigationWithinFile";
static const char animateWithinFileTimeMaxKey[] = "AnimateWithinFileTimeMax";
static const char displayAnnotationsKey[] = "DisplayAnnotations";
static const char annotationAlignmentKey[] = "AnnotationAlignment";
static const char minimalAnnotationContentKey[] = "MinimalAnnotationContent";
static const char groupPostfix[] = "textDisplaySettings";

namespace TextEditor {

void DisplaySettings::toSettings(QSettings *s) const
{
    s->beginGroup(groupPostfix);
    s->setValue(QLatin1String(displayLineNumbersKey), m_displayLineNumbers);
    s->setValue(QLatin1String(textWrappingKey), m_textWrapping);
    s->setValue(QLatin1String(visualizeWhitespaceKey), m_visualizeWhitespace);
    s->setValue(QLatin1String(visualizeIndentKey), m_visualizeIndent);
    s->setValue(QLatin1String(displayFoldingMarkersKey), m_displayFoldingMarkers);
    s->setValue(QLatin1String(highlightCurrentLineKey), m_highlightCurrentLine);
    s->setValue(QLatin1String(highlightBlocksKey), m_highlightBlocks);
    s->setValue(QLatin1String(animateMatchingParenthesesKey), m_animateMatchingParentheses);
    s->setValue(QLatin1String(highlightMatchingParenthesesKey), m_highlightMatchingParentheses);
    s->setValue(QLatin1String(markTextChangesKey), m_markTextChanges);
    s->setValue(QLatin1String(autoFoldFirstCommentKey), m_autoFoldFirstComment);
    s->setValue(QLatin1String(centerCursorOnScrollKey), m_centerCursorOnScroll);
    s->setValue(QLatin1String(openLinksInNextSplitKey), m_openLinksInNextSplit);
    s->setValue(QLatin1String(displayFileEncodingKey), m_displayFileEncoding);
    s->setValue(QLatin1String(displayFileLineEndingKey), m_displayFileLineEnding);
    s->setValue(QLatin1String(scrollBarHighlightsKey), m_scrollBarHighlights);
    s->setValue(QLatin1String(animateNavigationWithinFileKey), m_animateNavigationWithinFile);
    s->setValue(QLatin1String(displayAnnotationsKey), m_displayAnnotations);
    s->setValue(QLatin1String(annotationAlignmentKey), static_cast<int>(m_annotationAlignment));
    s->endGroup();
}

void DisplaySettings::fromSettings(QSettings *s)
{
    s->beginGroup(groupPostfix);
    *this = DisplaySettings(); // Assign defaults

    m_displayLineNumbers = s->value(QLatin1String(displayLineNumbersKey), m_displayLineNumbers).toBool();
    m_textWrapping = s->value(QLatin1String(textWrappingKey), m_textWrapping).toBool();
    m_visualizeWhitespace = s->value(QLatin1String(visualizeWhitespaceKey), m_visualizeWhitespace).toBool();
    m_visualizeIndent = s->value(QLatin1String(visualizeIndentKey), m_visualizeIndent).toBool();
    m_displayFoldingMarkers = s->value(QLatin1String(displayFoldingMarkersKey), m_displayFoldingMarkers).toBool();
    m_highlightCurrentLine = s->value(QLatin1String(highlightCurrentLineKey), m_highlightCurrentLine).toBool();
    m_highlightBlocks = s->value(QLatin1String(highlightBlocksKey), m_highlightBlocks).toBool();
    m_animateMatchingParentheses = s->value(QLatin1String(animateMatchingParenthesesKey), m_animateMatchingParentheses).toBool();
    m_highlightMatchingParentheses = s->value(QLatin1String(highlightMatchingParenthesesKey), m_highlightMatchingParentheses).toBool();
    m_markTextChanges = s->value(QLatin1String(markTextChangesKey), m_markTextChanges).toBool();
    m_autoFoldFirstComment = s->value(QLatin1String(autoFoldFirstCommentKey), m_autoFoldFirstComment).toBool();
    m_centerCursorOnScroll = s->value(QLatin1String(centerCursorOnScrollKey), m_centerCursorOnScroll).toBool();
    m_openLinksInNextSplit = s->value(QLatin1String(openLinksInNextSplitKey), m_openLinksInNextSplit).toBool();
    m_displayFileEncoding = s->value(QLatin1String(displayFileEncodingKey), m_displayFileEncoding).toBool();
    m_displayFileLineEnding = s->value(QLatin1String(displayFileLineEndingKey), m_displayFileLineEnding).toBool();
    m_scrollBarHighlights = s->value(QLatin1String(scrollBarHighlightsKey), m_scrollBarHighlights).toBool();
    m_animateNavigationWithinFile = s->value(QLatin1String(animateNavigationWithinFileKey), m_animateNavigationWithinFile).toBool();
    m_animateWithinFileTimeMax = s->value(QLatin1String(animateWithinFileTimeMaxKey), m_animateWithinFileTimeMax).toInt();
    m_displayAnnotations = s->value(QLatin1String(displayAnnotationsKey), m_displayAnnotations).toBool();
    m_annotationAlignment = static_cast<TextEditor::AnnotationAlignment>(
                s->value(QLatin1String(annotationAlignmentKey),
                         static_cast<int>(m_annotationAlignment)).toInt());
    m_minimalAnnotationContent = s->value(QLatin1String(minimalAnnotationContentKey), m_minimalAnnotationContent).toInt();
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
