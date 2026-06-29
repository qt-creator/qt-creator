// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Designer::Constants {

inline constexpr char INFO_READ_ONLY[] = "DesignerXmlEditor.ReadOnly";
inline constexpr char K_DESIGNER_XML_EDITOR_ID[] = "FormEditor.DesignerXmlEditor";
inline constexpr char C_DESIGNER_XML_EDITOR[] = "Designer Xml Editor";
inline constexpr char C_DESIGNER_XML_DISPLAY_NAME[]  = QT_TRANSLATE_NOOP("QtC::Designer", "Form Editor");

inline constexpr char SETTINGS_CATEGORY[] = "P.Designer";
inline constexpr char SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("QtC::Designer", "Designer");

// Context
inline constexpr char C_FORMEDITOR[] = "FormEditor.FormEditor";
inline constexpr char M_FORMEDITOR[] = "FormEditor.Menu";
inline constexpr char M_FORMEDITOR_PREVIEW[] = "FormEditor.Menu.Preview";

// Wizard type
inline constexpr char FORM_FILE_TYPE[] = "Qt4FormFiles";

enum DesignerSubWindows
{
    WidgetBoxSubWindow,
    ObjectInspectorSubWindow,
    PropertyEditorSubWindow,
    SignalSlotEditorSubWindow,
    ActionEditorSubWindow,
    DesignerSubWindowCount
};

enum EditModes
{
    EditModeWidgetEditor,
    EditModeSignalsSlotEditor,
    EditModeBuddyEditor,
    EditModeTabOrderEditor,
    NumEditModes
};

namespace Internal {
    enum { debug = 0 };
}
} // Designer::Constants
