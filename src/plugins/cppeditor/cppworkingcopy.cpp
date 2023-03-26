// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include "cppworkingcopy.h"

/*!
    \class CppEditor::WorkingCopy
    \brief The working copy holds among others the unsaved content of editors.

    The working copy holds
     - unsaved content of editors
     - uic-ed UI files (through \c AbstractEditorSupport)
     - the preprocessor configuration

    Contents are keyed on filename, and hold the revision in the editor and the editor's
    contents encoded as UTF-8.
*/

namespace CppEditor {

WorkingCopy::WorkingCopy() = default;

} // namespace CppEditor
