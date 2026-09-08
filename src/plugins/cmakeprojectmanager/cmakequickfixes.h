// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

QT_BEGIN_NAMESPACE
class QObject;
QT_END_NAMESPACE

namespace TextEditor {
class IAssistProvider;
class TextDocument;
} // namespace TextEditor

namespace CMakeProjectManager::Internal {

// The refactoring actions of the CMake editor. Offers to create the source
// files a command names but which are not on disk yet.
TextEditor::IAssistProvider &cmakeQuickFixAssistProvider();

// Marks the lines that name such a file with the quick fix light bulb.
void setupCMakeQuickFixMarkers(TextEditor::TextDocument *document);

#ifdef WITH_TESTS
QObject *createCMakeQuickFixesTest();
#endif

} // CMakeProjectManager::Internal
