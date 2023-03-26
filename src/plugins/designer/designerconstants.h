// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#pragma once

#include <QtGlobal>

namespace Designer {
namespace Constants {

const char INFO_READ_ONLY[] = "DesignerXmlEditor.ReadOnly";
const char K_DESIGNER_XML_EDITOR_ID[] = "FormEditor.DesignerXmlEditor";
const char C_DESIGNER_XML_EDITOR[] = "Designer Xml Editor";
const char C_DESIGNER_XML_DISPLAY_NAME[]  = QT_TRANSLATE_NOOP("QtC::Designer", "Form Editor");

const char SETTINGS_CATEGORY[] = "P.Designer";
const char SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("QtC::Designer", "Designer");

// Context
const char C_FORMEDITOR[] = "FormEditor.FormEditor";
const char M_FORMEDITOR[] = "FormEditor.Menu";
const char M_FORMEDITOR_PREVIEW[] = "FormEditor.Menu.Preview";

// Wizard type
const char FORM_FILE_TYPE[] = "Qt4FormFiles";
const char FORM_MIMETYPE[] = "application/x-designer";

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
} // Constants
} // Designer
