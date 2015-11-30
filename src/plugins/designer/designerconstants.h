/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing
**
** This file is part of Qt Creator.
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company.  For licensing terms and
** conditions see http://www.qt.io/terms-conditions.  For further information
** use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file.  Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, The Qt Company gives you certain additional
** rights.  These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
****************************************************************************/

#ifndef DESIGNERCONSTANTS_H
#define DESIGNERCONSTANTS_H

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

#endif //DESIGNERCONSTANTS_H
