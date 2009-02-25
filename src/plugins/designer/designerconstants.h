/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact:  Qt Software Information (qt-info@nokia.com)
**
** Commercial Usage
**
** Licensees holding valid Qt Commercial licenses may use this file in
** accordance with the Qt Commercial License Agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Nokia.
**
** GNU Lesser General Public License Usage
**
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** If you are unsure which license is appropriate for your use, please
** contact the sales department at qt-sales@nokia.com.
**
**************************************************************************/

#ifndef DESIGNERPLUGIN_CONSTANTS_H
#define DESIGNERPLUGIN_CONSTANTS_H

namespace Designer {
namespace Constants {

// context
const char * const C_FORMEDITOR         = "FormEditor";
const char * const T_FORMEDITOR         = "FormEditor.Toolbar";
const char * const M_FORMEDITOR         = "FormEditor.Menu";
const char * const M_FORMEDITOR_PREVIEW = "FormEditor.Menu.Preview";

// Wizard type
const char * const FORM_FILE_TYPE       = "Qt4FormFiles";
const char * const FORM_MIMETYPE = "application/x-designer";

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
