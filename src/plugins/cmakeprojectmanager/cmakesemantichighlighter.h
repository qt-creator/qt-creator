// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QObject>

namespace TextEditor { class TextDocument; }

namespace CMakeProjectManager::Internal {

// Has the keywords of every call of the document read as keywords, the ones
// the project spells out for the commands it defines itself included.  The
// syntax definition of CMake knows the commands CMake brings and nothing of
// the ones a project adds, which is why this is read off the source.
void setupCMakeSemanticHighlighter(TextEditor::TextDocument *document);

#ifdef WITH_TESTS
QObject *createCMakeSemanticHighlighterTest();
#endif

} // namespace CMakeProjectManager::Internal
