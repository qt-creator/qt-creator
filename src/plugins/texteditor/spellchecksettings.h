// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "texteditor_global.h"

#include <utils/aspects.h>

namespace TextEditor {

class SyntaxHighlighter;

class TEXTEDITOR_EXPORT SpellCheckLanguageAspect final : public Utils::StringSelectionAspect
{
public:
    using StringSelectionAspect::StringSelectionAspect;

    void fixupComboBox(QComboBox *comboBox) override;
};

class TEXTEDITOR_EXPORT SpellCheckSettings final : public Utils::AspectContainer
{
public:
    SpellCheckSettings();

    Utils::BoolAspect checkText{this};
    Utils::BoolAspect checkStrings{this};
    SpellCheckLanguageAspect language{this};
};

TEXTEDITOR_EXPORT SpellCheckSettings &spellCheckSettings();

// The language to check with, which is the configured one, or English, or the one the
// machine is set to. Empty when no dictionary is installed at all. The whole of
// Qt Creator checks in one language: the prose in an editor and the prose in a submit
// message are written in the same one.
TEXTEDITOR_EXPORT QString spellCheckLanguage();

// The language an editor checks the prose in its text with, empty if it checks none.
TEXTEDITOR_EXPORT QString editorSpellCheckLanguage();

// Keeps the spell check language of highlighter on editorSpellCheckLanguage(), for a
// highlighter that marks what it considers prose with spellCheck().
TEXTEDITOR_EXPORT void followSpellCheckSettings(SyntaxHighlighter *highlighter);

namespace Internal { void setupSpellCheckSettings(); }

} // namespace TextEditor
