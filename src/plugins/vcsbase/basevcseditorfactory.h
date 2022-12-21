// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include "vcsbase_global.h"

#include <texteditor/texteditor.h>

#include <functional>

namespace VcsBase {
class VcsBaseEditorParameters;

class VCSBASE_EXPORT VcsEditorFactory : public TextEditor::TextEditorFactory
{
public:
    VcsEditorFactory(const VcsBaseEditorParameters *parameters,
                     const EditorWidgetCreator editorWidgetCreator,
                     std::function<void(const Utils::FilePath &, const QString &)> describeFunc);

    ~VcsEditorFactory();
};

} // namespace VcsBase
