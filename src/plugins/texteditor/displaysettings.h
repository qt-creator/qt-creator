// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>

namespace TextEditor {

enum class AnnotationAlignment
{
    NextToContent,
    NextToMargin,
    RightSide,
    BetweenLines
};

class TEXTEDITOR_EXPORT DisplaySettingsData
{
public:
    DisplaySettingsData() = default;

    bool m_displayLineNumbers = true;
    bool m_textWrapping = false;
    bool m_visualizeWhitespace = false;
    bool m_visualizeIndent = true;
    bool m_displayFoldingMarkers = true;
    bool m_highlightCurrentLine = false;
    bool m_highlightBlocks = false;
    bool m_animateMatchingParentheses = true;
    bool m_highlightMatchingParentheses = true;
    bool m_markTextChanges = true ;
    bool m_autoFoldFirstComment = true;
    bool m_centerCursorOnScroll = false;
    bool m_openLinksInNextSplit = false;
    bool m_forceOpenLinksInNextSplit = false;
    bool m_displayFileEncoding = false;
    bool m_displayFileLineEnding = true;
    bool m_displayTabSettings = true;
    bool m_scrollBarHighlights = true;
    bool m_animateNavigationWithinFile = false;
    bool m_highlightSelection = true;
    int m_animateWithinFileTimeMax = 333; // read only setting
    bool m_displayAnnotations = true;
    AnnotationAlignment m_annotationAlignment = AnnotationAlignment::RightSide;
    int m_minimalAnnotationContent = 15;
    bool m_displayMinimap = false;

    bool equals(const DisplaySettingsData &ds) const;
};

class TEXTEDITOR_EXPORT DisplaySettings : public Utils::AspectContainer
{
public:
    DisplaySettings();

    Utils::BoolAspect displayLineNumbers{this};
    Utils::BoolAspect textWrapping{this};
    Utils::BoolAspect visualizeWhitespace{this};
    Utils::BoolAspect visualizeIndent{this};
    Utils::BoolAspect displayFoldingMarkers{this};
    Utils::BoolAspect highlightCurrentLine{this};
    Utils::BoolAspect highlightBlocks{this};
    Utils::BoolAspect animateMatchingParentheses{this};
    Utils::BoolAspect highlightMatchingParentheses{this};
    Utils::BoolAspect markTextChanges{this};
    Utils::BoolAspect autoFoldFirstComment{this};
    Utils::BoolAspect centerCursorOnScroll{this};
    Utils::BoolAspect openLinksInNextSplit{this};
    Utils::BoolAspect forceOpenLinksInNextSplit{this};
    Utils::BoolAspect displayFileEncoding{this};
    Utils::BoolAspect displayFileLineEnding{this};
    Utils::BoolAspect displayTabSettings{this};
    Utils::BoolAspect scrollBarHighlights{this};
    Utils::BoolAspect animateNavigationWithinFile{this};
    Utils::BoolAspect highlightSelection{this};
    Utils::IntegerAspect animateWithinFileTimeMax{this};
    Utils::BoolAspect displayAnnotations{this};
    Utils::TypedSelectionAspect<AnnotationAlignment> annotationAlignment{this};
    Utils::IntegerAspect minimalAnnotationContent{this};
    Utils::BoolAspect displayMinimap{this};

    void apply() override;

    DisplaySettingsData data() const;
};

TEXTEDITOR_EXPORT DisplaySettings &displaySettings();

namespace Internal { void setupDisplaySettings(); }

} // namespace TextEditor

Q_DECLARE_METATYPE(TextEditor::AnnotationAlignment)
