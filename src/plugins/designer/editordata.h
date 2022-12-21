// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

namespace SharedTools { class WidgetHost; }

namespace Designer {
    class FormWindowEditor;

namespace Internal {

// Associates the XML editor implementing the IEditor and its form widget host
class EditorData
{
public:
    explicit operator bool() const { return formWindowEditor != nullptr; }

    FormWindowEditor *formWindowEditor = nullptr;
    SharedTools::WidgetHost *widgetHost = nullptr;
};

} // namespace Internal
} // namespace Designer
