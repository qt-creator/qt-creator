/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DESIGNERPLUGIN_CONSTANTS_H
#define DESIGNERPLUGIN_CONSTANTS_H

#include <QtGlobal>

namespace Designer {
namespace Constants {

const char INFO_READ_ONLY[] = "DesignerXmlEditor.ReadOnly";
const char K_DESIGNER_XML_EDITOR_ID[] = "FormEditor.DesignerXmlEditor";
const char C_DESIGNER_XML_EDITOR[] = "Designer Xml Editor";
const char C_DESIGNER_XML_DISPLAY_NAME[]  = QT_TRANSLATE_NOOP("Designer", "Form Editor");

const char SETTINGS_CATEGORY[] = "P.Designer";
const char SETTINGS_CATEGORY_ICON[] = ":/core/images/category_design.png";
const char SETTINGS_TR_CATEGORY[] = QT_TRANSLATE_NOOP("Designer", "Designer");
const char SETTINGS_CPP_SETTINGS_ID[] = "Class Generation";
const char SETTINGS_CPP_SETTINGS_NAME[] = QT_TRANSLATE_NOOP("Designer", "Class Generation");

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

#endif //DESIGNERPLUGIN_CONSTANTS_H
