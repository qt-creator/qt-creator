// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <texteditor/textdocument.h>

namespace CMakeProjectManager::Internal {

class CMakeOutlineModel;

// The document of the CMake editor. It carries the outline of the file, which
// the outline pane and the combo box of the editor tool bar show.
class CMakeTextDocument final : public TextEditor::TextDocument
{
    Q_OBJECT

public:
    CMakeTextDocument();

    CMakeOutlineModel *outlineModel() const { return m_outlineModel; }

private:
    CMakeOutlineModel * const m_outlineModel;
};

void setupCMakeEditor();

} // CMakeProjectManager::Internal
