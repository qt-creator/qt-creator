/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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
** contact the sales department at http://qt.nokia.com/contact.
**
**************************************************************************/

#ifndef QMLDESIGNERPLUGIN_CONSTANTS_H
#define QMLDESIGNERPLUGIN_CONSTANTS_H

#include <coreplugin/coreconstants.h>

namespace QmlDesigner {
namespace Constants {

const char * const DELETE               = "QmlDesigner.Delete";

// context
const char * const C_DESIGN_MODE        = "QmlDesigner::DesignMode";
const char * const C_FORMEDITOR         = "QmlDesigner::FormEditor";

// actions
const char * const SWITCH_TEXT_DESIGN   = "QmlDesigner.SwitchTextDesign";

// mode
const char * const DESIGN_MODE_NAME     = "Design";
const int DESIGN_MODE_PRIORITY          = Core::Constants::P_MODE_EDIT - 1;

// Wizard type
const char * const FORM_MIMETYPE        = "application/x-qmldesigner";

namespace Internal {
    enum { debug = 0 };
}

} // Constants
} // QmlDesigner

#endif //QMLDESIGNERPLUGIN_CONSTANTS_H
