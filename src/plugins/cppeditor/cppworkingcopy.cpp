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

std::optional<QByteArray> WorkingCopy::source(const Utils::FilePath &fileName) const
{
    if (const auto value = get(fileName))
        return value->first;
    return {};
}

std::optional<QPair<QByteArray, unsigned>> WorkingCopy::get(const Utils::FilePath &fileName) const
{
    const auto it = _elements.constFind(fileName);
    if (it == _elements.constEnd())
        return {};
    return it.value();
}
} // namespace CppEditor
